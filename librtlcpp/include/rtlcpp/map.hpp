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

#ifndef RTLCPP_MAP_HPP
#define RTLCPP_MAP_HPP

#include <cassert>

#include "rtlcpp/hash.hpp"
#include "rtlcpp/vector.hpp"

namespace rtl {

template <typename Key, typename T, typename Alloc = rtl::RTDefaultAllocator>
class unordered_map {
 private:
  struct Entry {
   private:
    Key m_key;
    T* m_val;

   public:
    Key& key_val() { return m_key; }
    T*& val_val() { return m_val; }
    T* const& val_val() const { return m_val; }

    explicit Entry(Key const& key) : m_key(key), m_val(nullptr) {}

    ~Entry() { assert(m_val == nullptr); }

    Entry(Entry const&) = delete;
    Entry& operator=(Entry const&) = delete;

    Entry(Entry&& o) noexcept
        : m_key(std::move(o.m_key)), m_val(rtl::exchange(o.m_val, nullptr)) {}

    Entry& operator=(Entry&& o) noexcept {
      if (this != &o) {
        m_key = std::move(o.m_key);
        m_val = rtl::exchange(o.m_val, nullptr);
      }
      return *this;
    }

    bool construct(Alloc& a, T&& val) {
      if (m_val) {
        *m_val = std::move(val);
        return true;
      }

      T* tmp = static_cast<T*>(a.allocate(sizeof(T)));
      bool valid_tmp = tmp;
      if (!valid_tmp) {
        return false;
      }

      new (tmp) T(std::move(val));

      m_val = tmp;

      return true;
    }

    bool construct(Alloc& a, T const& val) {
      if (m_val) {
        *m_val = val;
        return true;
      }

      T* tmp = static_cast<T*>(a.allocate(sizeof(T)));
      bool valid_tmp = tmp;
      if (!valid_tmp) {
        return false;
      }

      new (tmp) T(val);

      m_val = tmp;

      return true;
    }

    void deconstruct(Alloc& a) {
      bool valid_val = m_val;
      if (!valid_val) {
        return;
      }
      m_val->~T();
      a.deallocate(static_cast<void*>(m_val));
      m_val = nullptr;
    }

    bool operator==(const Entry& rhs) const { return m_key == rhs.m_key; }
    bool operator!=(const Entry& rhs) const {
      bool rhs_eq_this = rhs == *this;
      return !rhs_eq_this;
    }
  };

  struct BucketGetOrCreateResult {
    int created_count;
    Entry* entry;
  };

  struct Bucket {
   private:
    rtl::vector<Entry, Alloc> m_entries;

   public:

    rtl::vector<Entry, Alloc>& entries() { return m_entries; }

    Bucket(Alloc* alloc) : m_entries(alloc) {}
    ~Bucket() = default;

    Bucket(Bucket const&) = delete;
    Bucket& operator=(Bucket const&) = delete;

    Bucket(Bucket&&) noexcept = default;
    Bucket& operator=(Bucket&&) noexcept = default;

    T* get(Key const& key) const {
      Entry e(key);

      const Entry* entry{nullptr};
      for (size_t i = 0U; i < m_entries.size(); i++) {
        if (m_entries[i] == e) {
          entry = &m_entries[i];
          break;
        }
      }

      return (entry) ? entry->val_val() : nullptr;
    }

    bool empty() const { return m_entries.empty(); }

    bool bucket_remove(Entry* e, Alloc& a) {
      bool valid_entry = e;
      if (!valid_entry) {
        return false;
      }

      Entry* entry{nullptr};
      size_t entry_idx{0U};
      for (size_t i = 0U; i < m_entries.size(); i++) {
        if (m_entries[i] == *e) {
          entry = &m_entries[i];
          entry_idx = i;
          break;
        }
      }

      valid_entry = entry;
      if (!valid_entry) {
        return false;
      }

      entry->deconstruct(a);
      m_entries.remove_fast(entry_idx);
      return true;
    }

