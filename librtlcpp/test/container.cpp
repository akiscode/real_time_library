
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

#include <array>

#include "gtest/gtest.h"
#include "rtlcpp/rtlcpp.hpp"
#include "rtlcpp/vector.hpp"

struct SystemAllocator {
  void* allocate(size_t sz) { return malloc(sz); }

  void deallocate(void* p) { free(p); }
};

struct TestStruct {
  int _i;
  TestStruct(int i) : _i(i) {}

  bool operator==(const TestStruct& rhs) const { return _i == rhs._i; }

  bool operator!=(const TestStruct& rhs) const { return !(rhs == *this); }

  TestStruct(TestStruct const&) = default;
  TestStruct& operator=(TestStruct const&) = default;

  TestStruct(TestStruct&& o) noexcept : _i(rtl::exchange(o._i, -1)) {}
  TestStruct& operator=(TestStruct&& o) noexcept {
    if (this != &o) {
      _i = rtl::exchange(o._i, -1);
    }
    return *this;
  }
};

struct DestructorStruct {
  bool* m_change;

  DestructorStruct(bool* change) : m_change(change) {}
  ~DestructorStruct() {
    if (m_change) *m_change = true;
  }

  DestructorStruct(DestructorStruct const&) = default;
  DestructorStruct& operator=(DestructorStruct const&) = default;

  DestructorStruct(DestructorStruct&& o) noexcept
      : m_change(rtl::exchange(o.m_change, nullptr)){};
  DestructorStruct& operator=(DestructorStruct&& o) noexcept {
    if (this != &o) {
      m_change = rtl::exchange(o.m_change, nullptr);
    }
    return *this;
  }
};

template <typename T>
bool insert_data_back_10(T& l) {
  bool rval{true};

  rval &= l.push_back({7});
  rval &= l.push_back({8});
  rval &= l.push_back({9});
  rval &= l.push_back({10});

  rval &= l.push_back({1});
  rval &= l.push_back({2});
  rval &= l.push_back({3});
  rval &= l.push_back({4});
  rval &= l.push_back({5});
  rval &= l.push_back({6});

  return rval;
}

class VectorTest : public ::testing::Test {
 protected:
  rtl::MMapMemoryResource mr1;
  rtl::MMapMemoryResource mr2;
  rtl::RTAllocatorST allocST;
  rtl::RTAllocatorMT allocMT;

  VectorTest() {
    // You can do set-up work for each test here.
  }

  ~VectorTest() override {
    // You can do clean-up work that doesn't throw exceptions here.
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).

    ASSERT_TRUE(mr1.init(10 * 1024));
    ASSERT_TRUE(mr2.init(10 * 1024));

    ASSERT_TRUE(allocST.init(mr1.get_buf(), mr1.get_capacity()));
    ASSERT_TRUE(allocMT.init(mr2.get_buf(), mr2.get_capacity()));
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).

    allocMT.uninit();
    allocST.uninit();

    mr2.uninit();
    mr1.uninit();
  }
};

TEST_F(VectorTest, MemoryLeakTest) {
  SystemAllocator s;
  rtl::vector<TestStruct, SystemAllocator> l(&s);
  for (int i = 0; i < 10; i++) {
    l.push_back({i});
  }

  rtl::vector<TestStruct, SystemAllocator> l2(&s);
  l2.push_back({123});
  l2.push_back({124});

  l = std::move(l2);

  rtl::vector<TestStruct, SystemAllocator> l3(std::move(l));
}

TEST_F(VectorTest, SmokeTest) {
  rtl::vector<TestStruct, rtl::RTAllocatorST> l(&allocST);
  ASSERT_TRUE(insert_data_back_10(l));
  ASSERT_EQ(l.size(), 10);
}

TEST_F(VectorTest, DestructorTest) {
  std::array<bool, 10> bools{};

  for (bool& b : bools) {
    ASSERT_FALSE(b);
  }

  {
    rtl::vector<DestructorStruct> l(&allocMT);
    for (size_t i = 0; i < bools.size(); i++) {
      ASSERT_TRUE(l.push_back({&bools[i]}));
    }

    for (bool& b : bools) {
      ASSERT_FALSE(b);
    }
  }

  for (bool& b : bools) {
    ASSERT_TRUE(b);
  }
}

