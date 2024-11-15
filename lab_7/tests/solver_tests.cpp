#include "database.h"
#include "mgraph_solver.h"
#include "parser.h"
#include "solver.h"
#include <gtest/gtest.h>
#include <initializer_list>

static Database buildDatabase(std::initializer_list<const char *> rules) {
  Database database;
  for (auto &rule : rules)
    database.addRule(RuleParser().ParseRule(rule));
  return database;
}

TEST(SolverTest, taskFromBook) {
  auto database = buildDatabase({
      "American(x) & Weapon(y) & Sells(x, y, z) & Hostile(z) -> Criminal(x)",
      "Missile(x) & Owns(Nono, x) -> Sells(West, x, Nono)",
      "Missile(x) -> Weapon(x)",
      "Enemy(x, America) -> Hostile(x)",
      "Owns(Nono, M1)",
      "Missile(M1)",
      "American(West)",
      "Enemy(Nono, America)",
  });

  auto target = RuleParser().ParseRule("Criminal(x)").getOutput();
  Solver solver(database);

  solver.solveForward(target);

  auto res = solver.next();
  EXPECT_TRUE(res);
  if (res)
    std::cout << "res: " << res->toString() << std::endl;
  solver.done();
}

TEST(SolverTest, sameVarUnify) {
  auto database = buildDatabase({
      "P(x, x, x)",
  });

  auto target = RuleParser().ParseRule("P(y, y, y)").getOutput();
  Solver solver(database);
  solver.solveForward(target);

  auto res = solver.next();
  solver.done();

  EXPECT_TRUE(res);
}

TEST(SolverTest, transitivity) {
  auto database = buildDatabase({
      "Less(x, y) & Less(y, z) -> Less(x, z)",
      "Less(3, 4)",
      "Less(4, 5)",
      "Less(5, 6)",
  });

  auto target = RuleParser().ParseRule("Less(3, 6)").getOutput();
  Solver solver(database);
  solver.solveForward(target);

  auto res = solver.next();
  solver.done();

  EXPECT_TRUE(res);
}

TEST(SolverTest, backtrackMax3) {
  GTEST_SKIP();

  auto database = buildDatabase({
      "max(a, b, c, a) :- less(b, a), less(c, a)",
      "max(_, b, c, b) :- less(c, b)",
      "max(_, _, c, c)",
      "less(1, 2)",
      "less(2, 3)",
      "less(3, 4)",
      "less(4, 5)",
      "less(5, 6)",
      "less(6, 7)",
  });

  auto target = RuleParser().ParseRule("max(1, 3, 2, x)").getOutput();
  Solver solver(database);
  solver.solveBackward(target);

  auto res = solver.next();
  solver.done();

  EXPECT_TRUE(res);
  EXPECT_EQ(res->toString(), "{x=3}");
}

TEST(SolverTest, backtrackMax3AllCombs) {
  auto database = buildDatabase({
      "max(a, b, c, a) :- less(b, a), less(c, a)",
      "max(_, b, c, b) :- less(c, b)",
      "max(_, _, c, c)",
      "less(1, 2)",
      "less(1, 3)",
      "less(2, 3)",
      "less(1, 4)",
      "less(2, 4)",
      "less(1, 5)",
      "less(2, 5)",
  });

  auto target = RuleParser().ParseRule("max(1, x, 2, x)").getOutput();
  MGraphSolver solver(database);
  solver.solveBackward(target);

  auto res1 = solver.next();
  auto res2 = solver.next();
  auto res3 = solver.next();
  solver.done();

  ASSERT_TRUE(res1);
  EXPECT_EQ(res1->toString(), "{x=3}");
  ASSERT_TRUE(res2);
  EXPECT_EQ(res2->toString(), "{x=4}");
  ASSERT_TRUE(res3);
  EXPECT_EQ(res3->toString(), "{x=5}");
}
