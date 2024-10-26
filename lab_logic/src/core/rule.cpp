#include "rule.h"
#include <algorithm>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <type_traits>
#include <utility>

bool contraryPair(const Rule &left, const Rule &right) {
  if (left.type == Rule::Type::inverse && right.type == Rule::Type::atom)
    return left.operands[0]->toString() == right.toString();
  if (left.type == Rule::Type::atom && right.type == Rule::Type::inverse)
    return left.toString() == right.operands[0]->toString();
  return false;
}

Rule::ptr disjunctReduction(Rule::ptr rule) {
  std::vector<Rule::ptr> terms = rule->getOperands();
  for (int i = terms.size() - 1; i >= 0; --i) {
    for (int j = i - 1; j >= 0; --j) {
      if (terms[i]->toString() == terms[j]->toString()) {
        terms.erase(terms.begin() + i);
        break;
      } else if (contraryPair(*terms[i], *terms[j]))
        return Rule::createTrue();
    }
  }
  return std::make_shared<Rule>(Rule::Type::disjunction, terms);
}

Rule::Rule(std::string name, std::vector<ptr> operands)
    : type(Type::atom), value(std::move(name)), operands(std::move(operands)) {}

Rule::Rule(Type type, std::vector<ptr> operands, std::set<std::string> vars)
    : type(type), vars(std::move(vars)), operands(std::move(operands)) {}

Rule::Rule(std::true_type) : type(Type::constant), value("1") {}

Rule::Rule(std::false_type) : type(Type::constant), value("0") {}

Rule::ptr Rule::createTrue() {
  return std::make_shared<Rule>(std::true_type{});
}

Rule::ptr Rule::createFalse() {
  return std::make_shared<Rule>(std::false_type{});
}

Rule::ptr Rule::createAtom(std::string value) {
  return std::make_shared<Rule>(std::move(value));
}

Rule::ptr Rule::createPredicate(std::string name, std::vector<ptr> operands) {
  return std::make_shared<Rule>(std::move(name), std::move(operands));
}

Rule::ptr Rule::createInverse(Rule::ptr rule) {
  return std::make_shared<Rule>(Type::inverse, std::vector{rule});
}

Rule::ptr Rule::createConjunction(Rule::ptr left, Rule::ptr right) {
  return std::make_shared<Rule>(Type::conjunction, std::vector{left, right});
}

Rule::ptr Rule::createDisjunction(Rule::ptr left, Rule::ptr right) {
  return std::make_shared<Rule>(Type::disjunction, std::vector{left, right});
}

Rule::ptr Rule::createImplication(Rule::ptr from, Rule::ptr to) {
  return createDisjunction(createInverse(from), to);
}

Rule::ptr Rule::createEquality(ptr left, ptr right) {
  return createConjunction(createImplication(left, right),
                           createImplication(right, left));
}

Rule::ptr Rule::createExists(std::set<std::string> vars, Rule::ptr rule) {
  return std::make_shared<Rule>(Type::exists, std::vector{rule},
                                std::move(vars));
}

Rule::ptr Rule::createForAll(std::set<std::string> vars, Rule::ptr rule) {
  return std::make_shared<Rule>(Type::forall, std::vector{rule},
                                std::move(vars));
}

bool Rule::operator==(const Rule &other) const {
  if (this == &other)
    return true;
  if (type == Type::atom || other.type == Type::atom) {
    if (type != other.type || value != other.value)
      return false;
    bool sameArgs = operands.size() == other.operands.size();
    for (int i = 0; sameArgs && i < operands.size(); ++i)
      sameArgs = sameArgs && operands[i] == other.operands[i];
    return sameArgs;
  }
  if (type != Type::inverse && operands.size() == 1)
    return *operands[0] == other;
  if (other.type != Type::inverse && other.operands.size() == 1)
    return *this == *other.operands[0];
  if (type != other.type || operands.size() != other.operands.size())
    return false;
  bool opEqual = true;
  for (int i = 0; opEqual && i < operands.size(); ++i)
    opEqual = opEqual && operands[i] == other.operands[i];
  return opEqual;
}

bool Rule::operator!=(const Rule &other) const { return !(*this == other); }

bool Rule::operator==(const std::string &val) const {
  return toString() == val;
}

bool Rule::operator!=(const std::string &val) const { return !(*this == val); }

