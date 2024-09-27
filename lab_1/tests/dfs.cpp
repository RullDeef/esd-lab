#include "graph_search.h"
#include <gtest/gtest.h>

TEST(DFS, SameSourceAndTarget) {
  std::list<Rule> rules = {
      Rule{1, 1, 100},
  };
  GraphSearch gs(std::move(rules), 1, 1);

  const auto actual = gs.DoDepthFirstSearch();

  const std::list<int> expected = {};
  ASSERT_EQ(expected, actual);
}

TEST(DFS, SimpleLinearPath) {
  std::list<Rule> rules = {
      Rule{3, 4, 102},
      Rule{1, 2, 100},
      Rule{2, 3, 101},
  };
  GraphSearch gs(std::move(rules), 1, 4);

  const auto actual = gs.DoDepthFirstSearch();

  const std::list<int> expected = {100, 101, 102};
  ASSERT_EQ(expected, actual);
}

TEST(DFS, SimpleNoSolution) {
  std::list<Rule> rules = {
      Rule{3, 4, 102},
      Rule{1, 2, 100},
      Rule{2, 3, 101},
  };
  GraphSearch gs(std::move(rules), 4, 1);

  const auto actual = gs.DoDepthFirstSearch();

  const std::list<int> expected = {};
  ASSERT_EQ(expected, actual);
}

TEST(DFS, SimpleLoopNoSolution) {
  std::list<Rule> rules = {
      Rule{1, 2, 100},
      Rule{2, 3, 101},
      Rule{3, 1, 102},
      Rule{4, 2, 103},
  };
  GraphSearch gs(std::move(rules), 1, 4);

  const auto actual = gs.DoDepthFirstSearch();

  const std::list<int> expected = {};
  ASSERT_EQ(expected, actual);
}

TEST(DFS, NotOptimalPath) {
  std::list<Rule> rules = {
      Rule{1, 4, 103}, Rule{3, 5, 102}, Rule{1, 2, 100},
      Rule{4, 5, 104}, Rule{2, 3, 101},
  };
  GraphSearch gs(std::move(rules), 1, 5);

  const auto actual = gs.DoDepthFirstSearch();

  const std::list<int> expected = {100, 101, 102};
  ASSERT_EQ(expected, actual);
}

TEST(DFS, ThroughLoop) {
  std::list<Rule> rules = {
      Rule{5, 6, 105}, Rule{2, 5, 104}, Rule{4, 2, 103},
      Rule{3, 4, 102}, Rule{2, 3, 101}, Rule{1, 2, 100},
  };
  GraphSearch gs(std::move(rules), 1, 6);

  const auto actual = gs.DoDepthFirstSearch();

  const std::list<int> expected = {100, 104, 105};
  ASSERT_EQ(expected, actual);
}