    Entry* bucket_get_entry(Key const& key) {
      Entry e{key};

      Entry* entry{nullptr};
      for (size_t i = 0U; i < m_entries.size(); i++) {
        if (m_entries[i] == e) {
          entry = &m_entries[i];
          break;
        }
      }

      return entry;
    }

    // Returns number of nodes created (-1 on error) and inserted node
    BucketGetOrCreateResult bucket_get_or_create_entry(Key const& key) {
      Entry e{key};

      Entry* entry{nullptr};
      for (size_t i = 0U; i < m_entries.size(); i++) {
        if (m_entries[i] == e) {
          entry = &m_entries[i];
          break;
        }
      }

      if (entry) {
        return {0, entry};
      }

      // Create a node and insert
      bool successful_push_back = m_entries.push_back(std::move(e));
      if (!successful_push_back) {
        return {-1, nullptr};
      }

      return {1, &m_entries.back()};
    }

    void deconstruct_all_entries(Alloc& a) {
      for (size_t i = 0U; i < m_entries.size(); i++) {
        m_entries[i].deconstruct(a);
      }
    }

    void delete_all_entries(Alloc& a) {
      deconstruct_all_entries(a);
      m_entries.clear();
    }
  };

  struct TableGetOrCreateResult {
    Entry* entry;
    Bucket* bucket;
    bool created;
  };

  struct Table {
   private:
    Alloc* m_alloc;
    rtl::vector<Bucket, Alloc> m_buckets;
    size_t m_total_entries;
    size_t m_num_buckets;
    uint_fast8_t m_power_of_2_size;
    rtl::hash<Key> m_hasher;

   public:
    Table(Alloc* alloc, uint_fast8_t initial_power_of_2 = 4U)
        : m_alloc(alloc),
          m_buckets(alloc),
          m_total_entries(0U),
          m_num_buckets(0U),
          m_power_of_2_size(initial_power_of_2),
          m_hasher() {
      if (m_power_of_2_size >= 32U) {
        m_power_of_2_size = 31U;
      }
      // Let's make it start ~16 buckets like other implementations
      if (m_power_of_2_size < 4U) {
        m_power_of_2_size = 4U;
      }
      m_num_buckets = expand(rtl::get_prime_power_of_2(m_power_of_2_size));
    }

    ~Table() = default;

    Table(Table const&) = delete;
    Table& operator=(Table const&) = delete;

    Table(Table&&) noexcept = default;
    Table& operator=(Table&&) noexcept = default;

    rtl::vector<Bucket, Alloc>& buckets() { return m_buckets; }
    uint_fast8_t get_power_of_2_size() const { return m_power_of_2_size; }
    size_t get_num_buckets() const { return m_num_buckets; }
    size_t const& get_total_entries() const { return m_total_entries; }
    size_t& get_total_entries() { return m_total_entries; }

    uint_fast8_t get_next_power_of_2() const {
      return static_cast<uint_fast8_t>(m_power_of_2_size + 1U);
    }

    void table_deconstruct_all_entries(Alloc& a) {
      for (size_t i = 0U; i < m_buckets.size(); i++) {
        m_buckets[i].deconstruct_all_entries(a);
      }
    }

    void table_delete_all_entries(Alloc& a) {
      for (size_t i = 0U; i < m_buckets.size(); i++) {
        m_buckets[i].delete_all_entries(a);
      }
      m_total_entries = 0U;
    }

    T* get(Key const& key) {
      if (m_num_buckets == 0U) {
        return nullptr;
      }

      uint32_t bucket_index = get_bucket_index(key);

      return m_buckets[bucket_index].get(key);
    }

    Entry* table_get_entry(Key const& key) {
      if (m_num_buckets == 0U) {
        return nullptr;
      }
      uint32_t bucket_index = get_bucket_index(key);
      return m_buckets[bucket_index].bucket_get_entry(key);
    }

