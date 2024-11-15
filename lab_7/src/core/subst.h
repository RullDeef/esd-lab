#pragma once

#include "atom.h"
#include "variable.h"
#include <map>
#include <optional>
#include <set>
#include <string>

class Subst {
public:
  bool insert(const std::string &var, Variable::ptr value);
  bool link(const std::string &var1, const std::string &var2);

  std::optional<Subst> operator+(const Subst &other) const;

  Variable::ptr apply(const Variable::ptr &term);
  Atom apply(Atom atom);

  std::string toString() const;

  bool empty() const { return m_pairs.empty(); }

private:
  bool ringValid(const std::set<std::string> &ring) const;
  void solveRecursion(Variable::ptr &value, int depth = 0);

private:
  std::map<std::string, Variable::ptr> m_pairs;
  std::list<std::set<std::string>> m_links;
};

namespace std {
static inline string to_string(const Subst &subst) { return subst.toString(); }
} // namespace std
