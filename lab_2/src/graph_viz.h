#pragma once

#include "rule.h"
#include <list>
#include <string>
#include <vector>

class GraphViz {
public:
  GraphViz &DefineGraph(const std::list<Rule> &rules);
  GraphViz &DrawSourceNodes(const std::vector<int> &sourceNodes);
  GraphViz &DrawClosedNodes(const std::list<int> &nodes);
  GraphViz &DrawForbiddenNodes(const std::list<int> &nodes);
  GraphViz &DrawForbiddenRules(const std::list<int> &rules);
  GraphViz &DrawDestinationNode(int node);
  GraphViz &DrawPath(const std::list<int> &ruleNumbers);
  void Export(const char *filename);

private:
  template <typename Iter>
  void DrawNodes(Iter begin, Iter end, const char *color,
                 const char *fillcolor);
  void DrawRules(const std::list<int> &rules, const char *color,
                 const char *fillcolor);

  std::list<std::string> m_graphDefinitions;
};
