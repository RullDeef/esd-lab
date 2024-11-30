#include "atom.h"
#include "utils.h"
#include "variable.h"
#include <algorithm>

Atom::Atom(bool inverse, std::string name, std::vector<Variable::ptr> arguments)
    : m_inverse(inverse), m_name(std::move(name)),
      m_arguments(std::move(arguments)) {}

bool Atom::isInverse() const { return m_inverse; }
const std::string &Atom::getName() const { return m_name; }
const std::vector<Variable::ptr> &Atom::getArguments() const {
  return m_arguments;
}

void Atom::commitVarNames(NameAllocator &allocator) const {
  for (auto &var : m_arguments)
    var->commitVarNames(allocator);
}

Atom Atom::renamedVars(NameAllocator &allocator) const {
  std::vector<Variable::ptr> arguments;
  std::transform(
      m_arguments.begin(), m_arguments.end(), std::back_inserter(arguments),
      [&allocator](auto &var) { return var->renamedVars(allocator); });
  return Atom(m_inverse, m_name, std::move(arguments));
}

std::string Atom::toString() const {
  std::string res;
  if (m_inverse)
    res = "~";
  res += m_name;
  if (!m_arguments.empty())
    res += "(" +
           toStringSep(m_arguments.begin(), m_arguments.end(), ", ",
                       mem_ptr_fn(&Variable::toString)) +
           ")";
  return res;
}
