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

#ifndef RTLCPP_RING_BUFFER_HPP
#define RTLCPP_RING_BUFFER_HPP

#include <atomic>
#include <cstring>

#include "rtlcpp/allocator.hpp"
#include "rtlcpp/utility.hpp"

namespace rtl {

/*!
 * A single producer, single consumer (SPSC) ring buffer that provides the
 * ability to use only contiguous regions of memory in the ring buffer as
 * well as conventional read/write abilities.
 *
 * This class is lock-free and thread-safe as long as the SPSC properties are
 * maintained.
 *
 * It is up to the user of this class to ensure that the SPSC properties are
 * maintained.  Namely that:
 * 1) Only the writing thread may call write(),  write_bytes(), alloc_contig()
 *    and commit_write().
 *
 * 2) Only the reading thread may call read(), read_contig() and commit_read()
 *
 * This class is padded on 64 bit cache lines to avoid false sharing.
 *
 * This class does not own the provided buffer.  It is up to another class
 * to manage the lifetime of the buffer.
 *
 */
class spsc_ringbuffer final {
 private:
  bool m_initialized;
  // Pads and alignas are to prevent false-sharing
  unsigned char pad0[64];
  unsigned char* m_buf;
  uint32_t m_capacity;
  uint32_t m_writable_capacity;

  /*!
   * Owned by the reading process.
   *
   * Represents a "on the value" index.  Meaning that
   * reading X bytes starts on the index and then continues
   * for X-1 bytes
   *
   */
  alignas(64) std::atomic<uint32_t> m_read_index;

  /*!
   * Owned by the writing process.
   *
   *
   * Represents a "on the value" index.  Meaning that
   * writing X bytes starts on the index and then continues
   * for X-1 bytes
   *
   */
  alignas(64) std::atomic<uint32_t> m_write_index;
  unsigned char pad1[64U - sizeof(decltype(m_write_index))];

 public:
  // clang-format off
  /*!
   * Used as the result value for compound_alloc_contig().
   *
   * compound_alloc_contig() will give the maximum available space (at the
   * time of calling) to write to the ring buffer and return the values
   * in the following way:
   *
   * First Buf:
   *
   *    First Buf (and the corresponding size variable) represents the
   *    amount of space from the current write index to either the end
   *    of the buffer or the read index.
   *
   * Second Buf:
   *    Second Buf (and the corresponding size variable) represents the
   *    amount of space from the beginning of the buffer to either the
   *    end of the buffer or the read index.  If there is "no room" to
   *    write from the beginning of the index then this is zero.
   *
   *
   * Here is how the following scenarios will play out:
   *
   * Buffer is full: first buf size and second buf size is zero, pointers
   * are undefined
   *
   * There is no way to "wrap around" the buffer and the only space to write
   * is from the first buffer: first buf size is non-zero, but second buf size
   * is zero.  Only the first buf pointer is defined.
   *
   * There is space to "wrap around" the buffer: first buf size and second buf
   * size are non-zero (representing the amount of space to write from those
   * buffers.  Both pointers are defined.
   *
   *
   * @verbatim
   *                  Scenario: Buffer Full
   *  ┌───────────────────────────────────────────────┐ ╔════════════════════════╗
   *  │                     RBuf                      │ ║  First Buf Ptr: Undef  ║
   *  ├─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┤ ║   First Buf Size: 0    ║
   *  │  0  │  1  │  2  │  3  │  4  │  5  │  6  │  7  │ ║                        ║
   *  └──▲──┴─────┴─────┴─────┴─────┴─────┴─────┴──▲──┘ ║                        ║
   *     │                                         │    ║ Second Buf Ptr: Undef  ║
   *   Read                                     Write   ║   Second Buf Size: 0   ║
   *  Index                                     Index   ╚════════════════════════╝
   *
   *  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   *
   *                Scenario: No Wrap Around
   *  ┌───────────────────────────────────────────────┐ ╔════════════════════════╗
   *  │                     RBuf                      │ ║ First Buf Ptr: RBuf[5] ║
   *  ├─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┤ ║   First Buf Size: 2    ║
   *  │  0  │  1  │  2  │  3  │  4  │  5  │  6  │  7  │ ║                        ║
   *  └──▲──┴─────┴─────┴─────┴─────┴──▲──┴─────┴─────┘ ║                        ║
   *     │                             │                ║ Second Buf Ptr: Undef  ║
   *   Read                         Write               ║   Second Buf Size: 0   ║
   *  Index                         Index               ╚════════════════════════╝
   *
   *
   *  ┌───────────────────────────────────────────────┐ ╔════════════════════════╗
   *  │                     RBuf                      │ ║ First Buf Ptr: RBuf[1] ║
   *  ├─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┤ ║   First Buf Size: 3    ║
   *  │  0  │  1  │  2  │  3  │  4  │  5  │  6  │  7  │ ║                        ║
   *  └─────┴──▲──┴─────┴─────┴─────┴──▲──┴─────┴─────┘ ║                        ║
   *           │                       │                ║ Second Buf Ptr: Undef  ║
   *        Write                    Read               ║   Second Buf Size: 0   ║
   *        Index                   Index               ╚════════════════════════╝
   *
   *  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   *
   *                Scenario: Wrap Around Available
   *  ┌───────────────────────────────────────────────┐ ╔════════════════════════╗
   *  │                     RBuf                      │ ║ First Buf Ptr: RBuf[5] ║
   *  ├─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┤ ║   First Buf Size: 3    ║
   *  │  0  │  1  │  2  │  3  │  4  │  5  │  6  │  7  │ ║                        ║
   *  └─────┴─────┴─────┴──▲──┴─────┴──▲──┴─────┴─────┘ ║                        ║
   *                       │           │                ║Second Buf Ptr: RBuf[0] ║
   *                     Read       Write               ║   Second Buf Size: 2   ║
   *                    Index       Index               ╚════════════════════════╝
   *
   *
   * @endverbatim
   */
  // clang-format on
  struct alloc_result {
   private:
    friend class spsc_ringbuffer;

