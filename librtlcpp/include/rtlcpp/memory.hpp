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

#ifndef RTLCPP_MEMORY_HPP
#define RTLCPP_MEMORY_HPP

#include <atomic>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <utility>

#include "rtlcpp/allocator.hpp"
#include "rtlcpp/utility.hpp"

namespace rtl {

/*
 * The following control_blk implementations are the auxiliary classes
 * used by shared_ptr and weak_ptr to perform reference counting garbage
 * collection.
 *
 * Normally one should use make_shared instead of constructing these
 * by hand.
 *
 */

template <typename T, typename Alloc>
struct control_blk {
 private:
  Alloc* m_alloc;
  T* m_data;

  /*!
   * The number of shared_ptrs pointing to this control block
   *
   * The data pointed to by m_data is deleted when this is zero
   */
  std::atomic<uint32_t> m_strong_count;

  /*!
   * The number of weak_ptrs pointing to this control block
   *
   * This number is +1 if m_strong_count is >0
   *
   * This control block should be deleted when this number is 0
   *
   */
  std::atomic<uint32_t> m_weak_count;

 public:
  control_blk(Alloc* alloc, T* data)
      : m_alloc(alloc), m_data(data), m_strong_count(0U), m_weak_count(0U) {}

  // We need to be able to support constructors/assignment
  // from derived classes as well
  template <typename U, typename std::enable_if<std::is_base_of<T, U>::value,
                                                int>::type = 0>
  control_blk(Alloc* alloc, U* data)
      : control_blk(alloc, static_cast<T*>(data)) {}

  ~control_blk() { deinit(); };

  control_blk(control_blk const&) = delete;
  control_blk& operator=(control_blk const&) = delete;

  control_blk(control_blk&& o) noexcept
      : m_alloc(rtl::exchange(o.m_alloc, nullptr)),
        m_data(rtl::exchange(o.m_data, nullptr)),
        m_strong_count(o.m_strong_count.load()),
        m_weak_count(o.m_weak_count.load()) {}

  control_blk& operator=(control_blk&& o) noexcept {
    if (this != &o) {
      deinit();
      m_alloc = rtl::exchange(o.m_alloc, nullptr);
      m_data = rtl::exchange(o.m_data, nullptr);
      m_strong_count = o.m_strong_count.load();
      m_weak_count = o.m_weak_count.load();
    }
    return *this;
  }

  uint32_t strong_count() const noexcept { return m_strong_count.load(); };
  uint32_t weak_count() const noexcept { return m_weak_count.load(); }

  T* get() const { return m_data; }

  T* get_weak() {
    uint32_t strong_count = m_strong_count.load();
    while (true) {
      if (strong_count == 0U) {
        return nullptr;
      }
      if (m_strong_count.compare_exchange_weak(strong_count,
                                               strong_count + 1U)) {
        return m_data;
      }
    }
  }

  void deinit() {
    bool valid_data = m_data;
    if (!valid_data) {
      return;
    }
    m_data->~T();
    bool valid_alloc = m_alloc;
    if (!valid_alloc) {
      return;
    }
    m_alloc->deallocate(static_cast<void*>(m_data));
    m_data = nullptr;
  }

  void inc_strong() {
    uint32_t prev_strong_count = m_strong_count.fetch_add(1U);

    if (prev_strong_count == 0U) {
      // If we went from zero to a non-zero number, lets add 1 to the weak_count
      (void)m_weak_count.fetch_add(1U);
    }
  }

  void inc_weak() { (void)m_weak_count.fetch_add(1U); }

  /*!
   * Decrements the strong counter
   * @return true if the control block should be deallocated, otherwise false
   */
  bool dec_strong() {
    uint32_t prev_val = m_strong_count.fetch_sub(1U);

    assert(prev_val != 0);

    // if prev_val was 1, that means it was the last strong pointer
    if (prev_val == 1U) {
      // De-allocate the pointed to object
      deinit();
      // Then we can decrement the weak_pointer
      return dec_weak();
    }

    return false;
  }

  /*!
   * Decrements the weak counter
   * @return true if the control block should be deallocated, otherwise false
   */
  bool dec_weak() {
    uint32_t prev_val = m_weak_count.fetch_sub(1U);

    assert(prev_val != 0);

    return (prev_val == 1U);
  }
};

template <typename T, typename Alloc>
class control_blk<T[], Alloc> {
 private:
  Alloc* m_alloc;
  size_t m_array_count;
  T* m_array;

