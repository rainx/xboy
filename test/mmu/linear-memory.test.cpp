
#include "gtest/gtest.h"

#include "mmu/linear-memory.hpp"

namespace mmu {
// class MemoryTest: public ::testing
TEST(linear_memory_test_case, test_linear_memory) {
  LinearMemory<100> memory;
  // should be a empty linear memory space with zero filled
  ASSERT_EQ(memory.get(0), 0);
  ASSERT_EQ(memory.get(99), 0);

  // test set and get
  memory.set(0, 10);
  ASSERT_EQ(memory.get(0), 10);

  // test setWord getWord
  // should be little endian
  ASSERT_EQ(memory.getWord(0), 10);

  // set the high byte
  memory.set(1, 11);
  ASSERT_NE(memory.getWord(0), 10);
  ASSERT_EQ(memory.getWord(0), 0x0b0a);

  // replace the word to 0x0a0b
  memory.setWord(0, 0x0a0b);
  ASSERT_EQ(memory.getWord(0), 0x0a0b);
}
} // namespace mmu