    alloc_result(unsigned char* first_buf_, uint32_t first_buf_sz_,
                 unsigned char* second_buf_, uint32_t second_buf_sz_,
                 bool write_ahead_of_read_)
        : first_buf(first_buf_),
          second_buf(second_buf_),
          first_buf_sz(first_buf_sz_),
          second_buf_sz(second_buf_sz_),
          write_ahead_of_read(write_ahead_of_read_) {}

   public:
    // This constructor is provided for looping, but this variable should be
    // considered valid until assigned from compound_alloc_contig()
    alloc_result() : alloc_result(nullptr, 0U, nullptr, 0U, false) {}

    unsigned char* first_buf;
    unsigned char* second_buf;
    uint32_t first_buf_sz;
    uint32_t second_buf_sz;
    //! True if write index >= read index
    bool write_ahead_of_read;
  };

  /*!
   * Construct a ring buffer with the given buffer and capacity.
   *
   * Note this ring buffer is a "one behind" ring buffer, meaning that
   * the actual writable capacity is capacity-1.  For instance,
   * if you provide a buffer of size 10 and provide 10 as the capacity,
   * then you will only be able to write 9 bytes total in the ring buffer.
   *
   * The provided buffer lifetime is not managed by the ring buffer class.
   *
   * @param buf the buffer for the ring buffer to use.
   * @param capacity the length of the buffer provided.  The ring buffer will
   *                 only be able to handle capacity-1 bytes.
   */
  spsc_ringbuffer(unsigned char* buf, uint32_t capacity);

  /*!
   * Constructs a ring buffer with a default constructor.
   *
   * *** IMPORTANT ***
   * Ring Buffer MUST have init() called before any other function is called
   */
  spsc_ringbuffer();

  //! Initializes the ring buffer (to be used after calling default ctor)
  void init(unsigned char* buf, uint32_t capacity);

  // Doesn't take ownership of buf, so no need to delete it
  ~spsc_ringbuffer() = default;

  // Can't have two ring buffers with one backing buffer, so delete copy ctors
  spsc_ringbuffer(spsc_ringbuffer const&) = delete;
  spsc_ringbuffer& operator=(spsc_ringbuffer const&) = delete;

  spsc_ringbuffer(spsc_ringbuffer&& o) noexcept;
  spsc_ringbuffer& operator=(spsc_ringbuffer&& o) noexcept;

