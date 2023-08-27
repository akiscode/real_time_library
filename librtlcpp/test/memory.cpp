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

#include <random>
#include <thread>
#include <type_traits>

#include "rtlcpp/rtlcpp.hpp"

struct MemSystemAllocator {
  void* allocate(size_t sz) { return malloc(sz); }
  void deallocate(void* p) { free(p); }
};
class UniquePointerTests : public ::testing::Test {
 protected:
  rtl::MMapMemoryResource mr1;
  rtl::RTAllocatorMT allocMT;

  UniquePointerTests() {
    // You can do set-up work for each test here.
  }

  ~UniquePointerTests() override {
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

struct Base {
  virtual ~Base() = default;
  std::string foo(int i) {
    (void)i;
    return std::string{"Base"};
  }
  virtual std::string vfoo() { return std::string{"Base"}; }
};

struct Derived : public Base {
  bool hi{false};
  std::string foo() { return std::string{"Derived"}; }
  virtual std::string vfoo() override { return std::string{"Derived"}; }
};

struct DerivedDestructorCheck : public Base {
  DerivedDestructorCheck(bool* change_) : change(change_) {}
  DerivedDestructorCheck() : DerivedDestructorCheck(nullptr) {}
  ~DerivedDestructorCheck() {
    if (change) {
      *change = true;
    }
  }
  bool* change;
  virtual std::string vfoo() override {
    return std::string{"DerivedDestructorCheck"};
  }
};

struct MultiConstruct {
  int i;
  int j;

  MultiConstruct() : i(0), j(0) {}
  MultiConstruct(int i_, int j_) : i(i_), j(j_) {}
  virtual ~MultiConstruct() = default;

  virtual int foo() { return i + j; }
};

struct DerivedMultiConstruct : public MultiConstruct {
  DerivedMultiConstruct() : MultiConstruct() {}
  DerivedMultiConstruct(int i_, int j_) : MultiConstruct(i_, j_) {}

  virtual int foo() override { return i + j + 1; }
};

TEST_F(UniquePointerTests, MemoryLeakTest) {
  MemSystemAllocator s;
  auto i = rtl::make_unique<int, MemSystemAllocator>(&s, 1);
  *i = 2;

  ASSERT_EQ(*i, 2);

  bool rval = std::is_base_of<Base, Derived>::value;
  ASSERT_TRUE(rval);

  rtl::unique_ptr<Base, MemSystemAllocator> i2{nullptr};

  i2 = rtl::make_unique<Derived, MemSystemAllocator>(&s);

  rtl::unique_ptr<Base, MemSystemAllocator> i3(std::move(i2));

  auto i4 = rtl::make_unique<Derived, MemSystemAllocator>(&s);

  ASSERT_TRUE(i4.get() != nullptr);
  i3 = std::move(i4);

  ASSERT_TRUE(i4.get() == nullptr);
  ASSERT_EQ(i3->foo(0), std::string("Base"));
  ASSERT_EQ(i3->vfoo(), std::string("Derived"));

  bool change{false};
  {
    rtl::unique_ptr<Base, MemSystemAllocator> c1 =
        rtl::make_unique<DerivedDestructorCheck, MemSystemAllocator>(&s,
                                                                     &change);

    ASSERT_EQ(c1->foo(0), std::string("Base"));
    ASSERT_EQ(c1->vfoo(), std::string("DerivedDestructorCheck"));
  }

  ASSERT_TRUE(change);
}

TEST_F(UniquePointerTests, ArrayMemoryLeakTest) {
  MemSystemAllocator s;
  auto a = rtl::make_unique<int[], MemSystemAllocator>(&s, 3, 1);
  ASSERT_EQ(a.array_size(), 3);
  for (size_t i = 0; i < a.array_size(); i++) {
    ASSERT_TRUE(a[i] == 1);
    a[i] = (int)i;
  }

  for (size_t i = 0; i < a.array_size(); i++) {
    ASSERT_TRUE(a[i] == (int)i);
  }

  ASSERT_TRUE(a);
  a = nullptr;
  ASSERT_FALSE(a);

  bool change[3] = {false, false, false};
  {
    rtl::unique_ptr<DerivedDestructorCheck[], MemSystemAllocator> c1;

    rtl::unique_ptr<DerivedDestructorCheck[], MemSystemAllocator> c2(
        rtl::make_unique<DerivedDestructorCheck[], MemSystemAllocator>(&s, 3));

    ASSERT_FALSE(c1);
    c1 = std::move(c2);
    ASSERT_TRUE(c1);

    for (size_t i = 0; i < c1.array_size(); i++) {
      ((DerivedDestructorCheck*)&c1[i])->change = &change[i];
    }

    for (size_t i = 0; i < c1.array_size(); i++) {
      ASSERT_EQ(c1[i].foo(0), std::string("Base"));
      ASSERT_EQ(c1[i].vfoo(), std::string("DerivedDestructorCheck"));
    }

    for (size_t i = 0; i < 3; i++) {
      ASSERT_FALSE(change[i]);
    }
  }

  for (size_t i = 0; i < 3; i++) {
    ASSERT_TRUE(change[i]);
  }
}

TEST_F(UniquePointerTests, MultiConstructorTests) {
  MemSystemAllocator s;
  {
    auto r1 = rtl::make_unique<MultiConstruct>(&s, 1, 2);
    auto r2 = rtl::make_unique<MultiConstruct>(&s);

    ASSERT_TRUE(r1 != r2);

    ASSERT_EQ(r1->foo(), 3);
    ASSERT_EQ(r2->foo(), 0);
    r1 = std::move(r2);
    ASSERT_EQ(r1->foo(), 0);
    ASSERT_FALSE(r2);

    auto* mc_ptr =
        (DerivedMultiConstruct*)s.allocate(sizeof(DerivedMultiConstruct));
    new (mc_ptr) DerivedMultiConstruct(3, 4);

    rtl::unique_ptr<MultiConstruct, MemSystemAllocator> r3 =
        rtl::make_unique<DerivedMultiConstruct>(&s, mc_ptr);

    ASSERT_EQ(r1->foo(), 0);
    ASSERT_EQ(r3->foo(), 8);
    r1 = std::move(r3);
    ASSERT_EQ(r1->foo(), 8);
    ASSERT_FALSE(r3);
  }

  {
    auto r1 = rtl::make_unique<MultiConstruct[]>(&s, 2, 1, 2);
    auto r2 = rtl::make_unique<MultiConstruct[]>(&s, 2);

    ASSERT_TRUE(r1 != r2);

    ASSERT_EQ(r1[0].foo(), 3);
    ASSERT_EQ(r1[1].foo(), 3);
    ASSERT_EQ(r2[0].foo(), 0);
    ASSERT_EQ(r2[1].foo(), 0);
    r1 = std::move(r2);
    ASSERT_EQ(r1[0].foo(), 0);
    ASSERT_EQ(r1[1].foo(), 0);
    ASSERT_FALSE(r2);

    auto* mc_ptr =
        (DerivedMultiConstruct*)s.allocate(sizeof(DerivedMultiConstruct) * 2);
    new (&mc_ptr[0]) DerivedMultiConstruct(3, 4);
    new (&mc_ptr[1]) DerivedMultiConstruct(4, 5);

    rtl::unique_ptr<DerivedMultiConstruct[], MemSystemAllocator> r3 =
        rtl::make_unique<DerivedMultiConstruct[]>(&s, 2, mc_ptr);

    ASSERT_EQ(r3.array_size(), 2);
    ASSERT_EQ(r1[0].foo(), 0);
    ASSERT_EQ(r1[1].foo(), 0);
    ASSERT_EQ(r3[0].foo(), 8);
    ASSERT_EQ(r3[1].foo(), 10);
  }
}

class ControlBlockTests : public ::testing::Test {
 protected:
  rtl::MMapMemoryResource mr1;
  rtl::RTAllocatorMT allocMT;

  ControlBlockTests() {
    // You can do set-up work for each test here.
  }

  ~ControlBlockTests() override {
    // You can do clean-up work that doesn't throw exceptions here.
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).

    ASSERT_TRUE(mr1.init(50 * 1024 * 1024));

    ASSERT_TRUE(allocMT.init(mr1.get_buf(), mr1.get_capacity()));
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).

    allocMT.uninit();

    mr1.uninit();
  }
};

