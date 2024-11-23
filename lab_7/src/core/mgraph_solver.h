#pragma once

#include "atom_hook.h"
#include "channel.h"
#include "database.h"
#include "name_allocator.h"
#include "solver.h"
#include <map>
#include <memory>

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

  MGraphSolver(std::shared_ptr<Database> database,
               std::map<std::string, std::shared_ptr<AtomHook>> atomHooks = {})
      : Solver(std::move(database)), m_atomHooks(std::move(atomHooks)) {}

protected:
  virtual void solveBackwardThreaded(Atom target,
                                     Channel<Subst> &output) override;

private:
  // checks hooks table and executes hooks if any,
  // otherwise fallbacks to basic 'generateOr'
  TaskChanPair<SubstEx> generateOr(Atom target, Subst baseSubst,
                                   NameAllocator allocator);

  TaskChanPair<SubstEx> generateOrBasic(Atom target, Subst baseSubst,
                                        NameAllocator allocator);

  TaskChanPair<SubstEx> generateAnd(std::vector<Atom> targets, Subst baseSubst,
                                    NameAllocator allocator);

  std::map<std::string, std::shared_ptr<AtomHook>> m_atomHooks;
};
