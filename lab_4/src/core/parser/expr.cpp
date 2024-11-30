#include "expr.h"
#include "../utils.h"
#include <algorithm>
#include <cctype>
#include <list>
#include <map>
#include <memory>
#include <string>

bool contraryPair(const Expr &left, const Expr &right) {
  if (left.type == Expr::Type::inverse && right.type == Expr::Type::atom)
    return left.operands[0]->toString() == right.toString();
  if (left.type == Expr::Type::atom && right.type == Expr::Type::inverse)
    return left.toString() == right.operands[0]->toString();
  return false;
}

Expr::ptr disjunctReduction(Expr::ptr rule) {
  std::vector<Expr::ptr> terms = rule->getOperands();
  for (int i = terms.size() - 1; i >= 0; --i) {
    for (int j = i - 1; j >= 0; --j) {
      if (terms[i]->toString() == terms[j]->toString()) {
        terms.erase(terms.begin() + i);
        break;
      } else if (contraryPair(*terms[i], *terms[j]))
        return Expr::createTrue();
    }
  }
  return std::make_shared<Expr>(Expr::Type::disjunction, terms);
}

Expr::Expr(std::string name, std::vector<ptr> operands)
    : type(Type::atom), value(std::move(name)), operands(std::move(operands)) {}

Expr::Expr(Type type, std::vector<ptr> operands, std::set<std::string> vars)
    : type(type), vars(std::move(vars)), operands(std::move(operands)) {}

Expr::Expr(std::true_type) : type(Type::constant), value("1") {}

Expr::Expr(std::false_type) : type(Type::constant), value("0") {}

Expr::ptr Expr::createTrue() {
  return std::make_shared<Expr>(std::true_type{});
}

Expr::ptr Expr::createFalse() {
  return std::make_shared<Expr>(std::false_type{});
}

Expr::ptr Expr::createAtom(std::string value) {
  return std::make_shared<Expr>(std::move(value));
}

Expr::ptr Expr::createTerm(std::string func, std::vector<std::string> vars) {
  std::vector<ptr> operands;
  for (auto &var : vars)
    operands.push_back(createAtom(std::move(var)));
  return std::make_shared<Expr>(std::move(func), std::move(operands));
}

Expr::ptr Expr::createPredicate(std::string name, std::vector<ptr> operands) {
  return std::make_shared<Expr>(std::move(name), std::move(operands));
}

Expr::ptr Expr::createInverse(Expr::ptr rule) {
  return std::make_shared<Expr>(Type::inverse, std::vector{rule});
}

Expr::ptr Expr::createConjunction(Expr::ptr left, Expr::ptr right) {
  return std::make_shared<Expr>(Type::conjunction, std::vector{left, right});
}

Expr::ptr Expr::createDisjunction(Expr::ptr left, Expr::ptr right) {
  return std::make_shared<Expr>(Type::disjunction, std::vector{left, right});
}

Expr::ptr Expr::createImplication(Expr::ptr from, Expr::ptr to) {
  return createDisjunction(createInverse(from), to);
}

Expr::ptr Expr::createEquality(ptr left, ptr right) {
  return createConjunction(createImplication(left, right),
                           createImplication(right, left));
}

Expr::ptr Expr::createExists(std::set<std::string> vars, Expr::ptr rule) {
  return std::make_shared<Expr>(Type::exists, std::vector{rule},
                                std::move(vars));
}

Expr::ptr Expr::createForAll(std::set<std::string> vars, Expr::ptr rule) {
  return std::make_shared<Expr>(Type::forall, std::vector{rule},
                                std::move(vars));
}

