#pragma once

#include <vector>

struct Rule {
  std::vector<int> srcNodes;
  int dstNode;
  int number;
  bool visited = false;
};
