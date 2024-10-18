#pragma once

#include "node.h"
#include "rule.h"
#include <list>
#include <map>

class GraphSearch {
public:
  GraphSearch(std::list<Rule> rules, std::vector<int> srcNode, int dstNode);

  /* поиск в ширину от данных */
  std::list<int> DoBreadthFirstSearch();

  const std::list<int> &GetClosedNodes() const { return m_closedNodes; }
  const std::list<int> &GetClosedRules() const { return m_closedRules; }

private:
  /* добавление потомков в очередь открытых вершин */
  void Step();

  /* формирование дерева решения */
  std::list<int> Mark();

  /* отладочная печать состояния поиска */
  void ShowState(bool header = false);

  std::list<Rule> m_rules;
  std::vector<int> m_srcNodes;
  int m_dstNode;
  std::map<int, Node> m_nodes;
  std::map<int, Rule *> m_rulesRef;
  std::list<int> m_closedNodes;
  std::list<int> m_closedRules;
  bool m_foundSolution = false;
  bool m_noSolution = false;
};
