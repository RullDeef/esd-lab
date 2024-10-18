#pragma once

#include "rule.h"
#include <list>
#include <map>
#include <string>
#include <vector>

class Dictionary {
public:
  explicit Dictionary(const char *filename);

  int FactsCount() const;
  const std::string &Fact(int node) const;
  const std::list<Rule> &Rules() const;

private:
  void Load(const char *filename);
  void AddRule(std::string precondition, std::string conclusion);
  std::vector<int> ParsePrecondition(const char *precondition);

  std::map<int, std::string> m_facts;
  std::map<std::string, int> m_factsInverse;
  std::list<Rule> m_rules;
};
