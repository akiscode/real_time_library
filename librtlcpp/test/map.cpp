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

#include "rtlcpp/map.hpp"

#include <gtest/gtest.h>

#include <random>
#include <unordered_map>

struct MapSystemAllocator {
  void* allocate(size_t sz) { return malloc(sz); }
  void deallocate(void* p) { free(p); }
};

class MapTest : public ::testing::Test {
 protected:
  rtl::MMapMemoryResource mr1;
  rtl::RTAllocatorMT allocMT;

  MapTest() {
    // You can do set-up work for each test here.
  }

  ~MapTest() override {
    // You can do clean-up work that doesn't throw exceptions here.
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).

    ASSERT_TRUE(mr1.init(std::min(static_cast<size_t>(50 * 1024 * 1024),
                                  rtl_tlsf_maximum_arena_size())));

    ASSERT_TRUE(allocMT.init(mr1.get_buf(), mr1.get_capacity()));
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).

    allocMT.uninit();

    mr1.uninit();
  }
};

TEST_F(MapTest, DeleteAllKeysTest) {
  MapSystemAllocator s;
  rtl::unordered_map<int, int, MapSystemAllocator> m(&s, 0.05);

  int last_deletion{-1};
  uint64_t times_in_transfer{0};
  uint64_t times_in_stable{0};
  for (int i = 0; i < 10000; i++) {
    ASSERT_TRUE(m.put(i % 1234, i % 1234));
    ASSERT_TRUE(m.contains(i % 1234));
    // Arbitrary values except: 52, 226 and 604.  They were chosen to trigger
    // the TRANSFER state
    if (i == 22 || i == 26 || i == 52 || i == 124 || i == 226 || i == 400 ||
        i == 604 || i == 9000) {
      if (last_deletion > 0) {
        // If last deletion was actually set...

        // We should be able to go through all values and match them
        for (int j = last_deletion + 1; j < i; j++) {
          bool rval = m.contains(j % 1234);
          ASSERT_TRUE(rval);
        }
      }

      if (m.get_state() == decltype(m)::MapState::TRANSFER) times_in_transfer++;
      if (m.get_state() == decltype(m)::MapState::STABLE) times_in_stable++;
      m.delete_all_keys();
      last_deletion = i;

      for (int j = 0; j < i; j++) {
        ASSERT_FALSE(m.contains(j % 1234));
      }
    }
  }

  // We spent at least some time in transfer before deletion
  ASSERT_TRUE(times_in_transfer > 2);
  ASSERT_TRUE(times_in_stable > 2);
}

TEST_F(MapTest, MemoryLeakTest) {
  MapSystemAllocator s;
  rtl::unordered_map<int, int, MapSystemAllocator> m(&s, 20);

  ASSERT_TRUE(m.put(1, 1));

  int* o = m.get(1);

  ASSERT_TRUE(o != nullptr);
  ASSERT_EQ(*o, 1);
  ASSERT_TRUE(m.put(1, 3));
  ASSERT_EQ(*o, 3);

  for (int i = 5; i < 10000; i++) {
    ASSERT_TRUE(m.put(i, i + 1));
  }

  ASSERT_EQ(*o, 3);

  for (int i = 5; i < 10000; i++) {
    int* tmp = m.get(i);
    ASSERT_TRUE(tmp != nullptr);
    ASSERT_EQ(*tmp, i + 1);
  }

  rtl::unordered_map<int, int, MapSystemAllocator> m2(&s, 20);
  ASSERT_TRUE(m2.put(1, 1));

  m = std::move(m2);

  rtl::unordered_map<int, int, MapSystemAllocator> m3(std::move(m));
}

TEST_F(MapTest, SmokeTest) {
  rtl::unordered_map<int, int> m(&allocMT, 20);

  ASSERT_TRUE(m.put(1, 1));
  ASSERT_TRUE(m.contains(1));
  ASSERT_FALSE(m.contains(2));

  int* o = m.get(1);
  size_t num_buckets = m.get_num_buckets();

  ASSERT_TRUE(o != nullptr);
  ASSERT_EQ(*o, 1);
  int num3 = 3;
  ASSERT_TRUE(m.put(1, num3));
  ASSERT_EQ(*o, 3);

  ASSERT_TRUE(m.put(5000, 123));
  int* o2 = m.get(5000);
  ASSERT_TRUE(o2 != nullptr);
  ASSERT_EQ(*o2, 123);

  for (int i = 5; i < 10000; i++) {
    ASSERT_TRUE(m.put(i, i + 1));
  }

  ASSERT_EQ(*o2, 5001);

  ASSERT_EQ(*o, 3);
  ASSERT_TRUE(num_buckets < m.get_num_buckets());

  for (int i = 5; i < 10000; i++) {
    int* tmp = m.get(i);
    ASSERT_TRUE(tmp != nullptr);
    ASSERT_EQ(*tmp, i + 1);
  }
}

TEST_F(MapTest, TransferTest) {
  rtl::unordered_map<size_t, size_t> m(&allocMT);

  ASSERT_TRUE(m.put(50, 123));

  size_t* o = m.get(50);
  ASSERT_NE(o, nullptr);
  ASSERT_EQ(*o, 123);

  for (size_t i = 0; i < 99999; i++) {
    if (i == 50) continue;
    ASSERT_TRUE(m.put(i, i + 1));
  }

  ASSERT_EQ(*o, 123);

  for (size_t i = 0; i < 99999; i++) {
    ASSERT_TRUE(m.put(i, i + 1));
  }

  ASSERT_EQ(*o, 51);

  ASSERT_TRUE(m.del(50));
  ASSERT_EQ(m.get(50), nullptr);
}

TEST_F(MapTest, BigContainerTest) {
  rtl::unordered_map<size_t, size_t> m(&allocMT);

  ASSERT_EQ(m.approx_buckets_needed(100000), 20001);

  ASSERT_TRUE(m.reserve(14286));

  for (size_t i = 0; i < 100000; i++) {
    ASSERT_TRUE(m.put(i, i + 1));
  }
}

TEST_F(MapTest, FinalizeTest) {
  rtl::unordered_map<size_t, size_t> m(&allocMT);

  ASSERT_TRUE(m.finalize());

  for (size_t i = 0; i < 100000; i++) {
    m.put(i, i + 1);
    if (m.get_state() == decltype(m)::MapState::TRANSFER) break;
  }

  ASSERT_TRUE(m.get_state() == decltype(m)::MapState::TRANSFER);

  ASSERT_TRUE(m.finalize());
  ASSERT_TRUE(m.get_state() == decltype(m)::MapState::STABLE);
}

TEST_F(MapTest, ResizeTest) {
  rtl::unordered_map<size_t, size_t> m(&allocMT);
  size_t old_buckets = m.get_num_buckets();
  ASSERT_TRUE(m.reserve(24));
  ASSERT_NE(old_buckets, m.get_num_buckets());
  // 37 is the nearest prime
  ASSERT_EQ(m.get_num_buckets(), 37);
  ASSERT_TRUE(m.reserve(12));
  // Won't add_to_pool down though
  ASSERT_EQ(m.get_num_buckets(), 37);
}

TEST_F(MapTest, DelTest) {
  rtl::unordered_map<size_t, size_t> m(&allocMT);
  ASSERT_TRUE(m.put(1, 1));

  ASSERT_NE(m.get(1), nullptr);

  ASSERT_TRUE(m.del(1));
  ASSERT_FALSE(m.del(2));

  ASSERT_EQ(m.get(1), nullptr);
}
