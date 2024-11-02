#pragma once

#include <algorithm>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

template <typename T>
bool set_overlaps(const std::set<T> &set1, const std::set<T> &set2) {
  std::set<T> ref;
  std::set_intersection(set1.begin(), set1.end(), set2.begin(), set2.end(),
                        std::inserter(ref, ref.end()), std::less());
  return !ref.empty();
}

class Variable;

struct VariableListNode {
  const Variable *var;
  const VariableListNode *prev;
};

class Variable {
public:
  using ptr = std::shared_ptr<Variable>;

  Variable(bool isConst, std::string value,
           std::vector<Variable::ptr> arguments = {})
      : m_isConst(isConst), m_value(std::move(value)),
        m_arguments(std::move(arguments)) {}

  bool isConst() const { return m_isConst; }
  bool isVariable() const { return !m_isConst && m_arguments.empty(); }
  bool isFuncSym() const { return !m_arguments.empty(); }

  bool hasVars(const VariableListNode *vlist = nullptr) const {
    if (hasSelf(vlist))
      return false;
    if (!m_isConst && m_arguments.empty())
      return true;
    VariableListNode next = {this, vlist};
    for (const auto &arg : m_arguments)
      if (arg->hasVars(&next))
        return true;
    return false;
  }

  const std::string &getValue() const { return m_value; }
  const std::vector<Variable::ptr> getArguments() const { return m_arguments; }

  std::string toString(const VariableListNode *vlist = nullptr) const {
    if (hasSelf(vlist))
      return "...";
    if (m_arguments.empty())
      return m_value;
    VariableListNode next = {this, vlist};
    std::string res = m_value + "(";
    bool first = true;
    for (const auto &arg : m_arguments) {
      if (!first)
        res += ", ";
      first = false;
      res += arg->toString(&next);
    }
    res += ")";
    return res;
  }

private:
  bool hasSelf(const VariableListNode *list) const {
    if (list == nullptr)
      return false;
    if (list->var == this)
      return true;
    return hasSelf(list->prev);
  }

  bool m_isConst;
  std::string m_value;
  std::vector<Variable::ptr> m_arguments;
};

class Atom {
public:
  Atom(bool inverse, std::string name,
       std::vector<Variable::ptr> arguments = {})
      : m_inverse(inverse), m_name(std::move(name)),
        m_arguments(std::move(arguments)) {}

  bool isInverse() const { return m_inverse; }
  const std::string &getName() const { return m_name; }
  const std::vector<Variable::ptr> &getArguments() const { return m_arguments; }

  std::string toString() const {
    std::string res;
    if (m_inverse)
      res = "~";
    res += m_name;
    if (!m_arguments.empty()) {
      res += "(";
      bool first = true;
      for (const auto &arg : m_arguments) {
        if (!first)
          res += ", ";
        first = false;
        res += arg->toString();
      }
      res += ")";
    }
    return res;
  }

private:
  bool m_inverse;
  std::string m_name;
  std::vector<Variable::ptr> m_arguments;
};

class Disjunct {
public:
  Disjunct(std::vector<Atom> atoms) : m_atoms(std::move(atoms)) {}

  Disjunct withoutNth(size_t i) const {
    std::vector<Atom> newAtoms(m_atoms);
    newAtoms.erase(newAtoms.begin() + i);
    return Disjunct(std::move(newAtoms));
  }

  Disjunct operator+(const Disjunct &other) const {
    std::vector<Atom> newAtoms(m_atoms);
    newAtoms.insert(newAtoms.end(), other.m_atoms.begin(), other.m_atoms.end());
    return Disjunct(std::move(newAtoms));
  }

  const Atom &operator[](size_t i) const { return m_atoms[i]; }

  size_t size() const { return m_atoms.size(); }

  auto begin() const { return m_atoms.begin(); }
  auto end() const { return m_atoms.end(); }

private:
  std::vector<Atom> m_atoms;
};

class Subst {
public:
  Subst &operator+=(const Subst &other) {
    for (const auto &[var, value] : other.m_pairs)
      m_pairs[var] = apply(value);
    for (const auto &otherRing : other.m_links) {
      bool intersected = false;
      for (auto &ring : m_links) {
        if (set_overlaps(ring, otherRing)) {
          ring.insert(otherRing.begin(), otherRing.end());
          intersected = true;
          break;
        }
      }
      if (!intersected)
        m_links.push_back(otherRing);
    }
    return *this;
  }

  bool insert(const std::string &var, Variable::ptr value) {
    if (m_pairs.count(var) > 0) {
      return m_pairs[var] == value;
    }
    m_pairs[var] = value;
    // to avoid check assigment does not conflicts with linked variables
    // propagate value to all linked variables
    for (auto &ring : m_links)
      if (ring.count(var) > 0)
        for (const auto &linkedVar : ring)
          m_pairs[linkedVar] = value;
    return true;
  }

