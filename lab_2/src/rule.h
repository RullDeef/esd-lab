#pragma once

#include <algorithm>
#include <vector>

struct Rule {
  std::vector<int> srcNodes;
  int dstNode;
  int number;
  bool visited = false;
  bool forbidden = false;
  unsigned int openIndex = 0;

  bool hasSource(int node) const {
    return std::find(srcNodes.begin(), srcNodes.end(), node) != srcNodes.end();
  }
};
