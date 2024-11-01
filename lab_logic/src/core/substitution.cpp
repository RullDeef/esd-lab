#include "substitution.h"
#include <iostream>
#include <memory>
#include <ostream>
#include <stdexcept>

Substitution &Substitution::operator+=(const Substitution &other) {
  for (auto [var, value] : other.pairs) {
    value = applyTo(value);
    for (auto &[myVar, myValue] : pairs) {
    }
    add(var, value);
  }
  return *this;
}

// also changes all contained vars in values
Rule::ptr Substitution::applyTo(Rule::ptr rule, bool topLevel) {
  if (rule->getType() == Rule::Type::inverse)
    return Rule::createInverse(applyTo(rule->getOperands()[0], topLevel));
  if (rule->getType() == Rule::Type::disjunction) {
    auto operands = rule->getOperands();
    for (auto &op : operands)
      op = applyTo(op, topLevel);
    return std::make_shared<Rule>(Rule::Type::disjunction, std::move(operands));
  }
  if (rule->type != Rule::Type::atom)
    throw std::runtime_error("invalid rule type for substitution");
  if (rule->operands.empty()) {
    return topLevel ? rule : has(rule->value) ? get(rule->value) : rule;
  }
  // check for recursive object
  for (auto &[var, value] : pairs) {
    if (value == rule) {
      std::cout << "recursive struct detected for: " << rule->toString()
                << std::endl;
      return rule;
    }
  }
  std::vector<Rule::ptr> operands;
  for (auto op : rule->operands) {
    // if (op->operands.empty())
    //   operands.push_back(has(op->value) ? get(op->value) : op);
    // else
    operands.push_back(applyTo(op, false));
  }
  return Rule::createPredicate(rule->value, operands);
}

bool Substitution::empty() const { return pairs.empty(); }

void Substitution::add(const std::string &var, Rule::ptr value) {
  pairs[var] = applyTo(value);
}

bool Substitution::has(const std::string &var) { return pairs.count(var) > 0; }

Rule::ptr Substitution::get(const std::string &var) { return pairs[var]; }

Rule::ptr &Substitution::operator[](const std::string &var) {
  return pairs[var];
}

std::string Substitution::toString() const {
  std::string res = "{";
  bool first = true;
  for (const auto &[var, value] : pairs) {
    if (!first)
      res += ", ";
    first = false;
    res += value->toString() + "/" + var;
  }
  res += "}";
  return res;
}

bool Substitution::conflicts(const Substitution& other) {
  for (auto [var, value] : other.pairs)
    if (has(var) && *get(var) != *value)
      return true;
  return false;
}