    // Either gets the entry or creates one
    TableGetOrCreateResult table_get_or_create_entry(Key const& key) {
      if (m_num_buckets == 0U) {
        return {nullptr, nullptr, false};
      }

      uint32_t bucket_index = get_bucket_index(key);

      BucketGetOrCreateResult res;

      res = m_buckets[bucket_index].bucket_get_or_create_entry(key);

      TableGetOrCreateResult rval;
      rval.entry = res.entry;
      rval.bucket = &m_buckets[bucket_index];
      rval.created = false;

      if (res.created_count > 0) {
        ++m_total_entries;
        rval.created = true;
      }

      return rval;
    }

    bool del(Key const& key, Alloc& a) {
      if (m_num_buckets == 0U) {
        return false;
      }

      uint32_t bucket_index = get_bucket_index(key);

      // Make a temporary entry to compare keys
      Entry e(key);
      bool rval = m_buckets[bucket_index].bucket_remove(&e, a);
      if (rval) {
        --m_total_entries;
      }
      return rval;
    }

   private:
    size_t expand(size_t num) {
      // Try to add_to_pool num spaces ahead of time...
      m_buckets.reserve(num);

      for (size_t i = 0U; i < num; i++) {
        // If i == 0 and we failed we reserved 0
        // If i == 1 and we failed we reserved 1
        // If i == 2 and we failed we reserved 2
        bool successful_push_back = m_buckets.push_back({m_alloc});
        if (!successful_push_back) {
          return i;
        }
      }

      return num;
    }

    // Assumes that m_num_buckets > 0
    uint32_t get_bucket_index(Key const& key) {
      return m_hasher(key) % m_num_buckets;
    }
  };

 public:
  enum class MapState : int {
    //! The map is in an errored state
    ERROR = 0,

    //! The map is contained solely in the main table
    STABLE,

    //! The map is in the process of being transferred to the secondary table
    TRANSFER

  };

 private:
  Alloc* m_alloc;
  Table* m_main;
  Table* m_secondary;
  MapState m_state;
  size_t m_max_load_factor_percent;
  size_t m_current_bucket_to_transfer;
  bool m_locked;

 public:
  /*!
   * Construct an allocator aware unordered map with a specified
   * max_load_factor.  The default for the max load factor is 5.0.  Since
   * unordered map uses chaining instead of probing, it is advisable to
   * set max load factor greater than one.
   *
   * Additionally, this unordered map utilizes amortized resizing to make sure
   * that worst case performance is capped at the cost of slightly reduced
   * average performance.
   *
   * @param alloc the provided allocator
   * @param max_load_factor the maximum load factor of the map
   */
  unordered_map(Alloc* alloc, float max_load_factor = static_cast<float>(5.0))
      : m_alloc(alloc),
        m_main(nullptr),
        m_secondary(nullptr),
        m_state(MapState::ERROR),
        m_max_load_factor_percent(
            static_cast<size_t>(max_load_factor * static_cast<float>(100))),
        m_current_bucket_to_transfer(0U),
        m_locked(false) {
    m_main = static_cast<Table*>(m_alloc->allocate(sizeof(Table)));

    if (m_main) {
      new (m_main) Table(m_alloc);
    }

    // If the allocation failed here then this will reset to error
    compute_state();
  }

  /*!
   * De-allocate and destroy both tables
   */
  ~unordered_map() {
    // m_alloc is not owned by the map, so no need to destruct
    if (m_main) {
      m_main->table_deconstruct_all_entries(*m_alloc);
      m_main->~Table();
      m_alloc->deallocate(static_cast<void*>(m_main));
      m_main = nullptr;
    }

    if (m_secondary) {
      m_secondary->table_deconstruct_all_entries(*m_alloc);
      m_secondary->~Table();
      m_alloc->deallocate(static_cast<void*>(m_secondary));
      m_secondary = nullptr;
    }
  }

  unordered_map(unordered_map const&) = delete;
  unordered_map& operator=(unordered_map const&) = delete;

