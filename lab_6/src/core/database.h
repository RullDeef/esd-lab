#pragma once

#include "name_allocator.h"
#include "rule.h"
#include <list>

class Database {
public:
  explicit Database(const char *filename = nullptr);

  size_t rulesCount() const;
  size_t factsCount() const;

  const Rule &getRule(size_t index) const;
  const Atom &getFact(size_t index) const;

  const std::list<Rule> &getRules() const;
  const std::list<Atom> &getFacts() const;

  // добавить правило в базу данных, переименовывая переменные
  void addRule(const Rule &rule);

private:
  std::list<Rule> m_rules;
  std::list<Atom> m_facts;

  NameAllocator m_allocator;
};

class WorkingDataset {
public:
  class AtomEx : public Atom {
  public:
    AtomEx(Atom &&atom, size_t gen = 0) : Atom(atom), m_gen(gen) {}

    size_t getGen() const { return m_gen; }

  private:
    size_t m_gen;
  };

  void addFact(Atom fact);
  void nextIteration();

  bool hasNewFactFor(const Atom &atom) const;

  const std::list<AtomEx> &getFacts(const std::string &name) {
    return m_facts[name];
  }

private:
  std::map<std::string, std::list<AtomEx>> m_facts;
  size_t m_iteration = 0;
};
