#pragma once

#include "rule.h"
#include "substitution.h"
#include <list>
#include <optional>
#include <set>

class Resolver {
public:
  std::optional<Substitution> unifyTerms(Rule::ptr left, Rule::ptr right,
                                         bool topLevel = true);

  std::optional<Substitution> unifyInversePair(Rule::ptr left, Rule::ptr right);

  std::optional<std::pair<Rule::ptr, Substitution>>
  tryResolve(const std::pair<Rule::ptr, Substitution> &disjunction1,
             const std::pair<Rule::ptr, Substitution> &disjunction2);

  bool Implies(std::list<Rule::ptr> source, Rule::ptr target);
  bool Implies(Rule::ptr source, Rule::ptr target);

private:
  std::list<std::pair<Rule::ptr, Substitution>>
  disjunctionsTransform(std::list<Rule::ptr> disjunctions,
                        std::set<std::string> &renamings);

  void PrintState();

  // в логике первого порядка вместе с элементарными конъюнктами нужно держать
  // подстановку, которая привела к данному конъюнкту
  std::list<std::pair<Rule::ptr, Substitution>> m_axiomSet;
  std::list<std::pair<Rule::ptr, Substitution>> m_referenceSet;
};