  /*!
   * The number of shared_ptrs pointing to this control block
   *
   * The data pointed to by m_data is deleted when this is zero
   */
  std::atomic<uint32_t> m_strong_count;

  /*!
   * The number of weak_ptrs pointing to this control block
   *
   * This number is +1 if m_strong_count is >0
   *
   * This control block should be deleted when this number is 0
   *
   */
  std::atomic<uint32_t> m_weak_count;

 public:
  control_blk(Alloc* alloc, size_t array_count, T* array)
      : m_alloc(alloc),
        m_array_count(array_count),
        m_array(array),
        m_strong_count(0U),
        m_weak_count(0U) {}

  uint32_t strong_count() const noexcept { return m_strong_count.load(); };
  uint32_t weak_count() const noexcept { return m_weak_count.load(); }

  T* get() const { return m_array; }
  size_t array_size() const { return m_array_count; }

  T* get_weak() {
    uint32_t strong_count = m_strong_count.load();
    while (true) {
      if (strong_count == 0U) {
        return nullptr;
      }
      if (m_strong_count.compare_exchange_weak(strong_count,
                                               strong_count + 1U)) {
        return m_array;
      }
    }
  }

  void deinit() {
    if (m_array == nullptr) {
      return;
    }
    for (size_t i = 0U; i < m_array_count; i++) {
      m_array[i].~T();
    }
    if (m_alloc == nullptr) {
      return;
    }
    m_alloc->deallocate(static_cast<void*>(m_array));
    m_array = nullptr;
  }

  void inc_strong() {
    uint32_t prev_strong_count = m_strong_count.fetch_add(1U);

    if (prev_strong_count == 0U) {
      // If we went from zero to a non-zero number, lets add 1 to the weak_count
      (void)m_weak_count.fetch_add(1U);
    }
  }

  void inc_weak() { (void)m_weak_count.fetch_add(1U); }

  /*!
   * Decrements the strong counter
   * @return true if the control block should be deallocated, otherwise false
   */
  bool dec_strong() {
    uint32_t prev_val = m_strong_count.fetch_sub(1U);

    assert(prev_val != 0);

    // if prev_val was 1, that means it was the last strong pointer
    if (prev_val == 1U) {
      // De-allocate the pointed to object
      deinit();
      // Then we can decrement the weak_pointer
      return dec_weak();
    }

    return false;
  }

  /*!
   * Decrements the weak counter
   * @return true if the control block should be deallocated, otherwise false
   */
  bool dec_weak() {
    uint32_t prev_val = m_weak_count.fetch_sub(1U);

    assert(prev_val != 0);

    return (prev_val == 1U);
  }
};

template <typename T, typename Alloc = rtl::RTDefaultAllocator>
class shared_ptr {
 private:
  Alloc* m_alloc;
  control_blk<T, Alloc>* m_control_blk;

 public:
  Alloc* get_alloc() const { return m_alloc; }
  control_blk<T, Alloc>* get_control_blk() const { return m_control_blk; }

  shared_ptr() : m_alloc(nullptr), m_control_blk(nullptr) {}
  shared_ptr(Alloc* alloc, control_blk<T, Alloc>* control_blk_)
      : m_alloc(alloc), m_control_blk(control_blk_) {
    if (m_control_blk) {
      m_control_blk->inc_strong();
    }
  }

  ~shared_ptr() { reset(); }

  shared_ptr(std::nullptr_t) : shared_ptr() {}

  shared_ptr& operator=(std::nullptr_t) {
    reset();
    return *this;
  }

  shared_ptr(shared_ptr&& o) noexcept
      : m_alloc(rtl::exchange(o.m_alloc, nullptr)),
        m_control_blk(rtl::exchange(o.m_control_blk, nullptr)) {}

  shared_ptr& operator=(shared_ptr&& o) noexcept {
    if (this != &o) {
      reset();
      m_alloc = rtl::exchange(o.m_alloc, nullptr);
      m_control_blk = rtl::exchange(o.m_control_blk, nullptr);
    }
    return *this;
  }

  // We need to be able to support constructors/assignment
  // from derived classes as well
  template <typename U, typename std::enable_if<std::is_base_of<T, U>::value,
                                                int>::type = 0>
  shared_ptr<T, Alloc>(shared_ptr<U, Alloc>&& o) noexcept
      : m_alloc(o.get_alloc()),
        m_control_blk(
            reinterpret_cast<control_blk<T, Alloc>*>(o.release_control_blk())) {
  }

