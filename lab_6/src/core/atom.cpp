#include "atom.h"

Atom::Atom(std::string name, std::vector<std::string> arguments)
    : m_name(std::move(name)), m_arguments(std::move(arguments)) {}

std::string Atom::toString() const {
  std::string res = m_name;
  if (!m_arguments.empty()) {
    res += '(';
    bool first = true;
    for (auto &arg : m_arguments) {
      if (!first)
        res += ", ";
      first = false;
      res += arg;
    }
    res += ')';
  }
  return res;
}
