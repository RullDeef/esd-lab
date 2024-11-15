#include "mgraph_solver.h"
#include <iostream>
#include <memory>
#include <thread>

void MGraphSolver::solveBackwardThreaded(Atom target, Channel<Subst> &output) {
  auto mid = std::make_shared<Channel<Subst>>();
  generateOr(target, Subst(), *mid);
  while (!output.isClosed()) {
    auto [subst, ok] = mid->get();
    // filter subst to include only relevant variables
    Subst filtered;
    for (auto varName : target.getAllVars())
      filtered.insert(varName,
                      subst.apply(std::make_shared<Variable>(false, varName)));
    if (!ok || !output.put(std::move(filtered)))
      return;
  }
}

void MGraphSolver::generateOr(Atom target, Subst baseSubst,
                              Channel<Subst> &output) {
  std::thread worker([this, target = std::move(target),
                      baseSubst = std::move(baseSubst), &output]() {
    std::cout << "proving " << target.toString() << std::endl;
    for (auto &rule : m_database.getRules()) {
      if (output.isClosed()) {
        std::cout << "output already closed!!!\n";
        break;
      }
      // standardize variables for rule
      Subst subst = baseSubst;
      if (!unify(target, rule.getOutput(), subst))
        continue;
      std::cout << "unified with rule " << rule.toString() << " "
                << subst.toString() << std::endl;
      if (rule.isFact()) {
        if (!output.put(subst))
          break;
        continue;
      }
      std::vector<Atom> subGoals;
      for (auto &atom : rule.getInputs())
        subGoals.push_back(subst.apply(atom));
      auto mid = std::make_shared<Channel<Subst>>();
      generateAnd(std::move(subGoals), std::move(subst), *mid);
      while (true) {
        auto [subst2, ok] = mid->get();
        if (!ok)
          break;
        std::cout << "generated subst: " << subst2.toString() << std::endl;
        if (!output.put(std::move(subst2))) {
          mid->close();
          return;
        }
      }
    }
    std::cout << "output closed!" << std::endl;
    output.close();
  });
  worker.detach();
}

void MGraphSolver::generateAnd(std::vector<Atom> targets, Subst baseSubst,
                               Channel<Subst> &output) {
  std::thread worker([this, targets = std::move(targets),
                      baseSubst = std::move(baseSubst), &output]() {
    if (targets.empty()) {
      output.put(baseSubst);
      output.close();
      return;
    }
    Subst subst = baseSubst;
    Atom first = subst.apply(targets.front());
    std::vector<Atom> rest(targets.begin() + 1, targets.end());
    for (auto &atom : rest)
      atom = subst.apply(atom);
    auto orChan = std::make_shared<Channel<Subst>>();
    generateOr(std::move(first), baseSubst, *orChan);
    while (true) {
      auto [subst, ok] = orChan->get();
      if (!ok) {
        output.close();
        return;
      }
      auto andChan = std::make_shared<Channel<Subst>>();
      generateAnd(rest, subst, *andChan);
      while (true) {
        auto [subst2, ok2] = andChan->get();
        if (!ok2)
          break;
        if (!output.put(std::move(subst2))) {
          andChan->close();
          return;
        }
      }
    }
    output.close();
  });
  worker.detach();
}
