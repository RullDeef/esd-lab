#pragma once

#include "channel.h"
#include "database.h"
#include "name_allocator.h"
#include "solver.h"

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
  TaskChanPair<SubstEx> generateOr(Atom target, Subst baseSubst,
                                   NameAllocator allocator);

  TaskChanPair<SubstEx> generateAnd(std::vector<Atom> targets, Subst baseSubst,
                                    NameAllocator allocator);
};