TEST_F(VectorTest, MoveTest) {
  rtl::vector<TestStruct> l1(&allocMT);
  ASSERT_TRUE(insert_data_back_10(l1));

  rtl::vector<TestStruct> l2(&allocMT);
  l2.push_back({11});
  ASSERT_EQ(l1.size(), 10);
  ASSERT_EQ(l2.size(), 1);
  ASSERT_TRUE(l1 != l2);

  l2 = std::move(l1);
  ASSERT_EQ(l1.size(), 0);
  ASSERT_EQ(l2.size(), 10);
  ASSERT_TRUE(l1 != l2);
  rtl::vector<TestStruct> l3(&allocMT);
  ASSERT_TRUE(insert_data_back_10(l3));
  ASSERT_TRUE(l2 == l3);
}

TEST_F(VectorTest, EqualityTest) {
  rtl::vector<TestStruct> l1(&allocMT);
  rtl::vector<TestStruct> l2(&allocMT);

  ASSERT_TRUE(l1 == l2);
  l2.push_back({2});
  ASSERT_FALSE(l1 == l2);
  l1.push_back({2});
  ASSERT_TRUE(l1 == l2);
  ASSERT_TRUE(insert_data_back_10(l1));
  ASSERT_FALSE(l1 == l2);
  ASSERT_TRUE(l1 != l2);
  ASSERT_TRUE(insert_data_back_10(l2));
  ASSERT_TRUE(l1 == l2);
}

TEST_F(VectorTest, CopyTest) {
  rtl::vector<TestStruct> l1(&allocMT);
  ASSERT_TRUE(l1.push_back({2}));
  ASSERT_EQ(l1.size(), 1);

  rtl::vector<TestStruct> l2(&allocMT), l3(&allocMT), l4(&allocMT);
  ASSERT_TRUE(insert_data_back_10(l3));
  ASSERT_EQ(l3.size(), 10);

  ASSERT_TRUE(l1.copy(l3));
  ASSERT_TRUE(l2.copy(l3));

  ASSERT_TRUE(l1 == l3);
  ASSERT_EQ(l1.size(), 10);

  ASSERT_TRUE(l2 == l3);
  ASSERT_EQ(l2.size(), 10);

  ASSERT_TRUE(l4.empty());
  ASSERT_EQ(l4.size(), 0);
  ASSERT_EQ(l4.capacity(), 0);

  ASSERT_FALSE(l4 == l1);

  ASSERT_TRUE(l4.copy(l1));

  ASSERT_TRUE(l4 == l1);
  ASSERT_EQ(l4.size(), 10);
  ASSERT_EQ(l4.capacity(), 16);
}

TEST_F(VectorTest, PushPopBackTest) {
  rtl::vector<TestStruct> l1(&allocMT);
  ASSERT_TRUE(l1.push_back({1}));
  TestStruct tmp{2};
  ASSERT_TRUE(l1.push_back(tmp));
  ASSERT_TRUE(l1.push_back({3}));
  ASSERT_TRUE(l1.push_back({4}));

  for (int i = 4; i > 0; i--) {
    auto& tmp = l1.back();
    ASSERT_TRUE(tmp._i == i);
    l1.pop_back();
  }
}

TEST_F(VectorTest, AtTest) {
  rtl::vector<TestStruct> l1(&allocMT);
  ASSERT_TRUE(l1.push_back({1}));
  ASSERT_TRUE(l1.push_back({2}));
  ASSERT_TRUE(l1.push_back({3}));
  ASSERT_TRUE(l1.push_back({4}));

  TestStruct* ts{nullptr};

  ASSERT_EQ(l1.size(), 4);
  ASSERT_FALSE(l1.at(&ts, -1));
  ASSERT_EQ(ts, nullptr);
  ASSERT_FALSE(l1.at(&ts, 4));
  ASSERT_EQ(ts, nullptr);

  for (int i = 0; i < 4; i++) {
    ASSERT_TRUE(l1.at(&ts, i));
    ASSERT_EQ(ts->_i, i + 1);
  }

  for (int i = 0; i < 4; i++) {
    ASSERT_EQ(l1[i], i + 1);
  }
}

