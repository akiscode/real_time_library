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

#ifndef RTLCPP_MUTEX_HPP
#define RTLCPP_MUTEX_HPP

#include <atomic>
#include <mutex>
#include <thread>

#include "rtlcpp/utility.hpp"

namespace rtl {

class IMutex {
 public:
  virtual ~IMutex() = default;

  virtual void lock() noexcept = 0;
  virtual bool try_lock() noexcept = 0;
  virtual void unlock() noexcept = 0;
};

class IMutexLockGuard {
 private:
  IMutex* m_mtx;

 public:
  explicit IMutexLockGuard(IMutex* mtx) : m_mtx(mtx) {
    if (m_mtx != nullptr) {
      m_mtx->lock();
    }
  }

  ~IMutexLockGuard() {
    if (m_mtx != nullptr) {
      m_mtx->unlock();
    }
  }

  IMutexLockGuard(IMutexLockGuard const&) = delete;
  IMutexLockGuard& operator=(IMutexLockGuard const&) = delete;

  IMutexLockGuard(IMutexLockGuard&&) noexcept = delete;
  IMutexLockGuard& operator=(IMutexLockGuard&&) noexcept = delete;
};

class MutexWrapper : public IMutex {
 private:
  std::mutex m_mtx;

 public:
  MutexWrapper() : m_mtx() {}

  virtual void lock() noexcept override { m_mtx.lock(); }
  virtual bool try_lock() noexcept override { return m_mtx.try_lock(); }
  virtual void unlock() noexcept override { m_mtx.unlock(); }
};

class NullMutex : public IMutex {
 public:
  virtual void lock() noexcept override {}
  virtual bool try_lock() noexcept override { return true; }
  virtual void unlock() noexcept override {}
};

class SpinLock : public IMutex {
 private:
  std::atomic<bool> m_lock;

 public:
  SpinLock() : m_lock(false) {}

  virtual void lock() noexcept override {
    while (true) {
      // Maybe we get lucky on the first try
      bool got_false = m_lock.exchange(true, std::memory_order_acquire);
      if (!got_false) {
        return;
      }

      while (m_lock.load(std::memory_order_relaxed)) {
        rtl::asm_cpu_relax();
      }
    }
  }

  virtual bool try_lock() noexcept override {
    return !m_lock.load(std::memory_order_relaxed) &&
           !m_lock.exchange(true, std::memory_order_acquire);
  }

  virtual void unlock() noexcept override {
    m_lock.store(false, std::memory_order_release);
  }
};

/*
 * The Slumber Concept
 *
 * RTL utilizes the "slumber" concept for classes that need to sleep/yield
 * to the OS scheduler.  A class satisfies the slumber concept when it provides
 * these member functions:
 *
 * Default Constructor
 * Copy Constructor/Copy Assignment Constructor
 * Constructor that takes in std::chrono::nanoseconds
 *
 * void wait()
 *
 * - wait() will signal the OS scheduler to yield this thread.  Either
 * for a certain amount of time or via OS primitives.
 *
 */

class SlumberViaYield {
 public:
  explicit SlumberViaYield(std::chrono::nanoseconds) {}
  void wait() { std::this_thread::yield(); }
};

class SlumberViaSleep {
 private:
  const std::chrono::nanoseconds m_sleep_duration;

 public:
  SlumberViaSleep() : m_sleep_duration(std::chrono::microseconds(200)) {}

  explicit SlumberViaSleep(std::chrono::nanoseconds sleep_duration)
      : m_sleep_duration(sleep_duration) {}

  void wait() { std::this_thread::sleep_for(m_sleep_duration); }
};

class SlumberViaProgressive {
 private:
  uint32_t m_loop_count;
  const uint32_t m_max_loop_count;
  SlumberViaSleep m_sleep;

 public:
  SlumberViaProgressive()
      : m_loop_count(0U), m_max_loop_count(3500U), m_sleep() {}

  explicit SlumberViaProgressive(uint32_t max_loop_count)
      : m_loop_count(0U), m_max_loop_count(max_loop_count), m_sleep() {}

  explicit SlumberViaProgressive(std::chrono::nanoseconds sleep_duration)
      : m_loop_count(0U), m_max_loop_count(3500U), m_sleep(sleep_duration) {}

  SlumberViaProgressive(uint32_t max_loop_count,
                        std::chrono::nanoseconds sleep_duration)
      : m_loop_count(0U),
        m_max_loop_count(max_loop_count),
        m_sleep(sleep_duration) {}

  void wait() {
    if (m_loop_count < m_max_loop_count) {
      m_loop_count++;
      asm_cpu_relax();
    } else {
      // Time to use the sleep member variable...
      m_sleep.wait();
    }
  }
};

}  // namespace rtl

#endif  // RTLCPP_MUTEX_HPP
