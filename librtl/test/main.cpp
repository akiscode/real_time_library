
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

// workaround missing "is_trivially_copyable" in g++ < 5.0
#if __GNUG__ && __GNUC__ < 5
#define IS_TRIVIALLY_COPYABLE(T) __has_trivial_copy(T)
#else
#define IS_TRIVIALLY_COPYABLE(T) std::is_trivially_copyable<T>::value
#endif


#include <gtest/gtest.h>
#include <stdio.h>
#include <iostream>
#include "rtl/rtl.h"



class RTLTest : public ::testing::Test {
 protected:
  // You can remove any or all of the following functions if their bodies would
  // be empty.

  RTLTest() {
    // You can do set-up work for each test here.
  }

  ~RTLTest() override {
    // You can do clean-up work that doesn't throw exceptions here.
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }

};

TEST_F(RTLTest, AlignTest) {
  size_t word_size = 8;

  ASSERT_EQ(rtl_align(word_size, 2), 8);
  ASSERT_EQ(rtl_align(word_size, 3), 8);
  ASSERT_EQ(rtl_align(word_size, 4), 8);
  ASSERT_EQ(rtl_align(word_size, 8), 8);
  ASSERT_EQ(rtl_align(word_size, 12), 16);
  ASSERT_EQ(rtl_align(word_size, 13), 16);
  ASSERT_EQ(rtl_align(word_size, 32), 32);
  ASSERT_EQ(rtl_align(word_size, 16), 16);

  word_size = 4;

  ASSERT_EQ(rtl_align(word_size, 2), 4);
  ASSERT_EQ(rtl_align(word_size, 3), 4);
  ASSERT_EQ(rtl_align(word_size, 8), 8);
  ASSERT_EQ(rtl_align(word_size, 12), 12);
  ASSERT_EQ(rtl_align(word_size, 13), 16);
  ASSERT_EQ(rtl_align(word_size, 16), 16);
  ASSERT_EQ(rtl_align(word_size, 32), 32);
  ASSERT_EQ(rtl_align(word_size, 60), 60);

  word_size = 2;

  ASSERT_EQ(rtl_align(word_size, 4), 4);

}
