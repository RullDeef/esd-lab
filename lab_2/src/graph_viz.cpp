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

GraphViz &GraphViz::DrawSourceNodes(const std::vector<int>& sourceNodes) {
  for (const auto&node : sourceNodes) {
    std::stringstream ss;
    ss << "    Node" << node
       << " [style = filled, color = blue, fillcolor = lightblue];\n";
    m_graphDefinitions.push_back(ss.str());
  }
  return *this;
}

GraphViz &GraphViz::DrawDestinationNode(int node) {
  std::stringstream ss;
  ss << "    Node" << node
     << " [style = filled, color = red, fillcolor = lightgreen];\n";
  m_graphDefinitions.push_back(ss.str());
  return *this;
}

GraphViz &GraphViz::DrawPath(const std::list<int> &ruleNumbers) {
  for (const auto &rule : ruleNumbers) {
    std::stringstream ss;
    ss << "    Rule" << rule
       << " [style = filled, color = green, fillcolor = lightgreen];\n";
    m_graphDefinitions.push_back(ss.str());
  }
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
