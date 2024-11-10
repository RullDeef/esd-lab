#include "database.h"
#include "parser.h"
#include "solver.h"
#include <gtest/gtest.h>
#include <initializer_list>

static Database buildDatabase(std::initializer_list<const char *> rules,
                              std::initializer_list<const char *> facts) {
  Database database;
  for (auto &rule : rules)
    database.addRule(RuleParser().ParseRule(rule));
  for (auto &fact : facts)
    database.addFact(RuleParser().ParseFact(fact));
  return database;
}

TEST(SolverTest, taskFromBook) {
  auto database = buildDatabase(
      {
          "American(x) & Weapon(y) & Sells(x, y, z) & Hostile(z) -> "
          "Criminal(x)",
          "Missile(x) & Owns(Nono, x) -> Sells(West, x, Nono)",
          "Missile(x) -> Weapon(x)",
          "Enemy(x, America) -> Hostile(x)",

      },
      {
          "Owns(Nono, M1)",
          "Missile(M1)",
          "American(West)",
          "Enemy(Nono, America)",
      });

  auto target = RuleParser().ParseFact("Criminal(x)");
  Solver solver(database);

  solver.solveForward(target);

  auto res = solver.next();
  EXPECT_TRUE(res);
  if (res)
    std::cout << "res: " << res->toString() << std::endl;
  solver.done();
}

TEST(SolverTest, transitivity) {
  GTEST_SKIP();
  auto database = buildDatabase(
      {
          "Less(x, y) & Less(y, z) -> Less(x, z)",
      },
      {
          "Less(3, 4)",
          "Less(4, 5)",
          "Less(5, 6)",
      });

  auto target = RuleParser().ParseFact("Less(3, 6)");
  Solver solver(database);
  solver.solveForward(target);

  auto res = solver.next();
  solver.done();

  EXPECT_TRUE(res);
}
