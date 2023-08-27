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

#ifndef RTLCPP_TASK_HPP
#define RTLCPP_TASK_HPP

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <rtlcpp/utility.hpp>
#include <thread>
#include <utility>

namespace rtl {

class PeriodicTaskOptions final {
 private:
  int m_policy;
  struct sched_param m_param;
  bool m_set_sched_params;

  // Timeout in microseconds, to have no timeout, set to zero
  std::chrono::microseconds m_timeout_micro;

 public:
  int get_policy() const { return m_policy; }
  const sched_param& get_param() const { return m_param; }
  bool get_set_sched_params() const { return m_set_sched_params; }
  const std::chrono::microseconds& get_timeout_micro() const {
    return m_timeout_micro;
  }
  /*
   * 4 Options:
   *  - None (no sched and no timeout)
   *  - SCHED (No timeout)
   *  - Timeout (No sched)
   *  - SCHED and Timeout
   *
   */
  PeriodicTaskOptions()
      : m_policy(0), m_param(), m_set_sched_params(false), m_timeout_micro(0) {}

  PeriodicTaskOptions(int policy, struct sched_param param)
      : m_policy(policy),
        m_param(param),
        m_set_sched_params(true),
        m_timeout_micro(0) {}

  PeriodicTaskOptions(std::chrono::microseconds timeout_micro)
      : m_policy(0),
        m_param(),
        m_set_sched_params(false),
        m_timeout_micro(timeout_micro) {}

  PeriodicTaskOptions(int policy, struct sched_param param,
                      std::chrono::microseconds timeout_micro)
      : m_policy(policy),
        m_param(param),
        m_set_sched_params(true),
        m_timeout_micro(timeout_micro) {}

  ~PeriodicTaskOptions() = default;

  PeriodicTaskOptions(PeriodicTaskOptions const& o)
      : m_policy(o.m_policy),
        m_param(o.m_param),
        m_set_sched_params(o.m_set_sched_params),
        m_timeout_micro(o.m_timeout_micro) {}

  PeriodicTaskOptions& operator=(PeriodicTaskOptions const& o) {
    if (this != &o) {
      m_policy = o.m_policy;
      m_param = o.m_param;
      m_set_sched_params = o.m_set_sched_params;
      m_timeout_micro = o.m_timeout_micro;
    }
    return *this;
  }

  PeriodicTaskOptions(PeriodicTaskOptions&& o) noexcept
      : m_policy(rtl::exchange(o.m_policy, 0)),
        m_param(o.m_param),
        m_set_sched_params(rtl::exchange(o.m_set_sched_params, false)),
        m_timeout_micro(
            rtl::exchange(o.m_timeout_micro, std::chrono::microseconds(0))) {}

  PeriodicTaskOptions& operator=(PeriodicTaskOptions&& o) noexcept {
    if (this != &o) {
      m_policy = rtl::exchange(o.m_policy, 0);
      m_param = o.m_param;
      m_set_sched_params = rtl::exchange(o.m_set_sched_params, false);
      m_timeout_micro =
          rtl::exchange(o.m_timeout_micro, std::chrono::microseconds(0));
    }

    return *this;
  }
};

class NotificationObject final {
 private:
  std::condition_variable* m_cv_ref;

 public:
  explicit NotificationObject(std::condition_variable& cv_ref)
      : m_cv_ref(&cv_ref) {}

  NotificationObject() : m_cv_ref(nullptr) {}

  ~NotificationObject() = default;

  NotificationObject(NotificationObject const& o) : m_cv_ref(o.m_cv_ref) {}

  NotificationObject& operator=(NotificationObject const& o) {
    if (this != &o) {
      m_cv_ref = o.m_cv_ref;
    }
    return *this;
  }

  NotificationObject(NotificationObject&& o) noexcept
      : m_cv_ref(rtl::exchange(o.m_cv_ref, nullptr)) {}