  template <typename U, typename std::enable_if<std::is_base_of<T, U>::value,
                                                int>::type = 0>
  shared_ptr<T, Alloc>& operator=(shared_ptr<U, Alloc>&& o) noexcept {
    reset();
    m_alloc = o.get_alloc();
    m_control_blk =
        reinterpret_cast<control_blk<T, Alloc>*>(o.release_control_blk());
    return *this;
  }

  shared_ptr(shared_ptr const& o)
      : m_alloc(o.m_alloc), m_control_blk(o.m_control_blk) {
    if (m_control_blk) {
      m_control_blk->inc_strong();
    }
  }

  shared_ptr& operator=(shared_ptr const& o) {
    if (this != &o) {
      reset();
      m_alloc = o.m_alloc;
      m_control_blk = o.m_control_blk;
      if (m_control_blk) {
        m_control_blk->inc_strong();
      }
    }
    return *this;
  }

  // We need to be able to support constructors/assignment
  // from derived classes as well
  template <typename U, typename std::enable_if<std::is_base_of<T, U>::value,
                                                int>::type = 0>
  shared_ptr<T, Alloc>(shared_ptr<U, Alloc> const& o)
      : m_alloc(o.get_alloc()),
        m_control_blk(
            reinterpret_cast<control_blk<T, Alloc>*>(o.get_control_blk())) {
    if (m_control_blk) {
      m_control_blk->inc_strong();
    }
  }

  template <typename U, typename std::enable_if<std::is_base_of<T, U>::value,
                                                int>::type = 0>
  shared_ptr<T, Alloc>& operator=(shared_ptr<U, Alloc> const& o) {
    reset();
    m_alloc = o.get_alloc();
    m_control_blk =
        reinterpret_cast<control_blk<T, Alloc>*>(o.get_control_blk());
    if (m_control_blk) {
      m_control_blk->inc_strong();
    }
    return *this;
  }

  template <typename Rval = typename std::remove_extent<T>::type&>
  typename std::enable_if<std::is_array<T>::value, Rval>::type operator[](
      size_t idx) const {
    return m_control_blk->get()[idx];
  }

  template <typename Rval = size_t>
  typename std::enable_if<std::is_array<T>::value, Rval>::type array_size()
      const {
    return m_control_blk ? m_control_blk->array_size() : 0;
  }

  T* operator->() const {
    return m_control_blk ? m_control_blk->get() : nullptr;
  }

  T& operator*() const { return *m_control_blk->get(); }

  typename std::remove_extent<T>::type* get() const {
    return m_control_blk ? m_control_blk->get() : nullptr;
  }

  explicit operator bool() const {
    return m_control_blk && m_control_blk->get();
  }

  void swap(shared_ptr& o) noexcept {
    std::swap(m_alloc, o.m_alloc);
    std::swap(m_control_blk, o.m_control_blk);
  }

  void reset() {
    if (m_control_blk == nullptr) {
      return;
    }
    assert(m_alloc != nullptr);
    if (m_control_blk->dec_strong()) {
      m_alloc->deallocate(static_cast<void*>(m_control_blk));
    }
    m_control_blk = nullptr;
  }

  control_blk<T, Alloc>* release_control_blk() noexcept {
    control_blk<T, Alloc>* rval = nullptr;
    std::swap(rval, m_control_blk);
    return rval;
  }

