#pragma once

#include "atom.h"
#include "channel.h"
#include "database.h"
#include "subst.h"
#include <optional>
#include <thread>

class Solver {
public:
  Solver(const Database &database);
  ~Solver();

  void solveForward(Atom target);
  void solveBackward(Atom target);

  // получить следующую подстановку, удовлетворяющую текущей цели заданной через
  // методы solveForward или solveBackward
  std::optional<Subst> next();

  // принудительно остановить поиск новых решений
  void done();

private:
  // проверить покрытие входов правила фактами из рабочей памяти. Дополнительно
  // вернуть флаг использования факта полученного на предыдущей итерации
  static std::shared_ptr<Channel<Subst>> unifyInputs(const Rule &rule,
                                                     WorkingDataset &workset);

  static bool unifyRest(std::vector<Atom>::const_iterator begin,
                        std::vector<Atom>::const_iterator end,
                        WorkingDataset &workset, const Subst &prev,
                        Channel<Subst> &channel, bool wasNewFact = false);

  static bool unify(const Atom &left, const Atom &right, Subst &subst);

  static Atom applySubst(const Subst &subst, const Atom &atom);

  std::thread m_solverThread;
  std::shared_ptr<Channel<Subst>> m_channel;
  bool m_stopRequest;
  const Database &m_database;
};
