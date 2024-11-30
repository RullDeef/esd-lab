#include "name_allocator.h"
#include "parser.h"
#include <gtest/gtest.h>

TEST(NameAllocatorTest, ruleRenaming) {
  auto rule = RuleParser().ParseRule("len(cons(_, x), succ(n)) :- len(x, n)");

  NameAllocator allocator;

  EXPECT_EQ(allocator.allocateRenaming("x"), "x1");
  EXPECT_EQ(allocator.allocateRenaming("n"), "n1");
  EXPECT_EQ(allocator.allocateRenaming("x"), "x1");
  EXPECT_EQ(allocator.allocateRenaming("n"), "n1");
}
