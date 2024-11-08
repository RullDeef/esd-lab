#pragma once

#include "atom.h"
#include <vector>

class Disjunct {
public:
  Disjunct(std::vector<Atom> atoms);

  Disjunct withoutNth(size_t i) const;
  Disjunct operator+(const Disjunct &other) const;

  void commitVarNames(NameAllocator &allocator) const;
  Disjunct renamedVars(NameAllocator &allocator) const;

  bool operator<(const Disjunct &other) const;
  const Atom &operator[](size_t i) const;

  size_t size() const;
  std::vector<Atom>::const_iterator begin() const;
  std::vector<Atom>::const_iterator end() const;

  std::string toString() const;

private:
  std::vector<Atom> m_atoms;
};
