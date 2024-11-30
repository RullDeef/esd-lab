#include "database.h"
#include "mgraph_solver.h"
#include "parser.h"
#include "solver.h"
#include <gtest/gtest.h>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <unistd.h>

static std::shared_ptr<Database>
buildDatabase(std::initializer_list<const char *> rules) {
  auto database = std::make_shared<Database>();
  for (auto &rule : rules)
    database->addRule(RuleParser().ParseRule(rule));
  return database;
}

static std::map<std::string, std::shared_ptr<AtomHook>> buildPredefinedHooks() {
  return {
      {"write", std::make_shared<WriteHook>()},
      {"add", std::make_shared<IntAddHook>()},
      {"mul", std::make_shared<IntMulHook>()},
      {"leq", std::make_shared<LeqHook>()},
      {"in_range", std::make_shared<InRangeHook>()},
  };
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
  auto solver = std::make_shared<Solver>(database);

  solver->solveForward(target);

  auto res = solver->next();
  EXPECT_TRUE(res);
  if (res)
    std::cout << "res: " << res->toString() << std::endl;
  solver->done();
}

TEST(SolverTest, sameVarUnify) {
  auto database = buildDatabase({
      "P(x, x, x)",
  });

  auto target = RuleParser().ParseRule("P(y, y, y)").getOutput();
  auto solver = std::make_shared<Solver>(database);
  solver->solveForward(target);

  auto res = solver->next();
  solver->done();

  EXPECT_TRUE(res);
}

TEST(SolverTest, unifyWithSubst) {
  Subst subst;
  subst.link("y", "y2");
  subst.insert("x3",
               Variable::createFuncSym("cons", std::vector{
                                                   Variable::createConst("A"),
                                                   Variable::createConst("Nil"),
                                               }));

  auto left = RuleParser().ParseRule("A(cons(A, Nil), y, Nil)").getOutput();
  auto right = RuleParser().ParseRule("A(cons(h, r), y3, x5)").getOutput();

  ASSERT_TRUE(Solver::unify(left, right, subst));
  ASSERT_EQ(subst.toString(),
            "{y=y2=y3=, h=A, r=Nil, x3=cons(A, Nil), x5=Nil}");
}

TEST(SolverTest, transitivity) {
  auto database = buildDatabase({
      "Less(x, y) & Less(y, z) -> Less(x, z)",
      "Less(3, 4)",
      "Less(4, 5)",
      "Less(5, 6)",
  });

  auto target = RuleParser().ParseRule("Less(3, 6)").getOutput();
  auto solver = std::make_shared<Solver>(database);
  solver->solveForward(target);

  auto res = solver->next();
  solver->done();

  EXPECT_TRUE(res);
}

TEST(SolverTest, backtrackMax3) {
  auto database = buildDatabase({
      "max(a, b, c, a) :- less(b, a), less(c, a)",
      "max(a, b, c, b) :- less(a, b), less(c, b)",
      "max(a, b, c, c) :- less(a, c), less(b, c)",
      "less(1, 2)",
      "less(2, 3)",
      "less(1, 3)",
  });

  auto target = RuleParser().ParseRule("max(1, 3, 2, x)").getOutput();
  auto solver = std::make_shared<MGraphSolver>(database);
  solver->solveBackward(target);

  auto res = solver->next();
  solver->done();

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
  auto solver = std::make_shared<MGraphSolver>(database);
  solver->solveBackward(target);

  auto res1 = solver->next();
  auto res2 = solver->next();
  auto res3 = solver->next();
  solver->done();

  ASSERT_TRUE(res1);
  EXPECT_EQ(res1->toString(), "{x=3}");
  ASSERT_TRUE(res2);
  EXPECT_EQ(res2->toString(), "{x=4}");
  ASSERT_TRUE(res3);
  EXPECT_EQ(res3->toString(), "{x=5}");
}

