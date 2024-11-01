#include "expr.h"
#include "expr_parser.h"
#include <gtest/gtest.h>

using namespace std::string_literals;

TEST(ExprTests, printer) {
  auto rule = Expr::createDisjunction(
      Expr::createConjunction(Expr::createAtom("A"),
                              Expr::createInverse(Expr::createAtom("B"))),
      Expr::createDisjunction(Expr::createInverse(Expr::createAtom("C")),
                              Expr::createAtom("D")));

  auto expected = "(A & ~B) + (~C + D)"s;
  auto actual = rule->toString();

  EXPECT_EQ(expected, actual);
}

TEST(ExprTests, cnfSimple) {
  auto rule = Expr::createInverse(Expr::createInverse(Expr::createAtom("A")));

  auto cnf = rule->toNormalForm();
  ASSERT_NE(cnf, nullptr);

  auto expected = "A"s;
  auto actual = cnf->toString();

  EXPECT_EQ(expected, actual);
}

TEST(ExprTests, cnfConjunctions) {
  // (A & D) & (B & C)
  auto rule = Expr::createConjunction(
      Expr::createConjunction(Expr::createAtom("A"), Expr::createAtom("D")),
      Expr::createConjunction(Expr::createAtom("B"), Expr::createAtom("C")));

  auto cnf = rule->toNormalForm();

  auto expected = "A & D & B & C"s;
  auto actual = cnf->toString();

  EXPECT_EQ(expected, actual);
}

TEST(ExprTests, cnfDisjunctions) {
  // (A + D) + (B + C)
  auto rule = Expr::createDisjunction(
      Expr::createDisjunction(Expr::createAtom("A"), Expr::createAtom("D")),
      Expr::createDisjunction(Expr::createAtom("B"), Expr::createAtom("C")));

  auto cnf = rule->toNormalForm();

  auto expected = "A + D + B + C"s;
  auto actual = cnf->toString();

  EXPECT_EQ(expected, actual);
}

TEST(ExprTests, cnfMedium) {
  // (~A & B) + A
  auto ruleA = Expr::createAtom("A");
  auto ruleB = Expr::createAtom("B");
  auto rule = Expr::createDisjunction(
      Expr::createConjunction(Expr::createInverse(ruleA), ruleB), ruleA);

  EXPECT_EQ("(~A & B) + A"s, rule->toString());

  auto cnf = rule->toNormalForm();

  auto expected = "B + A"s;
  auto actual = cnf->toString();

  EXPECT_EQ(expected, actual);
}

TEST(ExprTests, cnfEquality) {
  auto rule =
      Expr::createEquality(Expr::createAtom("A"), Expr::createAtom("B"));

  EXPECT_EQ("(~A + B) & (~B + A)", rule->toString());

  auto cnf = rule->toNormalForm();

  auto expected = "(~A + B) & (~B + A)";
  auto actual = cnf->toString();

  EXPECT_EQ(expected, actual);
}

TEST(ExprTests, cnfHard) {
  // ((A -> B) & A) -> B
  // ~((~A + B) & A) + B
  // (A & ~B) + ~A + B
  // (A + ~A + B) & (~B + ~A + B)
  auto ruleA = Expr::createAtom("A");
  auto ruleB = Expr::createAtom("B");
  auto rule = Expr::createImplication(
      Expr::createConjunction(Expr::createImplication(ruleA, ruleB), ruleA),
      ruleB);

  EXPECT_EQ("~((~A + B) & A) + B"s, rule->toString());

  auto cnf = rule->toNormalForm();

  auto expected = "1"s;
  auto actual = cnf->toString();

  EXPECT_EQ(expected, actual);
}

