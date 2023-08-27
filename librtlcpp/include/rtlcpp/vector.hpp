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

#ifndef RTLCPP_VECTOR_HPP
#define RTLCPP_VECTOR_HPP

#include <utility>  // for std::swap

#include "rtlcpp/allocator.hpp"
#include "rtlcpp/utility.hpp"

namespace rtl {

/*!
 * A  allocator aware vector class.
 *
 * This class is not thread-safe.
 *
 * @tparam T the type held in the vector
 * @tparam Alloc the allocator provided
 */
template <typename T, typename Alloc = RTDefaultAllocator>
class vector {
 private:
  Alloc* m_alloc;

  T* m_buf;

  //! The number of elements currently in the vector
  size_t m_count;

  //! The total number of elements currently supported without allocation
  size_t m_capacity;

 public:
  /*!
   * Construct a vector with a pointer to the allocator.
   *
   * The vector does not take ownership of the allocator.
   *
   * Count and capacity are set to zero
   *
   * @param alloc  the allocator provided
   */
  explicit vector(Alloc* alloc)
      : m_alloc(alloc), m_buf(nullptr), m_count(0U), m_capacity(0U) {}

  ~vector() {
    if (m_buf) {
      // First call destructors of all elements in the vector
      for (size_t i = 0U; i < m_count; i++) {
        // Do it in reverse order
        m_buf[m_count - 1U - i].~T();
      }

      // Then de-allocate the entire buf
      m_alloc->deallocate(m_buf);
      m_buf = nullptr;
    }
  }

  // Copy constructors are deleted, but you may use copy() to make
  // exception free copies
  vector<T, Alloc>(vector<T, Alloc> const&) = delete;
  vector<T, Alloc>& operator=(vector<T, Alloc> const&) = delete;

  vector<T, Alloc>(vector<T, Alloc>&& o) noexcept
      : m_alloc(rtl::exchange(o.m_alloc, nullptr)),
        m_buf(rtl::exchange(o.m_buf, nullptr)),
        m_count(rtl::exchange(o.m_count, 0)),
        m_capacity(rtl::exchange(o.m_capacity, 0)) {}

  vector<T, Alloc>& operator=(vector<T, Alloc>&& o) noexcept {
    if (this != &o) {
      if (m_buf) {
        for (size_t i = 0U; i < m_count; i++) {
          // Do it in reverse order
          m_buf[m_count - 1U - i].~T();
        }

        m_alloc->deallocate(m_buf);
        m_buf = nullptr;
      }

      m_alloc = rtl::exchange(o.m_alloc, nullptr);
      m_buf = rtl::exchange(o.m_buf, nullptr);
      m_count = rtl::exchange(o.m_count, 0);
      m_capacity = rtl::exchange(o.m_capacity, 0);
    }

    return *this;
  }

  //! Provides a element for element comparison of vector contents
  bool operator==(const vector& rhs) const {
    if (m_count != rhs.m_count) {
      return false;
    }

    for (size_t i = 0U; i < m_count; i++) {
      if (m_buf[i] != rhs.m_buf[i]) {
        return false;
      }
    }

    return true;
  }

  bool operator!=(const vector& rhs) const {
    bool rhs_eq_lhs = rhs == *this;
    return !rhs_eq_lhs;
  }

  /*!
   * Returns a reference to the element at the specified index.
   *
   * Behavior is undefined if index is out of bounds of vector
   */
  T& operator[](size_t index) { return m_buf[index]; }
  T const& operator[](size_t index) const { return m_buf[index]; }

  /*!
   * The out_var pointer is modified to point to a valid element in the
   * vector at index if this function returns true.  If this function
   * returns false, then out_var is not modified.
   *
   * This function will return false when index is out of bounds of the
   * vector or out_var is not defined.
   *
   * @param out_var the out variable pointer to modify
   * @param index the index to search in the vector at
   * @return true if out_var is valid to use, otherwise false
   */
  bool at(T** out_var, size_t index) {
    if (out_var == nullptr) {
      return false;
    }
    if (index < 0U || index >= m_count) {
      return false;
    }
    *out_var = &m_buf[index];
    return true;
  }

  /*!
   * Attempts to reserve new_capacity bytes in the vector.
   *
   * If this function returns true, then the function has been
   * expanded in capacity, otherwise it returns false.
   *
   * If new_capacity is less than or equal to current capacity,
   * this function returns true.
   *
   * This function may invalidate previously held pointers/references
   * to elements in the vector if it returns true.
   *
   * If this function returns false, no changes will be made to the vector
   * and it will be in a valid state to use.
   *
   * @param new_capacity the new capacity requested
   * @return  true if successful, otherwise false
   */
  bool reserve(size_t new_capacity) {
    if (new_capacity <= m_capacity) {
      return true;
    }

    T* new_buf = static_cast<T*>(m_alloc->allocate(new_capacity * sizeof(T)));

    if (new_buf == nullptr) {
      return false;
    }

    for (size_t i = 0U; i < m_count; i++) {
      size_t idx = m_count - 1U - i;
      new (static_cast<void*>(new_buf + idx)) T(std::move(m_buf[idx]));
      m_buf[idx].~T();
    }

    m_capacity = new_capacity;
    m_alloc->deallocate(static_cast<void*>(m_buf));
    m_buf = new_buf;

    return true;
  }

