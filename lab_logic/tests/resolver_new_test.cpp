#include "normalizer.h"
#include "parser/expr_parser.h"
#include "resolver_new.h"
#include <gtest/gtest.h>
#include <memory>

Atom parseAtom(const char *text) {
  auto disj = ExprNormalizer().scolemForm(ExprParser().Parse(text));
  return *disj[0].begin();
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
