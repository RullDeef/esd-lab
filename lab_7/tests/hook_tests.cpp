#include "atom_hook.h"
#include "database.h"
#include "mgraph_solver.h"
#include "parser.h"
#include <gtest/gtest.h>
#include <map>

static std::shared_ptr<Database>
buildDatabase(std::initializer_list<const char *> rules) {
  auto database = std::make_shared<Database>();
  for (auto &rule : rules)
    database->addRule(RuleParser().ParseRule(rule));
  return database;
}

static Atom parseGoal(const char *str) {
  return RuleParser().ParseRule(str).getOutput();
}

TEST(HookTest, simple) {
  auto database = buildDatabase({
      "min(a, b, a) :- write(First, Route), leq(a, b), cut",
      "min(_, b, b) :- write(Second, Route)",
  });
  std::map<std::string, std::shared_ptr<AtomHook>> atomHooks = {
      {"write", std::make_shared<WriteHook>()},
      {"leq", std::make_shared<LeqHook>()},
  };
  auto solver = std::make_shared<MGraphSolver>(database, atomHooks);

  solver->solveBackward(parseGoal("min(3, 2, x)"));

  auto res = solver->next();
  solver->done();

  ASSERT_TRUE(res);
  EXPECT_EQ(res->toString(), "{x=2}");
}
