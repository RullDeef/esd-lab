#pragma once

#include <vector>

struct Rule {
  std::vector<int> srcNodes;
  int dstNode;
  int number;
  bool visited = false;
  bool forbidden = false;
  unsigned int openIndex = 0;
};