std::string Rule::toString() const {
  std::string res = "";
  bool first = true;
  switch (type) {
  case Type::constant:
  case Type::atom:
    if (operands.empty())
      res = value;
    else {
      res = value + "(";
      bool first = true;
      for (auto operand : operands) {
        if (!first)
          res += ", ";
        res += operand->toString();
        first = false;
      }
      res += ")";
    }
    break;
  case Type::inverse:
    if (operands[0]->type == Type::atom || operands[0]->operands.size() == 1)
      res = "~" + operands[0]->toString();
    else
      res = "~(" + operands[0]->toString() + ")";
    break;
  case Type::conjunction:
    for (int i = 0; i < operands.size(); ++i) {
      std::string opVal = operands[i]->toString();
      if (operands[i]->type != Type::atom && operands[i]->operands.size() > 1 &&
          operands.size() > 1)
        opVal = "(" + opVal + ")";
      res += (i > 0 ? " & " : "") + opVal;
    }
    break;
  case Type::disjunction:
    for (int i = 0; i < operands.size(); ++i) {
      auto opVal = operands[i]->toString();
      if (operands[i]->type != Type::atom && operands[i]->operands.size() > 1 &&
          operands.size() > 1)
        opVal = "(" + opVal + ")";
      res += (i > 0 ? " + " : "") + opVal;
    }
    break;
  case Type::exists:
    res = "\\exists(";
    for (auto var : vars) {
      if (!first)
        res += ", ";
      res += var;
      first = false;
    }
    res += ") (" + operands[0]->toString() + ")";
    break;
  case Type::forall:
    res = "\\forall(";
    for (auto var : vars) {
      if (!first)
        res += ", ";
      res += var;
      first = false;
    }
    res += ") (" + operands[0]->toString() + ")";
    break;
  }
  return res;
}

std::list<Rule::ptr> Rule::getDisjunctionsList() const {
  return std::list<ptr>(operands.begin(), operands.end());
}

std::set<std::string> Rule::getFreeVars() const {
  std::set<std::string> res;
  if (type == Type::constant) {
    // res = {};
  } else if (type == Type::atom && !operands.empty()) {
    // recursion base case
    for (auto operand : operands) {
      if (operand->operands.empty())
        res.insert(operand->value);
      else {
        auto inner = operand->getFreeVars();
        res.insert(inner.begin(), inner.end());
      }
    }
  } else if (type == Type::exists || type == Type::forall) {
    auto inner = operands[0]->getFreeVars();
    std::set_difference(inner.begin(), inner.end(), vars.begin(), vars.end(),
                        std::inserter(res, res.end()));
  } else {
    for (auto operand : operands) {
      auto inner = operand->getFreeVars();
      res.insert(inner.begin(), inner.end());
    }
  }
  return res;
}

void Rule::renameVariable(const std::string &oldName,
                          const std::string &newName) {
  switch (type) {
  case Type::constant:
    break;
  case Type::atom:
    for (auto &op : operands) {
      if (op->operands.empty() && op->value == oldName)
        op->value = newName;
      else
        op->renameVariable(oldName, newName);
    }
    break;
  case Type::inverse:
  case Type::conjunction:
  case Type::disjunction:
    for (auto &op : operands)
      op->renameVariable(oldName, newName);
    break;
  case Type::exists:
  case Type::forall:
    if (vars.count(oldName) == 0)
      for (auto &op : operands)
        op->renameVariable(oldName, newName);
    break;
  }
}

Rule::ptr Rule::toNormalForm() {
  if (isCNF)
    return shared_from_this();
  switch (type) {
  case Type::constant:
    return shared_from_this();
  case Type::atom:
    return std::make_shared<Rule>(
        Type::conjunction,
        std::vector{std::make_shared<Rule>(Type::disjunction,
                                           std::vector{shared_from_this()})});
  case Type::inverse:
    return inverseToNormalForm();
  case Type::conjunction:
    return conjunctionToNormalForm();
  case Type::disjunction:
    return disjunctionToNormalForm();
  case Type::exists:
  case Type::forall:
    return quantifierToNormalForm();
  }
}

Rule::ptr Rule::inverseToNormalForm() {
  switch (operands[0]->type) {
  case Type::constant:
    if (operands[0]->value == "1")
      return createFalse();
    else
      return createTrue();
  case Type::atom:
    return std::make_shared<Rule>(
        Type::conjunction,
        std::vector{std::make_shared<Rule>(Type::disjunction,
                                           std::vector{shared_from_this()})});
  case Type::inverse:
    return operands[0]->operands[0]->toNormalForm();
  case Type::disjunction:
    return createConjunction(createInverse(operands[0]->operands[0]),
                             createInverse(operands[0]->operands[1]))
        ->toNormalForm();
  case Type::conjunction:
    return createDisjunction(createInverse(operands[0]->operands[0]),
                             createInverse(operands[0]->operands[1]))
        ->toNormalForm();
  case Type::exists:
    return createForAll(
        operands[0]->vars,
        createInverse(operands[0]->operands[0])->toNormalForm());
  case Type::forall:
    return createExists(
        operands[0]->vars,
        createInverse(operands[0]->operands[0])->toNormalForm());
  }
}