  uint32_t use_count() const noexcept {
    return m_control_blk ? m_control_blk->strong_count() : 0;
  }
};

template <typename T, typename Alloc>
bool operator==(const shared_ptr<T, Alloc>& lhs,
                const shared_ptr<T, Alloc>& rhs) {
  return lhs.get() == rhs.get();
}

template <typename T, typename Alloc>
bool operator!=(const shared_ptr<T, Alloc>& lhs,
                const shared_ptr<T, Alloc>& rhs) {
  bool rhs_eq_lhs = rhs == lhs;
  return !rhs_eq_lhs;
}

template <typename T, typename Alloc>
void swap(shared_ptr<T, Alloc>& l, shared_ptr<T, Alloc>& r) {
  l.swap(r);
}

// T2 is the same as T but we need a different typename for perfect forwarding
template <typename T, typename Alloc, typename T2,
          bool b = std::is_array<T>::value>
typename std::enable_if<!b, shared_ptr<T, Alloc>>::type make_shared(
    Alloc* alloc, T2&& t) {
  control_blk<T, Alloc>* tmp = static_cast<control_blk<T, Alloc>*>(
      alloc->allocate(sizeof(control_blk<T, Alloc>)));

  if (tmp == nullptr) {
    return shared_ptr<T, Alloc>();
  }

  T* data_tmp = static_cast<T*>(alloc->allocate(sizeof(T)));
  if (data_tmp == nullptr) {
    alloc->deallocate(static_cast<void*>(tmp));
    return shared_ptr<T, Alloc>();
  }

  new (data_tmp) T(std::forward<T2>(t));

  new (tmp) control_blk<T, Alloc>(alloc, data_tmp);

  return shared_ptr<T, Alloc>(alloc, tmp);
}

template <typename T, typename Alloc, bool b = std::is_array<T>::value,
          typename... Args>
typename std::enable_if<!b, shared_ptr<T, Alloc>>::type make_shared(
    Alloc* alloc, Args&&... args) {
  control_blk<T, Alloc>* tmp = static_cast<control_blk<T, Alloc>*>(
      alloc->allocate(sizeof(control_blk<T, Alloc>)));
  if (tmp == nullptr) {
    return shared_ptr<T, Alloc>();
  }

  T* data_tmp = static_cast<T*>(alloc->allocate(sizeof(T)));
  if (data_tmp == nullptr) {
    alloc->deallocate(static_cast<void*>(tmp));
    return shared_ptr<T, Alloc>();
  }

  new (data_tmp) T(std::forward<Args>(args)...);

  new (tmp) control_blk<T, Alloc>(alloc, data_tmp);

  return shared_ptr<T, Alloc>(alloc, tmp);
}

template <typename T, typename Alloc, bool b = std::is_array<T>::value,
          typename... Args>
typename std::enable_if<!b, shared_ptr<T, Alloc>>::type make_shared(
    Alloc* alloc) {
  control_blk<T, Alloc>* tmp = static_cast<control_blk<T, Alloc>*>(
      alloc->allocate(sizeof(control_blk<T, Alloc>)));
  if (tmp == nullptr) {
    return shared_ptr<T, Alloc>();
  }

  T* data_tmp = static_cast<T*>(alloc->allocate(sizeof(T)));
  if (data_tmp == nullptr) {
    alloc->deallocate(static_cast<void*>(tmp));
    return shared_ptr<T, Alloc>();
  }

  new (data_tmp) T();

  new (tmp) control_blk<T, Alloc>(alloc, data_tmp);

  return shared_ptr<T, Alloc>(alloc, tmp);
}

template <typename T, typename Alloc, bool b = std::is_array<T>::value,
          typename... Args>
typename std::enable_if<!b, shared_ptr<T, Alloc>>::type make_shared(
    Alloc* alloc, T* ptr) {
  control_blk<T, Alloc>* tmp = static_cast<control_blk<T, Alloc>*>(
      alloc->allocate(sizeof(control_blk<T, Alloc>)));
  if (tmp == nullptr) {
    return shared_ptr<T, Alloc>();
  }

  new (tmp) control_blk<T, Alloc>(alloc, ptr);

  return shared_ptr<T, Alloc>(alloc, tmp);
}

template <typename T, typename Alloc, typename... Args>
typename std::enable_if<std::is_array<T>::value, shared_ptr<T, Alloc>>::type
make_shared(Alloc* alloc, size_t count, Args&&... args) {
  control_blk<T, Alloc>* tmp = static_cast<control_blk<T, Alloc>*>(
      alloc->allocate(sizeof(control_blk<T, Alloc>)));
  if (tmp == nullptr) {
    return shared_ptr<T, Alloc>();
  }

  using type = typename std::remove_extent<T>::type;

  type* array_tmp = static_cast<type*>(alloc->allocate(sizeof(type) * count));
  if (array_tmp == nullptr) {
    alloc->deallocate(static_cast<void*>(tmp));
    return shared_ptr<T, Alloc>();
  }

  for (size_t i = 0U; i < count; i++) {
    new (&array_tmp[i]) type(std::forward<Args>(args)...);
  }

  new (tmp) control_blk<T, Alloc>(alloc, count, array_tmp);

  return shared_ptr<T, Alloc>(alloc, tmp);
}

template <typename T, typename Alloc, typename... Args>
typename std::enable_if<std::is_array<T>::value, shared_ptr<T, Alloc>>::type
make_shared(Alloc* alloc, size_t count) {
  control_blk<T, Alloc>* tmp = static_cast<control_blk<T, Alloc>*>(
      alloc->allocate(sizeof(control_blk<T, Alloc>)));
  if (tmp == nullptr) {
    return shared_ptr<T, Alloc>();
  }

  using type = typename std::remove_extent<T>::type;

  type* array_tmp = static_cast<type*>(alloc->allocate(sizeof(type) * count));
  if (array_tmp == nullptr) {
    alloc->deallocate(static_cast<void*>(tmp));
    return shared_ptr<T, Alloc>();
  }

  for (size_t i = 0U; i < count; i++) {
    new (&array_tmp[i]) type();
  }

  new (tmp) control_blk<T, Alloc>(alloc, count, array_tmp);

  return shared_ptr<T, Alloc>(alloc, tmp);
}

template <typename T, typename Alloc, typename... Args>
typename std::enable_if<std::is_array<T>::value, shared_ptr<T, Alloc>>::type
make_shared(Alloc* alloc, size_t count,
            typename std::remove_extent<T>::type* ptr) {
  control_blk<T, Alloc>* tmp = static_cast<control_blk<T, Alloc>*>(
      alloc->allocate(sizeof(control_blk<T, Alloc>)));
  if (tmp == nullptr) {
    return shared_ptr<T, Alloc>();
  }

  new (tmp) control_blk<T, Alloc>(alloc, count, ptr);

  return shared_ptr<T, Alloc>(alloc, tmp);
}

template <typename T, typename Alloc = rtl::RTDefaultAllocator>
class weak_ptr {
 private:
  Alloc* m_alloc;
  control_blk<T, Alloc>* m_control_blk;

