
#pragma once

#include "name_allocator.h"
#include <memory>
#include <set>
#include <string>
#include <vector>

class Variable;

struct VariableListNode {
  const Variable *var;
  const VariableListNode *prev;
};

static inline bool isVar(const std::string &name) {
  return name == "_" ||
         name.size() > 0 && std::isalpha(name[0]) && !std::isupper(name[0]);
}

class Variable : public std::enable_shared_from_this<Variable> {
public:
  using ptr = std::shared_ptr<Variable>;

  Variable(bool isConst, bool isQuoted, std::string value,
           std::vector<Variable::ptr> arguments);

  static ptr createConst(std::string value);
  static ptr createString(std::string value);
  static ptr createVariable(std::string name);
  static ptr createFuncSym(std::string name, std::vector<ptr> args);

  bool isConst() const;
  bool isVariable() const;
  bool isFuncSym() const;

  bool hasVars(const VariableListNode *vlist = nullptr) const;
  void commitVarNames(NameAllocator &allocator) const;
  Variable::ptr renamedVars(NameAllocator &allocator);

  void getAllVarsRecursive(std::set<std::string> &vars,
                           const VariableListNode *vlist = nullptr) const;

  const std::string &getValue() const;
  const std::vector<Variable::ptr> getArguments() const;

  void updateArgument(size_t i, Variable::ptr value);

  Variable::ptr clone(std::map<Variable *, Variable::ptr> *varMap = nullptr);

  std::string toString() const;

private:
  std::string toString(const VariableListNode *vlist) const;

  bool hasSelf(const VariableListNode *list) const;

  bool m_isConst;
  bool m_isQuoted;
  std::string m_value;
  std::vector<Variable::ptr> m_arguments;
};