  /*!
   * Returns the writable capacity (capacity - 1) of the ring buffer.
   * This is how many bytes the buffer can hold in total.
   */
  inline uint32_t writable_capacity() const { return m_writable_capacity; }

  /*!
   * Returns true if the ring buffer is empty
   */
  inline bool empty() const {
    return m_read_index.load(std::memory_order_acquire) ==
           m_write_index.load(std::memory_order_acquire);
  }

  /*!
   * Returns the approximate size of bytes written to the buffer.  The
   * function returns an approximate size because the buffer could be
   * read or written to in while this function is being called.
   *
   * If you call this function and the ring buffer is being written to,
   * it will be smaller than the actual size.
   *
   * If you call this function and the ring buffer is being read from,
   * it will be larger than the actual size.
   *
   */
  uint32_t approx_size() const;

  /*!
   * Returns the approximate number of bytes free in the buffer.  The
   * function returns an approximate size because the buffer could be
   * read or written to in while this function is being called.
   *
   * If you call this function and the ring buffer is being written to,
   * it will be larger than the actual number of free bytes.
   *
   * If you call this function and the ring buffer is being read from,
   * it will be smaller than the actual number of free bytes.
   *
   */
  inline uint32_t approx_free_bytes() const {
    return m_writable_capacity - approx_size();
  }

  /*!
   * Attempts to writes a certain number of bytes from input into the ring
   * buffer and returns the actual number of bytes written to the ring buffer.
   *
   * This may happen in a non-contiguous manner.
   *
   * If input is nullptr, then this returns 0.
   *
   * If sz is greater than the writable capacity, only the writable capacity
   * amount of bytes from input will be written.
   *
   * @param input the input buffer to write from
   * @param sz the amount of bytes to read from input to write into the buffer
   * @return the number of bytes from input actually written
   */
  uint32_t write_bytes(const unsigned char* input, uint32_t sz);

  /*!
   * Attempts to writes a certain number of bytes from input into the ring
   * buffer.
   *
   * This may happen in a non-contiguous manner.
   *
   * If the entire sz bytes can not be written into the ring buffer
   * then this function returns false and no bytes are written into the ring
   * buffer.
   *
   * If the entire sz bytes from input can be written into the ring buffer
   * then this function returns true.
   *
   * If input is nullptr, then this returns false.
   *
   * If sz is greater than the writable capacity, this function returns false
   *
   * If sz is zero, this function returns true
   *
   * @param input the input to read from
   * @param sz the number of bytes from input to write
   * @return true if all bytes were written.  Otherwise false.
   */
  bool write(const unsigned char* input, uint32_t sz);

  /*!
   *
   * Attempts to read a maximum of max_read bytes from the ring buffer into
   * the output buffer.  If the number of bytes to read from the ring buffer
   * are less than max_read then only that amount is read and put into output.
   *
   * The actual number of bytes read from the ring buffer is returned.
   *
   * If output is nullptr, 0 is returned.
   *
   * If max_read is greater than the writable capacity of the ring buffer then
   * only writable capacity bytes are read.
   *
   * This function assumes that output can handle up to max_read bytes.
   *
   * @param output the buffer to write the ring buffer bytes to
   * @param max_read the maximum number of bytes to read from the ring buffer
   * @return the number of bytes read from the ring buffer
   */
  uint32_t read(unsigned char* output, uint32_t max_read);

