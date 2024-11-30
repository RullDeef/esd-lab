#pragma once

#include "atom.h"
#include "disjunct.h"
#include "subst.h"
#include "variable.h"
#include <list>
#include <optional>
#include <set>
#include <vector>

class Subst;
struct ExtendedDisjunct {
  int id;
  Disjunct disjunct;
  Subst subst;
  int parent_id[2];
};

class Resolver {
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
