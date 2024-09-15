#pragma once

#include <list>
#include <map>
#include "node.h"
#include "rule.h"

class GraphSearch
{
public:
    GraphSearch(std::list<Rule> rules, int srcNode, int dstNode);

    /* запуск поиска в графе в глубину */ 
    std::list<int> DoDepthFirstSearch();

    /* запуск поиска в графе в ширину */
    std::list<int> DoBreadthFirstSearch();

private:
    /* метод потомки для поиска в глубину */
    int DescendantsDFS(int node);

    /* метод потомки для поиска в ширину */
    int DescendantsBFS(int node);

    /* отладочная печать состояния поиска */
    void ShowState(bool header = false);

    std::list<Rule> m_rules;
    std::map<int, Node> m_nodes;
    std::list<int> m_openNodes;
    std::list<int> m_closedNodes;
    int m_dstNode;
    bool m_foundSolution = false;
    bool m_noSolution = false;
};
