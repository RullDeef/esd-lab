#pragma once

#include "atom.h"
#include "channel.h"
#include "database.h"
#include "subst.h"
#include "variable.h"
#include <memory>
#include <optional>
#include <thread>

class Solver : public std::enable_shared_from_this<Solver> {
public:
  Solver(std::shared_ptr<Database> database);
  virtual ~Solver();

  void solveForward(Atom target);
  void solveBackward(Atom target);

  // получить следующую подстановку, удовлетворяющую текущей цели заданной через
  // методы solveForward или solveBackward
  std::optional<Subst> next();

  // принудительно остановить поиск новых решений
  void done();

  static bool unify(const Atom &left, const Atom &right, Subst &subst);
  static bool unify(Variable::ptr left, Variable::ptr right, Subst &subst);

protected:
  virtual void solveForwardThreaded(Atom target, Channel<Subst> &output);
  virtual void solveBackwardThreaded(Atom target, Channel<Subst> &output);

  // проверить покрытие входов правила фактами из рабочей памяти. Дополнительно
  // вернуть флаг использования факта полученного на предыдущей итерации
  static std::pair<std::thread, std::shared_ptr<Channel<Subst>>>
  unifyInputs(const Rule &rule, WorkingDataset &workset);

  static bool unifyRest(std::vector<Atom>::const_iterator begin,
                        std::vector<Atom>::const_iterator end,
                        WorkingDataset &workset, const Subst &prev,
                        Channel<Subst> &channel, bool wasNewFact = false);

  std::thread m_solverThread;
  std::shared_ptr<Channel<Subst>> m_channel;
  bool m_stopRequest;
  std::shared_ptr<Database> m_database;
};
