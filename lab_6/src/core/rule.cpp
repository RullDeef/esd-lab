#include "rule.h"

Rule::Rule(std::vector<Atom> inputs, Atom output)
    : m_inputs(std::move(inputs)), m_output(std::move(output)) {}

Rule::Rule(Atom output) : m_output(std::move(output)) {}

bool Rule::mightProve(const Atom &target) const {
  return m_output.getName() == target.getName() &&
         m_output.getArguments().size() == target.getArguments().size();
}

std::string Rule::toString() const {
  std::string res = m_output.toString();
  if (!m_inputs.empty()) {
    auto iter = m_inputs.begin();
    res += " :- " + iter->toString();
    while (++iter != m_inputs.end())
      res += ", " + iter->toString();
  }
  return res;
}
