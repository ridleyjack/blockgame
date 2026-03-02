#include "Engine/Memory/RingBuffer.hpp"

#include <gtest/gtest.h>

namespace ctn = engine::memory;

TEST(RingBufferTest, SimpleAllocations) {
  ctn::RingBuffer buffer{8};

  EXPECT_EQ(buffer.Allocate(2, 2), 0);
  EXPECT_EQ(buffer.Allocate(4, 2), 2);
  EXPECT_EQ(buffer.Allocate(2, 2), 6);
}

TEST(RingBufferTest, AllocationWhenFull) {
  ctn::RingBuffer buffer{8};

  EXPECT_EQ(buffer.Allocate(8, 2), 0);
  EXPECT_EQ(buffer.Allocate(2, 2), std::numeric_limits<std::uint64_t>::max());
}

TEST(RingBufferTest, SimpleFree) {
  ctn::RingBuffer buffer{8};

  EXPECT_EQ(buffer.Allocate(4, 2), 0);
  EXPECT_EQ(buffer.PopFront(), 0);
  EXPECT_EQ(buffer.Allocate(4, 2), 0);
}

TEST(RingBufferTest, LoopToFront) {
  ctn::RingBuffer buffer{8};

  EXPECT_EQ(buffer.Allocate(4, 2), 0);
  EXPECT_EQ(buffer.Allocate(4, 2), 4);
  EXPECT_EQ(buffer.PopFront(), 0);
  EXPECT_EQ(buffer.Allocate(4, 2), 0);
}

TEST(RingBufferTest, LoopToFrontLeavingBackFragment) {
  ctn::RingBuffer buffer{8};

  EXPECT_EQ(buffer.Allocate(4, 2), 0);
  EXPECT_EQ(buffer.Allocate(2, 2), 4);
  // Leave 7-8 Free
  EXPECT_EQ(buffer.PopFront(), 0);
  EXPECT_EQ(buffer.Allocate(4, 2), 0);
}

TEST(RingBufferTest, DoubleWrap) {
  ctn::RingBuffer buffer{8};

  EXPECT_EQ(buffer.Allocate(4, 2), 0);
  EXPECT_EQ(buffer.Allocate(4, 2), 4);
  EXPECT_EQ(buffer.PopFront(), 0);
  EXPECT_EQ(buffer.Allocate(4, 2), 0);
  EXPECT_EQ(buffer.Allocate(2, 2), std::numeric_limits<std::uint64_t>::max());
}

TEST(RingBufferTest, EmptyAfterFullFree) {
  ctn::RingBuffer buffer{8};

  EXPECT_EQ(buffer.Allocate(4, 2), 0);
  EXPECT_EQ(buffer.PopFront(), 0);

  EXPECT_EQ(buffer.Allocate(8, 2), 0);
}