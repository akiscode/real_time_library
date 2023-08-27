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

#include "rtlcpp/allocator.hpp"

#include <sys/mman.h>

#include <cassert>

#include "rtlcpp/utility.hpp"

namespace rtl {

MMapMemoryResource::MMapMemoryResource(MMapMemoryResource&& o) noexcept
    : m_initialized(rtl::exchange(o.m_initialized, false)),
      m_buf(rtl::exchange(o.m_buf, nullptr)),
      m_capacity(rtl::exchange(o.m_capacity, 0)) {}

MMapMemoryResource& MMapMemoryResource::operator=(
    MMapMemoryResource&& o) noexcept {
  if (this != &o) {
    m_initialized = rtl::exchange(o.m_initialized, false);
    m_buf = rtl::exchange(o.m_buf, nullptr);
    m_capacity = rtl::exchange(o.m_capacity, 0);
  }

  return *this;
}

bool MMapMemoryResource::init(size_t capacity) {
  if (m_initialized) {
    return true;
  }

  m_capacity = capacity;

  m_buf = mmap(NULL, m_capacity, PROT_READ | PROT_WRITE,
               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  // A map failed value that works with the MISRA standard
  void* MISRA_MAP_FAILED = reinterpret_cast<void*>(-1);
  assert(MISRA_MAP_FAILED == MAP_FAILED);

  if (m_buf == MISRA_MAP_FAILED) {
    return false;
  }

  m_initialized = true;

  return true;
}

void MMapMemoryResource::uninit() {
  if (!m_initialized) {
    return;
  }

  (void)munmap(m_buf, m_capacity);
  m_buf = nullptr;

  m_capacity = 0U;

  m_initialized = false;
}

namespace detail {

RTAllocator::RTAllocator(RTAllocator&& o) noexcept
    : m_initialized(rtl::exchange(o.m_initialized, false)),
      m_arena(rtl::exchange(o.m_arena, nullptr)),
      m_buf(rtl::exchange(o.m_buf, nullptr)),
      m_capacity(rtl::exchange(o.m_capacity, 0)) {}

RTAllocator& RTAllocator::operator=(RTAllocator&& o) noexcept {
  if (this != &o) {
    m_initialized = rtl::exchange(o.m_initialized, false);
    m_arena = rtl::exchange(o.m_arena, nullptr);
    m_buf = rtl::exchange(o.m_buf, nullptr);
    m_capacity = rtl::exchange(o.m_capacity, 0);
  }
  return *this;
}

void* RTAllocator::allocate(std::size_t bytes) {
  assert(m_initialized);
  return rtl_tlsf_alloc(m_arena, bytes);
}

void RTAllocator::deallocate(void* p) {
  assert(m_initialized);
  rtl_tlsf_free(m_arena, p);
}

bool RTAllocator::init(void* buf, size_t capacity) {
  if (m_initialized) {
    return true;
  }

  if (capacity == 0U) {
    return false;
  }

  m_capacity = capacity;

  m_buf = buf;

  if (rtl_tlsf_make_arena(&m_arena, m_buf, m_capacity) < 0) {
    return false;
  }

  m_initialized = true;

  return true;
}

void RTAllocator::uninit() {
  if (!m_initialized) {
    return;
  }

  m_buf = nullptr;
  m_arena = nullptr;
  m_capacity = 0U;
  m_initialized = false;
}

}  // namespace detail

}  // namespace rtl
