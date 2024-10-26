#include "substitution.h"
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
Rule::ptr Substitution::applyTo(Rule::ptr rule) {
  if (rule->getType() == Rule::Type::inverse)
    return Rule::createInverse(applyTo(rule->getOperands()[0]));
  if (rule->type != Rule::Type::atom)
    throw std::runtime_error("invalid rule type for substitution");
  if (rule->operands.empty())
    return rule;
  // check for recursive object
  for (auto &[var, value] : pairs)
    if (value == rule)
      return rule;
  std::vector<Rule::ptr> operands;
  for (auto op : rule->operands) {
    if (op->operands.empty())
      operands.push_back(has(op->value) ? get(op->value) : op);
    else
      operands.push_back(applyTo(op));
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
