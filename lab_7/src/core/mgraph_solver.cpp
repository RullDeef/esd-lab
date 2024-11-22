#include "mgraph_solver.h"
#include "name_allocator.h"
#include "subst.h"
#include <iostream>
#include <memory>
#include <thread>

static NameAllocator buildAllocator(const Subst &subst) {
  NameAllocator allocator;
  std::cout << "building allocator {";
  for (auto &name : subst.getAllVarNames()) {
    std::cout << name << ", ";
    allocator.allocateName(name);
  }
  std::cout << "}\n";
  return allocator;
}

static Rule standardize(const Rule &rule, const Subst &subst) {
  auto allocator = buildAllocator(subst);
  Atom output = rule.getOutput().renamedVars(allocator);
  std::vector<Atom> inputs;
  for (auto input : rule.getInputs())
    inputs.push_back(input.renamedVars(allocator));
  return Rule(std::move(inputs), std::move(output));
}

void MGraphSolver::solveBackwardThreaded(Atom target, Channel<Subst> &output) {
  auto mid = std::make_shared<Channel<SubstEx>>();
  generateOr(target, Subst(), *mid);
  while (!output.isClosed()) {
    auto [substEx, ok] = mid->get();
    std::cout << "GOT SUBST: " << substEx.subst.toString() << std::endl;
    // filter subst to include only relevant variables
    Subst filtered;
    for (auto varName : target.getAllVars())
      filtered.insert(varName, substEx.subst.apply(
                                   std::make_shared<Variable>(false, varName)));
    if (!ok || !output.put(std::move(filtered)))
      return;
  }
}

void MGraphSolver::generateOr(Atom target, Subst baseSubst,
                              Channel<SubstEx> &output) {
  std::thread worker([this, target = std::move(target),
                      baseSubst = std::move(baseSubst), &output]() {
    std::cout << "proving " << target.toString() << " with "
              << baseSubst.toString() << std::endl;
    for (auto &rule : m_database.getRules()) {
      if (output.isClosed()) {
        break;
      }
      std::cout << "base subst: " << baseSubst.toString() << std::endl;
      Subst subst = baseSubst;
      Rule stdRule = standardize(rule, subst);
      std::cout << "target:\n" << target.toString();
      if (!unify(target, stdRule.getOutput(), subst)) {
        std::cout << " not unified with\n"
                  << stdRule.getOutput().toString() << "\n";
        continue;
      }
      std::cout << " unified with\n"
                << stdRule.toString() << " " << subst.toString() << std::endl;
      if (stdRule.isFact()) {
        if (!output.put({subst, false}))
          break;
        continue;
      }
      std::vector<Atom> subGoals;
      for (auto &atom : stdRule.getInputs())
        subGoals.push_back(subst.apply(atom));
      auto mid = std::make_shared<Channel<SubstEx>>();
      generateAnd(std::move(subGoals), std::move(subst), *mid);
      bool wasCut = false;
      while (true) {
        auto [subst2, ok] = mid->get();
        if (!ok)
          break;
        wasCut = wasCut || subst2.cut;
        if (!output.put({std::move(subst2.subst), false})) {
          mid->close();
          return;
        }
      }
      if (wasCut) {
        std::cout << "rule " << target.toString() << " was cut!\n";
        break;
      }
    }
    output.close();
  });
  worker.detach();
}

void MGraphSolver::generateAnd(std::vector<Atom> targets, Subst baseSubst,
                               Channel<SubstEx> &output) {
  std::thread worker([this, targets = std::move(targets),
                      baseSubst = std::move(baseSubst), &output]() {
    if (targets.empty()) {
      output.put({baseSubst, false});
      output.close();
      return;
    }
    Subst subst = baseSubst;
    Atom first = subst.apply(targets.front());
    std::vector<Atom> rest(targets.begin() + 1, targets.end());
    for (auto &atom : rest)
      atom = subst.apply(atom);
    std::cout << "proving targets " << first.toString() << " and rest\n";
    if (first.toString() == "cut") {
      std::cout << "cut encountered!\n";
      auto andChan = std::make_shared<Channel<SubstEx>>();
      generateAnd(rest, subst, *andChan);
      while (true) {
        auto [substEx2, ok2] = andChan->get();
        if (!ok2)
          break;
        if (!output.put({std::move(substEx2.subst), true})) {
          andChan->close();
          return;
        }
      }
    } else {
      auto orChan = std::make_shared<Channel<SubstEx>>();
      generateOr(std::move(first), baseSubst, *orChan);
      while (true) {
        auto [substEx, ok] = orChan->get();
        if (!ok)
          break;
        auto andChan = std::make_shared<Channel<SubstEx>>();
        generateAnd(rest, substEx.subst, *andChan);
        while (true) {
          auto [substEx2, ok2] = andChan->get();
          if (!ok2)
            break;
          if (!output.put({std::move(substEx2.subst), false})) {
            andChan->close();
            orChan->close();
            return;
          }
        }
      }
    }
    output.close();
  });
  worker.detach();
}