template <typename U>
void shared_ptr_test(U& ctrl, std::atomic<bool>& go, bool rval_test) {
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_int_distribution<int> dist(10, 200);

  while (!go) {
  }

  for (int i = 0; i < 100; i++) {
    ctrl.inc_strong();
    std::this_thread::sleep_for(std::chrono::microseconds{dist(mt)});
    ASSERT_EQ(ctrl.dec_strong(), rval_test);
  }
}

TEST_F(ControlBlockTests, ControlBlockTest) {
  int num_threads = 10;
  int loops = 50;
  std::atomic<bool> go{false};

  auto* ptr =
      (DerivedDestructorCheck*)allocMT.allocate(sizeof(DerivedDestructorCheck));
  bool change{false};
  new (ptr) DerivedDestructorCheck(&change);

  rtl::control_blk<DerivedDestructorCheck, rtl::RTAllocatorMT> ctrl(&allocMT,
                                                                    ptr);

  std::vector<std::thread> threads;

  ctrl.inc_weak();

  for (int i = 0; i < loops; i++) {
    threads.clear();
    go = false;

    for (int j = 0; j < num_threads; j++) {
      std::thread tmp(shared_ptr_test<decltype(ctrl)>, std::ref(ctrl),
                      std::ref(go), false);
      threads.push_back(std::move(tmp));
    }

    // Start threads
    go = true;

    for (std::thread& t : threads) {
      if (t.joinable()) t.join();
    }
  }

  ASSERT_TRUE(ctrl.dec_weak());
}

