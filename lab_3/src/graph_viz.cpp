#include "graph_viz.h"
#include <fstream>
#include <set>
#include <sstream>

constexpr auto tmpFilename = ".tmp.graph.dot";

GraphViz &GraphViz::DefineGraph(const std::list<Rule> &rules) {
  std::set<int> nodes;
  for (const auto &rule : rules) {
    for (auto node : rule.srcNodes) {
      nodes.insert(node);
      std::stringstream ss;
      ss << "    Node" << node << " -> Rule" << rule.number
         << " [arrowsize = 0.5];\n";
      m_graphDefinitions.push_back(ss.str());
    }
    nodes.insert(rule.dstNode);
    {
      std::stringstream ss;
      ss << "    Rule" << rule.number << " -> Node" << rule.dstNode
         << " [arrowsize = 0.5];\n";
      m_graphDefinitions.push_back(ss.str());
    }
    {
      std::stringstream ss;
      ss << "    Rule" << rule.number << " [label = \"" << rule.number
         << "\", shape = square];\n";
      m_graphDefinitions.push_back(ss.str());
    }
  }
  for (auto node : nodes) {
    std::stringstream ss;
    ss << "    Node" << node << " [label = \"" << node
       << "\", shape = circle];\n";
    m_graphDefinitions.push_back(ss.str());
  }
  return *this;
}

GraphViz &GraphViz::DrawSourceNodes(const std::vector<int> &nodes) {
  DrawNodes(nodes.begin(), nodes.end(), "blue", "lightblue");
  return *this;
}

GraphViz &GraphViz::DrawClosedNodes(const std::list<int> &nodes) {
  DrawNodes(nodes.begin(), nodes.end(), "green", "lightgreen");
  return *this;
}

GraphViz &GraphViz::DrawForbiddenNodes(const std::list<int> &nodes) {
  DrawNodes(nodes.begin(), nodes.end(), "gray", "lightgray");
  return *this;
}

GraphViz &GraphViz::DrawForbiddenRules(const std::list<int> &rules) {
  DrawRules(rules, "gray", "lightgray");
  return *this;
}

GraphViz &GraphViz::DrawDestinationNode(int node) {
  const std::list<int> nodes = {node};
  DrawNodes(nodes.begin(), nodes.end(), "red", NULL);
  return *this;
}

GraphViz &GraphViz::DrawPath(const std::list<int> &ruleNumbers) {
  DrawRules(ruleNumbers, "green", "lightgreen");
  return *this;
}

void GraphViz::Export(const char *filename) {
  std::ofstream file(tmpFilename);
  if (!file.is_open())
    throw std::runtime_error("failed to open temp file");
  file << "digraph knowledge_base {\n"
       << "    forcelabels = true;\n"
       << "    rankdir = BT;\n";
  for (const auto &def : m_graphDefinitions)
    file << def;
  file << "}\n";
  file.close();
  std::string cmd = "dot -Tsvg " + std::string(tmpFilename) + " -o " + filename;
  if (system(cmd.c_str()) != 0)
    throw std::runtime_error("failed to convert dot graph to svg");
}

template <typename Iter>
void GraphViz::DrawNodes(Iter begin, Iter end, const char *color,
                         const char *fillcolor) {
  while (begin != end) {
    std::stringstream ss;
    ss << "    Node" << *begin++;
    if (fillcolor)
      ss << " [style = filled, color = " << color
         << ", fillcolor = " << fillcolor << "];\n";
    else
      ss << " [style = filled, color = " << color << "];\n";
    m_graphDefinitions.push_back(ss.str());
  }
}

void GraphViz::DrawRules(const std::list<int> &rules, const char *color,
                         const char *fillcolor) {
  for (const auto &rule : rules) {
    std::stringstream ss;
    ss << "    Rule" << rule;
    if (fillcolor)
      ss << " [style = filled, color = " << color
         << ", fillcolor = " << fillcolor << "];\n";
    else
      ss << " [style = filled, color = " << color << "];\n";
    m_graphDefinitions.push_back(ss.str());
  }
}
