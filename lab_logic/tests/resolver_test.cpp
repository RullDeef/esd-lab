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
      axioms.push_back(RuleParser().Parse("\\forall(x) (P(x) -> Q(x))")));
  EXPECT_NO_THROW(axioms.push_back(RuleParser().Parse("P(A)")));

  EXPECT_NO_THROW(target = RuleParser().Parse("Q(A)"));

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
  std::list<Rule::ptr> axioms = {RuleParser().Parse("A -> \\exists(x) P(x)"),
                                 RuleParser().Parse("\\exists(x) (Q(x) -> A)")};
  Rule::ptr target = RuleParser().Parse("\\exists(x) (Q(x) -> P(x))");

  bool satisfied = Resolver().Implies(axioms, target);
  EXPECT_FALSE(satisfied);
}

TEST(ResolverTest, contrapositionPredicates) {
  std::list<Rule::ptr> axioms = {
      RuleParser().Parse("\\forall(x) (P(x) -> \\exists(y) Q(x, y))"),
      RuleParser().Parse("~(\\exists(x) \\forall(y) Q(y, x))")};
  Rule::ptr target = RuleParser().Parse("\\exists(x) ~P(x)");

  bool satisfied = Resolver().Implies(axioms, target);
  EXPECT_TRUE(satisfied);
}

TEST(ResolverTest, transitivity) {
  std::list<Rule::ptr> axioms = {
      // P(x, y) <=> x < y --- is transitive relation
      RuleParser().Parse("\\forall(x, y, z) (P(x, z) & P(z, y) -> P(x, y))"),
      // some facts
      RuleParser().Parse("P(3, 4) & P(4, 5) & P(5, 6)"),
  };
  Rule::ptr target = RuleParser().Parse("P(3, 6)");

  bool satisfied = Resolver().Implies(axioms, target);
  EXPECT_TRUE(satisfied);
}

TEST(ResolverTest, lectionEx1) {
  std::list<Rule::ptr> axioms = {
      RuleParser().Parse("\\forall(x) (S(x) + M(x))"),
      RuleParser().Parse("~(\\exists(x1) (M(x1) & L(x1, Rain)))"),
      RuleParser().Parse("\\forall(x2) L(x2, Snow)"),
      RuleParser().Parse("\\forall(y) (L(Lena, y) = ~L(Petya, y))"),
      RuleParser().Parse("L(Petya, Rain)"),
      RuleParser().Parse("L(Petya, Snow)"),
  };
  Rule::ptr target = RuleParser().Parse("\\exists(x3) (M(x3) & ~S(x3))");

  bool satisfied = Resolver().Implies(axioms, target);
  EXPECT_FALSE(satisfied);
}
