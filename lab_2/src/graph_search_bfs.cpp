#include "graph_search_bfs.h"
#include "graph_search.h"
#include <algorithm>
#include <iostream>

GraphSearch::GraphSearch(std::list<Rule> rules, std::vector<int> srcNodes,
                         int dstNode)
    : m_rules(std::move(rules)), m_srcNodes(std::move(srcNodes)),
      m_dstNode(dstNode) {
  if (m_srcNodes.size() == 0)
    m_noSolution = true;
  else if (std::find(m_srcNodes.begin(), m_srcNodes.end(), dstNode) !=
           m_srcNodes.end())
    m_foundSolution = true;
  else {
    for (auto node : m_srcNodes) {
      m_closedNodes.push_back(node);
      m_nodes[node].closed = true;
    }
    for (auto &rule : m_rules)
      m_rulesRef[rule.number] = &rule;
  }
}

std::list<int> GraphSearch::DoBreadthFirstSearch() {
  ShowState(true);
  while (!m_foundSolution && !m_noSolution) {
    Step();
    ShowState();
  }
  if (m_noSolution)
    return {};
  return Mark();
}

void GraphSearch::Step() {
  std::vector<Rule> closingRules;
  for (auto &rule : m_rules) {
    if (rule.visited) {
      continue;
    }
    // Проверяем покрытие входов текущего правила
    bool all_inputs_closed = true;
    for (auto &node : rule.srcNodes) {
      if (!m_nodes[node].closed) {
        all_inputs_closed = false;
        break;
      }
    }
    // Если покрытие выполняется - закрываем правило
    if (all_inputs_closed) {
      rule.visited = true;
      closingRules.push_back(rule);
    }
  }
  for (auto &rule : closingRules) {
    m_closedRules.push_back(rule.number);
    m_closedNodes.push_back(rule.dstNode);
    m_nodes[rule.dstNode].closed = true;
    if (rule.dstNode == m_dstNode) {
      m_foundSolution = true;
      break;
    }
  }
  if (closingRules.empty())
    m_noSolution = true;
}

std::list<int> GraphSearch::Mark() {
  std::list<Rule> closedRules;
  for (const auto &rule : m_closedRules) {
    Rule newRule{};
    newRule.number = rule;
    newRule.srcNodes = m_rulesRef[rule]->srcNodes;
    newRule.dstNode = m_rulesRef[rule]->dstNode;
    closedRules.push_back(newRule);
  }
  // производим поиск в глубину от цели по закрытым правилам
  GraphSearchRev gsr(std::move(closedRules), m_srcNodes, m_dstNode);
  return gsr.DoDepthFirstSearch();
}

void GraphSearch::ShowState(bool header) {
  if (header)
    std::cout << "+------------------+" << std::endl
              << "| closed nodes (CN)|" << std::endl
              << "| closed rules (CR)|" << std::endl;
  std::cout << "+------------------+" << std::endl;
  std::cout << "|CN:";
  for (auto node : m_closedNodes)
    std::cout << ' ' << node;
  std::cout << std::endl << "|CR:";
  for (auto rule : m_closedRules)
    std::cout << ' ' << rule;
  std::cout << std::endl;
}
