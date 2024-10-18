#pragma once

#include "rule.h"
#include <list>

class Resolver {
public:
  bool Implies(std::list<Rule::ptr> source, Rule::ptr target);
  bool Implies(Rule::ptr source, Rule::ptr target);

private:
  void PrintState();

  std::list<Rule::ptr> m_axiomSet;
  std::list<Rule::ptr> m_referenceSet;
};
