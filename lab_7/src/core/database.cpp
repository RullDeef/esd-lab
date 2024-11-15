#include "database.h"
#include "name_allocator.h"
#include "parser.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>

Database::Database(const char *filename) {
  if (filename == nullptr)
    return; // load nothing
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "warning: failed to open database " << filename << std::endl;
    return;
  }
  int lineNumber = 0;
  std::string line;
  while (std::getline(file, line)) {
    lineNumber++;
    if (line.empty() || line[0] == '#')
      continue;
    try {
      auto rule = RuleParser().ParseRule(line.c_str());
      addRule(rule);
    } catch (std::exception &errRule) {
      std::cerr << filename << ":" << lineNumber
                << ": parse error: " << errRule.what() << std::endl;
    }
  }
}

size_t Database::rulesCount() const { return m_rules.size(); }

const std::list<Rule> &Database::getRules() const { return m_rules; }

const Rule &Database::getRule(size_t index) const {
  auto iter = m_rules.begin();
  std::advance(iter, index);
  return *iter;
}

const Rule &Database::addRule(const Rule &rule) {
  auto inputs = rule.getInputs();
  auto output = rule.getOutput();
  for (size_t i = 0; i < inputs.size(); ++i) {
    std::vector<std::string> args;
    for (auto &var : inputs[i].getArguments())
      args.push_back(isVar(var) ? m_allocator.allocateRenaming(var) : var);
    inputs[i] = Atom(inputs[i].getName(), std::move(args));
  }
  std::vector<std::string> args;
  for (auto &var : output.getArguments())
    args.push_back(isVar(var) ? m_allocator.allocateRenaming(var) : var);
  m_allocator.commit();
  output = Atom(output.getName(), std::move(args));
  auto newRule = Rule(std::move(inputs), std::move(output));
  std::cout << ">> " << newRule.toString() << std::endl;
  m_rules.push_back(std::move(newRule));
  return m_rules.back();
}

void WorkingDataset::addFact(Atom fact) {
  m_facts[fact.getName()].push_back({std::move(fact), m_iteration});
}

bool WorkingDataset::hasFact(Atom fact) {
  const auto &factList = m_facts[fact.getName()];
  return std::find(factList.begin(), factList.end(), fact) != factList.end();
}
void WorkingDataset::nextIteration() { m_iteration++; }

bool WorkingDataset::hasNewFactFor(const Atom &atom) const {
  if (m_facts.count(atom.getName()) != 0)
    for (auto &fact : m_facts.at(atom.getName()))
      if (fact.getGen() + 1 >= m_iteration)
        return true;
  return false;
}