TEST(SolverTest, listLen) {
  auto database = buildDatabase({
      "len(Nil, 0) :- cut",
      "len(cons(_, x), succ(n)) :- len(x, n)",
  });

  auto target = RuleParser()
                    .ParseRule("len(cons(A, cons(B, cons(C, Nil))), x)")
                    .getOutput();
  auto solver = std::make_shared<MGraphSolver>(database);
  solver->solveBackward(target);

  auto res = solver->next();
  solver->done();

  ASSERT_TRUE(res);
  EXPECT_EQ(res->toString(), "{x=succ(succ(succ(0)))}");
}

TEST(SolverTest, listLenReverse) {
  auto database = buildDatabase({
      "len(Nil, 0) :- cut",
      "len(cons(_, x), succ(n)) :- len(x, n)",
  });

  auto target =
      RuleParser().ParseRule("len(x, succ(succ(succ(0))))").getOutput();
  auto solver = std::make_shared<MGraphSolver>(database);
  solver->solveBackward(target);

  auto res = solver->next();
  solver->done();

  ASSERT_TRUE(res);
  EXPECT_EQ(res->toString(), "{x=cons(_, cons(_, cons(_, Nil)))}");
}

TEST(SolverTest, listReverse) {
  auto database = buildDatabase({
      "reverse(Nil, Nil) :- cut",
      "reverse(  x,   y) :- reverse_helper(x, y, Nil)",
      "reverse_helper(Nil, x, x)",
      "reverse_helper(cons(h, r), x, y) :- reverse_helper(r, x, cons(h, y))",
  });

  auto target = RuleParser()
                    .ParseRule("reverse(cons(A, cons(B, cons(C, Nil))), x)")
                    .getOutput();
  auto solver = std::make_shared<MGraphSolver>(database);
  solver->solveBackward(target);

  auto res = solver->next();
  solver->done();

  ASSERT_TRUE(res);
  EXPECT_EQ(res->toString(), "{x=cons(C, cons(B, cons(A, Nil)))}");
}

TEST(SolverTest, recursiveFuncSym) {
  auto database = buildDatabase({
      "link(x, x)",
  });

  auto target = RuleParser().ParseRule("link(cons(A, x), x)").getOutput();
  auto solver = std::make_shared<MGraphSolver>(database);
  solver->solveBackward(target);

  auto res = solver->next();
  solver->done();

  ASSERT_TRUE(res);
  EXPECT_EQ(res->toString(), "{x=cons(A, ...)}");
}

TEST(SolverTest, properCut) {
  auto database = buildDatabase({
      "q(A)",
      "q(B)",
      "q(C)",
      "p(A) :- !",
      "p(x) :- q(x), !, fail",
      "p(_)",
  });

  auto target = RuleParser().ParseRule("p(B)").getOutput();
  auto solver = std::make_shared<MGraphSolver>(database);
  solver->solveBackward(target);

  auto res = solver->next();
  solver->done();

  ASSERT_FALSE(res);
}

TEST(SolverTest, properCut2) {
  auto database = buildDatabase({
      "p(A) :- !",
      "p(B)",
      "p(C)",
  });

  auto target = RuleParser().ParseRule("p(x)").getOutput();
  auto solver = std::make_shared<MGraphSolver>(database);
  solver->solveBackward(target);

  auto res1 = solver->next();
  auto res2 = solver->next();
  solver->done();

  ASSERT_TRUE(res1);
  EXPECT_EQ(res1->toString(), "{x=A}");

  ASSERT_FALSE(res2);
}

TEST(SolverTest, primes) {
  auto database = buildDatabase({
      "divisible(x, y) :- mul(y, z, x), mul(y, z, x), !",
      "composite(x) :- in_range(v, 2, x), divisible(x, v), !",
      "prime(1) :- !, fail",
      "prime(x) :- composite(x), !, fail",
      "prime(_)",
  });
  auto hooks = buildPredefinedHooks();
  auto solver = std::make_shared<MGraphSolver>(database, hooks);

  auto target1 = RuleParser().ParseRule("prime(5)").getOutput();
  solver->solveBackward(target1);

  auto res1 = solver->next();
  solver->done();

  ASSERT_TRUE(res1);

  auto target2 = RuleParser().ParseRule("prime(12)").getOutput();
  solver->solveBackward(target2);

  auto res2 = solver->next();
  solver->done();

  ASSERT_FALSE(res2);
}
