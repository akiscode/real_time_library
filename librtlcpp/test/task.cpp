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
#include "rtlcpp/task.hpp"

#include <gtest/gtest.h>

#include <cerrno>

class TaskTest : public ::testing::Test {
 protected:
  std::atomic<bool> quit{false};

  bool gotten{false};
  int policy{-1};
  sched_param param{};

  TaskTest() {
    // You can do set-up work for each test here.
  }

  ~TaskTest() override {
    // You can do clean-up work that doesn't throw exceptions here.
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).

    quit = false;
    gotten = false;
    policy = -1;
    param.sched_priority = -1;
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }
};

struct SmokeTestFunctor {
  std::atomic<bool>* quit;
  bool* gotten;
  int* policy;
  struct sched_param* param;

  SmokeTestFunctor(std::atomic<bool>* quit_, bool* gotten_, int* policy_,
                   struct sched_param* param_)
      : quit(quit_), gotten(gotten_), policy(policy_), param(param_) {}

  bool operator()() {
    pthread_getschedparam(pthread_self(), policy, param);
    *gotten = true;

    // If no pointer is provided then don't quit
    return (quit) ? quit->load() : false;
  }
};

TEST_F(TaskTest, SmokeTest) {
  rtl::PeriodicTask<SmokeTestFunctor> task(
      SmokeTestFunctor{&quit, &gotten, &policy, &param});

  task.start();
  task.signal_shutdown();
  task.join();

  ASSERT_TRUE(gotten);
  ASSERT_NE(policy, -1);
  ASSERT_NE(param.sched_priority, -1);
}

TEST_F(TaskTest, SmokeTest2) {
  // Now test that we can quit from the functor itself
  // i.e. no signal_shutdown()
  rtl::PeriodicTask<SmokeTestFunctor> task(
      SmokeTestFunctor{&quit, &gotten, &policy, &param},
      rtl::PeriodicTaskOptions(std::chrono::microseconds(1)));

  task.start();

  quit = true;

  task.join();

  ASSERT_TRUE(gotten);
  ASSERT_NE(policy, -1);
  ASSERT_NE(param.sched_priority, -1);
}
TEST_F(TaskTest, SmokeTest3) {
  rtl::PeriodicTask<SmokeTestFunctor> task(
      SmokeTestFunctor{&quit, &gotten, &policy, &param});

  // Test that a task that just has its destructor called will signal shutdown
  // and stop
}

TEST_F(TaskTest, SmokeTest4) {
  // Setting SCHED_OTHER with a non-zero priority will result in an error
  sched_param sp{};
  sp.sched_priority = 23;

  rtl::PeriodicTask<SmokeTestFunctor> task(
      SmokeTestFunctor{&quit, &gotten, &policy, &param},
      rtl::PeriodicTaskOptions(SCHED_OTHER, sp));

  rtl::PeriodicTask<SmokeTestFunctor> task1(
      SmokeTestFunctor{&quit, &gotten, &policy, &param},
      rtl::PeriodicTaskOptions(SCHED_OTHER, sp, std::chrono::milliseconds(1)));

  task.start();
  task.signal_shutdown();
  task.join();

#ifdef __linux__
  ASSERT_TRUE(task.errored_out());
  ASSERT_EQ(task.error_num(), EINVAL);

  ASSERT_FALSE(gotten);
  ASSERT_EQ(policy, -1);
  ASSERT_NE(param.sched_priority, 23);
#endif
}
