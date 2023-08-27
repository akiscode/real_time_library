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

#ifndef RTLCPP_LRU_HPP
#define RTLCPP_LRU_HPP

#include <utility>

#include "rtlcpp/map.hpp"
#include "rtlcpp/object_pool.hpp"

namespace rtl {

/*!
 * An allocator aware least recently used cache.
 *
 * @tparam Key the key to reference cache entries by
 * @tparam T the type that keys reference in the cache
 * @tparam Alloc the provided allocator
 */
template <typename Key, typename T, typename Alloc = rtl::RTDefaultAllocator>
class lru {
 private:
  /*
   * Although RTL has a double-linked list class, but we aren't going to
   * use it.  LRU caches tend to be used in high performance applications, so
   * we want to make a new "internal" data structure that can be used
   * with the object pool class so that we don't needlessly allocate
   * and deallocate with rtl::list.
   *
   */
  struct LRUNode {
   private:
    Key key;
    T val;
    LRUNode* prev;
    LRUNode* next;

   public:
    Key& key_ref() { return key; }
    T& val_ref() { return val; }
    LRUNode*& prev_ref() { return prev; }
    LRUNode*& next_ref() { return next; }

    LRUNode() : key(), val(), prev(nullptr), next(nullptr) {}

    LRUNode(Key const& key_, T&& val_)
        : key(key_), val(std::move(val_)), prev(nullptr), next(nullptr) {}
    LRUNode(Key const& key_, T const& val_)
        : key(key_), val(val_), prev(nullptr), next(nullptr) {}

    ~LRUNode() {
      prev = nullptr;
      next = nullptr;
    }

    LRUNode(LRUNode const&) = delete;
    LRUNode& operator=(LRUNode const&) = delete;

    LRUNode(LRUNode&& o) noexcept
        : key(std::move(o.key)),
          val(std::move(o.val)),
          prev(rtl::exchange(o.prev, nullptr)),
          next(rtl::exchange(o.next, nullptr)) {}
    LRUNode& operator=(LRUNode&& o) noexcept {
      if (this != &o) {
        key = std::move(o.key);
        val = std::move(o.val);
        prev = rtl::exchange(o.prev, nullptr);
        next = rtl::exchange(o.next, nullptr);
      }
      return *this;
    }
  };

  LRUNode* m_head;
  LRUNode* m_tail;
  rtl::unordered_map<Key, LRUNode*, Alloc> m_map;
  rtl::object_pool<LRUNode, Alloc> m_pool;
  size_t m_capacity;
  size_t m_size;

 public:
  /*!
   * Construct a LRU cache with a given allocator and a certain
   * number of elements to keep in the cache
   *
   * @param alloc the provided allocator
   * @param capacity the number of elements to put in the LRU cache
   */
  lru(Alloc* alloc, size_t capacity)
      : m_head(nullptr),
        m_tail(nullptr),
        m_map(alloc),
        m_pool(alloc, capacity),
        m_capacity(capacity),
        m_size(0U) {
    if (m_map.reserve(m_map.approx_buckets_needed(capacity))) {
      m_map.lock_table_size();
    }
  }

  ~lru() {
    while (m_head) {
      pop_back();
    }
  }

  lru(lru const&) = delete;
  lru& operator=(lru const&) = delete;

  lru(lru&& o) noexcept
      : m_head(rtl::exchange(o.m_head, nullptr)),
        m_tail(rtl::exchange(o.m_tail, nullptr)),
        m_map(std::move(o.m_map)),
        m_pool(std::move(o.m_pool)),
        m_capacity(rtl::exchange(o.m_capacity, 0)),
        m_size(rtl::exchange(o.m_size, 0)) {}

  lru& operator=(lru&& o) noexcept {
    if (this != &o) {
      while (m_head) {
        pop_back();
      }

      m_head = rtl::exchange(o.m_head, nullptr);
      m_tail = rtl::exchange(o.m_tail, nullptr);
      m_map = std::move(o.m_map);
      m_pool = std::move(o.m_pool);
      m_capacity = rtl::exchange(o.m_capacity, 0);
      m_size = rtl::exchange(o.m_size, 0);
    }

    return *this;
  }

  void reset() {
    while (m_head) {
      pop_back();
    }
    m_map.delete_all_keys();
    m_size = 0U;
  }

  //! Returns the total capacity of the cache
  size_t capacity() const { return m_capacity; }

  //! Returns the number of items in the cache
  size_t size() const { return m_size; }

  //! Returns true if the cache is empty, otherwise false
  bool empty() const { return m_size == 0U; }

  //! Returns true if the key is in the cache, does not update usage position
  bool contains(Key const& key) { return m_map.contains(key); }