TEST_F(ControlBlockTests, ManualControlBlockTest) {
  auto* ptr =
      (DerivedDestructorCheck*)allocMT.allocate(sizeof(DerivedDestructorCheck));
  bool change{false};
  new (ptr) DerivedDestructorCheck(&change);

  rtl::control_blk<DerivedDestructorCheck, rtl::RTAllocatorMT> ctrl(&allocMT,
                                                                    ptr);

  ASSERT_EQ(ctrl.strong_count(), 0);
  ASSERT_EQ(ctrl.weak_count(), 0);

  ctrl.inc_strong();

  ASSERT_EQ(ctrl.strong_count(), 1);
  ASSERT_EQ(ctrl.weak_count(), 1);

  ctrl.inc_weak();

  ASSERT_EQ(ctrl.strong_count(), 1);
  ASSERT_EQ(ctrl.weak_count(), 2);

  ASSERT_FALSE(change);
  ASSERT_FALSE(ctrl.dec_strong());
  ASSERT_TRUE(change);
  ASSERT_EQ(ctrl.strong_count(), 0);
  ASSERT_EQ(ctrl.weak_count(), 1);
  ASSERT_TRUE(ctrl.dec_weak());
  ASSERT_TRUE(ctrl.get_weak() == nullptr);
}

TEST_F(ControlBlockTests, ManualControlBlockTest2) {
  auto* ptr =
      (DerivedDestructorCheck*)allocMT.allocate(sizeof(DerivedDestructorCheck));
  bool change{false};
  new (ptr) DerivedDestructorCheck(&change);

  rtl::control_blk<DerivedDestructorCheck, rtl::RTAllocatorMT> ctrl(&allocMT,
                                                                    ptr);

  ASSERT_EQ(ctrl.strong_count(), 0);
  ASSERT_EQ(ctrl.weak_count(), 0);

  ctrl.inc_strong();

  ASSERT_EQ(ctrl.strong_count(), 1);
  ASSERT_EQ(ctrl.weak_count(), 1);

  ctrl.inc_weak();

  ASSERT_EQ(ctrl.strong_count(), 1);
  ASSERT_EQ(ctrl.weak_count(), 2);

  ASSERT_FALSE(change);

  ASSERT_FALSE(ctrl.dec_weak());

  ASSERT_EQ(ctrl.strong_count(), 1);
  ASSERT_EQ(ctrl.weak_count(), 1);

  ASSERT_FALSE(change);
  ASSERT_TRUE(ctrl.dec_strong());
  ASSERT_TRUE(change);

  ASSERT_EQ(ctrl.strong_count(), 0);
  ASSERT_EQ(ctrl.weak_count(), 0);
}

