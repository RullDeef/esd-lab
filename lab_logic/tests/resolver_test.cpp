#include "parser/expr_parser.h"
#include "resolver.h"
#include <gtest/gtest.h>
#include <list>

TEST(ResolverTest, SimpleCase) {
  std::list<Expr::ptr> sources = {
      Expr::createImplication(Expr::createAtom("A"), Expr::createAtom("B")),
      Expr::createAtom("A"),
  };
  Expr::ptr target = Expr::createAtom("B");

  bool satisfied = Resolver().Implies(sources, target);
  ASSERT_TRUE(satisfied);
}

TEST(ResolverTest, ContrapositionHalf) {
  std::list<Expr::ptr> sources = {
      Expr::createImplication(Expr::createAtom("A"), Expr::createAtom("B")),
  };
  Expr::ptr target =
      Expr::createImplication(Expr::createInverse(Expr::createAtom("B")),
                              Expr::createInverse(Expr::createAtom("A")));

  bool satisfied = Resolver().Implies(sources, target);
  ASSERT_TRUE(satisfied);
}

TEST(ResolverTest, identity) {
  Expr::ptr source = Expr::createTrue();
  Expr::ptr target = Expr::createTrue();

  bool satisfied = Resolver().Implies(source, target);
  ASSERT_TRUE(satisfied);
}

TEST(ResolverTest, reversedIdentity) {
  Expr::ptr source = Expr::createTrue();
  Expr::ptr target = Expr::createFalse();

  bool satisfied = Resolver().Implies(source, target);
  ASSERT_FALSE(satisfied);
}

TEST(ResolverTest, equality) {
  Expr::ptr target = ExprParser().Parse("A = A");

  bool satisfied = Resolver().Implies(Expr::createTrue(), target);
  ASSERT_TRUE(satisfied);
}

TEST(ResolverTest, predicatesSimple) {
  std::list<Expr::ptr> axioms;
  Expr::ptr target;

  EXPECT_NO_THROW(
      axioms.push_back(ExprParser().Parse("\\forall(x) (P(x) -> Q(x))")));
  EXPECT_NO_THROW(axioms.push_back(ExprParser().Parse("P(A)")));

  EXPECT_NO_THROW(target = ExprParser().Parse("Q(A)"));

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
  Expr::ptr target;

  EXPECT_NO_THROW(target = ExprParser().Parse(
                      "\\forall(X) \\exists(Y) P(X, Y) & \\forall(X, Y) (P(X, "
                      "Y) -> ~P(Y, X)) & \\forall(X, Y, Z) (P(X, Y) -> (P(Y, "
                      "Z) -> P(X, Z)))"));

  bool satisfied = Resolver().Implies(nullptr, target);
  ASSERT_TRUE(satisfied);
}

TEST(ResolverTest, diffExists) {
  std::list<Expr::ptr> axioms = {ExprParser().Parse("A -> \\exists(x) P(x)"),
                                 ExprParser().Parse("\\exists(x) (Q(x) -> A)")};
  Expr::ptr target = ExprParser().Parse("\\exists(x) (Q(x) -> P(x))");

  bool satisfied = Resolver().Implies(axioms, target);
  EXPECT_FALSE(satisfied);
}

TEST(ResolverTest, contrapositionPredicates) {
  std::list<Expr::ptr> axioms = {
      ExprParser().Parse("\\forall(x) (P(x) -> \\exists(y) Q(x, y))"),
      ExprParser().Parse("~(\\exists(x) \\forall(y) Q(y, x))")};
  Expr::ptr target = ExprParser().Parse("\\exists(x) ~P(x)");

  bool satisfied = Resolver().Implies(axioms, target);
  EXPECT_TRUE(satisfied);
}

TEST(ResolverTest, transitivity) {
  std::list<Expr::ptr> axioms = {
      // P(x, y) <=> x < y --- is transitive relation
      ExprParser().Parse("\\forall(x, y, z) (P(x, z) & P(z, y) -> P(x, y))"),
      // some facts
      ExprParser().Parse("P(3, 4) & P(4, 5) & P(5, 6)"),
  };
  Expr::ptr target = ExprParser().Parse("P(3, 6)");

  bool satisfied = Resolver().Implies(axioms, target);
  EXPECT_TRUE(satisfied);
}

TEST(ResolverTest, lectionEx1) {
  std::list<Expr::ptr> axioms = {
      ExprParser().Parse("\\forall(x) (S(x) + M(x))"),
      ExprParser().Parse("~(\\exists(x1) (M(x1) & L(x1, Rain)))"),
      ExprParser().Parse("\\forall(x2) (S(x2) -> L(x2, Snow))"),
      ExprParser().Parse("\\forall(y) (L(Lena, y) = ~L(Petya, y))"),
      ExprParser().Parse("L(Petya, Rain)"),
      ExprParser().Parse("L(Petya, Snow)"),
  };
  Expr::ptr target = ExprParser().Parse("\\exists(x3) (M(x3) & ~S(x3))");

  bool satisfied = Resolver().Implies(axioms, target);
  EXPECT_TRUE(satisfied);
}

TEST(ResolverTest, booleanLogic) {
  // And(x, y, z) <=> x & y = z
  // Or(x, y, z)  <=> x + y = z
  std::list<Expr::ptr> axioms = {
      ExprParser().Parse("\\forall(x) And(x, B0, B0) & And(x, B1, x)"),
      ExprParser().Parse("\\forall(x) Or(x, B1, B1) & Or(x, B0, x)"),
      ExprParser().Parse("\\forall(x, y, z) (And(x, y, z) = And(y, x, z))"),
      ExprParser().Parse("\\forall(x, y, z) (Or(x, y, z) = Or(y, x, z))"),
  };
  // (1 & 0) + (0 + 1) = 1
  // Or(a, b, 1)
  // And(1, 0, a)
  // Or(0, 1, b)
  Expr::ptr target =
      ExprParser().Parse("Or(a, b, B1) & And(B1, B0, a) & Or(B0, B1, b)");

  bool satisfied = Resolver().Implies(axioms, target);
  EXPECT_TRUE(satisfied);
}

TEST(ResolverTest, listLength) {
  std::list<Expr::ptr> axioms = {
      ExprParser().Parse(
          "\\forall(y, z) (Len(y, z) -> \\forall(x) Len(Cons(x, y), succ(z)))"),
      ExprParser().Parse("Len(Nil, 0)"),
  };
  // Len([A, B, C], succ(succ(succ(0))))
  Expr::ptr target = ExprParser().Parse(
      "Len(Cons(A, Cons(B, Cons(C, Nil))), succ(succ(succ(0))))");

  bool satisfied = Resolver().Implies(axioms, target);
  EXPECT_TRUE(satisfied);
}
