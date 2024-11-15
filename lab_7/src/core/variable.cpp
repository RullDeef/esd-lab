
#include "variable.h"
#include <algorithm>

Variable::Variable(bool isConst, std::string value,
                   std::vector<Variable::ptr> arguments)
    : m_isConst(isConst), m_value(std::move(value)),
      m_arguments(std::move(arguments)) {}

bool Variable::isConst() const { return m_isConst; }
bool Variable::isVariable() const { return !m_isConst && m_arguments.empty(); }
bool Variable::isFuncSym() const { return !m_arguments.empty(); }

bool Variable::hasVars(const VariableListNode *vlist) const {
  if (hasSelf(vlist))
    return false;
  if (!m_isConst && m_arguments.empty())
    return true;
  VariableListNode next = {this, vlist};
  for (const auto &arg : m_arguments)
    if (arg->hasVars(&next))
      return true;
  return false;
}

void Variable::commitVarNames(NameAllocator &allocator) const {
  if (m_isConst)
    return;
  if (m_arguments.empty())
    allocator.allocateName(m_value);
  else
    for (auto &arg : m_arguments)
      arg->commitVarNames(allocator);
}

Variable::ptr Variable::renamedVars(NameAllocator &allocator) {
  // does not work for recursive functions for now!!!
  if (m_isConst)
    return shared_from_this();
  if (m_arguments.empty())
    return std::make_shared<Variable>(false,
                                      allocator.allocateRenaming(m_value));
  std::vector<Variable::ptr> arguments;
  std::transform(
      m_arguments.begin(), m_arguments.end(), std::back_inserter(arguments),
      [&allocator](auto &var) { return var->renamedVars(allocator); });
  return std::make_shared<Variable>(false, m_value, std::move(arguments));
}

const std::string &Variable::getValue() const { return m_value; }
const std::vector<Variable::ptr> Variable::getArguments() const {
  return m_arguments;
}

void Variable::updateArgument(size_t i, Variable::ptr value) {
  m_arguments[i] = value;
}

std::string Variable::toString() const { return toString(nullptr); }

std::string Variable::toString(const VariableListNode *vlist) const {
  if (hasSelf(vlist))
    return "...";
  if (m_arguments.empty())
    return m_value;
  VariableListNode next = {this, vlist};
  std::string res = m_value;
  if (!m_arguments.empty()) {
    res += '(';
    bool first = true;
    for (auto &arg : m_arguments) {
      if (!first)
        res += ", ";
      first = false;
      res += arg->toString(&next);
    }
    res += ')';
  }
  return res;
}

bool Variable::hasSelf(const VariableListNode *list) const {
  if (list == nullptr)
    return false;
  if (list->var == this)
    return true;
  return hasSelf(list->prev);
}
