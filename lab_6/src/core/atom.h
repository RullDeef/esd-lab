#pragma once

#include <string>
#include <vector>

class Atom {
public:
  Atom(std::string name = "?", std::vector<std::string> arguments = {});

  const std::string &getName() const { return m_name; }
  const std::vector<std::string> &getArguments() const { return m_arguments; }

  std::string toString() const;

private:
  std::string m_name;
  std::vector<std::string> m_arguments;
};

static inline bool isVar(const std::string &name) {
  return name.size() > 0 && std::isalpha(name[0]) && !std::isupper(name[0]);
}

namespace std {
static inline string to_string(const Atom &atom) { return atom.toString(); }
} // namespace std
