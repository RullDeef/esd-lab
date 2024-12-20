#include "normalizer.h"
#include "parser/expr_parser.h"
#include "resolver.h"
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

std::ostream &operator<<(std::ostream &stream, const Disjunct &value) {
  return stream << value.toString();
}

std::ostream &operator<<(std::ostream &stream,
                         const std::vector<Disjunct> &values) {
  bool first = true;
  for (auto &val : values) {
    if (!first)
      stream << ", ";
    first = false;
    stream << val;
  }
  return stream;
}

template <typename T> auto resolveNice(const T &axioms, const T &target) {
  std::cout << "axioms:  " << axioms << std::endl;
  std::cout << "~target: " << target << std::endl;

  auto res = Resolver().resolve(axioms, target);
  return res;
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

TEST(SubstTest, hardCase) {
  Subst left;
  left.insert(
      "x", std::make_shared<Variable>(
               false, "f", std::vector{std::make_shared<Variable>(true, "y")}));
  Subst right;
  right.insert("x", std::make_shared<Variable>(
                        false, "f",
                        std::vector{std::make_shared<Variable>(false, "A")}));

  auto sum = left + right;
  EXPECT_TRUE(sum);
  if (sum) {
    std::cout << "got " << sum->toString() << std::endl;
  }
}

TEST(DisjunctTest, renameVars) {
  Disjunct disj = parseDisjunct("P(x, y, x3) + Q(f(x1, y), z)");

  NameAllocator allocator;
  disj.commitVarNames(allocator);

  std::cout << "allocated names: " << allocator.toString() << std::endl;
  Disjunct disj2 = disj.renamedVars(allocator);
  std::cout << "allocated after: " << allocator.toString() << std::endl;

  EXPECT_EQ(disj2.toString(), "P(x2, y1, x4) + Q(f(x5, y1), z1)");
}

TEST(ResolverNewTest, unifySimple) {
  auto left = parseAtom("~R(x)");
  auto right = parseAtom("R(A)");

  auto res = Resolver().unify(left, right);
  EXPECT_TRUE(res);
  std::cout << "left: " << left.toString() << std::endl
            << "right: " << right.toString() << std::endl
            << "subst: " << res->toString() << std::endl;
}

TEST(ResolverNewTest, conflictLinkage) {
  auto left = parseAtom("~Q(S, x, F, G, y, H)");
  auto right = parseAtom("Q(y, A, F, G, x, H)");

  auto res = Resolver().unify(left, right);
  EXPECT_FALSE(res);
}

TEST(ResolverNewTest, conflictLinkage2) {
  auto left = parseAtom("~B(P, x, r(G, Q), K, T, y, U)");
  auto right = parseAtom("B(P, Q, r(G, Q), y, T, x, U)");

  auto res = Resolver().unify(left, right);
  EXPECT_FALSE(res);
}

TEST(ResolverNewTest, subFuncs) {
  auto left = parseAtom("~P(x, f(x, B, g(y)), g(C))");
  auto right = parseAtom("P(A, f(A, B, z), g(y))");

  auto res = Resolver().unify(left, right);
  ASSERT_TRUE(res);
  EXPECT_EQ(res->toString(), "{x=A, y=C, z=g(C)}");
}

TEST(ResolverNewTest, recursiveFunc) {
  auto left = parseAtom("~P(x, f(g(x)))");
  auto right = parseAtom("P(y, y)");

  auto res = Resolver().unify(left, right);
  ASSERT_TRUE(res);
  EXPECT_EQ(res->toString(), "{x=y=f(g(...))}");
}

TEST(ResolverNewTest, recursiveHard) {
  auto left = parseAtom("~H(x, g(z), z, o(x))");
  auto right = parseAtom("H(f(y), y, t(x), o(x))");

  auto res = Resolver().unify(left, right);
  ASSERT_TRUE(res);
  EXPECT_EQ(res->toString(),
            "{x=f(g(t(...))), y=g(t(f(...))), z=t(f(g(...)))}");
}

TEST(ResolverNewTest, recursiveConflict) {
  auto left = parseAtom("~R(x, y, g(y))");
  auto right = parseAtom("R(f(y), k(x), x)");

  auto res = Resolver().unify(left, right);
  EXPECT_FALSE(res);
}

TEST(ResolverNewTest, resolutionSimple) {
  auto axioms = parseDisjuncts("A & (~A + B)");
  auto target = parseDisjuncts("~B");

  auto res = resolveNice(axioms, target);

  EXPECT_TRUE(res);
  if (res)
    std::cout << "res: " << res->toString() << std::endl;
}

TEST(ResolverNewTest, resolutionTransitive) {
  auto axioms = parseDisjuncts("(A -> B) & (B -> C)");
  auto target = parseDisjuncts("~(A -> C)");

  auto res = resolveNice(axioms, target);

  EXPECT_TRUE(res);
  if (res)
    std::cout << "res: " << res->toString() << std::endl;
}

TEST(ResolverNewTest, resolutionPredicates) {
  auto axioms =
      parseDisjuncts("(A -> \\exists(x) P(x)) & (\\exists(x) (Q(x) -> A))");
  auto target = parseDisjuncts("~(\\exists(x) (Q(x) -> P(x)))");

  auto res = resolveNice(axioms, target);

  EXPECT_TRUE(res);
  if (res)
    std::cout << "res: " << res->toString() << std::endl;
}

TEST(ResolverNewTest, resolutionTransitivePred) {
  auto axioms =
      parseDisjuncts("(\\forall(x, y, z) ((P(x, z) & P(z, y)) -> P(x, y))) & "
                     "P(3, 4) & P(4, 5) & P(5, 6)");
  auto target = parseDisjuncts("~P(3, 6)");

  auto res = resolveNice(axioms, target);

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

  auto res = resolveNice(axioms, target);

  EXPECT_TRUE(res);
  if (res)
    std::cout << "res: " << res->toString() << std::endl;
}

TEST(ResolverNewTest, listLen2) {
  auto axioms = parseDisjuncts("\\forall(x, y, z) (Len(x, y) -> Len(cons(z, "
                               "x), succ(y))) & Len(Nil, 0)");
  auto target = parseDisjuncts("~(\\exists(x) Len(x, succ(succ(0))))");

  auto res = resolveNice(axioms, target);

  EXPECT_TRUE(res);
}
