// Copyright (c) 2023. Akiscode
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>

#include "rtlcpp/rtlcpp.hpp"

struct LRUSystemAllocator {
  void* allocate(size_t sz) { return malloc(sz); }
  void deallocate(void* p) { free(p); }
};

class LRUTest : public ::testing::Test {
 protected:
  rtl::MMapMemoryResource mr;
  rtl::RTAllocatorMT allocMT;

  LRUTest() {
    // You can do set-up work for each test here.
  }

  ~LRUTest() override {
    // You can do clean-up work that doesn't throw exceptions here.
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).

    ASSERT_TRUE(mr.init(std::min(static_cast<size_t>(50 * 1024 * 1024),
                                  rtl_tlsf_maximum_arena_size())));

    ASSERT_TRUE(allocMT.init(mr.get_buf(), mr.get_capacity()));
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).

    allocMT.uninit();

    mr.uninit();
  }
};

TEST_F(LRUTest, ResetTest) {
  LRUSystemAllocator s;
  rtl::lru<int, int, LRUSystemAllocator> lru(&s, 100);

  for (int i = 0; i < 10; i++) {
    ASSERT_TRUE(lru.put(i, i));
  }

  for (int i = 0; i < 10; i++) {
    ASSERT_TRUE(lru.contains(i));
  }

  ASSERT_FALSE(lru.empty());
  lru.reset();
  ASSERT_TRUE(lru.empty());

  for (int i = 0; i < 10; i++) {
    ASSERT_FALSE(lru.contains(i));
  }

  for (int i = 0; i < 10; i++) {
    ASSERT_TRUE(lru.put(i, i + 1));
  }

  for (int i = 0; i < 10; i++) {
    ASSERT_EQ(*lru.get_ptr(i), i + 1);
  }
}

TEST_F(LRUTest, MovingTest) {
  LRUSystemAllocator s;
  rtl::lru<int, int, LRUSystemAllocator> lru1(&s, 2);

  ASSERT_TRUE(lru1.put(2, 3));
  ASSERT_TRUE(lru1.contains(2));

  rtl::lru<int, int, LRUSystemAllocator> lru2(std::move(lru1));

  ASSERT_FALSE(lru1.contains(2));
  ASSERT_TRUE(lru2.contains(2));
}

TEST_F(LRUTest, MemoryLeakTest) {
  LRUSystemAllocator s;
  rtl::lru<int, int, LRUSystemAllocator> lru(&s, 2);

  ASSERT_TRUE(lru.put(2, 3));
  ASSERT_TRUE(lru.put(3, 3));
  ASSERT_TRUE(lru.put(3, 4));
  ASSERT_TRUE(lru.put(4, 4));
  ASSERT_TRUE(lru.put(6, 4));

  rtl::lru<int, int, LRUSystemAllocator> lru2(&s, 2);
  ASSERT_TRUE(lru2.put(3, 4));
  ASSERT_TRUE(lru2.put(4, 4));

  lru = std::move(lru2);

  rtl::lru<int, int, LRUSystemAllocator> lru3(std::move(lru));
}

TEST_F(LRUTest, SmokeTest) {
  rtl::lru<int, int> lru(&allocMT, 2);

  ASSERT_FALSE(lru.contains(2));
  ASSERT_TRUE(lru.put(2, 3));
  ASSERT_TRUE(lru.contains(2));

  int rval{-1};

  ASSERT_TRUE(lru.get(2, &rval));
  ASSERT_EQ(rval, 3);
  ASSERT_FALSE(lru.get(1, &rval));
  ASSERT_EQ(rval, 3);

  ASSERT_TRUE(lru.put(1, 1));
  ASSERT_TRUE(lru.put(1, 5));

  ASSERT_TRUE(lru.get(1, &rval));
  ASSERT_EQ(rval, 5);

  ASSERT_TRUE(lru.get(2, &rval));
  ASSERT_EQ(rval, 3);

  ASSERT_TRUE(lru.put(9, 10));

  ASSERT_FALSE(lru.get(1, &rval));
  ASSERT_EQ(rval, 3);

  ASSERT_TRUE(lru.get(9, &rval));
  ASSERT_EQ(rval, 10);
}