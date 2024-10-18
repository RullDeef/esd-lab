#include "rule.h"
#include <algorithm>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <type_traits>
#include <utility>

bool contraryPair(const Rule &left, const Rule &right) {
  if (left.type == Rule::Type::inverse && right.type == Rule::Type::atom)
    return left.operands[0]->value == right.value;
  if (left.type == Rule::Type::atom && right.type == Rule::Type::inverse)
    return left.value == right.operands[0]->value;
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

Rule::Rule(std::string value) : type(Type::atom), value(value) {}

Rule::Rule(Type type, std::vector<ptr> operands)
    : type(type), operands(std::move(operands)) {}

Rule::Rule(std::true_type) : type(Type::constant), value("1") {}

Rule::Rule(std::false_type) : type(Type::constant), value("0") {}

Rule::ptr Rule::createTrue() {
  return std::make_shared<Rule>(std::true_type{});
}

Rule::ptr Rule::createFalse() {
  return std::make_shared<Rule>(std::false_type{});
}

Rule::ptr Rule::createAtom(std::string value) {
  return std::make_shared<Rule>(value);
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

bool Rule::operator==(const Rule &other) const {
  if (this == &other)
    return true;
  if (type == Type::atom || other.type == Type::atom)
    return type == other.type && value == other.value;
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
  switch (type) {
  case Type::constant:
  case Type::atom:
    res = value;
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
      if (operands[i]->operands.size() > 1 && operands.size() > 1)
        opVal = "(" + opVal + ")";
      res += (i > 0 ? " & " : "") + opVal;
    }
    break;
  case Type::disjunction:
    for (int i = 0; i < operands.size(); ++i) {
      auto opVal = operands[i]->toString();
      if (operands[i]->operands.size() > 1 && operands.size() > 1)
        opVal = "(" + opVal + ")";
      res += (i > 0 ? " + " : "") + opVal;
    }
    break;
  }
  return res;
}

std::list<Rule::ptr> Rule::getDisjunctionsList() const {
  return std::list<ptr>(operands.begin(), operands.end());
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
  }
}

Rule::ptr Rule::conjunctionToNormalForm() {
  auto left = operands[0]->toNormalForm();
  auto right = operands[1]->toNormalForm();
  if (left->type == Type::constant)
    return left->value == "0" ? left : right;
  if (right->type == Type::constant)
    return right->value == "0" ? right : left;

  // drag all elementary disjunctions from both paths
  auto disjunctions = left->operands;
  std::copy_if(right->operands.begin(), right->operands.end(),
               std::back_inserter(disjunctions), [&disjunctions](auto x) {
                 return std::all_of(disjunctions.begin(), disjunctions.end(),
                                    [&x](auto d) { return *x != *d; });
               });
  return std::make_shared<Rule>(Type::conjunction, disjunctions);
}

Rule::ptr Rule::disjunctionToNormalForm() {
  auto left = operands[0]->toNormalForm();
  auto right = operands[1]->toNormalForm();
  if (left->type == Type::constant)
    return left->value == "1" ? left : right;
  if (right->type == Type::constant)
    return right->value == "1" ? left : right;

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
  return std::make_shared<Rule>(Type::conjunction, disjunctions);
}
