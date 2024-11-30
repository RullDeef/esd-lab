#include "database.h"
#include "name_allocator.h"
#include "parser.h"
#include "variable.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>

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
  for (size_t i = 0; i < inputs.size(); ++i)
    inputs[i] = renameVars(inputs[i]);
  auto output = renameVars(rule.getOutput());
  m_allocator.commit();
  auto newRule = Rule(std::move(inputs), std::move(output));
  std::cout << ">> " << newRule.toString() << std::endl;
  m_rules.push_back(std::move(newRule));
  return m_rules.back();
}

Atom Database::renameVars(const Atom &atom) {
  std::vector<Variable::ptr> args;
  for (auto &arg : atom.getArguments())
    args.push_back(renameVars(arg));
  return Atom(atom.getName(), std::move(args));
}

Variable::ptr Database::renameVars(const Variable::ptr &var) {
  if (var->isConst())
    return var;
  if (var->isVariable())
    return Variable::createVariable(
        m_allocator.allocateRenaming(var->getValue()));
  std::vector<Variable::ptr> args;
  for (auto &arg : var->getArguments())
    args.push_back(renameVars(arg));
  return Variable::createFuncSym(var->getValue(), std::move(args));
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