class SharedPointerTests : public ::testing::Test {
 protected:
  rtl::MMapMemoryResource mr1;
  rtl::RTAllocatorMT allocMT;

  SharedPointerTests() {
    // You can do set-up work for each test here.
  }

  ~SharedPointerTests() override {
    // You can do clean-up work that doesn't throw exceptions here.
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).

    ASSERT_TRUE(mr1.init(50 * 1024 * 1024));

    ASSERT_TRUE(allocMT.init(mr1.get_buf(), mr1.get_capacity()));
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).

    allocMT.uninit();

    mr1.uninit();
  }
};

TEST_F(SharedPointerTests, SmokeTest) {
  auto s1 = rtl::make_shared<Base>(&allocMT);
  rtl::shared_ptr<Derived> s2{rtl::make_shared<Derived>(&allocMT)};
  rtl::shared_ptr<Derived> s3{rtl::make_shared<Derived>(&allocMT)};

  s1 = s2;
  ASSERT_TRUE(s1);
  ASSERT_TRUE(s2);
  ASSERT_TRUE(s3);
  s3 = nullptr;
  ASSERT_FALSE(s3);

  ASSERT_EQ(s1->foo(0), std::string("Base"));
  ASSERT_EQ(s2->foo(), std::string("Derived"));

  ASSERT_EQ(s1->vfoo(), std::string("Derived"));
  ASSERT_EQ(s2->vfoo(), std::string("Derived"));

  ASSERT_TRUE(s1.get() == s2.get());
}

TEST_F(SharedPointerTests, MemoryLeakTest) {
  MemSystemAllocator s;
  auto s1 = rtl::make_shared<Base>(&s);
  auto s2{rtl::make_shared<Derived>(&s)};
  auto s3{rtl::make_shared<Derived>(&s)};

  Base* raw_ptr = (Base*)s.allocate(sizeof(Base));
  new (raw_ptr) Base();
  auto s4{rtl::make_shared<Base>(&s, raw_ptr)};

  ASSERT_TRUE(s2);
  s1 = std::move(s2);
  ASSERT_FALSE(s2);
  ASSERT_EQ(s2.use_count(), 0);
  ASSERT_EQ(s3.use_count(), 1);
  s2 = s3;
  ASSERT_EQ(s2.use_count(), 2);
  ASSERT_EQ(s3.use_count(), 2);

  ASSERT_EQ(s1->vfoo(), std::string("Derived"));
  ASSERT_EQ(s2->vfoo(), std::string("Derived"));
  ASSERT_EQ(s3->vfoo(), std::string("Derived"));
  ASSERT_EQ(s4->vfoo(), std::string("Base"));
}

TEST_F(SharedPointerTests, ArrayTest) {
  rtl::shared_ptr<int[]> r1 = rtl::make_shared<int[]>(&allocMT, 3);
  rtl::shared_ptr<int[]> r2 = rtl::make_shared<int[]>(&allocMT, 3, 1);

  int* arr = (int*)allocMT.allocate(sizeof(int) * 4);
  for (size_t i = 0; i < 4; i++) {
    new (&arr[i]) int();
  }

  auto r3{rtl::make_shared<int[]>(&allocMT, 4, arr)};
  for (size_t i = 0; i < 4; i++) {
    r3[i] = (int)i + 1;
  }

  for (size_t i = 0; i < r1.array_size(); i++) {
    ASSERT_EQ(r1[i], 0);
  }

  for (size_t i = 0; i < r2.array_size(); i++) {
    ASSERT_EQ(r2[i], 1);
  }

  for (size_t i = 0; i < r3.array_size(); i++) {
    ASSERT_EQ(r3[i], (int)i + 1);
  }
}

