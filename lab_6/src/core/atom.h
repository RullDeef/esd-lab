#pragma once

#include "name_allocator.h"
#include "variable.h"
#include <set>
#include <string>
#include <vector>

class Atom {
public:
  Atom(std::string name = "?", std::vector<Variable::ptr> arguments = {});

  const std::string &getName() const { return m_name; }
  const std::vector<Variable::ptr> &getArguments() const { return m_arguments; }

  // возвращает новый атом с переименованными переменными
  Atom renamedVars(NameAllocator &allocator) const;
  std::set<std::string> getAllVars() const;

  std::string toString() const;

  inline bool operator==(const Atom &other) const {
    return m_name == other.m_name && m_arguments == other.m_arguments;
  }

private:
  std::string m_name;
  std::vector<Variable::ptr> m_arguments;
};

namespace std {
static inline string to_string(const Atom &atom) { return atom.toString(); }
} // namespace std
