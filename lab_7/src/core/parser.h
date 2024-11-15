#pragma once

#include "rule.h"
#include <vector>

/*
  EBNF rule grammar:

  <rule>        ::= (<atom> | <rule-math> | <rule-prolog>) [ '.' ]
  <rule-math>   ::= <atom-list> '->' <atom>
  <rule-prolog> ::= <atom> ':-' <atom-list>
  <atom-list>   ::= <atom> [ (',' | '&') <atom-list> ]
  <atom>        ::= IDENT [ '(' <arg-list> ')' ]
  <arg-list>    ::= IDENT [ ',' <arg-list> ]
*/

class RuleParser {
public:
  Rule ParseRule(const char *str);

private:
  std::vector<Atom> ParseAtomList();
  Atom ParseAtom();
  std::string ParseIdent();

  bool Peek(const char *expected);
  bool Eat(const char *value);
  bool SkipWhitespace();
  void RaiseError(const char *message);

  const char *m_source = nullptr;
  size_t m_pos = 0;
};