  unordered_map(unordered_map&& o) noexcept
      : m_alloc(rtl::exchange(o.m_alloc, nullptr)),
        m_main(rtl::exchange(o.m_main, nullptr)),
        m_secondary(rtl::exchange(o.m_secondary, nullptr)),
        m_state(rtl::exchange(o.m_state, MapState::ERROR)),
        m_max_load_factor_percent(o.m_max_load_factor_percent),
        m_current_bucket_to_transfer(
            rtl::exchange(o.m_current_bucket_to_transfer, 0)),
        m_locked(o.m_locked) {}

  unordered_map& operator=(unordered_map&& o) noexcept {
    if (this != &o) {
      if (m_main) {
        m_main->table_deconstruct_all_entries(*m_alloc);
        m_main->~Table();
        m_alloc->deallocate(static_cast<void*>(m_main));
        m_main = nullptr;
      }

      if (m_secondary) {
        m_secondary->table_deconstruct_all_entries(*m_alloc);
        m_secondary->~Table();
        m_alloc->deallocate(static_cast<void*>(m_secondary));
        m_secondary = nullptr;
      }

      m_alloc = rtl::exchange(o.m_alloc, nullptr);
      m_main = rtl::exchange(o.m_main, nullptr);
      m_secondary = rtl::exchange(o.m_secondary, nullptr);
      m_state = rtl::exchange(o.m_state, MapState::ERROR);
      m_max_load_factor_percent = o.m_max_load_factor_percent;
      m_current_bucket_to_transfer =
          rtl::exchange(o.m_current_bucket_to_transfer, 0);
      m_locked = o.m_locked;
    }

    return *this;
  }

  //! Stop table from resizing
  void lock_table_size() { m_locked = true; }

  //! Allow table to resize
  void unlock_table_size() { m_locked = false; }

  //! Returns current state
  MapState get_state() const { return m_state; }

  //! Returns true if able to finish resizing efforts
  bool finalize() {
    if (m_state == MapState::ERROR) {
      return false;
    }

    if (m_state == MapState::STABLE) {
      return true;
    }

    assert(m_state == MapState::TRANSFER);
    bool transfer_complete = is_transfer_complete();
    while (!transfer_complete) {
      bool valid_partial_transfer = perform_partial_transfer();
      if (!valid_partial_transfer) {
        return false;
      }

      transfer_complete = is_transfer_complete();
    }

    end_resize();
    return true;
  }

  /*!
   * Returns true if function is able to reserve the requested
   * number of buckets.  Otherwise returns false
   *
   * ***IMPORTANT***
   * Will trigger a complete resize, not amortized
   *
   * @param number_of_buckets the number of buckets to resize
   * @return true if successful, otherwise false
   */
  bool reserve(uint32_t number_of_buckets) {
    if (m_state == MapState::ERROR) {
      return false;
    }
    if (number_of_buckets == 0U) {
      return false;
    }

    // Some number higher than 32
    uint_fast8_t next_power_of_2{250U};

    for (uint_fast8_t i = 0U; i < 32U; i++) {
      if (get_prime_power_of_2(i) >= number_of_buckets) {
        next_power_of_2 = i;
        break;
      }
    }

    // We never set it
    if (next_power_of_2 == 250U) {
      return false;
    }

    if (m_state == MapState::TRANSFER) {
      bool transfer_complete = is_transfer_complete();
      while (!transfer_complete) {
        bool valid_partial_transfer = perform_partial_transfer();
        if (!valid_partial_transfer) {
          return false;
        }

        transfer_complete = is_transfer_complete();
      }

      end_resize();
    }

    if (next_power_of_2 <= m_main->get_power_of_2_size()) {
      return true;
    }

    bool valid_begin_resize = begin_resize(next_power_of_2);

    if (!valid_begin_resize) {
      m_state = MapState::ERROR;
      return false;
    }

    bool transfer_complete = is_transfer_complete();
    while (!transfer_complete) {
      bool valid_partial_transfer = perform_partial_transfer();
      if (!valid_partial_transfer) {
        return false;
      }

      transfer_complete = is_transfer_complete();
    }

    end_resize();

    return true;
  }

