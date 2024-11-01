#include "resolver.h"
#include "rule_parser.h"
#include <gtest/gtest.h>
#include <list>

TEST(ResolverTest, SimpleCase) {
  std::list<Rule::ptr> sources = {
      Rule::createImplication(Rule::createAtom("A"), Rule::createAtom("B")),
      Rule::createAtom("A"),
  };
  Rule::ptr target = Rule::createAtom("B");

  bool satisfied = Resolver().Implies(sources, target);
  ASSERT_TRUE(satisfied);
}

TEST(ResolverTest, ContrapositionHalf) {
  std::list<Rule::ptr> sources = {
      Rule::createImplication(Rule::createAtom("A"), Rule::createAtom("B")),
  };
  Rule::ptr target =
      Rule::createImplication(Rule::createInverse(Rule::createAtom("B")),
                              Rule::createInverse(Rule::createAtom("A")));

  bool satisfied = Resolver().Implies(sources, target);
  ASSERT_TRUE(satisfied);
}

TEST(ResolverTest, identity) {
  Rule::ptr source = Rule::createTrue();
  Rule::ptr target = Rule::createTrue();

  bool satisfied = Resolver().Implies(source, target);
  ASSERT_TRUE(satisfied);
}

TEST(ResolverTest, reversedIdentity) {
  Rule::ptr source = Rule::createTrue();
  Rule::ptr target = Rule::createFalse();

  bool satisfied = Resolver().Implies(source, target);
  ASSERT_FALSE(satisfied);
}

TEST(ResolverTest, equality) {
  Rule::ptr target = RuleParser().Parse("A = A");

  bool satisfied = Resolver().Implies(Rule::createTrue(), target);
  ASSERT_TRUE(satisfied);
}

TEST(ResolverTest, predicatesSimple) {
  std::list<Rule::ptr> axioms;
  Rule::ptr target;

  EXPECT_NO_THROW(
      axioms.push_back(RuleParser().Parse("\\forall(X) (P(X) -> Q(X))")));
  EXPECT_NO_THROW(axioms.push_back(RuleParser().Parse("P(a)")));

  EXPECT_NO_THROW(target = RuleParser().Parse("Q(a)"));

  bool satisfied = Resolver().Implies(axioms, target);
  ASSERT_TRUE(satisfied);
}

TEST(ResolverTest, scolemForm) {
  /**
   * Задачи из
   * http://logic.math.msu.ru/wp-content/uploads/vml/2008/t34predicat.pdf
   *
   * 2. Доказать, что формула
   *
   *   \forall(x) \exists(y) P(x, y) & \forall(x, y) (P(x, y) -> ~P(y, x)) &
   *   & \forall(x, y, z) (P(x, y) -> (P(y, z) -> P(x, z)))
   *
   *   выполнима [, но не выполнима ни в какой конечной модели].
   */

  GTEST_SKIP();
  Rule::ptr target;

  EXPECT_NO_THROW(target = RuleParser().Parse(
                      "\\forall(X) \\exists(Y) P(X, Y) & \\forall(X, Y) (P(X, "
                      "Y) -> ~P(Y, X)) & \\forall(X, Y, Z) (P(X, Y) -> (P(Y, "
                      "Z) -> P(X, Z)))"));

  bool satisfied = Resolver().Implies(nullptr, target);
  ASSERT_TRUE(satisfied);
}

TEST(ResolverTest, diffExists) {
  std::list<Rule::ptr> axioms = {RuleParser().Parse("A -> \\exists(X) P(X)"),
                                 RuleParser().Parse("\\exists(X) (Q(X) -> A)")};
  Rule::ptr target = RuleParser().Parse("\\exists(X) (Q(X) -> P(X))");

  bool satisfied = Resolver().Implies(axioms, target);
  EXPECT_FALSE(satisfied);
}

TEST(ResolverTest, contrapositionPredicates) {
  std::list<Rule::ptr> axioms = {
      RuleParser().Parse("\\forall(X) (P(X) -> \\exists(Y) Q(X, Y))"),
      RuleParser().Parse("~(\\exists(X) \\forall(Y) Q(Y, X))")};
  Rule::ptr target = RuleParser().Parse("\\exists(X) ~P(X)");

  bool satisfied = Resolver().Implies(axioms, target);
  EXPECT_TRUE(satisfied);
}

TEST(ResolverTest, transitivity) {
  std::list<Rule::ptr> axioms = {
      // P(x, y) <=> x < y --- is transitive relation
      RuleParser().Parse("\\forall(X, Y, Z) (P(X, Z) & P(Z, Y) -> P(X, Y))"),
      // some facts
      RuleParser().Parse("P(3, 4) & P(4, 5) & P(5, 6)"),
  };
  Rule::ptr target = RuleParser().Parse("P(3, 6)");

  bool satisfied = Resolver().Implies(axioms, target);
  EXPECT_TRUE(satisfied);
}
