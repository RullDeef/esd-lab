#include "rule.h"
#include "rule_parser.h"
#include <gtest/gtest.h>

using namespace std::string_literals;

TEST(RuleTests, printer) {
  auto rule = Rule::createDisjunction(
      Rule::createConjunction(Rule::createAtom("A"),
                              Rule::createInverse(Rule::createAtom("B"))),
      Rule::createDisjunction(Rule::createInverse(Rule::createAtom("C")),
                              Rule::createAtom("D")));

  auto expected = "(A & ~B) + (~C + D)"s;
  auto actual = rule->toString();

  EXPECT_EQ(expected, actual);
}

TEST(RuleTests, cnfSimple) {
  auto rule = Rule::createInverse(Rule::createInverse(Rule::createAtom("A")));

  auto cnf = rule->toNormalForm();
  ASSERT_NE(cnf, nullptr);

  auto expected = "A"s;
  auto actual = cnf->toString();

  EXPECT_EQ(expected, actual);
}

TEST(RuleTests, cnfConjunctions) {
  // (A & D) & (B & C)
  auto rule = Rule::createConjunction(
      Rule::createConjunction(Rule::createAtom("A"), Rule::createAtom("D")),
      Rule::createConjunction(Rule::createAtom("B"), Rule::createAtom("C")));

  auto cnf = rule->toNormalForm();

  auto expected = "A & D & B & C"s;
  auto actual = cnf->toString();

  EXPECT_EQ(expected, actual);
}

TEST(RuleTests, cnfDisjunctions) {
  // (A + D) + (B + C)
  auto rule = Rule::createDisjunction(
      Rule::createDisjunction(Rule::createAtom("A"), Rule::createAtom("D")),
      Rule::createDisjunction(Rule::createAtom("B"), Rule::createAtom("C")));

  auto cnf = rule->toNormalForm();

  auto expected = "A + D + B + C"s;
  auto actual = cnf->toString();

  EXPECT_EQ(expected, actual);
}

TEST(RuleTests, cnfMedium) {
  // (~A & B) + A
  auto ruleA = Rule::createAtom("A");
  auto ruleB = Rule::createAtom("B");
  auto rule = Rule::createDisjunction(
      Rule::createConjunction(Rule::createInverse(ruleA), ruleB), ruleA);

  EXPECT_EQ("(~A & B) + A"s, rule->toString());

  auto cnf = rule->toNormalForm();

  auto expected = "B + A"s;
  auto actual = cnf->toString();

  EXPECT_EQ(expected, actual);
}

TEST(RuleTests, cnfEquality) {
  auto rule =
      Rule::createEquality(Rule::createAtom("A"), Rule::createAtom("B"));

  EXPECT_EQ("(~A + B) & (~B + A)", rule->toString());

  auto cnf = rule->toNormalForm();

  auto expected = "(~A + B) & (~B + A)";
  auto actual = cnf->toString();

  EXPECT_EQ(expected, actual);
}

TEST(RuleTests, cnfHard) {
  // ((A -> B) & A) -> B
  // ~((~A + B) & A) + B
  // (A & ~B) + ~A + B
  // (A + ~A + B) & (~B + ~A + B)
  auto ruleA = Rule::createAtom("A");
  auto ruleB = Rule::createAtom("B");
  auto rule = Rule::createImplication(
      Rule::createConjunction(Rule::createImplication(ruleA, ruleB), ruleA),
      ruleB);

  EXPECT_EQ("~((~A + B) & A) + B"s, rule->toString());

  auto cnf = rule->toNormalForm();

  auto expected = "1"s;
  auto actual = cnf->toString();

  EXPECT_EQ(expected, actual);
}

TEST(RuleTests, cnfVeryHard) {
  // contraposition rule:
  // (A -> B) <-> (~B -> ~A)
  auto ruleA = Rule::createAtom("A");
  auto ruleB = Rule::createAtom("B");
  auto rule =
      Rule::createEquality(Rule::createImplication(ruleA, ruleB),
                           Rule::createImplication(Rule::createInverse(ruleB),
                                                   Rule::createInverse(ruleA)));

  auto cnf = rule->toNormalForm();

  auto expected =
      "1"s; // "(A + B + ~A) & (~B + B + ~A) & (~B + ~A + B) & (A + ~A + B)"s;
  auto actual = cnf->toString();

  EXPECT_EQ(expected, actual);
}

TEST(RuleTests, identityReduction) {
  // ~(A <-> B)
  auto rule = Rule::createInverse(
      Rule::createEquality(Rule::createAtom("A"), Rule::createAtom("B")));

  auto cnf = rule->toNormalForm();
  auto expected = "(A + B) & (~B + ~A)";
  auto actual = cnf->toString();

  EXPECT_EQ(expected, actual);
}

TEST(RuleTests, predicateNormalization) {
  auto text = "P(X) + Q(Y) & R(Z, g(X))";
  Rule::ptr rule;

  EXPECT_NO_THROW(rule = RuleParser().Parse(text));

  rule = rule->toNormalForm();

  auto expected = "(P(X) + Q(Y)) & (P(X) + R(Z, g(X)))";
  auto actual = rule->toString();

  EXPECT_EQ(expected, actual);
}

TEST(RuleTests, differentPredicates) {
  auto text = "P(X) + P(Y)";
  Rule::ptr rule;

  EXPECT_NO_THROW(rule = RuleParser().Parse(text));

  rule = rule->toNormalForm();

  auto expected = "P(X) + P(Y)";
  auto actual = rule->toString();

  EXPECT_EQ(expected, actual);
}

TEST(RuleTests, simpleQuantifiers) {
  auto text = "P(X) & \\forall(Y) R(Y) + \\exists (Y) Q(Y, Z)";
  Rule::ptr rule;

  EXPECT_NO_THROW(rule = RuleParser().Parse(text));

  rule = rule->toNormalForm();

  auto expected =
      "\\forall(Y) (\\exists(Y1) ((P(X) + Q(Y1, Z)) & (R(Y) + Q(Y1, Z))))";
  auto actual = rule->toString();

  EXPECT_EQ(expected, actual);
}

TEST(RuleTests, redundantVars) {
  auto text = "\\forall(X, Y, Z) (P(X) + \\forall(Z, W) Q(Y))";
  Rule::ptr rule;

  EXPECT_NO_THROW(rule = RuleParser().Parse(text));

  rule = rule->toNormalForm();

  auto expected = "\\forall(X, Y) (P(X) + Q(Y))";
  auto actual = rule->toString();

  EXPECT_EQ(expected, actual);
}

TEST(RuleTests, transitivity) {
  auto text = "(gt(X, Y) & gt(Y, Z)) -> gt(X, Z)";
  Rule::ptr rule;

  EXPECT_NO_THROW(rule = RuleParser().Parse(text));

  rule = rule->toNormalForm();

  auto expected = "~gt(X, Y) + ~gt(Y, Z) + gt(X, Z)";
  auto actual = rule->toString();

  EXPECT_EQ(expected, actual);
}
