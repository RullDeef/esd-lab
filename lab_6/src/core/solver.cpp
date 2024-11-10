#include "solver.h"
#include "database.h"
#include <optional>

Solver::Solver(const Database &database) : m_database(database) {}

std::optional<Subst> Solver::solveForward(const Atom &target) {
  WorkingDataset workset;
  for (auto &fact : m_database.getFacts()) {
    // проверить, находится ли цель среди фактов в базе правил
    Subst subst;
    if (unify(fact, target, subst))
      return subst;
    else
      workset.addFact(fact);
  }
  bool newAdded = true;
  while (newAdded) {
    newAdded = false;
    workset.nextIteration();
    for (auto &rule : m_database.getRules()) {
      // проверить, что входы правила содержат атом из доказанных на предыдущем
      // шаге
      bool hasNewFacts = false;
      for (auto &input : rule.getInputs()) {
        if (workset.hasNewFactFor(input)) {
          hasNewFacts = true;
          break;
        }
      }
      // пропустить правило, если для него нет новых фактов
      if (!hasNewFacts)
        continue;
      // проверить покрытие входов из доказанных фактов
      auto subst = unifyInputs(rule, workset);
      if (!subst)
        continue;
      auto newFact = applySubst(*subst, rule.getOutput());
      // проверить, что новый факт удовлетворяет цели
      Subst res;
      if (unify(newFact, target, res))
        return res;
      workset.addFact(newFact);
      newAdded = true;
    }
  }
  return std::nullopt;
}

std::optional<Subst> Solver::solveBackward(const Atom &target) {
  return std::nullopt;
}

std::optional<Subst> Solver::unifyInputs(const Rule &rule,
                                         WorkingDataset &workset) {
  auto inputs = rule.getInputs();
  return unifyRest(inputs.begin(), inputs.end(), workset);
}

std::optional<Subst> Solver::unifyRest(std::vector<Atom>::const_iterator begin,
                                       std::vector<Atom>::const_iterator end,
                                       WorkingDataset &workset) {
  if (begin == end)
    return Subst();
  // проверить все возможные факты для данного атома, если нашли совпадение -
  // проверяем следующие атомы
  auto &curr = *begin++;
  for (const auto &fact : workset.getFacts(curr.getName())) {
    Subst subst;
    if (unify(curr, fact, subst))
      if (auto restSubst = unifyRest(begin, end, workset))
        if ((restSubst = subst + *restSubst))
          return restSubst;
  }
  return std::nullopt;
}

bool Solver::unify(const Atom &left, const Atom &right, Subst &subst) {
  if (left.getName() != right.getName())
    return false;
  auto &args1 = left.getArguments();
  auto &args2 = right.getArguments();
  if (args1.size() != args2.size())
    return false;
  for (size_t i = 0; i < args1.size(); ++i) {
    if (!isVar(args1[i]) && !isVar(args2[i])) {
      if (args1[i] != args2[i])
        return false;
    } else if (isVar(args1[i])) {
      if (!subst.insert(args1[i], args2[i]))
        return false;
    } else {
      if (!subst.insert(args2[i], args1[i]))
        return false;
    }
  }
  return true;
}

Atom Solver::applySubst(const Subst &subst, const Atom &atom) {
  auto args = atom.getArguments();
  for (auto &arg : args)
    arg = subst.apply(arg);
  return Atom(atom.getName(), std::move(args));
}
