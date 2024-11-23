#pragma once

#include "database.h"
#include "solver.h"
#include "subst.h"

struct AtomEx {
  Atom atom;
  std::list<Rule>::const_iterator label;
  AtomEx() = delete;
};

class PrologSolver : public Solver {
public:
  PrologSolver(std::shared_ptr<Database> database);

protected:
  virtual void solveBackwardThreaded(Atom target,
                                     Channel<Subst> &output) override;

private:
  void showState() const;

  std::deque<std::list<AtomEx>> m_resolventStack; // стек резольвент
  std::deque<std::list<AtomEx>> m_backtrackStack; // стек отката
  std::deque<Subst> m_varsStack; // стек подстановок (переменных)

  size_t m_targetCounter; // счетчик активных атомов
};
