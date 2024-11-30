#pragma once

#include "atom.h"
#include "disjunct.h"
#include "variable.h"
#include <list>
#include <map>
#include <optional>
#include <set>
#include <string>

class Subst {
public:
  std::optional<Subst> operator+(const Subst &other) const;

  bool insert(const std::string &var, Variable::ptr value);
  bool link(const std::string &var1, const std::string &var2);

  Variable::ptr apply(const Variable::ptr &term);
  Atom apply(const Atom &atom);
  Disjunct apply(const Disjunct &disj);

  std::string toString() const;

private:
  bool ringValid(const std::set<std::string> &ring) const;
  void solveRecursion(Variable::ptr &value, int depth = 0);

private:
  std::map<std::string, Variable::ptr> m_pairs;
  std::list<std::set<std::string>> m_links;
};