  NotificationObject& operator=(NotificationObject&& o) noexcept {
    if (this != &o) {
      m_cv_ref = rtl::exchange(o.m_cv_ref, nullptr);
    }
    return *this;
  }

  inline void notify_one() {
    if (m_cv_ref == nullptr) {
      return;
    }
    m_cv_ref->notify_one();
  }

  inline void notify_all() {
    if (m_cv_ref == nullptr) {
      return;
    }
    m_cv_ref->notify_all();
  }
};

template <typename Callable>
class PeriodicTask {
 private:
  std::condition_variable m_cv;
  std::mutex m_mtx;
  Callable m_call;
  std::thread m_thread;

  bool m_started;

  // This needs to be a regular bool since the mutex needs to protect it anyway
  // For synchronization with the condvar
  bool m_shutdown;

  std::atomic<int> m_errored_out_num;

  PeriodicTaskOptions m_options;

 public:
  PeriodicTask(Callable&& call,
               PeriodicTaskOptions options = PeriodicTaskOptions())
      : m_cv(),
        m_mtx(),
        m_call(std::move(call)),
        m_thread(),
        m_started(false),
        m_shutdown(false),
        m_errored_out_num(0),
        m_options(std::move(options)) {}

  ~PeriodicTask() {
    signal_shutdown();
    join();
  }

  PeriodicTask(PeriodicTask const&) = delete;
  PeriodicTask& operator=(PeriodicTask const&) = delete;

  /*
   * We can't move this class because if the thread starts and has a reference
   * to the mutex and condvar, then it can't be "switched" out from
   * underneath it
   */
  PeriodicTask(PeriodicTask&&) noexcept = delete;
  PeriodicTask& operator=(PeriodicTask&&) noexcept = delete;

  bool errored_out() const { return m_errored_out_num.load() != 0; }
  int error_num() const { return m_errored_out_num.load(); }

  void start(PeriodicTaskOptions options) {
    if (m_started) {
      return;
    }
    // Once started, can't start again
    m_started = true;

    m_options = std::move(options);
    priv_start();
  }

  void start() {
    if (m_started) {
      return;
    }
    // Once started, can't start again
    m_started = true;
    priv_start();
  }

  void signal_shutdown() {
    // Don't hold the mutex while notifying
    {
      std::lock_guard<std::mutex> lck(m_mtx);
      m_shutdown = true;
    }

    m_cv.notify_one();
  }

  void join() {
    if (m_thread.joinable()) {
      assert(m_started);
      m_thread.join();
    }
  }

  NotificationObject get_notification_obj() {
    return NotificationObject{m_cv};
  };

  void notify_one() { m_cv.notify_one(); }

  void notify_all() { m_cv.notify_all(); }

 private:
  void priv_start() {
    // No timeout option
    if (m_options.get_timeout_micro() == std::chrono::microseconds(0)) {
      m_thread = std::thread([&]() {
        if (m_options.get_set_sched_params()) {
          int rval = pthread_setschedparam(
              pthread_self(), m_options.get_policy(), &m_options.get_param());

          if (rval != 0) {
            m_errored_out_num = rval;
            return;
          }
        }

        while (true) {
          if (m_call()) {
            break;
          }

          {
            std::unique_lock<std::mutex> lck(m_mtx);
            if (m_shutdown) {
              break;
            }
            m_cv.wait(lck);
          }
        }
      });

    } else {
      m_thread = std::thread([&]() {
        if (m_options.get_set_sched_params()) {
          int rval = pthread_setschedparam(
              pthread_self(), m_options.get_policy(), &m_options.get_param());

          if (rval != 0) {
            m_errored_out_num = rval;
            return;
          }
        }

        while (true) {
          if (m_call()) {
            break;
          }

          {
            std::unique_lock<std::mutex> lck(m_mtx);
            if (m_shutdown) {
              break;
            }
            (void)m_cv.template wait_for(lck, m_options.get_timeout_micro());
          }
        }
      });
    }
  }
};

}  // namespace rtl

#endif  // RTLCPP_TASK_HPP
