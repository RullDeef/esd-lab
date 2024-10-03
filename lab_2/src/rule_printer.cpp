#include "rule_printer.h"
#include <iostream>

RulePrinter::RulePrinter(std::shared_ptr<Dictionary> dictionary)
    : m_dict(std::move(dictionary)) {}

void RulePrinter::PrintRule(Rule rule) const {
  std::cout << "[" << rule.number << ":(";
  for (size_t i = 0; i < rule.srcNodes.size(); ++i) {
    if (i > 0)
      std::cout << "^";
    std::cout << rule.srcNodes[i];
  }
  std::cout << ")->" << rule.dstNode << "](";
  for (size_t i = 0; i < rule.srcNodes.size(); ++i) {
    if (i > 0)
      std::cout << " & ";
    std::cout << m_dict->Fact(rule.srcNodes[i]);
  }
  std::cout << " -> " << m_dict->Fact(rule.dstNode) << ")" << std::endl;
}