  /*!
   * Clears out any elements in the vector, but doesn't perform any memory
   * operations regarding resizing.
   */
  void clear() {
    if (m_buf) {
      for (size_t i = 0U; i < m_count; i++) {
        // Do it in reverse order
        m_buf[m_count - 1U - i].~T();
      }
    }
    m_count = 0U;
  }

  /*!
   * Push an element to the "back" of the vector via copy ctor
   *
   * If this function returns true, it was successfully added,
   * otherwise this function will return false and the vector will not be
   * touched.
   *
   * @param value the value to be copy constructed
   * @return true if added, otherwise false
   */
  bool push_back(T const& value) {
    if (m_count == m_capacity) {
      if (m_capacity == 0U) {
        if (reserve(1U) == false) {
          return false;
        }
      } else {
        if (reserve(2U * m_capacity) == false) {
          return false;
        }
      }
    }

    new (m_buf + m_count) T(value);
    m_count++;

    return true;
  }

  /*!
   * Push an element to the "back" of the vector via move ctor
   *
   * If this function returns true, it was successfully added,
   * otherwise this function will return false and the vector will not be
   * touched.
   *
   * @param value the value to be move constructed
   * @return true if added, otherwise false
   */
  bool push_back(T&& value) {
    if (m_count == m_capacity) {
      if (m_capacity == 0U) {
        if (reserve(1U) == false) {
          return false;
        }
      } else {
        if (reserve(2U * m_capacity) == false) {
          return false;
        }
      }
    }

    new (m_buf + m_count) T(std::move(value));
    m_count++;

    return true;
  }

  /*!
   * Calls the last elements destructor and removes it from the vector
   *
   * This function will not do anything if there are no elements in the
   * vector.
   */
  void pop_back() {
    if (m_count == 0U) {
      return;
    }
    m_buf[m_count - 1U].~T();
    m_count--;
  }

  /*!
   * Calls the destructor and removes the element at index from the vector.
   *
   * This function is faster than remove_stable but will modify the order
   * that the elements in the vector are in.
   *
   * ***IMPORTANT***
   * References/pointers to elements in the vector may be invalidated
   * after this call.
   *
   * This function will do nothing if index is not within the bounds of the
   * vector.
   *
   * @param index the index element to remove
   */
  void remove_fast(size_t index) {
    if (index < 0U || index >= m_count) {
      return;
    }

    using std::swap;

    swap(m_buf[index], back());
    pop_back();
  }

  /*!
   * Calls the destructor and removes the element at index from the vector.
   *
   * This function is slower than remove_fast but will maintain the ordering
   * of elements in the vector.
   *
   * ***IMPORTANT***
   * References/pointers to elements in the vector may be invalidated
   * after this call.
   *
   * This function will do nothing if index is not within the bounds of the
   * vector.
   *
   * @param index
   */
  void remove_stable(size_t index) {
    if (index < 0U || index >= m_count) {
      return;
    }

    if (index == m_count - 1U) {
      pop_back();
      return;
    }

    using std::swap;

    for (size_t current = index; current < m_count - 1U; current++) {
      swap(m_buf[current], m_buf[current + 1U]);
    }

    pop_back();
  }

  /*!
   * Will attempt to copy elements from vector o to the current vector
   *
   * If this function returns true the copy was successful, otherwise
   * this function will return false and the vector will be untouched.
   *
   * This copy function may be used with vectors that have different
   * allocator types
   *
   * @tparam U any other allocator type
   * @param o the vector to copy from
   * @return true if the copy was successful, otherwise false
   */
  template <typename U>
  bool copy(vector<T, U> const& o) {
    vector<T, Alloc> rval(m_alloc);
    size_t o_cap = o.capacity();
    size_t o_size = o.size();
    const T* o_buf = o.get_buf();

    if (rval.reserve(o_cap) == false) {
      return false;
    }

    for (size_t i = 0U; i < o_size; i++) {
      if (rval.push_back(o_buf[i]) == false) {
        return false;
      }
    }

    *this = std::move(rval);
    return true;
  }

  /*!
   * Returns a front reference.
   *
   * Undefined behavior if the vector is empty
   */
  T& front() { return *m_buf; }
  T const& front() const { return *m_buf; }

  /*!
   * Returns a back reference.
   *
   * Undefined behavior if the vector is empty
   */
  T& back() { return *(m_buf + m_count - 1U); }
  T const& back() const { return *(m_buf + m_count - 1U); }

  //! Returns the number of elements in the vector
  size_t size() const noexcept { return m_count; }

  //! Retuns true if the vector is empty, otherwise false
  bool empty() const noexcept { return m_count == 0U; }

  //! Returns the capacity of the vector
  size_t capacity() const noexcept { return m_capacity; }

  //! Returns a raw pointer to the internal buffer
  T* get_buf() { return m_buf; }
  const T* get_buf() const { return m_buf; }
};

}  // namespace rtl

#endif  // RTLCPP_VECTOR_HPP