TEST(ExprTests, cnfVeryHard) {
  // contraposition rule:
  // (A -> B) <-> (~B -> ~A)
  auto ruleA = Expr::createAtom("A");
  auto ruleB = Expr::createAtom("B");
  auto rule =
      Expr::createEquality(Expr::createImplication(ruleA, ruleB),
                           Expr::createImplication(Expr::createInverse(ruleB),
                                                   Expr::createInverse(ruleA)));

  auto cnf = rule->toNormalForm();

  auto expected =
      "1"s; // "(A + B + ~A) & (~B + B + ~A) & (~B + ~A + B) & (A + ~A + B)"s;
  auto actual = cnf->toString();

  EXPECT_EQ(expected, actual);
}

TEST(ExprTests, identityReduction) {
  // ~(A <-> B)
  auto rule = Expr::createInverse(
      Expr::createEquality(Expr::createAtom("A"), Expr::createAtom("B")));

  auto cnf = rule->toNormalForm();
  auto expected = "(A + B) & (~B + ~A)";
  auto actual = cnf->toString();

  EXPECT_EQ(expected, actual);
}

TEST(ExprTests, predicateNormalization) {
  auto text = "P(X) + Q(Y) & R(Z, g(X))";
  Expr::ptr rule;

  EXPECT_NO_THROW(rule = ExprParser().Parse(text));

  rule = rule->toNormalForm();

  auto expected = "(P(X) + Q(Y)) & (P(X) + R(Z, g(X)))";
  auto actual = rule->toString();

  EXPECT_EQ(expected, actual);
}

TEST(ExprTests, differentPredicates) {
  auto text = "P(X) + P(Y)";
  Expr::ptr rule;

  EXPECT_NO_THROW(rule = ExprParser().Parse(text));

  rule = rule->toNormalForm();

  auto expected = "P(X) + P(Y)";
  auto actual = rule->toString();

  EXPECT_EQ(expected, actual);
}

TEST(ExprTests, simpleQuantifiers) {
  auto text = "P(X) & \\forall(y) R(y) + \\exists (y) Q(y, Z)";
  Expr::ptr rule;

  EXPECT_NO_THROW(rule = ExprParser().Parse(text));

  rule = rule->toNormalForm();

  auto expected =
      "\\forall(y) (\\exists(y1) ((P(X) + Q(y1, Z)) & (R(y) + Q(y1, Z))))";
  auto actual = rule->toString();

  EXPECT_EQ(expected, actual);
}

TEST(ExprTests, redundantVars) {
  auto text = "\\forall(x, y, z) (P(x) + \\forall(z, w) Q(y))";
  Expr::ptr rule;

  EXPECT_NO_THROW(rule = ExprParser().Parse(text));

  rule = rule->toNormalForm();

  auto expected = "\\forall(x, y) (P(x) + Q(y))";
  auto actual = rule->toString();

  EXPECT_EQ(expected, actual);
}

TEST(ExprTests, transitivity) {
  auto text = "(gt(X, Y) & gt(Y, Z)) -> gt(X, Z)";
  Expr::ptr rule;

  EXPECT_NO_THROW(rule = ExprParser().Parse(text));

  rule = rule->toNormalForm();

  auto expected = "~gt(X, Y) + ~gt(Y, Z) + gt(X, Z)";
  auto actual = rule->toString();

  EXPECT_EQ(expected, actual);
}

TEST(ExprTests, toNormalIdempotent) {
  auto rule =
      ExprParser().Parse("(~A + \\exists(x) P(x)) & \\exists(x) (~Q(x) + A)");

  auto ruleBefore = rule->toString();
  rule->toNormalForm();
  rule->toNormalForm();
  auto ruleAfter = rule->toString();

  EXPECT_EQ(ruleBefore, ruleAfter);
}

TEST(ExprTests, toScolemForm) {
  auto rule =
      ExprParser().Parse("(~A + \\exists(x) P(x)) & \\exists(x) (~Q(x) + A)");

  auto ruleBefore = rule->toString();
  rule->toScolemForm();
  rule->toScolemForm();
  auto ruleAfter = rule->toString();

  EXPECT_EQ(ruleBefore, ruleAfter);
}