  //! Returns the current number of buckets in the map
  size_t get_num_buckets() const {
    switch (m_state) {
      case MapState::ERROR:
        return 0U;
        break;
      case MapState::STABLE:
        return m_main->get_num_buckets();
        break;
      case MapState::TRANSFER:
        return m_secondary->get_num_buckets();
        break;
      default:
        return 0U;
        break;
    }
  }

  //! Returns true if key is contained in the map
  bool contains(Key const& key) const {
    switch (m_state) {
      case MapState::ERROR:
        return false;
        break;
      case MapState::STABLE: {
        return m_main->get(key) != nullptr;
      } break;
      case MapState::TRANSFER: {
        T* rval = m_secondary->get(key);

        bool valid_rval = rval;
        if (!valid_rval) {
          // Is it in main?
          rval = m_main->get(key);
        }

        return rval != nullptr;

      } break;
      default:
        return false;
        break;
    }
  }

  //! Returns a pointer to type T if key is in the map. Otherwise nullptr
  T* get(Key const& key) {
    switch (m_state) {
      case MapState::ERROR:
        return nullptr;
        break;
      case MapState::STABLE: {
        T* rval = m_main->get(key);

        if (should_resize()) {
          // Could potentially put us in transfer mode

          unsigned int new_power_of_2 = m_main->get_next_power_of_2();
          bool valid_resize = begin_resize(new_power_of_2);
          if (!valid_resize) {
            m_state = MapState::ERROR;
            return nullptr;
          }
        }

        return rval;

      } break;
      case MapState::TRANSFER: {
        T* rval = m_secondary->get(key);
        bool valid_rval = rval;

        if (!valid_rval) {
          // Is it in main?
          rval = m_main->get(key);
        }

        bool successful_partial_transfer = perform_partial_transfer();
        if (!successful_partial_transfer) {
          m_state = MapState::ERROR;
          return nullptr;
        }

        if (is_transfer_complete()) {
          end_resize();
        }

        return rval;
      } break;
      default:
        return nullptr;
        break;
    }
  }

  //! Puts value val in the hashmap referenced by key.  Will override entries.
  template <typename U>
  bool put(Key const& key, U&& val) {
    // The U&& is a universal reference
    switch (m_state) {
      case MapState::ERROR:
        return false;
        break;
      case MapState::STABLE: {
        bool rval{false};

        do {
          TableGetOrCreateResult res;

          res = m_main->table_get_or_create_entry(key);

          bool valid_entry = res.entry;
          bool valid_bucket = res.bucket;
          if (valid_entry && valid_bucket) {
            bool valid_construct =
                res.entry->construct(*m_alloc, std::forward<U>(val));
            if (!valid_construct) {
              // We failed to create an instance of the value
              // due to memory constraints. Roll back everything
              // if we created it.
              // construct() won't touch the original value if this returns
              // false, so we only have to remove the created entry
              // if we actually made a new one

              if (res.created) {
                res.bucket->bucket_remove(res.entry, *m_alloc);
              }

              break;
            }

            rval = true;
          }

        } while (false);

        if (should_resize()) {
          // Could potentially put us in transfer mode
          unsigned int new_power_of_2 = m_main->get_next_power_of_2();
          bool valid_begin_resize = begin_resize(new_power_of_2);
          if (!valid_begin_resize) {
            m_state = MapState::ERROR;
            return false;
          }
        }

        return rval;
      } break;
      case MapState::TRANSFER: {
        bool rval{false};

        do {
          TableGetOrCreateResult res;
          res = m_secondary->table_get_or_create_entry(key);

          // We directly insert into the secondary table,
          // knowing that our partial transfer process won't override
          // any keys that already exist from main

          bool valid_entry = res.entry;
          bool valid_bucket = res.bucket;

          if (valid_entry && valid_bucket) {
            if (res.created) {
              // If we created this then we want to transfer the old
              // entry if there is one
              // The move will set the original to have a nullptr,
              // meaning we can deconstruct it without an issue.

              Entry* main_entry = m_main->table_get_entry(key);

              if (main_entry) {
                *res.entry = std::move(*main_entry);
              }
            }

            bool construct_successful =
                res.entry->construct(*m_alloc, std::forward<U>(val));
            if (!construct_successful) {
              // We failed to create an instance of the value
              // due to memory constraints. Roll back everything
              // if we created it.
              // construct() won't touch the original value if this returns
              // false, so we only have to remove the created entry
              // if we actually made a new one

              if (res.created) {
                // Un-seed the entry we set before
                res.entry->val_val() = nullptr;
                res.bucket->bucket_remove(res.entry, *m_alloc);
              }

              break;
            }

            rval = true;
          }

        } while (false);

        bool successful_partial_transfer = perform_partial_transfer();

        if (!successful_partial_transfer) {
          m_state = MapState::ERROR;
          return false;
        }

        if (is_transfer_complete()) {
          end_resize();
        }

        return rval;
      } break;
      default:
        return false;
        break;
    }
  }