TEST_F(SharedPointerTests, WeakPointerConstructorTests) {
  MemSystemAllocator s;
  {
    auto r1 = rtl::make_shared<MultiConstruct>(&s, 1, 2);
    auto r2 = rtl::make_shared<DerivedMultiConstruct>(&s, 3, 4);
    ASSERT_TRUE(r1);
    ASSERT_TRUE(r2);

    using wptr_mc = rtl::weak_ptr<MultiConstruct, MemSystemAllocator>;
    using wptr_dmc = rtl::weak_ptr<DerivedMultiConstruct, MemSystemAllocator>;

    // Test all weak pointer constructors
    {
      wptr_mc w11{r1};
      wptr_mc w12;
      w12 = r1;

      wptr_dmc w21{r2};
      wptr_dmc w22;
      w22 = r2;

      ASSERT_TRUE(w11.lock());
      ASSERT_TRUE(w12.lock());
      ASSERT_TRUE(w21.lock());
      ASSERT_TRUE(w22.lock());

      ASSERT_TRUE(w11.lock() == r1);
      ASSERT_TRUE(w12.lock() == r1);
      ASSERT_TRUE(w21.lock() == r2);
      ASSERT_TRUE(w22.lock() == r2);
    }

    {
      wptr_mc w11{r2};
      wptr_mc w12;
      w12 = r2;

      ASSERT_EQ(w11.lock()->foo(), 8);
      ASSERT_EQ(w12.lock()->foo(), 8);
    }
    {
      wptr_mc w11{nullptr};
      wptr_mc w12;
      w12 = nullptr;

      ASSERT_FALSE(w11);
      ASSERT_FALSE(w12);
    }
    {
      wptr_mc w{r1};

      wptr_mc w11{std::move(w)};
      wptr_mc w12;

      ASSERT_TRUE(w11);
      ASSERT_FALSE(w12);
      ASSERT_TRUE(w11.lock() == r1);
      w12 = std::move(w11);
      ASSERT_FALSE(w11);
      ASSERT_TRUE(w12);
      ASSERT_TRUE(w12.lock() == r1);
    }

    {
      wptr_dmc w21{r2};

      ASSERT_TRUE(w21);
      wptr_mc w11{std::move(w21)};
      ASSERT_FALSE(w21);
      ASSERT_TRUE(w11);
      ASSERT_EQ(w11.lock()->foo(), 8);
    }

    {
      wptr_dmc w21{r2};
      wptr_mc w11;

      ASSERT_TRUE(w21);
      w11 = std::move(w21);
      ASSERT_FALSE(w21);
      ASSERT_TRUE(w11);
      ASSERT_EQ(w11.lock()->foo(), 8);
    }

    {
      wptr_dmc w21{r2};

      ASSERT_TRUE(w21);
      wptr_mc w11{w21};
      ASSERT_TRUE(w21);
      ASSERT_TRUE(w11);
      ASSERT_EQ(w11.lock()->foo(), 8);
      ASSERT_EQ(w21.lock()->foo(), 8);
    }

    {
      wptr_dmc w21{r2};
      wptr_mc w11;

      ASSERT_TRUE(w21);
      w11 = w21;
      ASSERT_TRUE(w21);
      ASSERT_TRUE(w11);
      ASSERT_EQ(w11.lock()->foo(), 8);
      ASSERT_EQ(w21.lock()->foo(), 8);
    }

    {
      wptr_dmc w{r2};
      wptr_dmc w1{w};
      wptr_dmc w2;
      w2 = w;

      ASSERT_TRUE(w && w1 && w2);
      ASSERT_TRUE(w.lock() == w1.lock());
      ASSERT_TRUE(w.lock() == w2.lock());
      ASSERT_TRUE(w.lock() == r2);

      ASSERT_EQ(w.lock()->foo(), 8);
      ASSERT_EQ(w1.lock()->foo(), 8);
      ASSERT_EQ(w2.lock()->foo(), 8);
    }
  }
}

