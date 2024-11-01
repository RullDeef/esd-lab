#pragma once

#include "expr.h"
#include <optional>
#include <set>

/*

Expr Grammar in EBNF:

<expr>       ::= <equ-expr>
<equ-expr>   ::= <disj-expr> <eq-sign> <equ-expr>
<disj-expr>  ::= <impl-expr> '+' <disj-expr>
<impl-expr>  ::= <conj-expr> '->' <impl-expr>
<conj-expr>  ::= <quant-expr> '&' <conj-expr>
<quant-expr> ::= [ <quant-list> ] <term-expr>
<term-expr>  ::= '(' <expr> ')' | <neg> | <atom>
<neg>        ::= '~' <quant-expr>
<equ-sign>   ::= '==' | '=' | '<=>' | '<->'
<atom>       ::= string [ '(' <atom-list> ')' ]
<atom-list>  ::= <atom> [ ',' <atom-list> ]
<quant-list> ::= <quantifier> [ <quant-list> ]
<quantifier> ::= '\forall' '(' <var-list> ')'
               | '\exists' '(' <var-list> ')'
<var-list>   ::= string [ ',' <var-list> ]

*/
class ExprParser {
public:
  Expr::ptr Parse(const char *str);

private:
  Expr::ptr ParseEquExpr();
  Expr::ptr ParseDisjExpr();
  Expr::ptr ParseImplExpr();
  Expr::ptr ParseConjExpr();
  Expr::ptr ParseQuantExpr();
  Expr::ptr ParseTermExpr();
  Expr::ptr ParseAtom();

  // (var1, var2, ...) with parentheses
  std::set<std::string> ParseVarList();

  // parse alpha-numeric identifier
  std::optional<std::string> ParseIdent();

  // returns true if next chars match expected ones
  bool Peek(const char *expect);

  // returns true upon success
  bool Eat(const char *value);

  // returns false if end reached
  bool SkipWhitespace();

  void RaiseError(const char *message);

  const char *m_Source = nullptr;
  size_t m_pos = 0;
};
