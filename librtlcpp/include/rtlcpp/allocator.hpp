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

#ifndef RTLCPP_CONCENTS_ALLOCATOR_HPP
#define RTLCPP_CONCENTS_ALLOCATOR_HPP

#include <cstddef>
#include <functional>
#include <mutex>
#include <utility>

#include "rtl/memory.h"
#include "rtlcpp/mutex.hpp"

namespace rtl {

/*!
 * Allows for a chunk of memory to be allocated via memory-map.
 *
 * Must have init() called before being used.
 *
 */
class MMapMemoryResource final {
 private:
  bool m_initialized;
  void* m_buf;
  size_t m_capacity;

 public:
  MMapMemoryResource() : m_initialized(false), m_buf(nullptr), m_capacity(0U) {}
  ~MMapMemoryResource() { uninit(); }

  MMapMemoryResource(MMapMemoryResource const&) = delete;
  MMapMemoryResource& operator=(MMapMemoryResource const&) = delete;

  MMapMemoryResource(MMapMemoryResource&& o) noexcept;
  MMapMemoryResource& operator=(MMapMemoryResource&& o) noexcept;

  void* get_buf() const { return m_buf; }
  size_t get_capacity() const { return m_capacity; }

  /*!
   * Initializes the memory map to capacity bytes.
   *
   * Returns true if successfully initialized, otherwise false
   *
   * Must be used before getting a buffer to be used
   *
   * @param capacity the number of bytes to use in the memory map
   * @return true if successful, otherwise false
   */
  bool init(size_t capacity);

  //! De-initializes the memory map.
  void uninit();
};

namespace detail {

class RTAllocator final {
 private:
  bool m_initialized;
  struct rtl_tlsf_arena* m_arena;
  void* m_buf;
  size_t m_capacity;

 public:
  RTAllocator()
      : m_initialized(false), m_arena(nullptr), m_buf(nullptr), m_capacity(0U) {}

  ~RTAllocator() { RTAllocator::uninit(); }

  RTAllocator(RTAllocator const&) = delete;
  RTAllocator& operator=(RTAllocator const&) = delete;

  RTAllocator(RTAllocator&& o) noexcept;
  RTAllocator& operator=(RTAllocator&& o) noexcept;

  bool is_initialized() const { return m_initialized; }

  void* allocate(std::size_t bytes);

  void deallocate(void* p);

  bool init(void* buf, size_t capacity);

  void uninit();
};

}  // namespace detail

/*
 * The Allocator Concept
 *
 * RTL utilizes its own allocator template concept.  A class satisfies
 * the allocator concept when it provides these member functions:
 *
 * void* allocate(size_t sz)
 * void deallocate(void* p);
 *
 * - allocate() allocates at least sz bytes and returns a pointer to those bytes
 *
 * - deallocate() takes the pointer returned by allocate() and frees them back
 * to the heap.
 *
 * Note that one can create a "SystemAllocator" class by trivially wrapping
 * malloc:
 *
 * struct SystemAllocator {
 *   void* allocate(size_t sz) { return malloc(sz); }
 *   void deallocate(void* p) { free(p); }
 * };
 *
 */

/*!
 *
 * RTAllocator satisfies the RTL Allocator Concept with a real time
 * allocator.  It takes in a Mutex template parameter which would make it
 * suitable for single or multi-threaded access
 *
 * ***IMPORTANT***
 *
 * DO NOT CALL FREE() ON POINTERS ALLOCATED VIA allocate() AND DO NOT
 * CALL deallocate() ON POINTERS ALLOCATED VIA malloc()
 *
 * @tparam Mutex a class that satisfies the Lockable concept
 */
template <typename Mutex>
class RTAllocator final {
 private:
  detail::RTAllocator m_alloc;
  mutable Mutex m_mtx;

 public:
  //! Constructs an empty allocator, init() still needs to be called
  RTAllocator() : m_alloc(), m_mtx() {}
  ~RTAllocator() { m_alloc.~RTAllocator(); }

  RTAllocator(RTAllocator&& o) noexcept : m_mtx() {
    std::unique_lock<Mutex> rhs_lk(o.m_mtx);
    m_alloc = std::move(o.m_alloc);
  }

  RTAllocator& operator=(RTAllocator&& o) noexcept {
    if (this != &o) {
      std::unique_lock<Mutex> lhs_lk(m_mtx, std::defer_lock);
      std::unique_lock<Mutex> rhs_lk(o.m_mtx, std::defer_lock);
      std::lock(lhs_lk, rhs_lk);
      m_alloc = std::move(o.m_alloc);
    }
    return *this;
  }

  //! True if initialized, otherwise false
  bool is_initialized() const {
    std::lock_guard<Mutex> lck(m_mtx);
    return m_alloc.is_initialized();
  }

  //! Allocates bytes with the underlying allocator
  void* allocate(std::size_t bytes) {
    std::lock_guard<Mutex> lck(m_mtx);
    return m_alloc.allocate(bytes);
  }

  //! Frees bytes allocated via allocate()
  void deallocate(void* p) {
    std::lock_guard<Mutex> lck(m_mtx);
    m_alloc.deallocate(p);
  }

  /*!
   * Initializes the allocator to use the buffer provided
   * and only use capacity bytes of the buffer provided.
   *
   * If this function returns true, then the allocator is
   * ready to use.  Otherwise it will return false.
   *
   * If buffers can't be statically allocated, the MMapMemoryResource
   * class can be used to allocate memory via MMap.
   *
   * @param buf the buffer to allocate from
   * @param capacity the number of bytes to use from the buffer
   * @return true if successful, otherwise false
   */
  bool init(void* buf, size_t capacity) {
    std::lock_guard<Mutex> lck(m_mtx);
    return m_alloc.init(buf, capacity);
  }

  //! Uninitializes the allocator
  void uninit() {
    std::lock_guard<Mutex> lck(m_mtx);
    m_alloc.uninit();
  }
};

//! A real time allocator that can be used only by one thread
using RTAllocatorST = RTAllocator<NullMutex>;

//! A real time allocator that can be accessed by multiple threads
using RTAllocatorMT = RTAllocator<std::mutex>;

using RTDefaultAllocator = RTAllocatorMT;

}  // namespace rtl

#endif  // RTLCPP_CONCENTS_ALLOCATOR_HPP
