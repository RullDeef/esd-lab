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
