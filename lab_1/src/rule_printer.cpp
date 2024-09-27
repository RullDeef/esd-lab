#include "rule_printer.h"
#include <iostream>

RulePrinter::RulePrinter(std::shared_ptr<Dictionary> dictionary)
    : m_dict(std::move(dictionary)) {}

void RulePrinter::PrintRule(Rule rule) const {
  std::cout << "[" << rule.number << ":" << rule.srcNode << "->" << rule.dstNode
            << "](" << m_dict->Fact(rule.srcNode) << " -> "
            << m_dict->Fact(rule.dstNode) << ")" << std::endl;
}
