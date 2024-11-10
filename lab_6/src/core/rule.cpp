#include "rule.h"
#include <stdexcept>

Rule::Rule(std::vector<Atom> inputs, Atom output)
    : m_inputs(std::move(inputs)), m_output(std::move(output)) {
  if (m_inputs.empty())
    throw std::runtime_error("empty source list in rule");
}

std::string Rule::toString() const {
  std::string res = m_inputs.front().toString();
  auto iter = m_inputs.begin();
  while (++iter != m_inputs.end())
    res += " & " + iter->toString();
  res += " -> " + m_output.toString();
  return res;
}
