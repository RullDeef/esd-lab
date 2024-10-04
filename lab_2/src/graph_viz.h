#pragma once

#include "rule.h"
#include <list>
#include <string>
#include <vector>

class GraphViz {
public:
  GraphViz &DefineGraph(const std::list<Rule> &rules);
  GraphViz &DrawSourceNodes(const std::vector<int>& sourceNodes);
  GraphViz &DrawDestinationNode(int node);
  GraphViz &DrawPath(const std::list<int> &ruleNumbers);
  void Export(const char *filename);

private:
  std::list<std::string> m_graphDefinitions;
};
