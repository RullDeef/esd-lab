#pragma once

#include "atom.h"

class Rule {
public:
  Rule(std::vector<Atom> inputs = {{}}, Atom output = {});

  const std::vector<Atom> &getInputs() const { return m_inputs; }
  const Atom &getOutput() const { return m_output; }

  std::string toString() const;

private:
  std::vector<Atom> m_inputs;
  Atom m_output;
};

namespace std {
static inline string to_string(const Rule &rule) { return rule.toString(); }
} // namespace std
