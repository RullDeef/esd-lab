
#include "variable.h"
#include <algorithm>
#include <memory>

Variable::Variable(bool isConst, bool isQuoted, std::string value,
                   std::vector<Variable::ptr> arguments)
    : m_isConst(isConst), m_isQuoted(isQuoted), m_value(std::move(value)),
      m_arguments(std::move(arguments)) {}

Variable::ptr Variable::createConst(std::string value) {
  return std::make_shared<Variable>(true, false, std::move(value),
                                    std::vector<ptr>{});
}

Variable::ptr Variable::createString(std::string value) {
  return std::make_shared<Variable>(true, true, std::move(value),
                                    std::vector<ptr>{});
}

Variable::ptr Variable::createVariable(std::string name) {
  return std::make_shared<Variable>(false, false, std::move(name),
                                    std::vector<ptr>{});
}

Variable::ptr Variable::createFuncSym(std::string name, std::vector<ptr> args) {
  return std::make_shared<Variable>(false, false, std::move(name),
                                    std::move(args));
}

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
    return createVariable(allocator.allocateRenaming(m_value));
  std::vector<Variable::ptr> arguments;
  std::transform(
      m_arguments.begin(), m_arguments.end(), std::back_inserter(arguments),
      [&allocator](auto &var) { return var->renamedVars(allocator); });
  return createFuncSym(m_value, std::move(arguments));
}

void Variable::getAllVarsRecursive(std::set<std::string> &vars,
                                   const VariableListNode *vlist) const {
  if (m_isConst || hasSelf(vlist))
    return;
  if (m_arguments.empty()) {
    vars.insert(m_value);
    return;
  }
  VariableListNode next = {this, vlist};
  for (auto &arg : m_arguments)
    arg->getAllVarsRecursive(vars, &next);
}

const std::string &Variable::getValue() const { return m_value; }
const std::vector<Variable::ptr> Variable::getArguments() const {
  return m_arguments;
}

void Variable::updateArgument(size_t i, ptr value) { m_arguments[i] = value; }

Variable::ptr Variable::clone(std::map<Variable *, ptr> *varMap) {
  if (m_isConst || m_arguments.empty())
    return shared_from_this(); // safe omit clone
  if (varMap == nullptr) {
    std::map<Variable *, ptr> defaultVarMap;
    defaultVarMap[this] = std::make_shared<Variable>(*this);
    for (int i = 0; i < m_arguments.size(); ++i)
      defaultVarMap[this]->updateArgument(
          i, m_arguments[i]->clone(&defaultVarMap));
    return defaultVarMap[this];
  } else {
    if (varMap->count(this) != 0)
      return varMap->at(this);
    (*varMap)[this] = std::make_shared<Variable>(*this);
    for (size_t i = 0; i < m_arguments.size(); ++i)
      (*varMap)[this]->updateArgument(i, m_arguments[i]->clone(varMap));
    return (*varMap)[this];
  }
}

std::string Variable::toString() const { return toString(nullptr); }

std::string Variable::toString(const VariableListNode *vlist) const {
  if (hasSelf(vlist))
    return "...";
  if (m_arguments.empty())
    return m_isQuoted ? '\"' + m_value + '\"' : m_value;
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
