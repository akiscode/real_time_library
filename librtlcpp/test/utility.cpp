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

#include "rtlcpp/utility.hpp"

#include <gtest/gtest.h>

class UtilityTest : public ::testing::Test {
 protected:
  UtilityTest() {
    // You can do set-up work for each test here.
  }

  ~UtilityTest() override {
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

TEST_F(UtilityTest, PowerOf2PrimeTest) {
  ASSERT_EQ(rtl::get_prime_power_of_2(0), 2);
  ASSERT_EQ(rtl::get_prime_power_of_2(1), 2);
  ASSERT_EQ(rtl::get_prime_power_of_2(2), 5);
  ASSERT_EQ(rtl::get_prime_power_of_2(3), 11);
  ASSERT_EQ(rtl::get_prime_power_of_2(4), 17);
  ASSERT_EQ(rtl::get_prime_power_of_2(5), 37);
  ASSERT_EQ(rtl::get_prime_power_of_2(6), 67);
  ASSERT_EQ(rtl::get_prime_power_of_2(7), 131);
  ASSERT_EQ(rtl::get_prime_power_of_2(8), 257);
  ASSERT_EQ(rtl::get_prime_power_of_2(9), 521);
  ASSERT_EQ(rtl::get_prime_power_of_2(10), 1031);
  ASSERT_EQ(rtl::get_prime_power_of_2(11), 2053);
  ASSERT_EQ(rtl::get_prime_power_of_2(12), 4099);
  ASSERT_EQ(rtl::get_prime_power_of_2(13), 8209);
  ASSERT_EQ(rtl::get_prime_power_of_2(14), 16411);
  ASSERT_EQ(rtl::get_prime_power_of_2(15), 32771);
  ASSERT_EQ(rtl::get_prime_power_of_2(16), 65537);
  ASSERT_EQ(rtl::get_prime_power_of_2(17), 131101);
  ASSERT_EQ(rtl::get_prime_power_of_2(18), 262147);
  ASSERT_EQ(rtl::get_prime_power_of_2(19), 524309);
  ASSERT_EQ(rtl::get_prime_power_of_2(20), 1048583);
  ASSERT_EQ(rtl::get_prime_power_of_2(21), 2097169);
  ASSERT_EQ(rtl::get_prime_power_of_2(22), 4194319);
  ASSERT_EQ(rtl::get_prime_power_of_2(23), 8388617);
  ASSERT_EQ(rtl::get_prime_power_of_2(24), 16777259);
  ASSERT_EQ(rtl::get_prime_power_of_2(25), 33554467);
  ASSERT_EQ(rtl::get_prime_power_of_2(26), 67108879);
  ASSERT_EQ(rtl::get_prime_power_of_2(27), 134217757);
  ASSERT_EQ(rtl::get_prime_power_of_2(28), 268435459);
  ASSERT_EQ(rtl::get_prime_power_of_2(29), 536870923);
  ASSERT_EQ(rtl::get_prime_power_of_2(30), 1073741827);
  ASSERT_EQ(rtl::get_prime_power_of_2(31), 2147483659);
  ASSERT_EQ(rtl::get_prime_power_of_2(32), 0);

}