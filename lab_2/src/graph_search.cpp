#include "graph_search.h"
#include <algorithm>
#include <iomanip>
#include <iostream>

GraphSearch::GraphSearch(std::list<Rule> rules, std::vector<int> srcNodes,
                         int dstNode)
    : m_rules(std::move(rules)), m_srcNodes(std::move(srcNodes)) {
  if (m_srcNodes.size() == 0)
    m_noSolution = true;
  else if (std::find(m_srcNodes.begin(), m_srcNodes.end(), dstNode) !=
           srcNodes.end())
    m_foundSolution = true;
  else {
    for (auto node : m_srcNodes)
      m_nodes[node].closed = true;
    m_openNodes.push_back(dstNode);
  }
}

std::list<int> GraphSearch::DoDepthFirstSearch() {
  ShowState(true);
  while (!m_foundSolution && !m_noSolution) {
    /* достаем вершину из списка открытых вершин (стек) */
    int node = m_openNodes.back();
    /* вызываем метод поиска потомков - заполняем список открытых правил */
    int count = DescendantsDFS(node);
    /* Если установили флаг решения в потомках, то решение получено (путь в
     * стеке) */
    if (m_foundSolution)
      break;
    else if (count == 0 && !m_openNodes.empty()) {
      // do backtracking
      // m_nodes[node].forbidden = true;
      // m_openNodes.pop_back();
    } else if (m_openNodes.empty())
      m_noSolution = true;
    ShowState();
  }
  if (m_noSolution)
    return {};
  /* раскрутка стека решения */
  std::list<int> solution;
  int lastNode = m_dstNode;
  for (auto node = m_openNodes.rbegin(); node != m_openNodes.rend(); ++node) {
    for (auto rule : m_rules) {
      if (rule.srcNode == *node && rule.dstNode == lastNode) {
        solution.push_front(rule.number);
        lastNode = *node;
        break;
      }
    }
  }
  return solution;
}

int GraphSearch::DescendantsDFS(int node) {
  int count = 0;
  for (auto &rule : m_rules) {
    if (rule.visited || rule.dstNode != node)
      continue;
    // check that source nodes are not forbidden
    bool rule_forbidden = false;
    bool all_closed = true;
    for (auto srcNode : rule.srcNodes) {
      const Node &nodeRef = m_nodes[srcNode];
      if (nodeRef.forbidden) {
        rule_forbidden = true;
        break;
      }
      all_closed = all_closed && nodeRef.closed;
    }
    if (rule_forbidden)
      continue;
    m_openRules.push_back(rule.number);
    /* проверяем, что нашли решение */
    if (all_closed) {
      m_foundSolution = true;
      break;
    }
    for (auto srcNode : rule.srcNodes)
      if (!m_nodes[srcNode].closed)
        m_openNodes.push_back(srcNode);
    rule.visited = true;
    ++count;
  }
  return count;
}

void GraphSearch::ShowState(bool header) {
  if (header)
    std::cout << "  open nodes  | closed nodes " << std::endl
              << "--------------+--------------" << std::endl;
  const size_t rows =
      std::max((3 + m_openNodes.size()) / 4, (3 + m_closedNodes.size()) / 4);
  for (size_t row = 0; row < rows; ++row) {
    std::cout << " ";
    for (size_t i = 4 * row; i < 4 * (row + 1); ++i) {
      if (i < m_openNodes.size()) {
        auto it = m_openNodes.begin();
        std::advance(it, i);
        std::cout << std::setw(3) << *it;
      } else
        std::cout << "   ";
    }
    std::cout << " | ";
    for (size_t i = 4 * row; i < 4 * (row + 1); ++i) {
      if (i < m_closedNodes.size()) {
        auto it = m_closedNodes.begin();
        std::advance(it, i);
        std::cout << std::setw(3) << *it;
      } else
        std::cout << "   ";
    }
    std::cout << std::endl;
  }
  std::cout << "--------------+--------------" << std::endl;
}
