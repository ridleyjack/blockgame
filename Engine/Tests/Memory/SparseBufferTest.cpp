#include "../../Src/Engine/Memory/SparseBuffer.hpp"

#include <gtest/gtest.h>

namespace ctn = engine::memory;

TEST(SparseBufferTest, SimpleAllocations) {
  ctn::SparseBuffer buffer{8};

  EXPECT_EQ(buffer.Allocate(2, 2), 0);
  EXPECT_EQ(buffer.Allocate(4, 2), 2);
  EXPECT_EQ(buffer.Allocate(2, 2), 6);
}

TEST(SparseBufferTest, AllocateOverCapacity) {
  ctn::SparseBuffer buffer{8};
  buffer.Allocate(6, 2);
  EXPECT_EQ(buffer.Allocate(4, 2), std::numeric_limits<std::uint64_t>::max());
}

TEST(SparseBufferTest, AllocateWithPadding) {
  ctn::SparseBuffer buffer{8};
  buffer.Allocate(3, 2);
  EXPECT_EQ(buffer.Allocate(1, 2), 4);
}

TEST(SparseBufferTest, ExactFit) {
  ctn::SparseBuffer buffer{8};
  buffer.Allocate(8, 1);
  EXPECT_EQ(buffer.Allocate(1, 1), std::numeric_limits<std::uint64_t>::max());
}

TEST(SparseBufferTest, AlignedOffsetOverCapacity) {
  ctn::SparseBuffer buffer{8};
  EXPECT_EQ(buffer.Allocate(4, 16), 0);
  EXPECT_EQ(buffer.Allocate(2, 16), std::numeric_limits<std::uint64_t>::max());
}

TEST(SparseBufferTest, BigAlignment) {
  ctn::SparseBuffer buffer{32};
  EXPECT_EQ(buffer.Allocate(4, 16), 0);
  EXPECT_EQ(buffer.Allocate(4, 16), 16);
}

TEST(SparseBufferTest, SingleFree) {
  ctn::SparseBuffer buffer{8};
  EXPECT_EQ(buffer.Allocate(4, 2), 0);
  buffer.Free(0);
}

TEST(SparseBufferTest, SequentialFree) {
  ctn::SparseBuffer buffer{8};
  EXPECT_EQ(buffer.Allocate(4, 2), 0);
  EXPECT_EQ(buffer.Allocate(2, 2), 4);
  EXPECT_EQ(buffer.Allocate(2, 2), 6);

  buffer.Free(6);
  buffer.Free(4);
  buffer.Free(0);

  EXPECT_EQ(buffer.Allocate(4, 2), 0);
  EXPECT_EQ(buffer.Allocate(2, 2), 4);
  EXPECT_EQ(buffer.Allocate(2, 2), 6);
}

TEST(SparseBufferTest, PartialFree) {
  ctn::SparseBuffer buffer{8};
  EXPECT_EQ(buffer.Allocate(4, 2), 0);
  EXPECT_EQ(buffer.Allocate(2, 2), 4);

  buffer.Free(4);

  EXPECT_EQ(buffer.Allocate(4, 2), 4);
}

TEST(SparseBufferTest, TwoSidedMerge) {
  ctn::SparseBuffer buffer{8};
  EXPECT_EQ(buffer.Allocate(4, 2), 0);
  EXPECT_EQ(buffer.Allocate(2, 2), 4);
  EXPECT_EQ(buffer.Allocate(2, 2), 6);

  buffer.Free(0);
  buffer.Free(6);
  buffer.Free(4);

  EXPECT_EQ(buffer.FreeCapacity(), 8);

  EXPECT_EQ(buffer.Allocate(4, 2), 0);
  EXPECT_EQ(buffer.Allocate(2, 2), 4);
  EXPECT_EQ(buffer.Allocate(2, 2), 6);
}

TEST(SparseBufferTest, FreeThenUseMiddle) {
  ctn::SparseBuffer buffer{12};
  EXPECT_EQ(buffer.Allocate(4, 2), 0);
  EXPECT_EQ(buffer.Allocate(4, 2), 4);
  EXPECT_EQ(buffer.Allocate(4, 2), 8);

  buffer.Free(4);

  EXPECT_EQ(buffer.FreeCapacity(), 4);
  EXPECT_EQ(buffer.Allocate(4, 2), 4);
}
