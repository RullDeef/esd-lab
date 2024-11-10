#pragma once

#include <map>
#include <optional>
#include <string>

class Subst {
public:
  bool insert(const std::string &var, std::string value);

  const std::string &apply(const std::string &var) const;

  std::optional<Subst> operator+(const Subst &other) const;

  std::string toString() const;

private:
  std::map<std::string, std::string> m_pairs;
};

namespace std {
static inline string to_string(const Subst &subst) { return subst.toString(); }
} // namespace std
