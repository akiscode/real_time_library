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

#include "rtlcpp/ring_buffer.hpp"

#include <cassert>

namespace rtl {

spsc_ringbuffer::spsc_ringbuffer(unsigned char *buf, uint32_t capacity)
    : m_initialized(true),
      m_buf(buf),
      m_capacity(capacity),
      m_writable_capacity((m_capacity > 0U) ? m_capacity - 1U : 0U),
      m_read_index(0U),
      m_write_index(0U) {
  // Stop all the warnings about unused private fields
  (void)pad0;
  (void)pad1;
}

spsc_ringbuffer::spsc_ringbuffer()
    : m_initialized(false),
      m_buf(nullptr),
      m_capacity(0U),
      m_writable_capacity(0U),
      m_read_index(0U),
      m_write_index(0U) {}

void spsc_ringbuffer::init(unsigned char *buf, uint32_t capacity) {
  if (m_initialized) {
    return;
  }

  spsc_ringbuffer rval(buf, capacity);
  *this = std::move(rval);
}

spsc_ringbuffer::spsc_ringbuffer(spsc_ringbuffer &&o) noexcept
    : m_initialized(rtl::exchange(o.m_initialized, false)),
      m_buf(rtl::exchange(o.m_buf, nullptr)),
      m_capacity(rtl::exchange(o.m_capacity, 0U)),
      m_writable_capacity(rtl::exchange(o.m_writable_capacity, 0U)),
      m_read_index(o.m_read_index.load()),
      m_write_index(o.m_write_index.load()) {}

spsc_ringbuffer &spsc_ringbuffer::operator=(spsc_ringbuffer &&o) noexcept {
  if (this != &o) {
    m_initialized = rtl::exchange(o.m_initialized, false);
    m_buf = rtl::exchange(o.m_buf, nullptr);
    m_capacity = rtl::exchange(o.m_capacity, 0U);
    m_writable_capacity = rtl::exchange(o.m_writable_capacity, 0U);
    m_read_index = o.m_read_index.load();
    m_write_index = o.m_write_index.load();
  }

  return *this;
}

uint32_t spsc_ringbuffer::approx_size() const {
  uint32_t write_index = m_write_index.load(std::memory_order_acquire);
  uint32_t read_index = m_read_index.load(std::memory_order_acquire);

  return get_bytes_written(write_index, read_index);
}

uint32_t spsc_ringbuffer::write_bytes(const unsigned char *input, uint32_t sz) {
  if (input == nullptr) {
    return 0U;
  }
  if (sz > m_writable_capacity) {
    sz = m_writable_capacity;
  }
  if (sz == 0U) {
    return 0U;
  }

  uint32_t write_index = m_write_index.load(std::memory_order_acquire);
  uint32_t read_index = m_read_index.load(std::memory_order_acquire);

  uint32_t bytes_written = get_bytes_written(write_index, read_index);

  uint32_t space_left = m_writable_capacity - bytes_written;

  uint32_t amt_bytes_to_write = std::min(sz, space_left);

  priv_write(input, amt_bytes_to_write, write_index, read_index);

  m_write_index.store((write_index + sz) % m_capacity,
                      std::memory_order_release);

  return amt_bytes_to_write;
}

bool spsc_ringbuffer::write(const unsigned char *input, uint32_t sz) {
  if (sz > m_writable_capacity) {
    return false;
  }
  if (sz == 0U) {
    return true;
  }
  if (input == nullptr) {
    return false;
  }

  uint32_t write_index = m_write_index.load(std::memory_order_acquire);
  uint32_t read_index = m_read_index.load(std::memory_order_acquire);

  uint32_t bytes_written = get_bytes_written(write_index, read_index);

  uint32_t space_left = m_writable_capacity - bytes_written;
  if (sz > space_left) {
    return false;
  }

  uint32_t amt_bytes_to_write = std::min(sz, space_left);

  priv_write(input, amt_bytes_to_write, write_index, read_index);

  m_write_index.store((write_index + sz) % m_capacity,
                      std::memory_order_release);

  return true;
}

uint32_t spsc_ringbuffer::read(unsigned char *output, uint32_t max_read) {
  if (output == nullptr) {
    return 0U;
  }
  if (max_read > m_writable_capacity) {
    max_read = m_writable_capacity;
  }
  if (max_read == 0U) {
    return 0U;
  }

  uint32_t write_index = m_write_index.load(std::memory_order_acquire);
  uint32_t read_index = m_read_index.load(std::memory_order_acquire);

  uint32_t bytes_written = get_bytes_written(write_index, read_index);

  uint32_t amt_bytes_to_read = std::min(max_read, bytes_written);

  if (write_index >= read_index) {
    // Can read in one chunk
    (void)std::memcpy(output, &m_buf[read_index], amt_bytes_to_read);
  } else {
    // Must split up in two chunks

    uint32_t until_end_of_buffer = m_writable_capacity - read_index + 1U;
    uint32_t from_beginning_of_buffer =
        (until_end_of_buffer > amt_bytes_to_read)
            ? 0U
            : amt_bytes_to_read - until_end_of_buffer;

    (void)std::memcpy(output, &m_buf[read_index], until_end_of_buffer);
    (void)std::memcpy(output + until_end_of_buffer, &m_buf[0],
                      from_beginning_of_buffer);
  }

  m_read_index.store((read_index + amt_bytes_to_read) % m_capacity,
                     std::memory_order_release);

  return amt_bytes_to_read;
}

unsigned char *spsc_ringbuffer::alloc_contig(uint32_t *sz,
                                             bool *end_of_buffer_ptr) {
  uint32_t requested_size = *sz;

  uint32_t write_index = m_write_index.load(std::memory_order_acquire);
  uint32_t read_index = m_read_index.load(std::memory_order_acquire);

  uint32_t largest_contiguous_block_size{0U};
  bool end_of_buffer;
  if (write_index >= read_index) {
    // This means that the largest contiguous block must be
    // the end of the buffer minus the write_index
    largest_contiguous_block_size = m_writable_capacity - write_index;
    // We add one because if the read index is not zero, then there must
    // be an extra "space" for the write buffer to wrap around to
    if (read_index != 0U) {
      largest_contiguous_block_size++;
      // Because the read_index isn't blocking the write,
      // this is the end of the buffer
      end_of_buffer = true;
    } else {
      // Technically we could wait for one more byte to free up
      end_of_buffer = false;
    }
  } else {
    largest_contiguous_block_size = read_index - write_index - 1U;
    end_of_buffer = false;
  }

  if (largest_contiguous_block_size < requested_size) {
    *sz = largest_contiguous_block_size;
    *end_of_buffer_ptr = end_of_buffer;
  }

  return &m_buf[write_index];
}

spsc_ringbuffer::alloc_result spsc_ringbuffer::compound_alloc_contig() {
  uint32_t write_index = m_write_index.load(std::memory_order_acquire);
  uint32_t read_index = m_read_index.load(std::memory_order_acquire);

  alloc_result rval(nullptr, 0U, nullptr, 0U, write_index >= read_index);

  uint32_t bytes_written = get_bytes_written(write_index, read_index);
  uint32_t free_bytes = m_writable_capacity - bytes_written;

  uint32_t largest_contiguous_block_size{0U};
  if (write_index >= read_index) {
    // This means that the largest contiguous block must be
    // the end of the buffer minus the write_index
    largest_contiguous_block_size = m_writable_capacity - write_index;
    if (read_index != 0U) {
      largest_contiguous_block_size++;
    }
  } else {
    largest_contiguous_block_size = read_index - write_index - 1U;
  }

  if (largest_contiguous_block_size == 0U) {
    return rval;
  }

  rval.first_buf = &m_buf[write_index];
  rval.first_buf_sz = largest_contiguous_block_size;

  // By definition, largest contiguous block size must be less than or equal
  // to the free space
  assert(largest_contiguous_block_size <= free_bytes);

  free_bytes = free_bytes - largest_contiguous_block_size;

  if (free_bytes == 0U) {
    return rval;
  }

  rval.second_buf = &m_buf[0U];
  rval.second_buf_sz = free_bytes;

  return rval;
}

void spsc_ringbuffer::commit_write(uint32_t sz) {
  if (sz == 0U) {
    return;
  }
  uint32_t write_index = m_write_index.load(std::memory_order_relaxed);
  write_index = (write_index + sz) % m_capacity;

  m_write_index.store(write_index, std::memory_order_release);
}

unsigned char *spsc_ringbuffer::read_contig(uint32_t *out_sz,
                                            bool *end_of_buffer_ptr) {
  uint32_t requested_size = *out_sz;

  uint32_t write_index = m_write_index.load(std::memory_order_acquire);
  uint32_t read_index = m_read_index.load(std::memory_order_acquire);

  uint32_t largest_contiguous_block_size{0U};
  bool end_of_buffer;

  if (write_index >= read_index) {
    largest_contiguous_block_size = write_index - read_index;
    end_of_buffer = false;
  } else {
    largest_contiguous_block_size = m_writable_capacity - read_index + 1U;
    end_of_buffer = true;
  }

  if (largest_contiguous_block_size < requested_size) {
    *out_sz = largest_contiguous_block_size;
    *end_of_buffer_ptr = end_of_buffer;
  }

  return &m_buf[read_index];
}

void spsc_ringbuffer::commit_read(uint32_t sz) {
  if (sz == 0U) {
    return;
  }
  uint32_t read_index = m_read_index.load(std::memory_order_relaxed);
  read_index = (read_index + sz) % m_capacity;

  m_read_index.store(read_index, std::memory_order_release);
}

uint32_t spsc_ringbuffer::get_bytes_written(uint32_t write_index,
                                            uint32_t read_index) const {
  uint32_t bytes_written{0U};

  if (write_index >= read_index) {
    bytes_written = write_index - read_index;
  } else {
    bytes_written = m_writable_capacity - read_index + 1U;
    bytes_written += write_index;
  }

  return bytes_written;
}

void spsc_ringbuffer::priv_write(const unsigned char *input,
                                 uint32_t amt_bytes_to_write,
                                 uint32_t write_index, uint32_t read_index) {
  if (write_index >= read_index) {
    // There is enough space left, but it needs to be split into different
    // writes

    uint32_t until_end_of_buffer = m_writable_capacity - write_index + 1U;

    uint32_t from_beginning_of_buffer =
        (until_end_of_buffer > amt_bytes_to_write)
            ? 0U
            : amt_bytes_to_write - until_end_of_buffer;

    (void)std::memcpy(&m_buf[write_index], input, until_end_of_buffer);
    (void)std::memcpy(&m_buf[0], input + until_end_of_buffer,
                      from_beginning_of_buffer);

  } else {
    // There is enough space left and it is in a contiguous chunk
    (void)std::memcpy(&m_buf[write_index], input, amt_bytes_to_write);
  }
}

}  // namespace rtl
