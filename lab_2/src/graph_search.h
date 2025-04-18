#pragma once

#include "node.h"
#include "rule.h"
#include <list>
#include <map>

class GraphSearchRev {
public:
  GraphSearchRev(std::list<Rule> rules, std::vector<int> srcNode, int dstNode);

  /* запуск поиска в графе в глубину от цели */
  std::list<int> DoDepthFirstSearch();

  const std::list<int> &GetClosedNodes() const { return m_closedNodes; }
  const std::list<int> &GetClosedRules() const { return m_closedRules; }
  std::list<int> GetForbiddenNodes() const;
  std::list<int> GetForbiddenRules() const;

private:
  /* метод потомки для поиска в глубину */
  int DescendantsDFS(int node);

  /* текущая вершина запрещается */
  void Backtrack(int node);

  void Mark(int node);
  /* отладочная печать состояния поиска */
  void ShowState(bool header = false);

  std::list<Rule> m_rules;
  std::map<int, Node> m_nodes;
  std::map<int, Rule *> m_rulesRef;
  std::list<int> m_openNodes;
  std::list<int> m_openRules;
  std::list<int> m_closedNodes;
  std::list<int> m_closedRules;
  std::vector<int> m_srcNodes;
  bool m_foundSolution = false;
  bool m_noSolution = false;
};
