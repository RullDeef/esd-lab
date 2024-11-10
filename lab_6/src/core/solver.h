#pragma once

#include "atom.h"
#include "database.h"
#include "subst.h"
#include <optional>

class Solver {
public:
  Solver(const Database &database);

  std::optional<Subst> solveForward(const Atom &target);
  std::optional<Subst> solveBackward(const Atom &target);

private:
  // проверить покрытие входов правила фактами из рабочей памяти. Дополнительно
  // вернуть флаг использования факта полученного на предыдущей итерации
  static std::optional<Subst> unifyInputs(const Rule &rule,
                                          WorkingDataset &workset);

  static std::optional<Subst> unifyRest(std::vector<Atom>::const_iterator begin,
                                        std::vector<Atom>::const_iterator end,
                                        WorkingDataset &workset);

  static bool unify(const Atom &left, const Atom &right, Subst &subst);

  static Atom applySubst(const Subst &subst, const Atom &atom);

  const Database &m_database;
};
