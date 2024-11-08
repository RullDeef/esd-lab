#pragma once

#include <algorithm>
#include <functional>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <queue>
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

class NameAllocator {
public:
  bool allocateName(std::string name) {
    auto [base, index] = splitIndexed(std::move(name));
    if (m_allocated.count(base) == 0) {
      m_allocated[base] = std::list{index};
      return true;
    }
    auto &indexList = m_allocated[base];
    auto pos = std::lower_bound(indexList.begin(), indexList.end(), index);
    if (pos != indexList.end() && *pos == index)
      return false;
    indexList.insert(pos, index);
    return true;
  }

  // commit must be called in order to apply all changes made with this method
  std::string allocateRenaming(std::string original) {
    if (m_working.count(original) != 0)
      return m_working[original];
    auto [base, index] = splitIndexed(original);
    if (m_allocated.count(base) == 0) {
      auto newName = joinIndexed(base, ++index);
      m_allocated[base] = std::list{index};
      m_working[original] = newName;
      return newName;
    }
    auto &indexList = m_allocated[base];
    std::list<int>::iterator pos;
    do {
      pos = std::lower_bound(indexList.begin(), indexList.end(), ++index);
    } while (pos != indexList.end() && *pos == index);
    indexList.insert(pos, index);
    auto newName = joinIndexed(base, index);
    m_working[original] = newName;
    return newName;
  }

  void commit() { m_working.clear(); }

  void deallocate(std::string name) {
    auto [base, index] = splitIndexed(std::move(name));
    if (m_allocated.count(base) == 0)
      return;
    auto &indexList = m_allocated[base];
    auto pos = std::find(indexList.begin(), indexList.end(), index);
    if (pos != indexList.end()) {
      indexList.erase(pos);
      if (indexList.empty())
        m_allocated.erase(base);
    }
  }

  std::string toString() const {
    std::string res = "{";
    bool first = true;
    for (auto &[base, indexList] : m_allocated) {
      if (!first)
        res += " ";
      first = false;
      res += base + ":";
      for (auto &index : indexList)
        res += std::to_string(index) + ";";
    }
    res += "}";
    if (!m_working.empty()) {
      res += "[";
      for (auto &[key, val] : m_working) {
        res += key + ":" + val + ",";
      }
      res += "]";
    }
    return res;
  }

private:
  std::pair<std::string, int> splitIndexed(std::string name) {
    if (name.empty())
      throw std::runtime_error("empty name for indexing");
    int lastPos = name.size() - 1;
    while (lastPos >= 0 && std::isalnum(name[lastPos]) &&
           !std::isalpha(name[lastPos]))
      lastPos--;
    if (lastPos == -1)
      throw std::runtime_error("numeric only name is invalid");
    int index = 0;
    if (lastPos < name.size() - 1)
      index = std::atoi(name.c_str() + lastPos + 1);
    return std::make_pair(name.substr(0, lastPos + 1), index);
  }

  std::string joinIndexed(std::string name, int index) {
    return name + std::to_string(index);
  }

  std::map<std::string, std::list<int>> m_allocated;
  std::map<std::string, std::string> m_working;
};

class Variable;

struct VariableListNode {
  const Variable *var;
  const VariableListNode *prev;
};

class Variable : public std::enable_shared_from_this<Variable> {
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

  void commitVarNames(NameAllocator &allocator) const {
    if (m_isConst)
      return;
    if (m_arguments.empty())
      allocator.allocateName(m_value);
    else
      for (auto &arg : m_arguments)
        arg->commitVarNames(allocator);
  }

  Variable::ptr renamedVars(NameAllocator &allocator) {
    // does not work for recursive functions for now!!!
    if (m_isConst)
      return shared_from_this();
    if (m_arguments.empty())
      return std::make_shared<Variable>(false,
                                        allocator.allocateRenaming(m_value));
    std::vector<Variable::ptr> arguments;
    std::transform(
        m_arguments.begin(), m_arguments.end(), std::back_inserter(arguments),
        [&allocator](auto &var) { return var->renamedVars(allocator); });
    return std::make_shared<Variable>(false, m_value, std::move(arguments));
  }

  const std::string &getValue() const { return m_value; }
  const std::vector<Variable::ptr> getArguments() const { return m_arguments; }

  void updateArgument(size_t i, Variable::ptr value) { m_arguments[i] = value; }

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

  void commitVarNames(NameAllocator &allocator) const {
    for (auto &var : m_arguments)
      var->commitVarNames(allocator);
  }

  Atom renamedVars(NameAllocator &allocator) const {
    std::vector<Variable::ptr> arguments;
    std::transform(
        m_arguments.begin(), m_arguments.end(), std::back_inserter(arguments),
        [&allocator](auto &var) { return var->renamedVars(allocator); });
    return Atom(m_inverse, m_name, std::move(arguments));
  }

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

  void commitVarNames(NameAllocator &allocator) const {
    for (auto &atom : m_atoms)
      atom.commitVarNames(allocator);
  }

  Disjunct renamedVars(NameAllocator &allocator) const {
    std::vector<Atom> atoms;
    std::transform(
        m_atoms.begin(), m_atoms.end(), std::back_inserter(atoms),
        [&allocator](auto &atom) { return atom.renamedVars(allocator); });
    allocator.commit();
    return Disjunct(std::move(atoms));
  }

  bool operator<(const Disjunct &other) const {
    return m_atoms.size() < other.m_atoms.size();
  }

