#include "atom.h"
#include "variable.h"

Atom::Atom(std::string name, std::vector<Variable::ptr> arguments)
    : m_name(std::move(name)), m_arguments(std::move(arguments)) {}

std::set<std::string> Atom::getAllVars() const {
  std::set<std::string> vars;
  for (auto &arg : m_arguments)
    arg->getAllVarsRecursive(vars);
  return vars;
}

std::string Atom::toString() const {
  std::string res = m_name;
  if (!m_arguments.empty()) {
    res += '(';
    bool first = true;
    for (auto &arg : m_arguments) {
      if (!first)
        res += ", ";
      first = false;
      res += arg->toString();
    }
    res += ')';
  }
  return res;
}