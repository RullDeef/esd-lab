#include "channel.h"
#include <future>
#include <gtest/gtest.h>

TEST(ChannelTest, unbuffered) {
  Channel<int> chan;

  auto future = std::async(std::launch::async, [&chan]() { chan.put(321); });

  auto [val, ok] = chan.get();
  future.wait();

  EXPECT_TRUE(ok);
  EXPECT_EQ(val, 321);
}

TEST(ChannelTest, unbufferedStress) {
  Channel<int> chan;

  auto futurePut = std::async(std::launch::async, [&chan]() {
    for (int i = 0; i < 10; ++i) {
      EXPECT_TRUE(chan.put(i));
    }
    chan.close();
  });

  for (int i = 0; i < 10; ++i) {
    auto [val, ok] = chan.get();
    EXPECT_TRUE(ok);
    EXPECT_EQ(val, i);
  }

  futurePut.wait();
  auto [endVal, ok] = chan.get();
  EXPECT_FALSE(ok);
}

TEST(ChannelTest, unbufferedMultiPut) {
  Channel<int> chan;

  std::atomic<int> sum = 0;

  std::vector<std::future<void>> futurePuts;
  for (int i = 0; i < 10; ++i) {
    futurePuts.emplace_back(std::async(std::launch::async, [&chan]() {
      for (int i = 0; i < 100; ++i)
        EXPECT_TRUE(chan.put(i));
    }));
  }

  auto futureGet = std::async(std::launch::async, [&chan, &sum]() {
    for (int i = 0; i < 1000; ++i) {
      auto [val, ok] = chan.get();
      EXPECT_TRUE(ok);
      sum += val;
    }
  });

  for (auto &fut : futurePuts)
    fut.wait();
  chan.close();
  futureGet.wait();
  EXPECT_EQ(sum, 10 * (0 + 99) * 100 / 2);
}

TEST(ChannelTest, buffered) {
  ChannelBuf<int> chan(1);

  chan.put(123);
  auto [val, ok] = chan.get();

  EXPECT_TRUE(ok);
  EXPECT_EQ(val, 123);
}

TEST(ChannelTest, inOtherThread) {
  ChannelBuf<int> chan;

  auto future = std::async(std::launch::async, [&chan]() {
    chan.put(10);
    chan.put(20);
    chan.put(30);
    chan.close();
  });

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

  future.wait();
}
