#include "parser.h"
#include "variable.h"
#include <cstring>
#include <memory>
#include <stdexcept>

/*
  EBNF rule grammar:

  <rule>        ::= (<atom> | <rule-math> | <rule-prolog>) [ '.' ]
  <rule-math>   ::= <atom-list> '->' <atom>
  <rule-prolog> ::= <atom> ':-' <atom-list>
  <atom-list>   ::= <atom> [ (',' | '&') <atom-list> ]
  <atom>        ::= IDENT [ '(' <arg-list> ')' ]
  <arg-list>    ::= IDENT [ ',' <arg-list> ]
*/

Rule RuleParser::ParseRule(const char *str) {
  m_source = str;
  m_pos = 0;
  Atom firstAtom = ParseAtom();
  if (!SkipWhitespace())
    return Rule(std::move(firstAtom));
  if (Eat(".")) {
    if (SkipWhitespace())
      RaiseError("junk after end");
    return Rule(std::move(firstAtom));
  }
  if (Eat(":-")) {
    auto sources = ParseAtomList();
    Eat(".");
    if (SkipWhitespace())
      RaiseError("rule not fully parsed");
    return Rule(std::move(sources), std::move(firstAtom));
  }
  m_pos = 0;
  auto sources = ParseAtomList();
  if (!Eat("->"))
    RaiseError("'&' or ',' or '->' expected");
  auto target = ParseAtom();
  Eat(".");
  if (SkipWhitespace())
    RaiseError("rule not fully parsed");
  return Rule(std::move(sources), std::move(target));
}

std::vector<Atom> RuleParser::ParseAtomList() {
  std::vector<Atom> atoms;
  do {
    auto atom = ParseAtom();
    atoms.push_back(std::move(atom));
  } while (Eat("&") || Eat(","));
  return atoms;
}

Atom RuleParser::ParseAtom() {
  auto name = ParseIdent();
  if (!Eat("("))
    return Atom(std::move(name));
  std::vector<Variable::ptr> args;
  do {
    args.push_back(ParseArg());
  } while (Eat(","));
  if (!Eat(")"))
    RaiseError("')' expected");
  return Atom(std::move(name), std::move(args));
}

Variable::ptr RuleParser::ParseArg() {
  auto name = ParseIdent();
  std::vector<Variable::ptr> args;
  if (Eat("(")) {
    do {
      args.push_back(ParseArg());
    } while (Eat(","));
    if (!Eat(")"))
      RaiseError("')' expected");
  }
  const bool isConst = !isVar(name) && args.empty();
  return std::make_shared<Variable>(isConst, std::move(name), std::move(args));
}

std::string RuleParser::ParseIdent() {
  SkipWhitespace();
  size_t size = 0;
  while (m_source[m_pos + size] && (std::isalnum(m_source[m_pos + size]) ||
                                    m_source[m_pos + size] == '_'))
    size++;
  if (size == 0)
    RaiseError("identifier expected");
  auto name = std::string(m_source + m_pos, m_source + m_pos + size);
  m_pos += size;
  return std::move(name);
}

bool RuleParser::Peek(const char *expected) {
  return SkipWhitespace() &&
         strncmp(m_source + m_pos, expected, strlen(expected)) == 0;
}

bool RuleParser::Eat(const char *value) {
  if (!Peek(value))
    return false;
  m_pos += std::strlen(value);
  return true;
}

bool RuleParser::SkipWhitespace() {
  while (m_source[m_pos] && m_source[m_pos] == ' ')
    m_pos++;
  return m_source[m_pos] != '\0';
}

void RuleParser::RaiseError(const char *message) {
  std::string message_str = message;
  message_str += " at " + std::to_string(m_pos);
  throw std::runtime_error(message_str);
}
