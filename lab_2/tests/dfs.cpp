#include "graph_search.h"
#include <gtest/gtest.h>

TEST(DFS, SameSourceAndTarget) {
  std::list<Rule> rules = {
      Rule{{1}, 1, 100},
  };
  GraphSearch gs(std::move(rules), {1}, 1);

  const auto actual = gs.DoDepthFirstSearch();

  const std::list<int> expected = {};
  ASSERT_EQ(expected, actual);
}

TEST(DFS, SimpleLinearPath) {
  std::list<Rule> rules = {
      Rule{{3}, 4, 102},
      Rule{{1}, 2, 100},
      Rule{{2}, 3, 101},
  };
  GraphSearch gs(std::move(rules), {1}, 4);

  const auto actual = gs.DoDepthFirstSearch();

  const std::list<int> expected = {100, 101, 102};
  ASSERT_EQ(expected, actual);
}

TEST(DFS, SimpleNoSolution) {
  std::list<Rule> rules = {
      Rule{{3}, 4, 102},
      Rule{{1}, 2, 100},
      Rule{{2}, 3, 101},
  };
  GraphSearch gs(std::move(rules), {4}, 1);

  const auto actual = gs.DoDepthFirstSearch();

  const std::list<int> expected = {};
  ASSERT_EQ(expected, actual);
}

TEST(DFS, SimpleLoopNoSolution) {
  std::list<Rule> rules = {
      Rule{{1}, 2, 100},
      Rule{{2}, 3, 101},
      Rule{{3}, 1, 102},
      Rule{{4}, 2, 103},
  };
  GraphSearch gs(std::move(rules), {1}, 4);

  const auto actual = gs.DoDepthFirstSearch();

  const std::list<int> expected = {};
  ASSERT_EQ(expected, actual);
}

TEST(DFS, NotOptimalPath) {
  std::list<Rule> rules = {
      Rule{{1}, 4, 103}, Rule{{4}, 5, 104}, Rule{{2}, 3, 101},
      Rule{{3}, 5, 102}, Rule{{1}, 2, 100},
  };
  GraphSearch gs(std::move(rules), {1}, 5);

  const auto actual = gs.DoDepthFirstSearch();

  const std::list<int> expected = {100, 101, 102};
  ASSERT_EQ(expected, actual);
}

TEST(DFS, ThroughLoop) {
  std::list<Rule> rules = {
      Rule{{5}, 6, 105}, Rule{{2}, 5, 104}, Rule{{4}, 2, 103},
      Rule{{3}, 4, 102}, Rule{{2}, 3, 101}, Rule{{1}, 2, 100},
  };
  GraphSearch gs(std::move(rules), {1}, 6);

  const auto actual = gs.DoDepthFirstSearch();

  const std::list<int> expected = {100, 104, 105};
  ASSERT_EQ(expected, actual);
}

TEST(DFS, MultiGraph) {
  std::list<Rule> rules = {
    Rule{{3, 4}, 5, 101},
    Rule{{1, 2}, 5, 100},
    Rule{{5, 6}, 7, 102},
  };
  GraphSearch gs(std::move(rules), {3, 4, 6}, 7);

  const auto actual = gs.DoDepthFirstSearch();

  const std::list<int> expected = {101, 102};
  ASSERT_EQ(expected, actual);
}

static std::list<Rule> buildComplexGraph() {
  return std::list<Rule> {
    Rule{{1, 2}, 3, 100},
    Rule{{2, 3, 4}, 5, 101},
    Rule{{6, 7}, 4, 102},
    Rule{{8, 9}, 3, 103},
    Rule{{5, 10}, 11, 104},
    Rule{{4, 12, 13}, 10, 105},
    Rule{{14, 15}, 13, 106},
    Rule{{16, 17}, 18, 107},
    Rule{{19, 20}, 16, 108},
    Rule{{10, 17}, 11, 109},
    Rule{{21, 22}, 10, 110},
    Rule{{23, 24}, 17, 111},
  };
}

TEST(DFS, ComplexGraph) {
  auto rules = buildComplexGraph();
  GraphSearch gs(std::move(rules), {2, 8, 9, 4}, 5);

  const auto actual = gs.DoDepthFirstSearch();

  std::list<int> expected = {103, 101};
  ASSERT_EQ(expected, actual);
}