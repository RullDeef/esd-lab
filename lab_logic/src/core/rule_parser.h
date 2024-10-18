#pragma once

#include "rule.h"

/*

Rule Grammar in EBNF:

<rule>      ::= <equ-rule>
<equ-rule>  ::= <disj-rule> <eq-sign> <equ-rule>
<disj-rule> ::= <impl-rule> '+' <disj-rule>
<impl-rule> ::= <conj-rule> '->' <impl-rule>
<conj-rule> ::= <term-rule> '&' <conj-rule>
<term-rule> ::= '(' <rule> ')' | <neg> | <atom>
<neg>       ::= '~' <term-rule>
<equ-sign>  ::= '==' | '=' | '<=>' | '<->'
<atom>      ::= string

*/
class RuleParser {
public:
  Rule::ptr Parse(const char *str);

private:
  Rule::ptr ParseEquRule();
  Rule::ptr ParseDisjRule();
  Rule::ptr ParseImplRule();
  Rule::ptr ParseConjRule();
  Rule::ptr ParseTermRule();
  Rule::ptr ParseAtom();

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