  /*!
   * If you need contiguous regions of memory from the ring buffer, this
   * function (along with commit_write()) will help you get it.
   *
   * This function will return a pointer that will allow you to write a
   * certain number of bytes to the ring buffer.  You must provide a pointer
   * to the requested number of bytes sz.
   *
   * When this function returns, it will modify the sz variable to indicate
   * how many bytes you may actually write to the returned pointer.  This may
   * be zero.
   *
   * If the modified sz variable is equal to the original value, then you may
   * ignore the end_of_buffer_ptr boolean and write sz bytes to the returned
   * pointer (make sure to call commit_write still, see below).
   *
   * However, if the sz variable is modified to be less than the original value,
   * the function differentiates between two different scenarios:
   *
   * 1) If the contiguous memory is bounded because you are reaching the start
   *    of already written memory (from a previous write) then end_of_buffer_ptr
   *    is set to false.  In this scenario, one could wait until the reading
   *    thread reads more to clear up more space.
   *
   * 2) If the contiguous memory is bounded because you are reaching the end
   *    of the buffer itself, then end_of_buffer_ptr is modified to be true.
   *    In this scenario, it makes no difference how long one waits, no more
   *    space will be available.  One could write a sentinel value to provide
   *    meaning at a higher application layer.
   *
   *
   * ****IMPORTANT****
   *
   * After calling this function, determining an action and writing values to
   * the returned pointer, it is important to call commit_write with the number
   * of bytes written to the pointer.  This will commit the write, otherwise
   * it will be as if nothing happened.
   *
   * Any call to write or write_bytes before calling commit_write will
   * invalidate the pointer returned by alloc_contig and alloc_contig must be
   * called again to get a new pointer.
   *
   * alloc_contig can be safely called multiple times before calling
   * commit_write.
   *
   * @param sz a pointer to the initial size, modified to be the number of bytes
   *           available to write
   * @param end_of_buffer_ptr modified to true if the end of buffer is reached,
   *                          otherwise false
   * @return a pointer to write into the ring buffer
   */
  unsigned char* alloc_contig(uint32_t* sz, bool* end_of_buffer_ptr);

  //! Allocates the maximum available space and returns alloc_result (see docs)
  alloc_result compound_alloc_contig();

  /*!
   * Commits sz bytes written to the ring buffer.  Call after alloc_contig by
   * the writing thread.
   *
   * @param sz the number of written bytes to commit to the ring buffer
   */
  void commit_write(uint32_t sz);

  /*!
   * Similar to alloc_contig(), this function allows you to read only contiguous
   * regions of memory.  Additionally (like alloc_contig()) you must call
   * commit_read() when done with this function to register the read.
   *
   * This function will return a pointer that will allow you to read a certain
   * number of bytes from the ring buffer.  You must provide a pointer to the
   * requested number of bytes out_sz.
   *
   * When this function returns, the out_sz will be modified to be the number
   * of bytes you may read from the returned pointer.
   *
   * If the modified out_sz variable is equal to the original value, then you
   * may ignore the end_of_buffer_ptr boolean and read out_sz bytes from the
   * returned pointer (make sure to call commit_read still, see blow).
   *
   * However, if the out_sz variable is modified to tbe less than the original
   * value, the function differentiates between two different scenarios:
   *
   * 1) If the contiguous memory is bounded because you ran out of readable
   *    bytes (i.e. the ring buffer is empty) then end_of_buffer_ptr is set to
   *    false.
   *
   * 2) If the contiguous memory is bounded because you are reaching the end
   *    of the buffer itself, then end_of_buffer_ptr is set to true.
   *
   * ****IMPORTANT****
   *
   * After calling this function, determining an action and reading values to
   * the returned pointer, it is important to call commit_read with the number
   * of bytes read from the pointer.  This will commit the read, otherwise it
   * will be as if nothing happened.
   *
   * Any call to read before calling commit_read will invalidate the pointer
   * returned by read_contig and read_contig must be called again to get a
   * new pointer.
   *
   * read_contig can be safely called multiple times before calling commit_read.
   *
   * @param out_sz a pointer to the initial size, modified to be the number of
   *               bytes available to read
   * @param end_of_buffer_ptr modified true if the end of the buffer is reached,
   *                          otherwise false
   * @return a pointer to read from the ring buffer
   */
  unsigned char* read_contig(uint32_t* out_sz, bool* end_of_buffer_ptr);

  /*!
   * Commits sz bytes read from the ring buffer.  Call after read_contig by
   * the reading thread.
   *
   * @param sz the number of read bytes to commit to the ring buffer
   */
  void commit_read(uint32_t sz);

 private:
  uint32_t get_bytes_written(uint32_t write_index, uint32_t read_index) const;

  void priv_write(const unsigned char* input, uint32_t amt_bytes_to_write,
                  uint32_t write_index, uint32_t read_index);
};

}  // namespace rtl

#endif  // RTLCPP_RING_BUFFER_HPP
