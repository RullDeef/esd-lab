#include "parser/expr_parser.h"
#include <gtest/gtest.h>

TEST(ParserTest, SimpleCase) {
  const char *str = "A";
  Expr::ptr rule;

  EXPECT_NO_THROW(rule = ExprParser().Parse(str));

  const char *expected = "A";
  EXPECT_EQ(expected, rule->toString());
}

TEST(ParserTest, SimpleNegation) {
  const char *str = " ~ B  ";
  Expr::ptr rule;

  EXPECT_NO_THROW(rule = ExprParser().Parse(str));

  auto expected = "~B";
  EXPECT_EQ(expected, rule->toString());
}

TEST(ParserTest, DoubleNegation) {
  const char *str = "~~C";
  Expr::ptr rule;

  EXPECT_NO_THROW(rule = ExprParser().Parse(str));

  EXPECT_EQ(str, rule->toString());
}

TEST(ParserTest, Parenthesis) {
  const char *str = "( ~((~A) ) )";
  Expr::ptr rule;

  EXPECT_NO_THROW(rule = ExprParser().Parse(str));

  const char *expected = "~~A";
  EXPECT_EQ(expected, rule->toString());
}

TEST(ParserTest, Conjunction) {
  const char *str = "A & ~B";
  Expr::ptr rule;

  EXPECT_NO_THROW(rule = ExprParser().Parse(str));

  EXPECT_EQ(str, rule->toString());
}

TEST(ParserTest, Disjunction) {
  const char *str = "A + ( B +C)";
  Expr::ptr rule;

  EXPECT_NO_THROW(rule = ExprParser().Parse(str));

  const char *expected = "A + (B + C)";
  EXPECT_EQ(expected, rule->toString());
}

TEST(ParserTest, Contraposition) {
  const char *str = "(A -> B) == (~B -> ~A)";
  Expr::ptr rule;

  EXPECT_NO_THROW(rule = ExprParser().Parse(str));

  const char *expected = "(~(~A + B) + (~~B + ~A)) & (~(~~B + ~A) + (~A + B))";
  EXPECT_EQ(expected, rule->toString());
}

TEST(ParserTest, SimplePredicate) {
  auto text = "P(X, Y, g(a))";
  Expr::ptr rule;

  EXPECT_NO_THROW(rule = ExprParser().Parse(text));

  auto expected = "P(X, Y, g(a))";
  auto actual = rule->toString();

  EXPECT_EQ(expected, actual);
}

TEST(ParserTest, WithOperators) {
  auto text = "P(X) + ~Q(X, Y) -> R(Y, g(X))";
  Expr::ptr rule;

  EXPECT_NO_THROW(rule = ExprParser().Parse(text));

  auto expected = "P(X) + (~~Q(X, Y) + R(Y, g(X)))";
  auto actual = rule->toString();

  EXPECT_EQ(expected, actual);
}

TEST(ParserTest, Exists) {
  auto text = "\\exists(X) (P(X) -> \\exists(Y) Q(X, Y))";
  Expr::ptr rule;

  EXPECT_NO_THROW(rule = ExprParser().Parse(text));

  auto expected = "\\exists(X) (~P(X) + \\exists(Y) (Q(X, Y)))";
  auto actual = rule->toString();

  EXPECT_EQ(expected, actual);
}

TEST(ParserTest, MultipleVarsInQuantifiers) {
  auto text = "\\exists (a, b) \\forall (X) P(X, a)";
  Expr::ptr rule;

  EXPECT_NO_THROW(rule = ExprParser().Parse(text));

  auto expected = "\\exists(a, b) (\\forall(X) (P(X, a)))";
  auto actual = rule->toString();

  EXPECT_EQ(expected, actual);
}
