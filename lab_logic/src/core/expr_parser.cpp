#include "expr_parser.h"
#include <cstring>
#include <stdexcept>

/*

Expr Grammar in EBNF:

<expr>      ::= <equ-expr>
<equ-expr>  ::= <disj-expr> [<eq-sign> <equ-expr>]
<disj-expr> ::= <impl-expr> ['+' <disj-expr>]
<impl-expr> ::= <conj-expr> ['->' <impl-expr>]
<conj-expr> ::= <term-expr> ['&' <conj-expr>]
<term-expr> ::= '(' <equ-expr> ')' | <neg> | <atom>
<neg>       ::= '~' <term-expr>
<equ-sign>  ::= '==' | '=' | '<=>' | '<->'
<neg>       ::= '~' <term-expr>
<equ-sign>  ::= '==' | '=' | '<=>' | '<->'
<atom>      ::= string [ '(' <atom-list> ')' | <atom-list> ]
<atom-list> ::= <atom> [ ',' <atom-list> ]

*/

Expr::ptr ExprParser::Parse(const char *str) {
  m_Source = str;
  m_pos = 0;
  auto res = ParseEquExpr();
  if (SkipWhitespace())
    RaiseError("expr not fully parsed");
  m_Source = nullptr;
  return res;
}

Expr::ptr ExprParser::ParseEquExpr() {
  auto left = ParseDisjExpr();
  if (!Eat("==") && !Eat("=") && !Eat("<=>") && !Eat("<->"))
    return left;
  auto right = ParseEquExpr();
  return Expr::createEquality(left, right);
}

Expr::ptr ExprParser::ParseDisjExpr() {
  auto left = ParseImplExpr();
  if (!Eat("+"))
    return left;
  auto right = ParseDisjExpr();
  return Expr::createDisjunction(left, right);
}

Expr::ptr ExprParser::ParseImplExpr() {
  auto left = ParseConjExpr();
  if (!Eat("->"))
    return left;
  auto right = ParseImplExpr();
  return Expr::createImplication(left, right);
}

Expr::ptr ExprParser::ParseConjExpr() {
  auto left = ParseQuantExpr();
  if (!Eat("&"))
    return left;
  auto right = ParseConjExpr();
  return Expr::createConjunction(left, right);
}

Expr::ptr ExprParser::ParseQuantExpr() {
  if (!SkipWhitespace())
    RaiseError("quantifier non-term expected");
  if (Eat("\\exists")) {
    auto vars = ParseVarList();
    auto expr = ParseQuantExpr();
    return Expr::createExists(std::move(vars), std::move(expr));
  } else if (Eat("\\forall")) {
    auto vars = ParseVarList();
    auto expr = ParseQuantExpr();
    return Expr::createForAll(std::move(vars), std::move(expr));
  }
  return ParseTermExpr();
}

Expr::ptr ExprParser::ParseTermExpr() {
  if (Eat("(")) {
    auto expr = ParseEquExpr();
    if (!Eat(")"))
      RaiseError("unmatched parenthesis");
    return expr;
  } else if (Eat("~"))
    return Expr::createInverse(ParseTermExpr());
  return ParseAtom();
}

Expr::ptr ExprParser::ParseAtom() {
  auto name = ParseIdent();
  if (!name)
    RaiseError("atom expected");
  SkipWhitespace();
  std::vector<Expr::ptr> operands;
  // parse predicate arguments
  if (Eat("(")) {
    do {
      auto operand = ParseAtom();
      operands.push_back(std::move(operand));
    } while (Eat(","));
    if (!Eat(")"))
      RaiseError("')' expected");
  }
  return Expr::createPredicate(std::move(*name), std::move(operands));
}

std::set<std::string> ExprParser::ParseVarList() {
  if (!Eat("("))
    RaiseError("'(' expected");
  std::set<std::string> vars;
  do {
    auto ident = ParseIdent();
    if (!ident)
      RaiseError("var name expected");
    vars.insert(*ident);
  } while (Eat(","));
  if (!Eat(")"))
    RaiseError("')' expected");
  return vars;
}

std::optional<std::string> ExprParser::ParseIdent() {
  SkipWhitespace();
  size_t size = 0;
  while (m_Source[m_pos + size] && std::isalnum(m_Source[m_pos + size]))
    size++;
  if (size == 0)
    return std::nullopt;
  auto name = std::string(m_Source + m_pos, m_Source + m_pos + size);
  m_pos += size;
  return std::move(name);
}

bool ExprParser::Peek(const char *expect) {
  return SkipWhitespace() &&
         strncmp(m_Source + m_pos, expect, strlen(expect)) == 0;
}

bool ExprParser::Eat(const char *value) {
  if (!Peek(value))
    return false;
  m_pos += strlen(value);
  return true;
}

bool ExprParser::SkipWhitespace() {
  while (m_Source[m_pos] && m_Source[m_pos] == ' ')
    m_pos++;
  return m_Source[m_pos] != '\0';
}

void ExprParser::RaiseError(const char *message) {
  std::string message_str = message;
  message_str += " at " + std::to_string(m_pos);
  throw std::runtime_error(message_str);
}
