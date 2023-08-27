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

#include "rtlcpp/object_pool.hpp"

#include <gtest/gtest.h>

#include <array>

struct TestStruct {
  int _i;
  TestStruct() : _i(0) {}
  TestStruct(int i) : _i(i) {}

  void reset() { _i = 0; }

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

  DestructorStruct() : DestructorStruct(nullptr) {}
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

class ObjectPoolTest : public ::testing::Test {
 protected:
  rtl::MMapMemoryResource mr1;
  rtl::RTAllocatorMT allocMT;

  ObjectPoolTest() {
    // You can do set-up work for each test here.
  }

  ~ObjectPoolTest() override {
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

struct SystemAllocator {
  void* allocate(size_t sz) { return malloc(sz); }

  void deallocate(void* p) { free(p); }
};

TEST_F(ObjectPoolTest, MemoryLeakTest) {
  SystemAllocator s;
  rtl::object_pool<TestStruct, SystemAllocator> pool(&s, 0);

  TestStruct* i = pool.get();
  TestStruct* i1 = pool.get();
  TestStruct* i2 = pool.get();
  TestStruct* i3 = pool.get();

  pool.put(i);
  pool.put(i1);
  pool.put(i2);
  pool.put(i3);

  rtl::object_pool<TestStruct, SystemAllocator> pool2(&s, 1);
  pool2.put(pool2.get());

  pool = std::move(pool2);

  rtl::object_pool<TestStruct, SystemAllocator> pool3(std::move(pool));
}

TEST_F(ObjectPoolTest, SmokeTest) {
  rtl::object_pool<TestStruct> pool(&allocMT, 0);

  ASSERT_EQ(pool.size(), 0);

  ASSERT_EQ(pool.add_to_pool(5), 5);
  ASSERT_EQ(pool.size(), 5);

  for (int i = 0; i < 10; i++) {
    auto t = pool.get_auto();
    t.get_data()->_i = 1;
  }

  ASSERT_EQ(pool.size(), 5);

  std::array<TestStruct*, 5> arr{};
  for (int i = 0; i < 5; i++) {
    arr[i] = pool.get();
  }

  ASSERT_EQ(pool.size(), 0);

  ASSERT_NE(pool.get(), nullptr);
}

struct FakeAllocator {
  int num_of_successful_mallocs{0};

  void* allocate(size_t sz) {
    if (num_of_successful_mallocs > 2) {
      return nullptr;
    }

    num_of_successful_mallocs++;
    return malloc(sz);
  }

  void deallocate(void* p) { free(p); }
};

TEST_F(ObjectPoolTest, FailedAllocator) {
  FakeAllocator falloc;

  rtl::object_pool<TestStruct, FakeAllocator> pool(&falloc, 0);

  ASSERT_EQ(pool.size(), 0);

  // Each add_to_pool takes two allocations (the push_back and allocate)
  // So if we stop at 2 mallocs in our fake allocator we should get
  // only one allocation with add_to_pool
  ASSERT_EQ(pool.add_to_pool(3), 2);
}

TEST_F(ObjectPoolTest, DestructorTest) {
  bool change = false;
  {
    rtl::object_pool<DestructorStruct> pool(&allocMT, 1);

    {
      auto obj = pool.get_auto();

      obj.get_data()->m_change = &change;
      ASSERT_FALSE(change);
    }

    ASSERT_TRUE(change);
  }
  ASSERT_TRUE(change);
}

TEST_F(ObjectPoolTest, PoolDestructorTest) {
  bool change = false;
  {
    rtl::object_pool<DestructorStruct> pool(&allocMT, 1);

    {
      auto obj = pool.get_auto();

      obj.get_data()->m_change = &change;
      ASSERT_FALSE(change);
    }

    ASSERT_TRUE(change);
  }
  ASSERT_TRUE(change);
}
