#include "resolver.h"
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
