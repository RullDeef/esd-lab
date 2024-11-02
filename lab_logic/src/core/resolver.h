#pragma once

#include "parser/expr.h"
#include "substitution.h"
#include <list>
#include <optional>
#include <set>

class Resolver {
public:
  std::optional<Substitution> unifyTerms(Expr::ptr left, Expr::ptr right,
                                         bool topLevel = true);

  std::optional<Substitution> unifyInversePair(Expr::ptr left, Expr::ptr right);

  std::optional<std::pair<Expr::ptr, Substitution>>
  tryResolve(const std::pair<Expr::ptr, Substitution> &disjunction1,
             const std::pair<Expr::ptr, Substitution> &disjunction2);

  bool Implies(std::list<Expr::ptr> source, Expr::ptr target);
  bool Implies(Expr::ptr source, Expr::ptr target);

private:
  std::list<std::pair<Expr::ptr, Substitution>>
  disjunctionsTransform(std::list<Expr::ptr> disjunctions,
                        std::set<std::string> &renamings);

  void PrintState();

  // в логике первого порядка вместе с элементарными конъюнктами нужно держать
  // подстановку, которая привела к данному конъюнкту
  std::list<std::pair<Expr::ptr, Substitution>> m_axiomSet;
  std::list<std::pair<Expr::ptr, Substitution>> m_referenceSet;
};
