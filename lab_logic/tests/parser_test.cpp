#include "rule_parser.h"
#include <gtest/gtest.h>

TEST(ParserTest, SimpleCase) {
  const char *str = "A";
  Rule::ptr rule;

  EXPECT_NO_THROW(rule = RuleParser().Parse(str));

  const char *expected = "A";
  EXPECT_EQ(expected, rule->toString());
}

TEST(ParserTest, SimpleNegation) {
  const char *str = " ~ B  ";
  Rule::ptr rule;

  EXPECT_NO_THROW(rule = RuleParser().Parse(str));

  auto expected = "~B";
  EXPECT_EQ(expected, rule->toString());
}

TEST(ParserTest, DoubleNegation) {
  const char *str = "~~C";
  Rule::ptr rule;

  EXPECT_NO_THROW(rule = RuleParser().Parse(str));

  EXPECT_EQ(str, rule->toString());
}

TEST(ParserTest, Parenthesis) {
  const char *str = "( ~((~A) ) )";
  Rule::ptr rule;

  EXPECT_NO_THROW(rule = RuleParser().Parse(str));

  const char *expected = "~~A";
  EXPECT_EQ(expected, rule->toString());
}

TEST(ParserTest, Conjunction) {
  const char *str = "A & ~B";
  Rule::ptr rule;

  EXPECT_NO_THROW(rule = RuleParser().Parse(str));

  EXPECT_EQ(str, rule->toString());
}

TEST(ParserTest, Disjunction) {
  const char *str = "A + ( B +C)";
  Rule::ptr rule;

  EXPECT_NO_THROW(rule = RuleParser().Parse(str));

  const char *expected = "A + (B + C)";
  EXPECT_EQ(expected, rule->toString());
}

TEST(ParserTest, Contraposition) {
  const char *str = "(A -> B) == (~B -> ~A)";
  Rule::ptr rule;

  EXPECT_NO_THROW(rule = RuleParser().Parse(str));

  const char *expected = "(~(~A + B) + (~~B + ~A)) & (~(~~B + ~A) + (~A + B))";
  EXPECT_EQ(expected, rule->toString());
}

TEST(ParserTest, SimplePredicate) {
  auto text = "P(X, Y, g(a))";
  Rule::ptr rule;

  EXPECT_NO_THROW(rule = RuleParser().Parse(text));

  auto expected = "P(X, Y, g(a))";
  auto actual = rule->toString();

  EXPECT_EQ(expected, actual);
}

TEST(ParserTest, WithOperators) {
  auto text = "P(X) + ~Q(X, Y) -> R(Y, g(X))";
  Rule::ptr rule;

  EXPECT_NO_THROW(rule = RuleParser().Parse(text));

  auto expected = "P(X) + (~~Q(X, Y) + R(Y, g(X)))";
  auto actual = rule->toString();

  EXPECT_EQ(expected, actual);
}

TEST(ParserTest, Exists) {
  auto text = "\\exists(X) (P(X) -> \\exists(Y) Q(X, Y))";
  Rule::ptr rule;

  EXPECT_NO_THROW(rule = RuleParser().Parse(text));

  auto expected = "\\exists(X) (~P(X) + \\exists(Y) (Q(X, Y)))";
  auto actual = rule->toString();

  EXPECT_EQ(expected, actual);
}

TEST(ParserTest, MultipleVarsInQuantifiers) {
  auto text = "\\exists (a, b) \\forall (X) P(X, a)";
  Rule::ptr rule;

  EXPECT_NO_THROW(rule = RuleParser().Parse(text));

  auto expected = "\\exists(a, b) (\\forall(X) (P(X, a)))";
  auto actual = rule->toString();

  EXPECT_EQ(expected, actual);
}
