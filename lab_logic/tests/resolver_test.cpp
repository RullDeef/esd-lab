#include "resolver.h"
#include "rule_parser.h"
#include <gtest/gtest.h>
#include <list>

TEST(ResolverTest, SimpleCase) {
  std::list<Rule::ptr> sources = {
      Rule::createImplication(Rule::createAtom("A"), Rule::createAtom("B")),
      Rule::createAtom("A"),
  };
  Rule::ptr target = Rule::createAtom("B");

  bool satisfied = Resolver().Implies(sources, target);
  ASSERT_TRUE(satisfied);
}

TEST(ResolverTest, ContrapositionHalf) {
  std::list<Rule::ptr> sources = {
      Rule::createImplication(Rule::createAtom("A"), Rule::createAtom("B")),
  };
  Rule::ptr target =
      Rule::createImplication(Rule::createInverse(Rule::createAtom("B")),
                              Rule::createInverse(Rule::createAtom("A")));

  bool satisfied = Resolver().Implies(sources, target);
  ASSERT_TRUE(satisfied);
}

TEST(ResolverTest, identity) {
  Rule::ptr source = Rule::createTrue();
  Rule::ptr target = Rule::createTrue();

  bool satisfied = Resolver().Implies(source, target);
  ASSERT_TRUE(satisfied);
}

TEST(ResolverTest, reversedIdentity) {
  Rule::ptr source = Rule::createTrue();
  Rule::ptr target = Rule::createFalse();

  bool satisfied = Resolver().Implies(source, target);
  ASSERT_FALSE(satisfied);
}

TEST(ResolverTest, equality) {
  Rule::ptr target = RuleParser().Parse("A = A");

  bool satisfied = Resolver().Implies(Rule::createTrue(), target);
  ASSERT_TRUE(satisfied);
}
