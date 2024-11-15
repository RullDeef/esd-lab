#include "subst.h"
#include "atom.h"
#include <optional>

bool Subst::insert(const std::string &var, std::string value) {
  if (var == "_" || value == "_")
    return true;
  auto linked = apply(var);
  value = apply(value);
  if (linked == value)
    return true;
  if (!isVar(linked)) {
    if (!isVar(value))
      return value == linked;
    m_pairs[value] = linked;
    return true;
  }
  m_pairs[linked] = value;
  return true;
}

const std::string &Subst::apply(const std::string &var) const {
  if (m_pairs.count(var) != 0)
    return apply(m_pairs.at(var));
  return var;
}

std::optional<Subst> Subst::operator+(const Subst &other) const {
  Subst res = *this;
  for (auto &[key, value] : other.m_pairs)
    if (!res.insert(key, value))
      return std::nullopt;
  return res;
}

std::string Subst::toString() const {
  std::string res = "{";
  bool first = true;
  for (auto &[key, value] : m_pairs) {
    if (!first)
      res += ", ";
    first = false;
    res += key;
    res += "=";
    res += value;
  }
  res += "}";
  return res;
}
