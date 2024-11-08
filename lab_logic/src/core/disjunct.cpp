#include "disjunct.h"
#include "utils.h"
#include <algorithm>
#include <functional>

Disjunct::Disjunct(std::vector<Atom> atoms) : m_atoms(std::move(atoms)) {}

Disjunct Disjunct::withoutNth(size_t i) const {
  std::vector<Atom> newAtoms(m_atoms);
  newAtoms.erase(newAtoms.begin() + i);
  return Disjunct(std::move(newAtoms));
}

Disjunct Disjunct::operator+(const Disjunct &other) const {
  std::vector<Atom> newAtoms(m_atoms);
  newAtoms.insert(newAtoms.end(), other.m_atoms.begin(), other.m_atoms.end());
  return Disjunct(std::move(newAtoms));
}

void Disjunct::commitVarNames(NameAllocator &allocator) const {
  for (auto &atom : m_atoms)
    atom.commitVarNames(allocator);
}

Disjunct Disjunct::renamedVars(NameAllocator &allocator) const {
  std::vector<Atom> atoms;
  std::transform(
      m_atoms.begin(), m_atoms.end(), std::back_inserter(atoms),
      [&allocator](auto &atom) { return atom.renamedVars(allocator); });
  allocator.commit();
  return Disjunct(std::move(atoms));
}

bool Disjunct::operator<(const Disjunct &other) const {
  return m_atoms.size() < other.m_atoms.size();
}

const Atom &Disjunct::operator[](size_t i) const { return m_atoms[i]; }

size_t Disjunct::size() const { return m_atoms.size(); }

std::vector<Atom>::const_iterator Disjunct::begin() const {
  return m_atoms.begin();
}
std::vector<Atom>::const_iterator Disjunct::end() const {
  return m_atoms.end();
}

std::string Disjunct::toString() const {
  return toStringSep(m_atoms.begin(), m_atoms.end(), " + ",
                     std::mem_fn(&Atom::toString));
}