TEST_F(VectorTest, ReserveTest) {
  rtl::vector<TestStruct> l1(&allocMT);
  ASSERT_TRUE(l1.reserve(0));
  ASSERT_EQ(l1.size(), 0);
  ASSERT_EQ(l1.capacity(), 0);

  ASSERT_TRUE(l1.reserve(100));
  ASSERT_EQ(l1.size(), 0);
  ASSERT_EQ(l1.capacity(), 100);

  ASSERT_TRUE(l1.reserve(100));
  ASSERT_EQ(l1.size(), 0);
  ASSERT_EQ(l1.capacity(), 100);

  ASSERT_TRUE(l1.reserve(80));
  ASSERT_EQ(l1.size(), 0);
  ASSERT_EQ(l1.capacity(), 100);
  l1.push_back(TestStruct(1));
  l1.push_back(TestStruct(2));
  l1.push_back(TestStruct(3));

  ASSERT_EQ(l1.size(), 3);
  ASSERT_TRUE(l1.reserve(500));
  ASSERT_EQ(l1.size(), 3);
  ASSERT_EQ(l1.capacity(), 500);
}

TEST_F(VectorTest, RemoveFastTest) {
  rtl::vector<TestStruct> l1(&allocMT);
  l1.push_back({1});
  l1.push_back({2});
  l1.push_back({3});
  l1.push_back({4});

  ASSERT_EQ(l1.size(), 4);

  l1.remove_fast(8);

  ASSERT_EQ(l1.size(), 4);

  for (int i = 0; i < 4; i++) ASSERT_EQ(l1[i], i + 1);

  l1.remove_fast(1);
  ASSERT_EQ(l1.size(), 3);

  std::vector<int> expected = {1, 4, 3};

  for (int i = 0; i < 3; i++) {
    ASSERT_EQ(l1[i], expected[i]);
  }

  l1.remove_fast(2);

  ASSERT_EQ(l1.size(), 2);

  expected = {1, 4};

  for (int i = 0; i < 2; i++) {
    ASSERT_EQ(l1[i], expected[i]);
  }
}

TEST_F(VectorTest, RemoveStableTest) {
  rtl::vector<TestStruct> l1(&allocMT);
  l1.push_back({1});
  l1.push_back({2});
  l1.push_back({3});
  l1.push_back({4});

  ASSERT_EQ(l1.size(), 4);

  l1.remove_stable(8);

  ASSERT_EQ(l1.size(), 4);

  for (int i = 0; i < 4; i++) ASSERT_EQ(l1[i], i + 1);

  l1.remove_stable(1);
  ASSERT_EQ(l1.size(), 3);

  std::vector<int> expected = {1, 3, 4};

  for (int i = 0; i < 3; i++) {
    ASSERT_EQ(l1[i], expected[i]);
  }

  l1.remove_stable(2);

  ASSERT_EQ(l1.size(), 2);

  expected = {1, 3};

  for (int i = 0; i < 2; i++) {
    ASSERT_EQ(l1[i], expected[i]);
  }
}

TEST_F(VectorTest, PopBackTest) {
  std::array<bool, 10> bools{};
  rtl::vector<DestructorStruct> l(&allocMT);

  for (bool& b : bools) {
    ASSERT_FALSE(b);
  }

  {
    for (size_t i = 0; i < bools.size(); i++) {
      ASSERT_TRUE(l.push_back({&bools[i]}));
    }

    for (bool& b : bools) {
      ASSERT_FALSE(b);
    }
  }

  l.pop_back();

  for (int i = 0; i < 9; i++) {
    ASSERT_FALSE(bools[i]);
  }

  ASSERT_TRUE(bools[9]);
}
