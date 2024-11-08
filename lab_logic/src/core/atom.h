#pragma once

#include "variable.h"
#include <string>
#include <vector>

class Atom {
public:
  Atom(bool inverse, std::string name,
       std::vector<Variable::ptr> arguments = {});

  bool isInverse() const;
  const std::string &getName() const;
  const std::vector<Variable::ptr> &getArguments() const;

  void commitVarNames(NameAllocator &allocator) const;
  Atom renamedVars(NameAllocator &allocator) const;

  std::string toString() const;

private:
  bool m_inverse;
  std::string m_name;
  std::vector<Variable::ptr> m_arguments;
};