  const Atom &operator[](size_t i) const { return m_atoms[i]; }

  size_t size() const { return m_atoms.size(); }

  auto begin() const { return m_atoms.begin(); }
  auto end() const { return m_atoms.end(); }

  std::string toString() const {
    std::string res;
    bool first = true;
    for (auto &atom : m_atoms) {
      if (!first)
        res += " + ";
      first = false;
      res += atom.toString();
    }
    return res;
  }

private:
  std::vector<Atom> m_atoms;
};

class Subst;
struct ExtendedDisjunct;
class ResolverNew {
public:
  std::optional<Subst> unify(const Variable::ptr &left,
                             const Variable::ptr &right);

  std::optional<Subst> unify(const Atom &left, const Atom &right);

  std::optional<Disjunct> resolve(const Disjunct &left, const Disjunct &right);

  std::optional<ExtendedDisjunct> resolve(const ExtendedDisjunct &left,
                                          const ExtendedDisjunct &right,
                                          int nextID);

  void populateIndexes(const std::vector<ExtendedDisjunct> &disjuncts,
                       std::set<int> &set, int index = -1);

  void printResolutionChain(const std::vector<ExtendedDisjunct> &disjuncts);

  std::optional<Subst> resolve(std::list<ExtendedDisjunct> axioms,
                               std::list<ExtendedDisjunct> target);

  std::optional<Subst> resolve(const std::vector<Disjunct> &axioms,
                               const std::vector<Disjunct> &target);
};

class Subst {
public:
  std::optional<Subst> operator+(const Subst &other) const {
    if (m_pairs.empty() && m_links.empty())
      return other;
    if (other.m_pairs.empty() && other.m_links.empty())
      return *this;
    // start building combined substitution
    Subst newSubst = *this;
    for (const auto &[var, value] : other.m_pairs) {
      if (m_pairs.count(var) != 0 &&
          m_pairs.at(var)->toString() != value->toString()) {
        // попытка унификации двух термов с возможной генерацией новой
        // подстановки
        auto auxSubst = ResolverNew().unify(m_pairs.at(var), value);
        if (!auxSubst) // унификация невозможна - конфликт
          return std::nullopt;
        newSubst.insert(var, auxSubst->apply(value));
      } else if (!newSubst.insert(var, value))
        return std::nullopt;
    }
    for (const auto &otherRing : other.m_links) {
      bool intersected = false;
      for (auto &ring : newSubst.m_links) {
        if (set_overlaps(ring, otherRing)) {
          ring.insert(otherRing.begin(), otherRing.end());
          if (!ringValid(ring))
            return std::nullopt;
          intersected = true;
          break;
        }
      }
      if (!intersected) {
        if (!ringValid(otherRing))
          return std::nullopt;
        newSubst.m_links.push_back(otherRing);
      }
    }
    // solve recursive references
    for (auto &[var, value] : newSubst.m_pairs)
      newSubst.solveRecursion(value);
    return newSubst;
  }

  bool insert(const std::string &var, Variable::ptr value) {
    if (m_pairs.count(var) > 0) {
      return m_pairs[var]->toString() == value->toString();
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

  Variable::ptr apply(const Variable::ptr &term) {
    if (term->isConst())
      return term;
    if (term->isVariable()) {
      if (m_pairs.count(term->getValue()) > 0)
        return m_pairs[term->getValue()];
      // проверяем наличие переменной в связанном кольце неозначенных переменных
      for (auto &ring : m_links)
        if (ring.count(term->getValue()) > 0)
          return std::make_shared<Variable>(false, *ring.begin());
      return term;
    }
    // если term - функциональный символ - выполняем все остальное
    if (!term->hasVars())
      return term;
    auto name = term->getValue();
    std::vector<Variable::ptr> args;
    for (auto arg : term->getArguments())
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
      if (m_pairs.count(*ring.begin()) > 0)
        res += m_pairs.at(*ring.begin())->toString();
    }
    first = true;
    for (const auto &[var, value] : m_pairs) {
      if (accounted.count(var) == 0) {
        if (!first)
          res += ", ";
        first = false;
        res += var + "=" + value->toString();
      }
    }
    res += "}";
    return res;
  }

private:
  bool ringValid(const std::set<std::string> &ring) const {
    if (m_pairs.count(*ring.begin()) == 0) {
      // all other items also must not have value
      for (const auto &item : ring) {
        if (m_pairs.count(item) != 0)
          return false;
      }
      return true;
    }
    auto firstValue = m_pairs.at(*ring.begin())->toString();
    for (const auto &item : ring) {
      if (m_pairs.count(item) == 0)
        return false;
      if (m_pairs.at(item)->toString() != firstValue)
        return false;
    }
    return true;
  }

  void solveRecursion(Variable::ptr &value, int depth = 0) {
    if (depth > 10)
      return;
    if (value->isFuncSym()) {
      auto args = value->getArguments();
      for (size_t i = 0; i < args.size(); ++i) {
        if (args[i]->isVariable()) {
          value->updateArgument(i, apply(args[i]));
        } else if (args[i]->isFuncSym()) {
          solveRecursion(args[i], depth + 1);
          value->updateArgument(i, args[i]);
        }
      }
    }
  }

private:
  std::map<std::string, Variable::ptr> m_pairs;
  std::list<std::set<std::string>> m_links;
};

struct ExtendedDisjunct {
  int id;
  Disjunct disjunct;
  Subst subst;
  int parent_id[2];
};
