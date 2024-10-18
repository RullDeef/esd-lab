#include "resolver.h"
#include "rule_parser.h"
#include <iostream>
#include <sstream>

std::list<Rule::ptr> parseRules(const std::string &line) {
  if (line.empty())
    return {};
  std::list<Rule::ptr> rules;
  std::string ruleString;
  std::istringstream ss(line);
  while (ss.good()) {
    std::getline(ss, ruleString, ',');
    rules.push_back(RuleParser().Parse(ruleString.c_str()));
  }
  return rules;
}

void repl() {
  std::string line;
  RuleParser parser;
  std::list<Rule::ptr> axioms;
  Rule::ptr conclusion;
  while (true) {
    std::cout << "enter axioms (comma-separated): ";
    if (!std::getline(std::cin, line))
      break;
    try {
      axioms = parseRules(line);
    } catch (std::exception &err) {
      std::cout << "failed to parse: " << err.what() << std::endl;
      continue;
    }
    std::cout << "enter conclusion: ";
    if (!std::getline(std::cin, line))
      break;
    try {
      conclusion = parser.Parse(line.c_str());
    } catch (std::exception &err) {
      std::cout << "failed to parse: " << err.what() << std::endl;
      continue;
    }
    if (Resolver().Implies(axioms, conclusion))
      std::cout << "resoved: yes\n";
    else
      std::cout << "resolved: no\n";
  }
}

int main(int argc, char **argv) {
  std::cout << "repl for resolution method:\n";
  repl();
  return 0;
}
