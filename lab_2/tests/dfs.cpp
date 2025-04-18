#include "graph_search.h"
#include <gtest/gtest.h>

TEST(DFS, SameSourceAndTarget) {
  std::list<Rule> rules = {
      {{1}, 1, 100},
  };
  GraphSearchRev gs(std::move(rules), {1}, 1);

  const auto actual = gs.DoDepthFirstSearch();

  const std::list<int> expected = {};
  ASSERT_EQ(expected, actual);
}

TEST(DFS, SimpleLinearPath) {
  std::list<Rule> rules = {
      {{3}, 4, 102},
      {{1}, 2, 100},
      {{2}, 3, 101},
  };
  GraphSearchRev gs(std::move(rules), {1}, 4);

  const auto actual = gs.DoDepthFirstSearch();

  const std::list<int> expected = {100, 101, 102};
  ASSERT_EQ(expected, actual);
}

TEST(DFS, SimpleNoSolution) {
  std::list<Rule> rules = {
      {{3}, 4, 102},
      {{1}, 2, 100},
      {{2}, 3, 101},
  };
  GraphSearchRev gs(std::move(rules), {4}, 1);

  const auto actual = gs.DoDepthFirstSearch();

  const std::list<int> expected = {};
  ASSERT_EQ(expected, actual);
}

TEST(DFS, SimpleLoopNoSolution) {
  std::list<Rule> rules = {
      {{1}, 2, 100},
      {{2}, 3, 101},
      {{3}, 1, 102},
      {{4}, 2, 103},
  };
  GraphSearchRev gs(std::move(rules), {1}, 4);

  const auto actual = gs.DoDepthFirstSearch();

  const std::list<int> expected = {};
  ASSERT_EQ(expected, actual);
}

TEST(DFS, NotOptimalPath) {
  std::list<Rule> rules = {
      {{1}, 4, 103}, {{4}, 5, 104}, {{2}, 3, 101}, {{3}, 5, 102}, {{1}, 2, 100},
  };
  GraphSearchRev gs(std::move(rules), {1}, 5);

  const auto actual = gs.DoDepthFirstSearch();

  const std::list<int> expected = {100, 101, 102};
  ASSERT_EQ(expected, actual);
}

TEST(DFS, ThroughLoop) {
  std::list<Rule> rules = {
      {{5}, 6, 105}, {{2}, 5, 104}, {{4}, 2, 103},
      {{3}, 4, 102}, {{2}, 3, 101}, {{1}, 2, 100},
  };
  GraphSearchRev gs(std::move(rules), {1}, 6);

  const auto actual = gs.DoDepthFirstSearch();

  const std::list<int> expected = {100, 104, 105};
  ASSERT_EQ(expected, actual);
}

TEST(DFS, MultiGraph) {
  std::list<Rule> rules = {
      {{3, 4}, 5, 101},
      {{1, 2}, 5, 100},
      {{5, 6}, 7, 102},
  };
  GraphSearchRev gs(std::move(rules), {3, 4, 6}, 7);

  const auto actual = gs.DoDepthFirstSearch();

  const std::list<int> expected = {101, 102};
  ASSERT_EQ(expected, actual);
}

static std::list<Rule> buildComplexGraph() {
  return {
      {{1, 2}, 3, 100},    {{2, 3, 4}, 5, 101}, {{6, 7}, 4, 102},
      {{8, 9}, 3, 103},    {{5, 10}, 11, 104},  {{4, 12, 13}, 10, 105},
      {{14, 15}, 13, 106}, {{16, 17}, 18, 107}, {{19, 20}, 16, 108},
      {{10, 17}, 11, 109}, {{21, 22}, 10, 110}, {{23, 24}, 17, 111},
  };
}

TEST(DFS, ComplexGraph) {
  auto rules = buildComplexGraph();
  GraphSearchRev gs(std::move(rules), {2, 8, 9, 4}, 5);

  const auto actual = gs.DoDepthFirstSearch();

  std::list<int> expected = {103, 101};
  ASSERT_EQ(expected, actual);
}
