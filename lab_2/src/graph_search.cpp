#include "graph_search.h"
#include <algorithm>
#include <iostream>

GraphSearch::GraphSearch(std::list<Rule> rules, std::vector<int> srcNodes,
                         int dstNode)
    : m_rules(std::move(rules)), m_srcNodes(std::move(srcNodes)) {
  if (m_srcNodes.size() == 0)
    m_noSolution = true;
  else if (std::find(m_srcNodes.begin(), m_srcNodes.end(), dstNode) !=
           m_srcNodes.end())
    m_foundSolution = true;
  else {
    for (auto node : m_srcNodes)
      m_nodes[node].closed = true;
    for (auto &rule : m_rules)
      m_rulesRef[rule.number] = &rule;
    m_openNodes.push_back(dstNode);
  }
}

std::list<int> GraphSearch::DoDepthFirstSearch() {
  ShowState(true);
  while (!m_foundSolution && !m_noSolution) {
    int node = m_openNodes.back();
    int count = DescendantsDFS(node);
    if (m_foundSolution) {
      Mark(node);
      if (m_foundSolution)
        break;
    } else if (count == 0 && !m_openNodes.empty())
      Backtrack(node);
    else if (m_openNodes.empty()) {
      m_noSolution = true;
      break;
    }
    ShowState();
  }
  if (m_noSolution)
    return {};
  return m_closedRules;
}

int GraphSearch::DescendantsDFS(int node) {
  int count = 0;
  for (auto &rule : m_rules) {
    if (rule.visited || rule.dstNode != node || rule.forbidden)
      continue;
    // check that source nodes are not forbidden
    bool all_closed = true;
    for (auto srcNode : rule.srcNodes) {
      const Node &nodeRef = m_nodes[srcNode];
      if (nodeRef.forbidden) {
        rule.forbidden = true;
        break;
      }
      all_closed = all_closed && nodeRef.closed;
    }
    if (rule.forbidden)
      continue;
    m_openRules.push_back(rule.number);
    /* check that all inputs satisfied */
    if (all_closed) {
      m_closedRules.push_back(rule.number);
      m_foundSolution = true;
      break;
    }
    m_rulesRef[rule.number]->openIndex = m_openNodes.size();
    for (auto srcNode : rule.srcNodes)
      if (!m_nodes[srcNode].closed)
        m_openNodes.push_back(srcNode);
    rule.visited = true;
    ++count;
  }
  return count;
}

void GraphSearch::Backtrack(int node) {
  // node at top of open node stack needs to be forbidden
  m_nodes[node].forbidden = true;
  // then rule at top of open rules stack needs to be forbidden
  int ruleNum = m_openRules.back();
  auto ruleRef = m_rulesRef[ruleNum];
  int dstNode = ruleRef->dstNode;
  ruleRef->forbidden = true;
  //  all input nodes of that rule must be removed from open nodes
  for (const auto &rule : m_rules) {
    if (rule.number == ruleNum) {
      while (ruleRef->openIndex < m_openNodes.size())
        m_openNodes.pop_back();
      break;
    }
  }
  // new open rule must be searched to close previous node in open stack
  // 1. check if previous open rule resolves current top of open nodes
  // 2. if true - continue with this rule
  //    if not - remove this rule and corresponding nodes and retry
  if (m_openRules.empty())
    m_noSolution = true;
  else if (m_rulesRef[m_openRules.back()]->dstNode != dstNode)
    Backtrack(dstNode);
}

void GraphSearch::Mark(int node) {
  // 0. mark given node as closed
  // 1. pop all rules that have same dst node
  // 2. if current top of opened rules has closed inputs - repeat
  // 3. dont forget to update m_foundSolution variable if real solution was
  // found!
  m_nodes[node].closed = true;
  while (!m_openRules.empty() &&
         m_rulesRef[m_openRules.back()]->dstNode == node)
    m_openRules.pop_back();
  // real solution found
  if (m_openRules.empty())
    return;
  m_openNodes.pop_back();
  // check current top
  int rule = m_openRules.back();
  bool all_closed = true;
  for (auto srcNode : m_rulesRef[rule]->srcNodes) {
    if (!m_nodes[srcNode].closed) {
      all_closed = false;
      break;
    }
  }
  if (all_closed)
    Mark(m_rulesRef[rule]->dstNode);
  else
    m_foundSolution = false;
}

void GraphSearch::ShowState(bool header) {
  if (header)
    std::cout << "+------------------+" << std::endl
              << "| open nodes   (ON)|" << std::endl
              << "| open rules   (OR)|" << std::endl
              << "| closed nodes (CN)|" << std::endl
              << "| closed rules (CR)|" << std::endl;
  std::cout << "+------------------+" << std::endl << "|ON:";
  for (auto node : m_openNodes)
    std::cout << ' ' << node;
  std::cout << std::endl << "|OR:";
  for (auto rule : m_openRules)
    std::cout << ' ' << rule;
  std::cout << std::endl << "|CN:";
  for (auto node : m_closedNodes)
    std::cout << ' ' << node;
  std::cout << std::endl << "|CR:";
  for (auto rule : m_closedRules)
    std::cout << ' ' << rule;
  std::cout << std::endl;
}
