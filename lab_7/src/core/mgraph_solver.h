#pragma once

#include "channel.h"
#include "database.h"
#include "name_allocator.h"
#include "solver.h"
#include <thread>

class MGraphSolver : public Solver {
public:
  struct AtomEx {
    Atom atom;
    std::list<Rule>::const_iterator label;
    AtomEx() = delete;
  };

  struct SubstEx {
    Subst subst;
    bool cut;
  };

  MGraphSolver(std::shared_ptr<Database> database)
      : Solver(std::move(database)) {}

protected:
  virtual void solveBackwardThreaded(Atom target,
                                     Channel<Subst> &output) override;

private:
  std::thread generateOr(Atom target, Subst baseSubst, NameAllocator allocator,
                         Channel<SubstEx> &output);

  std::thread generateAnd(std::vector<Atom> targets, Subst baseSubst,
                          NameAllocator allocator, Channel<SubstEx> &output);
};
