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

#ifndef RTLCPP_OBJECT_POOL_HPP
#define RTLCPP_OBJECT_POOL_HPP

#include "rtlcpp/vector.hpp"

namespace rtl {

/*!
 * An object pool that can provide objects of type T.
 *
 * This object pool works with the the provided allocator to re-use
 * objects that have been passed back to it.
 *
 * If the object pool is empty and get() is called, it will allocate new object
 * plus some extra defined as "elasticity".  An elasticity of X will mean that
 * when the pool is empty and get() is called, the pool will attempt to
 * allocate X objects instead of just 1.
 *
 * This class is not thread-safe.
 *
 * @tparam T the type to store in the allocator
 * @tparam Alloc  the allocator provided
 */
template <typename T, typename Alloc = RTDefaultAllocator>
class object_pool {
 private:
  rtl::vector<T*, Alloc> m_vector;
  Alloc* m_alloc;
  size_t m_elasticity;

 public:
  /*!
   * Obj is an object that contains a pointer to the originating object pool as
   * well as a pointer to object of type T.
   *
   * When the destructor of this object is called, it automatically puts the
   * object of type T back into the pool.
   */
  struct Obj {
   private:
    object_pool<T, Alloc>* pool;
    // You can modify the data, but you can't modify the pointer
    T* const data;

   public:
    T* get_data() const { return data; }

    ~Obj() {
      if (data) {
        pool->put(data);
      }
    }

    Obj(Obj const&) = default;
    Obj& operator=(Obj const&) = default;

    Obj(Obj&&) noexcept = default;
    Obj& operator=(Obj&&) noexcept = default;

   private:
    Obj(object_pool<T, Alloc>* pool_, T* data_) : pool(pool_), data(data_) {}
    friend class object_pool<T, Alloc>;
  };

  /*!
   * Create an object pool with the given parameters.
   *
   * This constructor will attempt to create num_objects in the initial
   * pool.
   *
   * Elasticity can never be set to less than 1.
   *
   * This class does not take ownership of the allocator
   *
   * @param alloc the allocator provided
   * @param num_objects the number of objects to initially put into the pool
   * @param elasticity the elasticity of the pool
   */
  object_pool(Alloc* alloc, size_t num_objects, size_t elasticity = 1U)
      : m_vector(alloc), m_alloc(alloc), m_elasticity(elasticity) {
    if (m_elasticity < 1U) {
      m_elasticity = 1U;
    }
    (void)add_to_pool(num_objects);
  }

  /*!
   *  Will NOT call destructors of objects in the pool since they are
   *  called with the put() function
   */
  ~object_pool() {
    for (size_t i = 0U; i < m_vector.size(); i++) {
      T* t = m_vector[i];
      m_alloc->deallocate(t);
    }
  }

  object_pool(object_pool const&) = delete;
  object_pool& operator=(object_pool const&) = delete;

  object_pool(object_pool&& o) noexcept
      : m_vector(std::move(o.m_vector)),
        m_alloc(rtl::exchange(o.m_alloc, nullptr)),
        m_elasticity(o.m_elasticity) {}

  object_pool& operator=(object_pool&& o) noexcept {
    if (this != &o) {
      m_vector = std::move(o.m_vector);
      m_alloc = rtl::exchange(o.m_alloc, nullptr);
      m_elasticity = o.m_elasticity;
    }
    return *this;
  }

  /*!
   * Adds num_objects objects to the object pool by pre-allocating
   * memory.  However, it does not construct the objects.
   *
   * Returns the number of objects actually allocated which may be less
   * than the provided num_objects due to memory constraints
   *
   * @param num_objects the number of objects (memory wise) to add to the pool
   * @return the number of objects pre-allocated in the pool
   */
  size_t add_to_pool(size_t num_objects) {
    m_vector.reserve(m_vector.size() + num_objects);

    for (size_t i = 0U; i < num_objects; i++) {
      T* tmp = static_cast<T*>(m_alloc->allocate(sizeof(T)));

      // If i == 0 and we failed we reserved 0
      // If i == 1 and we failed we reserved 1
      // If i == 2 and we failed we reserved 2
      if (tmp == nullptr) {
        return i;
      }

      if (m_vector.push_back(tmp) == false) {
        return i;
      }
    }

    return num_objects;
  }

  /*!
   * Gets a pointer to an object from the object pool.
   *
   * Arguments passed into this function via args will be perfect
   * forwarded to the constructor of T.
   *
   * If the pool is empty, then the pool will be refilled by the
   * elasticity parameter amount.
   *
   * If no object can be gotten or allocated, nullptr is returned.
   *
   * ***IMPORTANT***
   *
   * Pointers received from this function must be returned via the put()
   * call otherwise memory will leak.
   *
   * @param args the arguments to perfect forward to the constructor of T
   * @return a pointer to an object from the pool or nullptr on failure
   */
  template <class... Args>
  T* get(Args&&... args) {
    if (m_vector.empty()) {
      if (add_to_pool(m_elasticity) < 1) {
        return nullptr;
      }
    }

    T* rval = m_vector.back();
    m_vector.pop_back();

    new (rval) T(std::forward<Args>(args)...);

    return rval;
  }

  /*!
   * Returns an Obj class that contains a pointer to an object T.
   *
   * When Obj goes out of scope and its destructor is called, it
   * automatically calls put() on the pointer it contains.
   *
   * The pointer in Obj may contain nullptr if memory could not be allocated
   * in the underlying get() call.
   *
   * args is perfect forwarded to the T ctor similar to the get() call
   *
   * @param args the arguments to perfect forward to the constructor of T
   * @return A Obj that can contain the pointer to the data
   */
  template <class... Args>
  Obj get_auto(Args&&... args) {
    Obj rval(this, get(std::forward<Args>(args)...));

    return rval;
  }

  /*!
   * Given a pointer to type T that was gotten from get(), reinsert it back
   * into the pool.
   *
   * This function will call ~T on the pointer provided before adding it
   * back into the pool.
   *
   * If nullptr is provided, nothing happens.
   *
   * ***IMPORTANT***
   * This function should only be provided with pointers to objects
   * that have been received via get()
   *
   * @param in the pointer to put back into the pool
   */
  void put(T* in) {
    if (in == nullptr) {
      return;
    }
    in->~T();
    m_vector.push_back(in);
  }

  //! The number of elements in the pool
  size_t size() const noexcept { return m_vector.size(); }

  //! Returns true if the pool is empty, otherwise false
  bool empty() const noexcept { return m_vector.empty(); }

  //! Returns the elasticity parameter
  size_t elasticity() const noexcept { return m_elasticity; }

  //! Sets the elasticity parameter
  void set_elasticity(size_t elasticity) noexcept { m_elasticity = elasticity; }
};

}  // namespace rtl

#endif  // RTLCPP_OBJECT_POOL_HPP
