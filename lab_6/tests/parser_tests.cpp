#include "parser.h"
#include <gtest/gtest.h>

TEST(ParserTest, Fact) {
  Atom fact;

  ASSERT_NO_FATAL_FAILURE(
      fact =
          RuleParser().ParseRule(" MadeFrom (   Table , Wood )  ").getOutput());

  EXPECT_EQ(fact.toString(), "MadeFrom(Table, Wood)");
}

TEST(ParserTest, Rule) {
  Rule rule;

  ASSERT_NO_FATAL_FAILURE(
      rule = RuleParser().ParseRule("P(x, A) & G(B, y) -> P(x, y)"));

  EXPECT_EQ(rule.toString(), "P(x, y) :- P(x, A), G(B, y)");
}