  //! Deletes all keys held by the map
  void delete_all_keys() {
    switch (m_state) {
      case MapState::ERROR:
        return;  // Don't do anything
        break;
      case MapState::STABLE: {
        m_main->table_delete_all_entries(*m_alloc);
        return;
      } break;
      case MapState::TRANSFER: {
        m_main->table_delete_all_entries(*m_alloc);
        m_secondary->table_delete_all_entries(*m_alloc);
        (void)finalize();
        return;

      } break;
      default:
        break;
    }
  }

  //! Deletes value pointed to by key
  bool del(Key const& key) {
    switch (m_state) {
      case MapState::ERROR:
        return false;  // Don't do anything
        break;
      case MapState::STABLE: {
        bool rval = m_main->del(key, *m_alloc);

        if (should_resize()) {
          // Could potentially put us in transfer mode
          unsigned int new_power_of_2 = m_main->get_next_power_of_2();
          bool valid_resize = begin_resize(new_power_of_2);
          if (!valid_resize) {
            m_state = MapState::ERROR;
            return false;
          }
        }

        return rval;
      } break;
      case MapState::TRANSFER: {
        bool rval = m_main->del(key, *m_alloc);

        if (!rval) {
          // If we didn't delete in main, then delete in secondary
          rval = m_secondary->del(key, *m_alloc);
        }

        bool successful_transfer = perform_partial_transfer();
        if (!successful_transfer) {
          m_state = MapState::ERROR;
          return false;
        }
        if (is_transfer_complete()) {
          end_resize();
        }

        return rval;

      } break;
      default:
        return false;  // Don't do anything
        break;
    }

    return false;
  }

  /*!
   * Returns the number of buckets needed given the number of items one expects
   * and the current load factor.
   */
  uint32_t approx_buckets_needed(uint32_t expected_item_count) const {
    return ((expected_item_count * 100U) /
            static_cast<uint32_t>(m_max_load_factor_percent)) +
           1U;
  }

 private:
  /*
   * In order to facilitate real-time behavior, we amortize any resizing
   * over the various operations.  This along with our O(1) allocator gives
   * us bounded operations.  The rules while resizing are:
   *
   * - During lookup or delete operations, check both main and secondary
   tables
   * - Only insert into the secondary table
   * - Perform partial transfers at every opportunity
   *
   * - Once all transfers are complete, deallocate the previous table and
   reset
   *
   */

  bool should_resize() const {
    // This function should only be called in stable state
    assert(m_state == MapState::STABLE);
    assert(m_secondary == nullptr);

    // If we lock the table, we should never resize
    if (m_locked) {
      return false;
    }

    size_t main_buckets = m_main->get_num_buckets();
    size_t main_key_count = m_main->get_total_entries();

    // loadfactor = key_count/num_buckets

    size_t max_key_count = (m_max_load_factor_percent * main_buckets) / 100U;

    return main_key_count >= max_key_count;
  }

