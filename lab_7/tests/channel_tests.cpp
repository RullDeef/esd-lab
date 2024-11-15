#include "channel.h"
#include <gtest/gtest.h>
#include <thread>

TEST(ChannelTest, normalCase) {
  Channel<int> chan;

  chan.put(123);
  auto [val, ok] = chan.get();

  EXPECT_TRUE(ok);
  EXPECT_EQ(val, 123);
}

TEST(ChannelTest, inOtherThread) {
  Channel<int> chan;

  std::thread([&chan]() {
    chan.put(10);
    chan.put(20);
    chan.put(30);
    chan.close();
  }).detach();

  auto [val1, ok1] = chan.get();
  EXPECT_EQ(val1, 10);
  EXPECT_TRUE(ok1);

  auto [val2, ok2] = chan.get();
  EXPECT_EQ(val2, 20);
  EXPECT_TRUE(ok2);

  auto [val3, ok3] = chan.get();
  EXPECT_EQ(val3, 30);
  EXPECT_TRUE(ok3);

  auto [val4, ok4] = chan.get();
  EXPECT_FALSE(ok4);
}
