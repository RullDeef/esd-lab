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
  // структура подстановки с флагом отсечения
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
  TaskChanPair<SubstEx> generateOr(Atom target, Subst baseSubst,
                                   NameAllocator allocator);

  TaskChanPair<SubstEx> generateOrBasic(Atom target, Subst baseSubst,
                                        NameAllocator allocator);

  TaskChanPair<SubstEx> generateAnd(std::vector<Atom> targets, Subst baseSubst,
                                    NameAllocator allocator);

  // таблица обработчиков специальных процедур
  std::map<std::string, std::shared_ptr<AtomHook>> m_atomHooks;
};