  bool link(const std::string &var1, const std::string &var2) {
    if (m_pairs.count(var1) > 0 && m_pairs.count(var2) > 0) {
      if (m_pairs.at(var1) != m_pairs.at(var2))
        return false;
    }
    for (auto ring : m_links) {
      if (ring.count(var1) > 0) {
        ring.insert(var2);
        if (m_pairs.count(var1) > 0)
          m_pairs[var2] = m_pairs[var1];
        return true;
      } else if (ring.count(var2) > 0) {
        ring.insert(var1);
        if (m_pairs.count(var2) > 0)
          m_pairs[var1] = m_pairs[var2];
        return true;
      }
    }
    m_links.push_back({var1, var2});
    if (m_pairs.count(var1) > 0)
      m_pairs[var2] = m_pairs[var1];
    else if (m_pairs.count(var2) > 0)
      m_pairs[var1] = m_pairs[var2];
    return true;
  }

  bool conflicts(const Subst &other) const {
    for (const auto &[var, value] : other.m_pairs)
      if (m_pairs.count(var) != 0 && m_pairs.at(var) != value)
        return true;
    return false;
  }

  Variable::ptr apply(const Variable::ptr &term) {
    if (term->isConst())
      return term;
    if (term->isVariable()) {
      if (m_pairs.count(term->getValue()) > 0)
        return m_pairs[term->getValue()];
      return term;
    }
    // если term - функциональный символ - выполняем все остальное
    if (!term->hasVars())
      return term;
    auto name = term->getValue();
    std::vector<Variable::ptr> args;
    for (const auto &arg : term->getArguments())
      args.push_back(apply(arg));
    return std::make_shared<Variable>(false, std::move(name), std::move(args));
  }

  Atom apply(const Atom &atom) {
    std::vector<Variable::ptr> newArgs;
    for (const auto &arg : atom.getArguments())
      newArgs.push_back(apply(arg));
    return Atom(atom.isInverse(), atom.getName(), std::move(newArgs));
  }

  Disjunct apply(const Disjunct &disj) {
    std::vector<Atom> atoms;
    for (const auto &atom : disj)
      atoms.push_back(apply(atom));
    return Disjunct(std::move(atoms));
  }

  std::string toString() const {
    std::set<std::string> accounted;
    std::string res = "{";
    bool first = true;
    for (const auto &ring : m_links) {
      if (!first)
        res += ", ";
      first = false;
      for (const auto &var : ring) {
        res += var + "=";
        accounted.insert(var);
      }
      res += m_pairs.at(*ring.begin())->toString();
    }
    for (const auto &[var, value] : m_pairs) {
      if (accounted.count(var) == 0) {
        if (!first)
          res += ", ";
        res += var + "=" + value->toString();
      }
    }
    res += "}";
    return res;
  }

private:
  std::map<std::string, Variable::ptr> m_pairs;
  std::list<std::set<std::string>> m_links;
};

class ResolverNew {
public:
  std::optional<Subst> unify(const Variable::ptr &left,
                             const Variable::ptr &right) {
    if (left->isConst() && right->isConst()) {
      if (left->getValue() != right->getValue())
        return std::nullopt;
      else
        return std::make_optional<Subst>();
    }
    if (left->isVariable() && !right->isVariable()) {
      Subst res;
      res.insert(left->getValue(), right);
      return std::move(res);
    }
    if (!left->isVariable() && right->isVariable()) {
      Subst res;
      res.insert(right->getValue(), left);
      return std::move(res);
    }
    if (left->isVariable() && right->isVariable()) {
      Subst res;
      res.link(left->getValue(), right->getValue());
      return std::move(res);
    }
    // ниже - унификация функциональных символов
    if (left->getValue() != right->getValue())
      return std::nullopt;
    const auto &args1 = left->getArguments();
    const auto &args2 = right->getArguments();
    if (args1.size() != args2.size())
      return std::nullopt;
    Subst res;
    for (int i = 0; i < args1.size(); ++i) {
      auto subst = unify(args1[i], args2[i]);
      if (!subst || res.conflicts(*subst))
        return std::nullopt;
      res += *subst;
    }
    return std::move(res);
  }

  std::optional<Subst> unify(const Atom &left, const Atom &right) {
    if (left.getName() != right.getName() ||
        left.isInverse() == right.isInverse())
      return std::nullopt;
    const auto &args1 = left.getArguments();
    const auto &args2 = right.getArguments();
    if (args1.size() != args2.size())
      return std::nullopt;
    Subst res;
    for (int i = 0; i < args1.size(); ++i) {
      auto subst = unify(args1[i], args2[i]);
      if (!subst || res.conflicts(*subst))
        return std::nullopt;
      res += *subst;
    }
    return std::move(res);
  }

  std::optional<Disjunct> resolve(const Disjunct &left, const Disjunct &right) {
    for (int i = 0; i < left.size(); ++i)
      for (int j = 0; j < right.size(); ++j)
        if (auto subst = unify(left[i], right[j]))
          return subst->apply(left.withoutNth(i) + right.withoutNth(j));
    return std::nullopt;
  }
};
