#include "rule_parser.h"
#include <cctype>
#include <cstring>
#include <iostream>
#include <optional>
#include <stdexcept>

/*

Rule Grammar in EBNF:

<rule>      ::= <equ-rule>
<equ-rule>  ::= <disj-rule> [<eq-sign> <equ-rule>]
<disj-rule> ::= <impl-rule> ['+' <disj-rule>]
<impl-rule> ::= <conj-rule> ['->' <impl-rule>]
<conj-rule> ::= <term-rule> ['&' <conj-rule>]
<term-rule> ::= '(' <equ-rule> ')' | <neg> | <atom>
<neg>       ::= '~' <term-rule>
<equ-sign>  ::= '==' | '=' | '<=>' | '<->'
<neg>       ::= '~' <term-rule>
<equ-sign>  ::= '==' | '=' | '<=>' | '<->'
<atom>      ::= string [ '(' <atom-list> ')' | <atom-list> ]
<atom-list> ::= <atom> [ ',' <atom-list> ]

*/

Rule::ptr RuleParser::Parse(const char *str) {
  m_Source = str;
  m_pos = 0;
  auto res = ParseEquRule();
  if (SkipWhitespace())
    RaiseError("rule not fully parsed");
  m_Source = nullptr;
  return res;
}

Rule::ptr RuleParser::ParseEquRule() {
  auto left = ParseDisjRule();
  if (!Eat("==") && !Eat("=") && !Eat("<=>") && !Eat("<->"))
    return left;
  auto right = ParseEquRule();
  return Rule::createEquality(left, right);
}

Rule::ptr RuleParser::ParseDisjRule() {
  auto left = ParseImplRule();
  if (!Eat("+"))
    return left;
  auto right = ParseDisjRule();
  return Rule::createDisjunction(left, right);
}

Rule::ptr RuleParser::ParseImplRule() {
  auto left = ParseConjRule();
  if (!Eat("->"))
    return left;
  auto right = ParseImplRule();
  return Rule::createImplication(left, right);
}

Rule::ptr RuleParser::ParseConjRule() {
  auto left = ParseQuantRule();
  if (!Eat("&"))
    return left;
  auto right = ParseConjRule();
  return Rule::createConjunction(left, right);
}

Rule::ptr RuleParser::ParseQuantRule() {
  if (!SkipWhitespace())
    RaiseError("quantifier non-term expected");
  if (Eat("\\exists")) {
    auto vars = ParseVarList();
    auto rule = ParseQuantRule();
    return Rule::createExists(std::move(vars), std::move(rule));
  } else if (Eat("\\forall")) {
    auto vars = ParseVarList();
    auto rule = ParseQuantRule();
    return Rule::createForAll(std::move(vars), std::move(rule));
  }
  return ParseTermRule();
}

Rule::ptr RuleParser::ParseTermRule() {
  if (Eat("(")) {
    auto rule = ParseEquRule();
    if (!Eat(")"))
      RaiseError("unmatched parenthesis");
    return rule;
  } else if (Eat("~"))
    return Rule::createInverse(ParseTermRule());
  return ParseAtom();
}

Rule::ptr RuleParser::ParseAtom() {
  auto name = ParseIdent();
  if (!name)
    RaiseError("atom expected");
  SkipWhitespace();
  std::vector<Rule::ptr> operands;
  // parse predicate arguments
  if (Eat("(")) {
    do {
      auto operand = ParseAtom();
      operands.push_back(std::move(operand));
    } while (Eat(","));
    if (!Eat(")"))
      RaiseError("')' expected");
  }
  return Rule::createPredicate(std::move(*name), std::move(operands));
}

std::set<std::string> RuleParser::ParseVarList() {
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

std::optional<std::string> RuleParser::ParseIdent() {
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

bool RuleParser::Peek(const char *expect) {
  return SkipWhitespace() &&
         strncmp(m_Source + m_pos, expect, strlen(expect)) == 0;
}

bool RuleParser::Eat(const char *value) {
  if (!Peek(value))
    return false;
  m_pos += strlen(value);
  return true;
}

bool RuleParser::SkipWhitespace() {
  while (m_Source[m_pos] && m_Source[m_pos] == ' ')
    m_pos++;
  return m_Source[m_pos] != '\0';
}

void RuleParser::RaiseError(const char *message) {
  std::string message_str = message;
  message_str += " at " + std::to_string(m_pos);
  throw std::runtime_error(message_str);
}