 public:
  Alloc* get_alloc() const { return m_alloc; }
  control_blk<T, Alloc>* get_control_blk() const { return m_control_blk; }

  weak_ptr() : m_alloc(nullptr), m_control_blk(nullptr) {}
  ~weak_ptr() { reset(); }

  weak_ptr(shared_ptr<T, Alloc> const& o)
      : m_alloc(o.get_alloc()), m_control_blk(o.get_control_blk()) {
    if (m_control_blk) {
      m_control_blk->inc_weak();
    }
  }

  weak_ptr& operator=(shared_ptr<T, Alloc> const& o) {
    reset();
    m_alloc = o.get_alloc();
    m_control_blk = static_cast<control_blk<T, Alloc>*>(o.get_control_blk());
    if (m_control_blk) {
      m_control_blk->inc_weak();
    }
    return *this;
  }

  // We need to be able to support constructors/assignment
  // from derived classes as well
  template <typename U, typename std::enable_if<std::is_base_of<T, U>::value,
                                                int>::type = 0>
  weak_ptr<T, Alloc>(shared_ptr<U, Alloc> const& o) noexcept
      : m_alloc(o.get_alloc()),
        m_control_blk(
            reinterpret_cast<control_blk<T, Alloc>*>(o.get_control_blk())) {
    if (m_control_blk) {
      m_control_blk->inc_weak();
    }
  }

  template <typename U, typename std::enable_if<std::is_base_of<T, U>::value,
                                                int>::type = 0>
  weak_ptr<T, Alloc>& operator=(shared_ptr<U, Alloc> const& o) {
    reset();
    m_alloc = o.get_alloc();
    m_control_blk =
        reinterpret_cast<control_blk<T, Alloc>*>(o.get_control_blk());
    if (m_control_blk) {
      m_control_blk->inc_weak();
    }
    return *this;
  }

  weak_ptr(std::nullptr_t) : weak_ptr() {}

  weak_ptr& operator=(std::nullptr_t) {
    reset();
    return *this;
  }

  weak_ptr(weak_ptr&& o) noexcept
      : m_alloc(rtl::exchange(o.m_alloc, nullptr)),
        m_control_blk(rtl::exchange(o.m_control_blk, nullptr)) {}

  weak_ptr& operator=(weak_ptr&& o) noexcept {
    if (this != &o) {
      reset();
      m_alloc = rtl::exchange(o.m_alloc, nullptr);
      m_control_blk = rtl::exchange(o.m_control_blk, nullptr);
    }
    return *this;
  }