TEST_F(SharedPointerTests, MultiConstructorTests) {
  MemSystemAllocator s;
  {
    auto r1a = rtl::make_shared<MultiConstruct>(&s, 1, 2);
    auto r1 = r1a;
    auto r2a{rtl::make_shared<MultiConstruct>(&s)};
    auto r2{std::move(r2a)};

    ASSERT_TRUE(r1 != r2);
    ASSERT_EQ(r1.use_count(), 2);
    ASSERT_EQ(r2.use_count(), 1);

    ASSERT_EQ(r1->foo(), 3);
    ASSERT_EQ(r2->foo(), 0);
    r1 = r2;

    ASSERT_EQ(r1.use_count(), 2);
    ASSERT_EQ(r2.use_count(), 2);

    ASSERT_EQ(r1->foo(), 0);
    ASSERT_EQ(r2->foo(), 0);

    ASSERT_TRUE(r1 == r2);

    auto* mc_ptr =
        (DerivedMultiConstruct*)s.allocate(sizeof(DerivedMultiConstruct));
    new (mc_ptr) DerivedMultiConstruct(3, 4);

    rtl::shared_ptr<MultiConstruct, MemSystemAllocator> r3a =
        rtl::make_shared<DerivedMultiConstruct>(&s, mc_ptr);

    rtl::shared_ptr<MultiConstruct, MemSystemAllocator> r3 = r3a;

    ASSERT_EQ(r1->foo(), 0);
    ASSERT_EQ(r3->foo(), 8);
    r1 = r3;
    ASSERT_EQ(r1->foo(), 8);
    ASSERT_TRUE(r3);
    r1 = std::move(r3);
    ASSERT_FALSE(r3);

    auto r4a = rtl::make_shared<DerivedMultiConstruct>(&s, 9, 4);

    rtl::shared_ptr<MultiConstruct, MemSystemAllocator> r4{r4a};

    ASSERT_EQ(r4->foo(), 14);
  }

  {
    auto r1 = rtl::make_shared<MultiConstruct[]>(&s, 2, 1, 2);
    auto r2 = rtl::make_shared<MultiConstruct[]>(&s, 2);

    ASSERT_TRUE(r1 != r2);

    ASSERT_EQ(r1.use_count(), 1);
    ASSERT_EQ(r2.use_count(), 1);

    ASSERT_EQ(r1[0].foo(), 3);
    ASSERT_EQ(r1[1].foo(), 3);
    ASSERT_EQ(r2[0].foo(), 0);
    ASSERT_EQ(r2[1].foo(), 0);
    r1 = r2;

    ASSERT_EQ(r1.use_count(), 2);
    ASSERT_EQ(r2.use_count(), 2);

    ASSERT_TRUE(r1 == r2);

    ASSERT_EQ(r1[0].foo(), 0);
    ASSERT_EQ(r1[1].foo(), 0);
    ASSERT_EQ(r2[0].foo(), 0);
    ASSERT_EQ(r2[1].foo(), 0);

    auto* mc_ptr =
        (DerivedMultiConstruct*)s.allocate(sizeof(DerivedMultiConstruct) * 2);
    new (&mc_ptr[0]) DerivedMultiConstruct(3, 4);
    new (&mc_ptr[1]) DerivedMultiConstruct(4, 5);

    rtl::shared_ptr<DerivedMultiConstruct[], MemSystemAllocator> r3 =
        rtl::make_shared<DerivedMultiConstruct[]>(&s, 2, mc_ptr);

    ASSERT_EQ(r3.array_size(), 2);
    ASSERT_EQ(r1[0].foo(), 0);
    ASSERT_EQ(r1[1].foo(), 0);
    ASSERT_EQ(r3[0].foo(), 8);
    ASSERT_EQ(r3[1].foo(), 10);
  }
}

TEST_F(SharedPointerTests, ExtensionTests) {
  MemSystemAllocator s;
  bool change{false};
  {
    rtl::weak_ptr<DerivedDestructorCheck, MemSystemAllocator> w;
    rtl::shared_ptr<DerivedDestructorCheck, MemSystemAllocator> r2;
    {
      auto r1 = rtl::make_shared<DerivedDestructorCheck>(&s, &change);
      w = r1;
      ASSERT_FALSE(change);
      r2 = w.lock();
      ASSERT_TRUE(r2);
    }
    ASSERT_FALSE(change);
  }
  ASSERT_TRUE(change);
}
