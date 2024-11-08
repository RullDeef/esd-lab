#include "normalizer.h"
#include "parser/expr_parser.h"
#include "resolver_new.h"
#include <gtest/gtest.h>
#include <memory>

Atom parseAtom(const char *text) {
  auto disj = ExprNormalizer().scolemForm(ExprParser().Parse(text));
  return *disj[0].begin();
}

Disjunct parseDisjunct(const char *text) {
  auto disj = ExprNormalizer().scolemForm(ExprParser().Parse(text));
  return disj[0];
}

std::vector<Disjunct> parseDisjuncts(const char *text) {
  return ExprNormalizer().scolemForm(ExprParser().Parse(text));
}

TEST(NameAllocTest, simpleCase) {
  NameAllocator allocator;

  EXPECT_TRUE(allocator.allocateName("x"));
  EXPECT_TRUE(allocator.allocateName("y"));
  EXPECT_TRUE(allocator.allocateName("x1"));

  EXPECT_FALSE(allocator.allocateName("y0"));

  EXPECT_EQ(allocator.allocateRenaming("x"), "x2");
}

TEST(SubstTest, conflictingRings) {
  Subst left;
  left.insert("x", std::make_shared<Variable>(true, "A"));
  left.insert("y", std::make_shared<Variable>(true, "B"));
  Subst right;
  right.link("x", "y");

  auto sum = left + right;
  EXPECT_FALSE(sum);
  if (sum) {
    std::cout << "got " << sum->toString() << std::endl;
  }
}

TEST(DisjunctTest, renameVars) {
  Disjunct disj = parseDisjunct("P(x, y, x3) + Q(f(x1, y), z)");

  NameAllocator allocator;
  disj.commitVarNames(allocator);

  std::cout << "allocated names: " << allocator.toString() << std::endl;
  Disjunct disj2 = disj.renamedFreeVars(allocator);
  std::cout << "allocated after: " << allocator.toString() << std::endl;

  EXPECT_EQ(disj2.toString(), "P(x2, y1, x4) + Q(f(x5, y1), z1)");
}

TEST(ResolverNewTest, unifySimple) {
  Atom left(false, "P", {std::make_shared<Variable>(false, "x")});
  Atom right(true, "P", {std::make_shared<Variable>(true, "Ara")});

  auto res = ResolverNew().unify(left, right);
  EXPECT_TRUE(res);
  std::cout << "left: " << left.toString() << std::endl
            << "right: " << right.toString() << std::endl
            << "subst: " << res->toString() << std::endl;
}

TEST(ResolverNewTest, conflictLinkage) {
  auto left = parseAtom("~Q(S, x, F, G, y, H)");
  auto right = parseAtom("Q(y, A, F, G, x, H)");

  auto res = ResolverNew().unify(left, right);
  EXPECT_FALSE(res);
}

TEST(ResolverNewTest, conflictLinkage2) {
  auto left = parseAtom("~B(P, x, r(G, Q), K, T, y, U)");
  auto right = parseAtom("B(P, Q, r(G, Q), y, T, x, U)");

  auto res = ResolverNew().unify(left, right);
  EXPECT_FALSE(res);
}

TEST(ResolverNewTest, subFuncs) {
  auto left = parseAtom("~P(x, f(x, B, g(y)), g(C))");
  auto right = parseAtom("P(A, f(A, B, z), g(y))");

  auto res = ResolverNew().unify(left, right);
  ASSERT_TRUE(res);
  EXPECT_EQ(res->toString(), "{x=A, y=C, z=g(C)}");
}

TEST(ResolverNewTest, recursiveFunc) {
  auto left = parseAtom("~P(x, f(g(x)))");
  auto right = parseAtom("P(y, y)");

  auto res = ResolverNew().unify(left, right);
  ASSERT_TRUE(res);
  EXPECT_EQ(res->toString(), "{x=y=f(g(...))}");
}

TEST(ResolverNewTest, recursiveHard) {
  auto left = parseAtom("~H(x, g(z), z, o(x))");
  auto right = parseAtom("H(f(y), y, t(x), o(x))");

  auto res = ResolverNew().unify(left, right);
  ASSERT_TRUE(res);
  EXPECT_EQ(res->toString(),
            "{x=f(g(t(...))), y=g(t(f(...))), z=t(f(g(...)))}");
}

TEST(ResolverNewTest, recursiveConflict) {
  auto left = parseAtom("~R(x, y, g(y))");
  auto right = parseAtom("R(f(y), k(x), x)");

  auto res = ResolverNew().unify(left, right);
  EXPECT_FALSE(res);
}

TEST(ResolverNewTest, resolutionSimple) {
  auto axioms = parseDisjuncts("A & (~A + B)");
  auto target = parseDisjuncts("~B");

  auto res = ResolverNew().resolve(axioms, target);

  EXPECT_TRUE(res);
  if (res)
    std::cout << "res: " << res->toString() << std::endl;
}

TEST(ResolverNewTest, resolutionTransitive) {
  auto axioms = parseDisjuncts("(A -> B) & (B -> C)");
  auto target = parseDisjuncts("~(A -> C)");

  auto res = ResolverNew().resolve(axioms, target);

  EXPECT_TRUE(res);
  if (res)
    std::cout << "res: " << res->toString() << std::endl;
}

TEST(ResolverNewTest, resolutionPredicates) {
  auto axioms =
      parseDisjuncts("(A -> \\exists(x) P(x)) & (\\exists(x) (Q(x) -> A))");
  auto target = parseDisjuncts("~(\\exists(x) (Q(x) -> P(x)))");

  auto res = ResolverNew().resolve(axioms, target);

  EXPECT_TRUE(res);
  if (res)
    std::cout << "res: " << res->toString() << std::endl;
}

TEST(ResolverNewTest, resolutionTransitivePred) {
  auto axioms =
      parseDisjuncts("(\\forall(x, y, z) ((P(x, z) & P(z, y)) -> P(x, y))) & "
                     "P(3, 4) & P(4, 5) & P(5, 6)");
  auto target = parseDisjuncts("~P(3, 6)");

  auto res = ResolverNew().resolve(axioms, target);

  EXPECT_TRUE(res);
  if (res)
    std::cout << "res: " << res->toString() << std::endl;
}

TEST(ResolverNewTest, resolutionLectionEx1) {
  auto axioms = parseDisjuncts("(\\forall(x) (S(x) + M(x))) & "
                               "~(\\exists(x) (M(x) & L(x, Lena))) & "
                               "(\\forall(x) (S(x) -> L(x, Snow))) & "
                               "(\\forall(y) (L(Lena, y) = ~L(Petya, y))) & "
                               "L(Petya, Rain) & "
                               "L(Petya, Snow)");
  auto target = parseDisjuncts("~(\\exists(x) (M(x) & ~S(x)))");

  auto res = ResolverNew().resolve(axioms, target);

  EXPECT_TRUE(res);
  if (res)
    std::cout << "res: " << res->toString() << std::endl;
}