Rule::ptr Rule::conjunctionToNormalForm() {
  auto left = operands[0]->toNormalForm();
  auto right = operands[1]->toNormalForm();

  // constant propagation
  if (left->type == Type::constant)
    return left->value == "0" ? left : right;
  if (right->type == Type::constant)
    return right->value == "0" ? right : left;

  // extract all quantifiers from both operands (with optional renaming)
  std::map<std::string, std::string> renamings;
  std::vector<std::pair<Type, std::set<std::string>>> quantifiers;
  left = extractFrontQuantifiers(renamings, quantifiers, left);
  right = extractFrontQuantifiers(renamings, quantifiers, right);

  // drag all elementary disjunctions from both paths
  auto disjunctions = left->operands;
  std::copy_if(right->operands.begin(), right->operands.end(),
               std::back_inserter(disjunctions), [&disjunctions](auto x) {
                 return std::all_of(disjunctions.begin(), disjunctions.end(),
                                    [&x](auto d) { return *x != *d; });
               });
  ptr res = std::make_shared<Rule>(Type::conjunction, disjunctions);

  // apply quantifiers if any
  for (int i = quantifiers.size() - 1; i >= 0; --i) {
    if (quantifiers[i].first == Type::exists)
      res = createExists(quantifiers[i].second, res);
    else
      res = createForAll(quantifiers[i].second, res);
  }

  return res;
}

Rule::ptr Rule::disjunctionToNormalForm() {
  auto left = operands[0]->toNormalForm();
  auto right = operands[1]->toNormalForm();

  // constants propagation
  if (left->type == Type::constant)
    return left->value == "1" ? left : right;
  if (right->type == Type::constant)
    return right->value == "1" ? left : right;

  // extract all quantifiers
  std::map<std::string, std::string> renamings;
  std::vector<std::pair<Type, std::set<std::string>>> quantifiers;
  left = extractFrontQuantifiers(renamings, quantifiers, left);
  right = extractFrontQuantifiers(renamings, quantifiers, right);

  auto left_dsj = left->operands;
  auto right_dsj = right->operands;
  std::vector<ptr> disjunctions;
  for (size_t i = 0; i < left_dsj.size(); ++i) {
    for (size_t j = 0; j < right_dsj.size(); ++j) {
      auto args = left_dsj[i]->operands;
      std::copy_if(right_dsj[j]->operands.begin(), right_dsj[j]->operands.end(),
                   std::back_inserter(args), [&args](auto x) {
                     return std::all_of(
                         args.begin(), args.end(),
                         [&x](const Rule::ptr &d) { return *x != *d; });
                   });
      auto newRule = std::make_shared<Rule>(Type::disjunction, args);
      if (std::all_of(disjunctions.begin(), disjunctions.end(),
                      [&newRule](auto d) { return *d != *newRule; }))
        disjunctions.push_back(newRule);
    }
  }

  // check contrary disjunctions
  for (int i = disjunctions.size() - 1; i >= 0; --i) {
    disjunctions[i] = disjunctReduction(disjunctions[i]);
    if (disjunctions[i]->type == Type::constant)
      disjunctions.erase(disjunctions.begin() + i);
  }
  if (disjunctions.empty())
    return createTrue();

  auto res = std::make_shared<Rule>(Type::conjunction, disjunctions);

  // apply quantifiers if any
  for (int i = quantifiers.size() - 1; i >= 0; --i) {
    if (quantifiers[i].first == Type::exists)
      res = createExists(quantifiers[i].second, res);
    else
      res = createForAll(quantifiers[i].second, res);
  }

  return res;
}

Rule::ptr Rule::quantifierToNormalForm() {
  auto rule = operands[0]->toNormalForm();
  auto ruleVars = rule->getFreeVars();
  std::set<std::string> newVars;
  for (const auto &var : vars)
    if (ruleVars.count(var) > 0)
      newVars.insert(var);
  if (newVars.empty())
    return rule;
  return std::make_shared<Rule>(type, std::vector{rule}, newVars);
}

Rule::ptr Rule::extractFrontQuantifiers(
    std::map<std::string, std::string> &renamings,
    std::vector<std::pair<Type, std::set<std::string>>> &quantifiers,
    ptr rule) {
  if (renamings.empty()) {
    while (rule->type == Type::forall || rule->type == Type::exists) {
      for (const auto &var : rule->vars)
        renamings[var] = var;
      quantifiers.emplace_back(rule->type, std::move(rule->vars));
      rule = rule->operands[0];
    }
  } else {
    while (rule->type == Type::forall || rule->type == Type::exists) {
      // check for renaming
      std::set<std::string> newVars;
      for (auto var : rule->vars) {
        int count = renamings.count(var);
        if (count == 0) {
          renamings[var] = var;
          newVars.insert(var);
        } else {
          auto newName = var + std::to_string(count);
          renamings[newName] = var;
          rule->operands[0]->renameVariable(var, newName);
          newVars.insert(newName);
        }
      }
      if (!newVars.empty())
        quantifiers.emplace_back(rule->type, std::move(newVars));
      rule = rule->operands[0];
    }
  }
  return rule;
}
