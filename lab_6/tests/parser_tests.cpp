#include "parser.h"
#include <gtest/gtest.h>

TEST(ParserTest, Fact) {
  Atom fact;

  ASSERT_NO_FATAL_FAILURE(
      fact = RuleParser().ParseFact(" MadeFrom (   Table , Wood )  "));

  EXPECT_EQ(fact.toString(), "MadeFrom(Table, Wood)");
}

TEST(ParserTest, Rule) {
  Rule rule;

  ASSERT_NO_FATAL_FAILURE(
      rule = RuleParser().ParseRule("P(x, A) & G(B, y) -> P(x, y)"));

  EXPECT_EQ(rule.toString(), "P(x, A) & G(B, y) -> P(x, y)");
}
