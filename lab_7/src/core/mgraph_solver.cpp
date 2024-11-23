#include "mgraph_solver.h"
#include "channel.h"
#include "name_allocator.h"
#include "solver.h"
#include "subst.h"
#include <memory>
#include <thread>

static void saturateAllocator(NameAllocator &allocator, const Subst &subst) {
  for (auto &name : subst.getAllVarNames())
    allocator.allocateName(name);
}

static Rule standardize(const Rule &rule, NameAllocator &allocator) {
  Atom output = rule.getOutput().renamedVars(allocator);
  std::vector<Atom> inputs;
  for (auto input : rule.getInputs())
    inputs.push_back(input.renamedVars(allocator));
  allocator.commit();
  return Rule(std::move(inputs), std::move(output));
}

void MGraphSolver::solveBackwardThreaded(Atom target, Channel<Subst> &output) {
  auto [worker, mid] = generateOr(target, Subst(), NameAllocator());
  while (!output.isClosed()) {
    auto [substEx, ok] = mid->get();
    if (!ok) {
      output.close();
      break;
    }
    // filter subst to include only relevant variables
    Subst filtered;
    for (auto varName : target.getAllVars())
      filtered.insert(varName, substEx.subst.apply(
                                   std::make_shared<Variable>(false, varName)));
    if (!output.put(std::move(filtered)))
      break;
  }
  mid->close();
}

TaskChanPair<MGraphSolver::SubstEx>
MGraphSolver::generateOr(Atom target, Subst baseSubst,
                         NameAllocator allocator) {
  auto output = std::make_shared<Channel<SubstEx>>();
  std::jthread worker([this, target = std::move(target),
                       baseSubst = std::move(baseSubst),
                       allocator = std::move(allocator), output]() {
    for (auto &rule : m_database->getRules()) {
      if (output->isClosed()) {
        break;
      }
      NameAllocator subAllocator = allocator;
      Subst subst = baseSubst;
      Rule stdRule = standardize(rule, subAllocator);
      if (!unify(target, stdRule.getOutput(), subst))
        continue;
      if (stdRule.isFact()) {
        if (!output->put({subst, false}))
          break;
        continue;
      }
      std::vector<Atom> subGoals;
      for (auto &atom : stdRule.getInputs())
        subGoals.push_back(subst.apply(atom));
      auto [worker, mid] = generateAnd(std::move(subGoals), std::move(subst),
                                       std::move(subAllocator));
      bool wasCut = false;
      while (true) {
        auto [subst2, ok] = mid->get();
        if (!ok)
          break;
        wasCut = wasCut || subst2.cut;
        if (!output->put({std::move(subst2.subst), false})) {
          mid->close();
          return;
        }
      }
      if (wasCut)
        break;
      mid->close();
    }
    output->close();
  });
  return std::make_pair(std::move(worker), output);
}

TaskChanPair<MGraphSolver::SubstEx>
MGraphSolver::generateAnd(std::vector<Atom> targets, Subst baseSubst,
                          NameAllocator allocator) {
  auto output = std::make_shared<Channel<SubstEx>>();
  std::jthread worker([this, targets = std::move(targets),
                       baseSubst = std::move(baseSubst),
                       allocator = std::move(allocator), output]() {
    if (targets.empty()) {
      output->put({baseSubst, false});
      output->close();
      return;
    }
    Subst subst = baseSubst;
    Atom first = subst.apply(targets.front());
    std::vector<Atom> rest(targets.begin() + 1, targets.end());
    for (auto &atom : rest)
      atom = subst.apply(atom);
    if (first.toString() == "cut") {
      auto [worker, andChan] = generateAnd(rest, subst, std::move(allocator));
      while (true) {
        auto [substEx2, ok2] = andChan->get();
        if (!ok2)
          break;
        if (!output->put({std::move(substEx2.subst), true})) {
          andChan->close();
          return;
        }
      }
    } else {
      auto [orWorker, orChan] =
          generateOr(std::move(first), baseSubst, allocator);
      while (true) {
        auto [substEx, ok] = orChan->get();
        if (!ok)
          break;
        NameAllocator subAllocator = allocator;
        saturateAllocator(subAllocator, substEx.subst);
        auto [andWorker, andChan] =
            generateAnd(rest, substEx.subst, std::move(subAllocator));
        while (true) {
          auto [substEx2, ok2] = andChan->get();
          if (!ok2)
            break;
          if (!output->put({std::move(substEx2.subst), false})) {
            andChan->close();
            orChan->close();
            return;
          }
        }
        andChan->close();
      }
      orChan->close();
    }
    output->close();
  });
  return std::make_pair(std::move(worker), output);
}
