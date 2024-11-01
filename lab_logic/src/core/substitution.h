#pragma once

#include "rule.h"

class Substitution {
public:
  // empty substitution constructor
  Substitution() = default;

  Substitution &operator+=(const Substitution &other);

  // also changes all contained vars in values
  Rule::ptr applyTo(Rule::ptr rule, bool topLevel = true);

  bool empty() const;

  void add(const std::string &var, Rule::ptr value);
  bool has(const std::string &var);

  Rule::ptr get(const std::string &var);
  Rule::ptr &operator[](const std::string &var);

  std::string toString() const;

  bool conflicts(const Substitution& other);

private:
  std::map<std::string, Rule::ptr> pairs;
};
