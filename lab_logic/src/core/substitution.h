#pragma once

#include "expr.h"

class Substitution {
public:
  // empty substitution constructor
  Substitution() = default;

  Substitution &operator+=(const Substitution &other);

  // also changes all contained vars in values
  Expr::ptr applyTo(Expr::ptr rule, bool topLevel = true);

  bool empty() const;

  void add(const std::string &var, Expr::ptr value);
  bool has(const std::string &var);

  Expr::ptr get(const std::string &var);
  Expr::ptr &operator[](const std::string &var);

  std::string toString() const;

  bool conflicts(const Substitution &other);

private:
  std::map<std::string, Expr::ptr> pairs;
};