bool Expr::operator==(const Expr &other) const {
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

bool Expr::operator!=(const Expr &other) const { return !(*this == other); }

bool Expr::operator==(const std::string &val) const {
  return toString() == val;
}

bool Expr::operator!=(const std::string &val) const { return !(*this == val); }

std::string Expr::toString() const {
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

      res = value + "(" +
            toStringSep(operands.begin(), operands.end(), ", ",
                        mem_ptr_fn(&Expr::toString)) +
            ")";
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

std::list<Expr::ptr> Expr::getDisjunctionsList() const {
  return std::list<ptr>(operands.begin(), operands.end());
}

std::set<std::string> Expr::getFreeVars() const {
  std::set<std::string> res;
  if (type == Type::constant) {
    // res = {};
  } else if (type == Type::atom && !operands.empty()) {
    // recursion base case
    for (auto operand : operands) {
      if (operand->operands.empty() && !std::isupper(operand->value[0]) &&
          !('0' <= operand->value[0] && operand->value[0] <= '9'))
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

Expr::ptr Expr::withRenamedVariable(const std::string &oldName,
                                    const std::string &newName) {
  std::vector<ptr> newOperands;
  std::set<std::string> newVars;
  switch (type) {
  case Type::constant:
    return shared_from_this();
  case Type::atom:
    if (operands.empty())
      return shared_from_this();
    else {
      for (auto op : operands) {
        if (op->operands.empty() && op->value == oldName)
          newOperands.push_back(createAtom(newName));
        else
          newOperands.push_back(op->withRenamedVariable(oldName, newName));
      }
      return createPredicate(value, std::move(newOperands));
    }
    break;
  case Type::inverse:
  case Type::conjunction:
  case Type::disjunction:
    for (auto &op : operands)
      newOperands.push_back(op->withRenamedVariable(oldName, newName));
    return std::make_shared<Expr>(type, std::move(newOperands));
  case Type::exists:
  case Type::forall:
    newVars = vars;
    auto iter = newVars.find(oldName);
    if (iter != newVars.end()) {
      newVars.erase(iter);
      newVars.insert(newName);
    }
    for (auto &op : operands)
      newOperands.push_back(op->withRenamedVariable(oldName, newName));
    return std::make_shared<Expr>(type, std::move(newOperands), newVars);
  }
  return nullptr;
}

Expr::ptr Expr::withReplacedVariable(const std::string &varName, ptr term) {
  std::vector<ptr> newOperands;
  switch (type) {
  case Type::constant:
    return shared_from_this();
  case Type::atom:
    if (operands.empty())
      return shared_from_this();
    else {
      for (auto &op : operands) {
        if (op->operands.empty() && op->value == varName)
          newOperands.push_back(term);
        else
          newOperands.push_back(op->withReplacedVariable(varName, term));
      }
      return createPredicate(value, std::move(newOperands));
    }
    break;
  case Type::inverse:
  case Type::conjunction:
  case Type::disjunction:
    for (auto &op : operands)
      newOperands.push_back(op->withReplacedVariable(varName, term));
    return std::make_shared<Expr>(type, std::move(newOperands));
  case Type::exists:
  case Type::forall:
    if (vars.count(varName) == 0) {
      for (auto &op : operands)
        newOperands.push_back(op->withReplacedVariable(varName, term));
    } else {
      newOperands = operands;
    }
    return std::make_shared<Expr>(type, std::move(newOperands), vars);
  }
  return nullptr;
}

Expr::ptr Expr::toNormalForm() {
  if (isCNF)
    return shared_from_this();
  switch (type) {
  case Type::constant:
    return shared_from_this();
  case Type::atom:
    return std::make_shared<Expr>(
        Type::conjunction,
        std::vector{std::make_shared<Expr>(Type::disjunction,
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
  return nullptr;
}

Expr::ptr Expr::inverseToNormalForm() {
  switch (operands[0]->type) {
  case Type::constant:
    if (operands[0]->value == "1")
      return createFalse();
    else
      return createTrue();
  case Type::atom:
    return std::make_shared<Expr>(
        Type::conjunction,
        std::vector{std::make_shared<Expr>(Type::disjunction,
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
  return nullptr;
}

Expr::ptr Expr::conjunctionToNormalForm() {
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
  ptr res = std::make_shared<Expr>(Type::conjunction, disjunctions);

  // apply quantifiers if any
  for (int i = quantifiers.size() - 1; i >= 0; --i) {
    if (quantifiers[i].first == Type::exists)
      res = createExists(quantifiers[i].second, res);
    else
      res = createForAll(quantifiers[i].second, res);
  }

  return res;
}

Expr::ptr Expr::disjunctionToNormalForm() {
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
                         [&x](const Expr::ptr &d) { return *x != *d; });
                   });
      auto newRule = std::make_shared<Expr>(Type::disjunction, args);
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

  auto res = std::make_shared<Expr>(Type::conjunction, disjunctions);

  // apply quantifiers if any
  for (int i = quantifiers.size() - 1; i >= 0; --i) {
    if (quantifiers[i].first == Type::exists)
      res = createExists(quantifiers[i].second, res);
    else
      res = createForAll(quantifiers[i].second, res);
  }

  return res;
}

Expr::ptr Expr::quantifierToNormalForm() {
  auto rule = operands[0]->toNormalForm();
  auto ruleVars = rule->getFreeVars();
  std::set<std::string> newVars;
  for (const auto &var : vars)
    if (ruleVars.count(var) > 0)
      newVars.insert(var);
  if (newVars.empty())
    return rule;
  return std::make_shared<Expr>(type, std::vector{rule}, newVars);
}

Expr::ptr Expr::toScolemForm(int *replacementCounter) {
  auto rule = toNormalForm();

  int replacementCounterLocal = 0; // f0(...), f1(...), ....
  if (replacementCounter == nullptr)
    replacementCounter = &replacementCounterLocal;
  std::vector<std::string> vars;
  while (rule->type == Type::exists || rule->type == Type::forall) {
    ptr next = rule->operands[0];
    if (rule->type == Type::exists) {
      for (auto var : rule->vars) {
        next = next->withReplacedVariable(
            var, createTerm("F" + std::to_string(*replacementCounter), vars));
        ++*replacementCounter;
      }
    } else {
      for (auto var : rule->vars)
        vars.push_back(var);
    }
    rule = next;
  }

  return rule;
}

Expr::ptr Expr::extractFrontQuantifiers(
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
          auto newName = nextIndexed(var);
          while (renamings.count(newName) > 0 && rule->vars.count(newName) > 0)
            newName = nextIndexed(newName);
          renamings[newName] = var;
          rule->operands[0] =
              rule->operands[0]->withRenamedVariable(var, newName);
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
