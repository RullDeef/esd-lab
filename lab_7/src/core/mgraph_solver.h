#pragma once

#include "channel.h"
#include "database.h"
#include "solver.h"

class MGraphSolver : public Solver {
public:
  struct AtomEx {
    Atom atom;
    std::list<Rule>::const_iterator label;
    AtomEx() = delete;
  };

  MGraphSolver(const Database &database) : Solver(database) {}

protected:
  virtual void solveBackwardThreaded(Atom target,
                                     Channel<Subst> &output) override;

private:
  void generateOr(Atom target, Subst baseSubst, Channel<Subst> &output);

  void generateAnd(std::vector<Atom> targets, Subst baseSubst,
                   Channel<Subst> &output);
};
