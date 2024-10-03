#pragma once

#include <vector>

struct Rule {
  std::vector<int> srcNodes;
  int dstNode;
  int number;
  bool visited = false;
  bool forbidden = false;
  int openIndex = 0; // size of opened nodes stack at rule open
};
