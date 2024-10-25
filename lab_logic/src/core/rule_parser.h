#pragma once

#include "rule.h"
#include <optional>

/*

Rule Grammar in EBNF:

<rule>       ::= <equ-rule>
<equ-rule>   ::= <disj-rule> <eq-sign> <equ-rule>
<disj-rule>  ::= <impl-rule> '+' <disj-rule>
<impl-rule>  ::= <conj-rule> '->' <impl-rule>
<conj-rule>  ::= <quant-rule> '&' <conj-rule>
<quant-rule> ::= [ <quant-list> ] <term-rule>
<term-rule>  ::= '(' <rule> ')' | <neg> | <atom>
<neg>        ::= '~' <quant-rule>
<equ-sign>   ::= '==' | '=' | '<=>' | '<->'
<atom>       ::= string [ '(' <atom-list> ')' ]
<atom-list>  ::= <atom> [ ',' <atom-list> ]
<quant-list> ::= <quantifier> [ <quant-list> ]
<quantifier> ::= '\forall' '(' <var-list> ')'
               | '\exists' '(' <var-list> ')'
<var-list>   ::= string [ ',' <var-list> ]

*/
class RuleParser {
public:
  Rule::ptr Parse(const char *str);

private:
  Rule::ptr ParseEquRule();
  Rule::ptr ParseDisjRule();
  Rule::ptr ParseImplRule();
  Rule::ptr ParseConjRule();
  Rule::ptr ParseQuantRule();
  Rule::ptr ParseTermRule();
  Rule::ptr ParseAtom();

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
