#pragma once

#include "rule.h"
#include "substitution.h"
#include <list>
#include <set>

class Resolver {
public:
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