  // We need to be able to support constructors/assignment
  // from derived classes as well
  template <typename U, typename std::enable_if<std::is_base_of<T, U>::value,
                                                int>::type = 0>
  weak_ptr<T, Alloc>(weak_ptr<U, Alloc>&& o) noexcept
      : m_alloc(o.get_alloc()),
        m_control_blk(
            reinterpret_cast<control_blk<T, Alloc>*>(o.release_control_blk())) {
  }

  template <typename U, typename std::enable_if<std::is_base_of<T, U>::value,
                                                int>::type = 0>
  weak_ptr<T, Alloc>& operator=(weak_ptr<U, Alloc>&& o) noexcept {
    reset();
    m_alloc = o.get_alloc();
    m_control_blk =
        reinterpret_cast<control_blk<T, Alloc>*>(o.release_control_blk());
    return *this;
  }

  weak_ptr(weak_ptr const& o)
      : m_alloc(o.m_alloc), m_control_blk(o.m_control_blk) {
    if (m_control_blk) {
      m_control_blk->inc_weak();
    }
  }

  weak_ptr& operator=(weak_ptr const& o) {
    if (this != &o) {
      reset();
      m_alloc = o.m_alloc;
      m_control_blk = o.m_control_blk;
      if (m_control_blk) {
        m_control_blk->inc_weak();
      }
    }
    return *this;
  }

  // We need to be able to support constructors/assignment
  // from derived classes as well
  template <typename U, typename std::enable_if<std::is_base_of<T, U>::value,
                                                int>::type = 0>
  weak_ptr<T, Alloc>(weak_ptr<U, Alloc> const& o)
      : m_alloc(o.get_alloc()),
        m_control_blk(
            reinterpret_cast<control_blk<T, Alloc>*>(o.get_control_blk())) {
    if (m_control_blk) {
      m_control_blk->inc_weak();
    }
  }

  template <typename U, typename std::enable_if<std::is_base_of<T, U>::value,
                                                int>::type = 0>
  weak_ptr<T, Alloc>& operator=(weak_ptr<U, Alloc> const& o) {
    reset();
    m_alloc = o.get_alloc();
    m_control_blk =
        reinterpret_cast<control_blk<T, Alloc>*>(o.get_control_blk());
    if (m_control_blk) {
      m_control_blk->inc_weak();
    }
    return *this;
  }

  void reset() {
    if (m_control_blk == nullptr) {
      return;
    }
    assert(m_alloc != nullptr);
    if (m_control_blk->dec_weak()) {
      m_alloc->deallocate(static_cast<void*>(m_control_blk));
    }
    m_control_blk = nullptr;
  }

  control_blk<T, Alloc>* release_control_blk() noexcept {
    control_blk<T, Alloc>* rval = nullptr;
    std::swap(rval, m_control_blk);
    return rval;
  }

  uint32_t use_count() const noexcept {
    return m_control_blk ? m_control_blk->strong_count() : 0;
  }

  bool expired() const noexcept { return use_count() == 0; }

  shared_ptr<T, Alloc> lock() const {
    if (expired()) {
      return shared_ptr<T, Alloc>();
    }
    return shared_ptr<T, Alloc>(m_alloc, m_control_blk);
  }

  explicit operator bool() const {
    return m_control_blk && m_control_blk->get();
  }

  void swap(weak_ptr& o) noexcept {
    std::swap(m_alloc, o.m_alloc);
    std::swap(m_control_blk, o.m_control_blk);
  }
};

template <typename T, typename Alloc = rtl::RTDefaultAllocator>
class unique_ptr {
 private:
  Alloc* m_alloc;
  T* m_data;

 public:
  unique_ptr() : m_alloc(nullptr), m_data(nullptr) {}
  unique_ptr(Alloc* alloc, T* data) : m_alloc(alloc), m_data(data) {}

  ~unique_ptr() { reset(); }

  unique_ptr(std::nullptr_t) : unique_ptr() {}

  // When we assign a nullptr we will reset the value
  unique_ptr& operator=(std::nullptr_t) {
    reset();
    return *this;
  }

  unique_ptr(unique_ptr&& o) noexcept
      : m_alloc(rtl::exchange(o.m_alloc, nullptr)),
        m_data(rtl::exchange(o.m_data, nullptr)) {}

