#include "resolver_new.h"

std::optional<Subst> ResolverNew::unify(const Variable::ptr &left,
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
    if (!subst) // возник конфликт подстановок
      return std::nullopt;
    res = *subst;
  }
  return std::move(res);
}

std::optional<Subst> ResolverNew::unify(const Atom &left, const Atom &right) {
  if (left.getName() != right.getName() ||
      left.isInverse() == right.isInverse())
    return std::nullopt;
  auto &args1 = left.getArguments();
  auto &args2 = right.getArguments();
  if (args1.size() != args2.size())
    return std::nullopt;
  Subst res;
  for (int i = 0; i < args1.size(); ++i) {
    auto subst = unify(args1[i], args2[i]);
    if (!subst)
      return std::nullopt;
    subst = res + *subst;
    if (!subst) // возник конфликт подстановок
      return std::nullopt;
    res = *subst;
  }
  return std::move(res);
}

std::optional<Disjunct> ResolverNew::resolve(const Disjunct &left,
                                             const Disjunct &right) {
  for (int i = 0; i < left.size(); ++i)
    for (int j = 0; j < right.size(); ++j)
      if (auto subst = unify(left[i], right[j]))
        return subst->apply(left.withoutNth(i) + right.withoutNth(j));
  return std::nullopt;
}

std::optional<ExtendedDisjunct>
ResolverNew::resolve(const ExtendedDisjunct &left,
                     const ExtendedDisjunct &right, int nextID) {
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

void ResolverNew::populateIndexes(
    const std::vector<ExtendedDisjunct> &disjuncts, std::set<int> &set,
    int index) {
  if (index == -1)
    index = disjuncts.size() - 1;
  set.insert(index);
  if (disjuncts[index].parent_id[0] != -1) {
    populateIndexes(disjuncts, set, disjuncts[index].parent_id[0]);
    populateIndexes(disjuncts, set, disjuncts[index].parent_id[1]);
  }
}

void ResolverNew::printResolutionChain(
    const std::vector<ExtendedDisjunct> &disjuncts) {
  std::set<int> indexSet;
  populateIndexes(disjuncts, indexSet);

  std::map<int, int> renum;
  for (auto index : indexSet) {
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

std::optional<Subst> ResolverNew::resolve(std::list<ExtendedDisjunct> axioms,
                                          std::list<ExtendedDisjunct> target) {
  // combine all disjuncts in single vector
  std::vector<ExtendedDisjunct> disjuncts(target.begin(), target.end());
  std::copy(axioms.begin(), axioms.end(), std::back_inserter(disjuncts));

  // rename variables
  NameAllocator allocator;
  disjuncts.front().disjunct.commitVarNames(allocator);
  for (int i = 1; i < disjuncts.size(); ++i)
    disjuncts[i].disjunct = disjuncts[i].disjunct.renamedVars(allocator);

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
      res->disjunct = res->disjunct.renamedVars(allocator);
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
std::optional<Subst> ResolverNew::resolve(const std::vector<Disjunct> &axioms,
                                          const std::vector<Disjunct> &target) {
  std::list<ExtendedDisjunct> extAxioms;
  std::list<ExtendedDisjunct> extTarget;
  for (auto &axiom : axioms)
    extAxioms.push_back({-1, axiom, {}, {-1, -1}});
  for (auto &tar : target)
    extTarget.push_back({-1, tar, {}, {-1, -1}});
  return resolve(std::move(extAxioms), std::move(extTarget));
}
