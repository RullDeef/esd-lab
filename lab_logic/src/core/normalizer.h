#pragma once

#include "disjunct.h"
#include "parser/expr.h"

class ExprNormalizer {
public:
  std::vector<Disjunct> scolemForm(Expr::ptr expr);

private:
  Expr::ptr normalForm(Expr::ptr expr);
  Expr::ptr inverseToNormalForm(Expr::ptr expr);
  Expr::ptr conjunctionToNormalForm(Expr::ptr expr);
  Expr::ptr disjunctionToNormalForm(Expr::ptr expr);
  Expr::ptr quantifierToNormalForm(Expr::ptr expr);

  Expr::ptr extractFrontQuantifiers(
      std::map<std::string, std::string> &renamings,
      std::vector<std::pair<Expr::Type, std::set<std::string>>> &quantifiers,
      Expr::ptr rule);

  Expr::ptr disjunctReduction(Expr::ptr expr);
  Atom transformAtomExpr(Expr::ptr expr);
  Variable::ptr transformVarExpr(Expr::ptr expr);

  int m_funcCounter = 0;
};