  unique_ptr& operator=(unique_ptr&& o) noexcept {
    if (this != &o) {
      reset();
      m_alloc = rtl::exchange(o.m_alloc, nullptr);
      m_data = rtl::exchange(o.m_data, nullptr);
    }
    return *this;
  }

  // We need to be able to support constructors/assignment
  // from derived classes as well
  template <typename U, typename std::enable_if<std::is_base_of<T, U>::value,
                                                int>::type = 0>
  unique_ptr<T, Alloc>(unique_ptr<U, Alloc>&& o) noexcept
      : m_alloc(o.get_alloc()), m_data(o.release()) {}

  template <typename U, typename std::enable_if<std::is_base_of<T, U>::value,
                                                int>::type = 0>
  unique_ptr& operator=(unique_ptr<U, Alloc>&& o) noexcept {
    unique_ptr<T, Alloc> tmp{o.get_alloc(), o.release()};
    reset();
    *this = std::move(tmp);
    return *this;
  }

  unique_ptr(unique_ptr const&) = delete;
  unique_ptr& operator=(unique_ptr const&) = delete;

  T* operator->() const { return m_data; }
  T& operator*() const { return *m_data; }

  T* get() const { return m_data; }
  explicit operator bool() const { return m_data; }
  Alloc* get_alloc() const { return m_alloc; }

  void swap(unique_ptr& o) noexcept {
    std::swap(m_alloc, o.m_alloc);
    std::swap(m_data, o.m_data);
  }

  T* release() noexcept {
    T* rval = nullptr;
    std::swap(rval, m_data);
    return rval;
  }

  void reset() {
    T* to_del = release();
    if (to_del == nullptr) {
      return;
    }
    to_del->~T();
    if (m_alloc) {
      m_alloc->deallocate(static_cast<void*>(to_del));
    }
    m_alloc = nullptr;
  }
};

template <typename T, typename Alloc>
void swap(unique_ptr<T, Alloc>& l, unique_ptr<T, Alloc>& r) {
  l.swap(r);
}

// T2 is the same as T but we need a different typename for perfect forwarding
template <typename T, typename Alloc, bool b = std::is_array<T>::value,
          typename T2>
typename std::enable_if<!b, unique_ptr<T, Alloc>>::type make_unique(
    Alloc* alloc, T2&& t) {
  T* tmp = static_cast<T*>(alloc->allocate(sizeof(T)));
  if (tmp == nullptr) {
    return unique_ptr<T, Alloc>();
  }
  new (tmp) T(std::forward<T2>(t));

  return unique_ptr<T, Alloc>(alloc, tmp);
}

template <typename T, typename Alloc, bool b = std::is_array<T>::value,
          typename... Args>
typename std::enable_if<!b, unique_ptr<T, Alloc>>::type make_unique(
    Alloc* alloc, Args&&... args) {
  T* tmp = static_cast<T*>(alloc->allocate(sizeof(T)));
  if (tmp == nullptr) {
    return unique_ptr<T, Alloc>();
  }
  new (tmp) T(std::forward<Args>(args)...);

  return unique_ptr<T, Alloc>(alloc, tmp);
}

template <typename T, bool b = std::is_array<T>::value, typename Alloc,
          typename... Args>
typename std::enable_if<!b, unique_ptr<T, Alloc>>::type make_unique(
    Alloc* alloc) {
  T* tmp = static_cast<T*>(alloc->allocate(sizeof(T)));
  if (tmp == nullptr) {
    return unique_ptr<T, Alloc>();
  }

  new (tmp) T();

  return unique_ptr<T, Alloc>(alloc, tmp);
}

template <typename T, bool b = std::is_array<T>::value, typename Alloc,
          typename... Args>
typename std::enable_if<!b, unique_ptr<T, Alloc>>::type make_unique(
    Alloc* alloc, T* ptr) {
  return unique_ptr<T, Alloc>(alloc, ptr);
}

template <typename T, typename Alloc>
class unique_ptr<T[], Alloc> {
 private:
  Alloc* m_alloc;
  T* m_array;
  size_t m_array_count;

 public:
  unique_ptr() : m_alloc(nullptr), m_array(nullptr), m_array_count(0U) {}
  unique_ptr(Alloc* alloc, T* array, size_t array_count)
      : m_alloc(alloc), m_array(array), m_array_count(array_count) {}

  ~unique_ptr() { reset(); }