  /*!
   * If the cache contains an entry from the provided key, this function
   * will return true and copy the value into the provided out_var variable.
   *
   * If this cache does not contain the value referenced by key, then this
   * function returns false and the cache is not affected.
   *
   * If this function returns true, the referenced nodes usage position is
   * updated to be the most recently used node.  If this function returns
   * false, then the usage position is not affected.
   *
   * @param key the key to reference
   * @param out_var the out variable to copy to if this function returns true
   * @return true if found and processed, otherwise false
   */
  bool get(Key const& key, T* out_var) {
    LRUNode** map_node = m_map.get(key);
    bool valid_map_node = map_node;
    if (!valid_map_node) {
      return false;
    }

    LRUNode* node = *map_node;

    // "Remove" the list from the node structure
    take_node(node);

    // We copy the value here since values may be evicted
    // without specifically calling a del() function
    *out_var = node->val_ref();

    push_front(node);

    return true;
  }

  /*!
   * If the cache contains an entry from the provided key, this function
   * will return a pointer to that variable
   *
   * If this cache does not contain the value referenced by key, then this
   * function returns nullptr
   *
   * If this function returns non-nullptr, the referenced nodes usage position
   * is updated to be the most recently used node.  If this function returns
   * nullptr, then the usage position is not affected.
   *
   * ***IMPORTANT***
   * It is important that all other calls to this LRU cache not be made until
   * you are done with the pointer.  DO NOT STORE THE POINTER!  It may be
   * invalidated by any other function call on the LRU cache instance.
   *
   * @param key the key to reference
   * @return pointer to value, otherwise nullptr
   *
   */
  T* get_ptr(Key const& key) {
    LRUNode** map_node = m_map.get(key);
    bool valid_map_node = map_node;
    if (!valid_map_node) {
      return nullptr;
    }

    LRUNode* node = *map_node;

    // "Remove" the list from the node structure
    take_node(node);

    push_front(node);

    return &node->val_ref();
  }

  /*!
   * Puts the value of val into the cache to be referenced by key
   *
   * If this function returns true, then the value was successfully
   * inserted into the cache and it is the most recently used value in
   * the cache.
   *
   * If this function returns false, then the value could not be inserted
   * into the cache.
   *
   * If the cache is at capacity, the least recently used value will be
   * evicted from the cache.
   *
   * @param key the key to tie the value to in the cache
   * @param val the value to insert into the cache
   * @return true if inserted, otherwise false
   */
  template <typename U>
  bool put(Key const& key, U&& val) {
    // NOTE: because of the typename U, this is a universal reference
    LRUNode** map_node = m_map.get(key);

    if (map_node) {
      LRUNode* node = *map_node;
      // "Remove" the list from the node structure
      // If this returns false, we are still in a valid state
      take_node(node);

      // Already present in the cache, perfect forward it in
      node->key_ref() = key;
      node->val_ref() = std::forward<U>(val);
      push_front(node);
      return true;
    }

    if (m_size == m_capacity) {
      Key key_to_remove = back()->key_ref();
      m_map.del(key_to_remove);
      pop_back();
    }

    LRUNode* new_node = m_pool.get(key, std::forward<U>(val));
    bool put_successful = m_map.put(key, new_node);
    if (!put_successful) {
      m_pool.put(new_node);
      return false;
    }

    push_front(new_node);

    return true;
  }

 private:
  LRUNode* back() { return m_tail; }

  void pop_back() {
    switch (m_size) {
      case 0U: {
        return;
      } break;
      case 1U: {
        LRUNode* node = m_head;

        m_head = nullptr;
        m_tail = nullptr;

        m_pool.put(node);

      } break;
      default: {
        LRUNode* node = m_tail;
        m_tail = m_tail->prev_ref();
        m_tail->next_ref() = nullptr;

        m_pool.put(node);

      } break;
    }

    --m_size;
  }

  void push_front(LRUNode* node) {
    assert(node->next_ref() == nullptr);
    assert(node->prev_ref() == nullptr);

    switch (m_size) {
      case 0U: {
        m_head = node;
        m_tail = node;
        node->prev_ref() = nullptr;
        node->next_ref() = nullptr;
      } break;
      default: {
        node->next_ref() = m_head;
        node->prev_ref() = nullptr;

        m_head->prev_ref() = node;
        m_head = node;

      } break;
    }

    ++m_size;
  }

  void take_node(LRUNode* node) {
    assert(node);
    bool valid_node = node;
    if (!valid_node) {
      return;
    }

    LRUNode* prev = node->prev_ref();
    LRUNode* next = node->next_ref();
    assert(m_size != 0);

    switch (m_size) {
      case 0U: {
        // Should never happen
        return;
      } break;
      case 1U: {
        node->prev_ref() = nullptr;
        node->next_ref() = nullptr;
        m_head = nullptr;
        m_tail = nullptr;
      } break;
      default: {
        // At least 2 nodes

        node->prev_ref() = nullptr;
        node->next_ref() = nullptr;

        if (node == m_head) {
          m_head = next;
        } else if (node == m_tail) {
          m_tail = prev;
        } else {
          prev->next_ref() = next;
          next->prev_ref() = prev;
        }
      } break;
    }

    --m_size;
  }
};

}  // namespace rtl

#endif  // RTLCPP_LRU_HPP