  bool begin_resize(unsigned int new_power_of_2) {
    // This function should only be called in stable state
    assert(m_state == MapState::STABLE);
    assert(m_main != nullptr);
    assert(m_secondary == nullptr);

    m_secondary = static_cast<Table*>(m_alloc->allocate(sizeof(Table)));
    bool valid_secondary = m_secondary;
    if (!valid_secondary) {
      return false;
    }

    new (m_secondary) Table(m_alloc, static_cast<uint_fast8_t>(new_power_of_2));

    // Now we check that we actually expanded our table...if not this was for
    // nothing

    size_t expected_num_of_buckets = rtl::get_prime_power_of_2(new_power_of_2);

    if (expected_num_of_buckets != m_secondary->get_num_buckets()) {
      m_secondary->~Table();
      m_alloc->deallocate(m_secondary);
      m_secondary = nullptr;
      return false;
    }

    m_state = MapState::TRANSFER;
    m_current_bucket_to_transfer = 0U;

    return true;
  }

  bool perform_partial_transfer() {
    assert(m_state == MapState::TRANSFER);
    assert(m_main != nullptr);
    assert(m_secondary != nullptr);

    size_t number_of_keys_to_transfer{512U};

    size_t main_buckets = m_main->get_num_buckets();
    for (; m_current_bucket_to_transfer < main_buckets;
         m_current_bucket_to_transfer++) {
      Bucket& bucket = m_main->buckets()[m_current_bucket_to_transfer];

      bool bucket_empty = bucket.empty();
      while (!bucket_empty) {
        if (number_of_keys_to_transfer <= 0U) {
          return true;
        }
        --number_of_keys_to_transfer;

        /*
         * Gameplan:
         *
         * Find an entry from the original main bucket
         * Call get_or_create_entry for the secondary table
         * If the new table created a new entry, we can move the
         * entry from main over.  Otherwise, we just want to
         * remove the main entry since we don't want to overwrite
         * anything
         */

        Entry& e = bucket.entries().back();

        TableGetOrCreateResult res;

        res = m_secondary->table_get_or_create_entry(e.key_val());

        bool valid_entry = res.entry;
        bool valid_bucket = res.bucket;

        if (!valid_entry || !valid_bucket) {
          return false;
        }

        if (res.created) {
          // If we actually created a new entry, then we should move it in
          // Otherwise we should just discard the old entry

          *res.entry = std::move(e);
        }

        // If we didn't create a new entry, then this should be already moved
        assert(e.val_val() == nullptr);

        bucket.entries().pop_back();
        bucket_empty = bucket.empty();

        m_main->get_total_entries()--;
      }
    }

    return true;
  }

  void end_resize() {
    assert(m_state == MapState::TRANSFER);
    assert(m_main != nullptr);
    assert(m_secondary != nullptr);
    assert(is_transfer_complete());

    m_main->~Table();
    m_alloc->deallocate(static_cast<void*>(m_main));
    m_main = m_secondary;
    m_secondary = nullptr;
    m_state = MapState::STABLE;
    m_current_bucket_to_transfer = 0U;
  }

  bool is_transfer_complete() {
    assert(m_state == MapState::TRANSFER);
    assert(m_main != nullptr);
    assert(m_secondary != nullptr);

    return m_main->get_total_entries() == 0;
  }

  void compute_state() {
    if (m_main == nullptr) {
      m_state = MapState::ERROR;
      return;
    }

    // If the master is not nullptr, and the secondary is nullptr then we are
    // stable.
    // If the secondary is not nullptr, we are in the process of
    // transferring.

    m_state = (m_secondary == nullptr) ? MapState::STABLE : MapState::TRANSFER;
  }
};

}  // namespace rtl

#endif  // RTLCPP_MAP_HPP