  unique_ptr(std::nullptr_t) : unique_ptr() {}

  // When we assign a nullptr we will reset the value
  unique_ptr& operator=(std::nullptr_t) {
    reset();
    return *this;
  }

  unique_ptr(unique_ptr&& o) noexcept
      : m_alloc(rtl::exchange(o.m_alloc, nullptr)),
        m_array(rtl::exchange(o.m_array, nullptr)),
        m_array_count(rtl::exchange(o.m_array_count, 0)) {}

  unique_ptr& operator=(unique_ptr&& o) noexcept {
    if (this != &o) {
      reset();
      m_alloc = rtl::exchange(o.m_alloc, nullptr);
      m_array = rtl::exchange(o.m_array, nullptr);
      m_array_count = rtl::exchange(o.m_array_count, 0);
    }
    return *this;
  }

  unique_ptr(unique_ptr const&) = delete;
  unique_ptr& operator=(unique_ptr const&) = delete;

  T& operator[](size_t idx) const { return m_array[idx]; }

  T* get() const { return m_array; }
  size_t array_size() const { return m_array_count; }

  explicit operator bool() const { return m_array; }
  Alloc* get_alloc() const { return m_alloc; }

  void swap(unique_ptr& o) noexcept {
    std::swap(m_alloc, o.m_alloc);
    std::swap(m_array, o.m_array);
    std::swap(m_array_count, o.m_array_count);
  }

  T* release() noexcept {
    T* rval = nullptr;
    std::swap(rval, m_array);
    return rval;
  }

  void reset() {
    T* to_del = release();
    if (to_del) {
      for (size_t i = 0U; i < m_array_count; i++) {
        to_del[i].~T();
      }
      if (m_alloc) {
        m_alloc->deallocate(static_cast<void*>(to_del));
      }
    }
    m_array_count = 0U;
    m_alloc = nullptr;
  }
};

template <typename T, typename Alloc>
void swap(unique_ptr<T[], Alloc>& l, unique_ptr<T[], Alloc>& r) {
  l.swap(r);
}

template <typename T, typename Alloc, typename... Args>
typename std::enable_if<std::is_array<T>::value, unique_ptr<T, Alloc>>::type
make_unique(Alloc* alloc, size_t count, Args&&... args) {
  if (count == 0U) {
    return unique_ptr<T, Alloc>();
  }
  using type = typename std::remove_extent<T>::type;
  type* tmp = static_cast<type*>(alloc->allocate(sizeof(type) * count));
  if (tmp == nullptr) {
    return unique_ptr<T, Alloc>();
  }
  for (size_t i = 0U; i < count; i++) {
    new (&tmp[i]) type(std::forward<Args>(args)...);
  }
  return unique_ptr<T, Alloc>(alloc, tmp, count);
}

template <typename T, typename Alloc, typename... Args>
typename std::enable_if<std::is_array<T>::value, unique_ptr<T, Alloc>>::type
make_unique(Alloc* alloc, size_t count) {
  if (count == 0U) {
    return unique_ptr<T, Alloc>();
  }
  using type = typename std::remove_extent<T>::type;
  type* tmp = static_cast<type*>(alloc->allocate(sizeof(type) * count));
  if (tmp == nullptr) {
    return unique_ptr<T, Alloc>();
  }
  for (size_t i = 0U; i < count; i++) {
    new (&tmp[i]) type();
  }
  return unique_ptr<T, Alloc>(alloc, tmp, count);
}

template <typename T, typename Alloc, typename... Args>
typename std::enable_if<std::is_array<T>::value, unique_ptr<T, Alloc>>::type
make_unique(Alloc* alloc, size_t count,
            typename std::remove_extent<T>::type* ptr) {
  if (count == 0U) {
    return unique_ptr<T, Alloc>();
  }
  return unique_ptr<T, Alloc>(alloc, ptr, count);
}

template <typename T, typename Alloc>
bool operator==(const unique_ptr<T, Alloc>& lhs,
                const unique_ptr<T, Alloc>& rhs) {
  return lhs.get() == rhs.get();
}

template <typename T, typename Alloc>
bool operator!=(const unique_ptr<T, Alloc>& lhs,
                const unique_ptr<T, Alloc>& rhs) {
  bool rhs_eq_lhs = rhs == lhs;
  return !rhs_eq_lhs;
}

}  // namespace rtl

#endif  // RTLCPP_MEMORY_HPP
