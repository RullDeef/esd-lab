#include "rule.h"
#include <gtest/gtest.h>

using namespace std::string_literals;

class RuleTests : public ::testing::Test {};

TEST_F(RuleTests, printer) {
  auto rule = Rule::createDisjunction(
      Rule::createConjunction(Rule::createAtom("A"),
                              Rule::createInverse(Rule::createAtom("B"))),
      Rule::createDisjunction(Rule::createInverse(Rule::createAtom("C")),
                              Rule::createAtom("D")));

  auto expected = "(A & ~B) + (~C + D)"s;
  auto actual = rule->toString();

  EXPECT_EQ(expected, actual);
}

TEST_F(RuleTests, cnfSimple) {
  auto rule = Rule::createInverse(Rule::createInverse(Rule::createAtom("A")));

  auto cnf = rule->toNormalForm();
  ASSERT_NE(cnf, nullptr);

  auto expected = "A"s;
  auto actual = cnf->toString();

  EXPECT_EQ(expected, actual);
}

TEST_F(RuleTests, cnfConjunctions) {
  // (A & D) & (B & C)
  auto rule = Rule::createConjunction(
      Rule::createConjunction(Rule::createAtom("A"), Rule::createAtom("D")),
      Rule::createConjunction(Rule::createAtom("B"), Rule::createAtom("C")));

  auto cnf = rule->toNormalForm();

  auto expected = "A & D & B & C"s;
  auto actual = cnf->toString();

  EXPECT_EQ(expected, actual);
}

TEST_F(RuleTests, cnfDisjunctions) {
  // (A + D) + (B + C)
  auto rule = Rule::createDisjunction(
      Rule::createDisjunction(Rule::createAtom("A"), Rule::createAtom("D")),
      Rule::createDisjunction(Rule::createAtom("B"), Rule::createAtom("C")));

  auto cnf = rule->toNormalForm();

  auto expected = "A + D + B + C"s;
  auto actual = cnf->toString();

  EXPECT_EQ(expected, actual);
}

TEST_F(RuleTests, cnfMedium) {
  // (~A & B) + A
  auto ruleA = Rule::createAtom("A");
  auto ruleB = Rule::createAtom("B");
  auto rule = Rule::createDisjunction(
      Rule::createConjunction(Rule::createInverse(ruleA), ruleB), ruleA);

  EXPECT_EQ("(~A & B) + A"s, rule->toString());

  auto cnf = rule->toNormalForm();

  auto expected = "(~A + A) & (B + A)"s;
  auto actual = cnf->toString();

  EXPECT_EQ(expected, actual);
}

TEST_F(RuleTests, cnfHard) {
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

  auto expected = "(A + ~A + B) & (~B + ~A + B)"s;
  auto actual = cnf->toString();

  EXPECT_EQ(expected, actual);
}

TEST_F(RuleTests, cnfVeryHard) {
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
      "(A + B + ~A) & (~B + B + ~A) & (~B + ~A + B) & (A + ~A + B)"s;
  auto actual = cnf->toString();

  EXPECT_EQ(expected, actual);
}
