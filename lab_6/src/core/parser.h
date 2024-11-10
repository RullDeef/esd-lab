#pragma once

#include "rule.h"

/*
  EBNF rule grammar:

  <rule>      ::= <atom-list> '->' <atom>
  <atom-list> ::= <atom> [ '&' <atom-list> ]
  <atom>      ::= IDENT [ '(' <arg-list> ')' ]
  <arg-list>  ::= IDENT [ ',' <arg-list> ]

  <fact>      ::= <atom>
*/

class RuleParser {
public:
  Rule ParseRule(const char *str);
  Atom ParseFact(const char *str);

private:
  Atom ParseAtom();
  std::string ParseIdent();

  bool Peek(const char *expected);
  bool Eat(const char *value);
  bool SkipWhitespace();
  void RaiseError(const char *message);

  const char *m_source = nullptr;
  size_t m_pos = 0;
};
