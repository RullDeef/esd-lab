#include "expr_parser.h"
#include "resolver.h"
#include <exception>
#include <iostream>
#include <sstream>

std::list<Expr::ptr> parseRules(const std::string &line) {
  if (line.empty())
    return {};
  std::list<Expr::ptr> rules;
  std::string ruleString;
  std::istringstream ss(line);
  while (ss.good()) {
    std::getline(ss, ruleString, ';');
    rules.push_back(ExprParser().Parse(ruleString.c_str()));
  }
  return rules;
}

void replResolve() {
  std::string line;
  ExprParser parser;
  std::list<Expr::ptr> axioms;
  Expr::ptr conclusion;
  std::cout << "enter 'end' to exit repl\n";
  while (true) {
    std::cout << "enter axioms (colon-separated): ";
    if (!std::getline(std::cin, line) || line == "end")
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
      std::cout << "resolved: yes\n";
    else
      std::cout << "resolved: no\n";
  }
}

void replUnify() {
  std::string line;
  ExprParser parser;
  Expr::ptr left;
  Expr::ptr right;
  std::cout << "enter 'end' to exit repl\n";
  while (true) {
    std::cout << "enter first atom: ";
    if (!std::getline(std::cin, line) || line == "end")
      break;
    try {
      left = parser.Parse(line.c_str());
    } catch (std::exception &err) {
      std::cout << "failed to parse: " << err.what() << std::endl;
      continue;
    }
    std::cout << "enter second atom: ";
    if (!std::getline(std::cin, line) || line == "end")
      break;
    try {
      right = parser.Parse(line.c_str());
    } catch (std::exception &err) {
      std::cout << "failed to parse: " << err.what() << std::endl;
      continue;
    }
    auto res = Resolver().unifyTerms(left, right);
    if (res)
      std::cout << "unified successfully, subst: " << res->toString()
                << std::endl;
    else
      std::cout << "unification failed" << std::endl;
  }
}

void repl() {
  std::string line;
  while (true) {
    std::cout << "select mode (u - unify, r - resolve): ";
    if (!std::getline(std::cin, line))
      break;
    if (line == "u")
      replUnify();
    else if (line == "r")
      replResolve();
    else
      std::cout << "unknown mode\n";
  }
}

int main(int argc, char **argv) {
  std::cout << "repl for resolution method:\n";
  repl();
  return 0;
}
