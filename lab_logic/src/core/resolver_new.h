#pragma once

#include <algorithm>
#include <execution>
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

  Variable::ptr renamedFreeVars(NameAllocator &allocator) {
    // does not work for recursive functions for now!!!
    if (m_isConst)
      return shared_from_this();
    if (m_arguments.empty())
      return std::make_shared<Variable>(false,
                                        allocator.allocateRenaming(m_value));
    std::vector<Variable::ptr> arguments;
    std::transform(
        m_arguments.begin(), m_arguments.end(), std::back_inserter(arguments),
        [&allocator](auto &var) { return var->renamedFreeVars(allocator); });
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

  std::string toStringAddr(const VariableListNode *vlist = nullptr) const {
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
      res += arg->toStringAddr(&next);
    }
    res += ")";
    return "[" + std::to_string(((unsigned long)this) % 10000) + "]" + res;
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

  Atom renamedFreeVars(NameAllocator &allocator) const {
    std::vector<Variable::ptr> arguments;
    std::transform(
        m_arguments.begin(), m_arguments.end(), std::back_inserter(arguments),
        [&allocator](auto &var) { return var->renamedFreeVars(allocator); });
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

  Disjunct renamedFreeVars(NameAllocator &allocator) const {
    std::vector<Atom> atoms;
    std::transform(
        m_atoms.begin(), m_atoms.end(), std::back_inserter(atoms),
        [&allocator](auto &atom) { return atom.renamedFreeVars(allocator); });
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

class Subst {
public:
  std::optional<Subst> operator+(const Subst &other) const {
    for (const auto &[var, value] : other.m_pairs)
      if (m_pairs.count(var) != 0 &&
          m_pairs.at(var)->toString() != value->toString()) {
        return std::nullopt;
      }
    // start building combined substitution
    Subst newSubst = *this;
    for (const auto &[var, value] : other.m_pairs)
      if (!newSubst.insert(var, value))
        return std::nullopt;
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
      if (left->getValue() != right->getValue())
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
      if (!subst)
        return std::nullopt;
      subst = res + *subst;
      if (!subst)
        return std::nullopt;
      res = *subst;
    }
    return std::move(res);
  }

  std::optional<Subst> unify(const Atom &left, const Atom &right) {
    // std::cout << "unify atoms (" << left.toString() << ", " <<
    // right.toString()
    //           << ")\n";
    if (left.getName() != right.getName() ||
        left.isInverse() == right.isInverse())
      return std::nullopt;
    const auto &args1 = left.getArguments();
    const auto &args2 = right.getArguments();
    if (args1.size() != args2.size())
      return std::nullopt;
    Subst res;
    for (int i = 0; i < args1.size(); ++i) {
      // std::cout << "unify(" << args1[i]->toString() << ", "
      //           << args2[i]->toString() << ") -> ";
      auto subst = unify(args1[i], args2[i]);
      // if (!subst)
      //   std::cout << " no\n";
      // else
      //   std::cout << subst->toString() << "\n";
      if (!subst)
        return std::nullopt;
      subst = res + *subst;
      if (!subst)
        return std::nullopt;
      res = *subst;
      // std::cout << "res = " << res.toString() << "\n";
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

  std::optional<ExtendedDisjunct> resolve(const ExtendedDisjunct &left,
                                          const ExtendedDisjunct &right,
                                          int nextID) {
    for (int i = 0; i < left.disjunct.size(); ++i) {
      for (int j = 0; j < right.disjunct.size(); ++j) {
        if (auto subst = unify(left.disjunct[i], right.disjunct[j])) {
          auto disj = subst->apply(left.disjunct.withoutNth(i) +
                                   right.disjunct.withoutNth(j));
          return ExtendedDisjunct{nextID, disj, *subst, {left.id, right.id}};
        }
      }
    }
    return std::nullopt;
  }

  template <typename T>
  void populateIndexes(const std::vector<ExtendedDisjunct> &disjuncts, T &set,
                       int index = -1) {
    if (index == -1)
      index = disjuncts.size() - 1;
    set.insert(index);
    if (disjuncts[index].parent_id[0] != -1) {
      populateIndexes(disjuncts, set, disjuncts[index].parent_id[0]);
      populateIndexes(disjuncts, set, disjuncts[index].parent_id[1]);
    }
  }

  void printResolutionChain(const std::vector<ExtendedDisjunct> &disjuncts) {
    std::set<int> indexSet;
    populateIndexes(disjuncts, indexSet);
    std::priority_queue<int, std::vector<int>, std::greater<int>> indexQueue;
    for (auto index : indexSet)
      indexQueue.push(index);

    std::map<int, int> renum;
    for (int index = indexQueue.top(); !indexQueue.empty();
         indexQueue.pop(), index = indexQueue.top()) {
      int num = renum.size() + 1;
      renum[index] = num;
      if (disjuncts[index].parent_id[0] == -1) {
        // axiom, used
        std::cout << num << ") " << disjuncts[index].disjunct.toString()
                  << std::endl;
      } else {
        std::cout << num << ") " << disjuncts[index].disjunct.toString() << " ("
                  << renum[disjuncts[index].parent_id[0]] << " + "
                  << renum[disjuncts[index].parent_id[1]] << ") "
                  << disjuncts[index].subst.toString() << "\n";
      }
    }
  }

  std::optional<Subst> resolve(std::list<ExtendedDisjunct> axioms,
                               std::list<ExtendedDisjunct> target) {
    // combine all disjuncts in single vector
    std::vector<ExtendedDisjunct> disjuncts(target.begin(), target.end());
    std::copy(axioms.begin(), axioms.end(), std::back_inserter(disjuncts));

    // rename variables
    NameAllocator allocator;
    disjuncts.front().disjunct.commitVarNames(allocator);
    for (int i = 1; i < disjuncts.size(); ++i)
      disjuncts[i].disjunct = disjuncts[i].disjunct.renamedFreeVars(allocator);

    // reset ids
    for (int i = 0; i < disjuncts.size(); ++i)
      disjuncts[i].id = i;

    // combinations loop
    std::optional<Subst> result;
    constexpr auto max_iter = 100;
    for (int right_id = 1;
         right_id < disjuncts.size() && right_id < max_iter && !result;
         ++right_id) {
      for (int left_id = 0; left_id < right_id; ++left_id) {
        auto res =
            resolve(disjuncts[left_id], disjuncts[right_id], disjuncts.size());
        if (!res)
          continue;
        res->disjunct = res->disjunct.renamedFreeVars(allocator);
        disjuncts.push_back(*res);
        if (res->disjunct.size() == 0) {
          result = res->subst;
          break;
        }
      }
    }

    // output resolution steps
    if (result)
      printResolutionChain(disjuncts);

    return result;
  }

  // Алгоритм резолюции цель уже должна быть приведена к КНФ с примененным
  // отрицанием
  template <typename T> std::optional<Subst> resolve(T axioms, T target) {
    std::list<ExtendedDisjunct> extAxioms;
    std::list<ExtendedDisjunct> extTarget;
    for (auto &axiom : axioms)
      extAxioms.push_back({-1, axiom, {}, {-1, -1}});
    for (auto &tar : target)
      extTarget.push_back({-1, tar, {}, {-1, -1}});
    return resolve(std::move(extAxioms), std::move(extTarget));
  }
};
