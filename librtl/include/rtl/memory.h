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

#ifndef RTL_MEMORY_H
#define RTL_MEMORY_H

#include <stddef.h>  // For size_t
#include <stdint.h>  // For uint32_t, etc

#include "pounds.h"  // For memory alignment

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/*!
 * \brief rtl_align Returns a size that is aligned to the word size for
 * allocations
 *
 * \param word_size The word size of the processor (usually sizeof(intptr_t))
 *
 * \param sz The size you want to align
 *
 * \return the total size that will match the alignment
 */
static inline size_t rtl_align(size_t word_size, size_t sz) {
  size_t value_one = 1U;
  return (sz + word_size - value_one) & ~(word_size - value_one);
}

//! Opaque type for arena
struct rtl_tlsf_arena;

//! Returns the minimum size that is needed to make an arena
size_t rtl_tlsf_minimum_arena_size(void);

//! Returns the maximum size that an arena can handle
size_t rtl_tlsf_maximum_arena_size(void);

/*!
 * \brief rtl_tlsf_make_arena constructs an arena from an existing memory region
 *
 * First declare a pointer to the arena opaque type, then pass it as a parameter
 * to this function along with a pointer to a memory buffer (the "memory"
 * parameter).
 *
 * The buffer that the arena uses is captured via the memory pointer and the
 * size of the memory buffer is in bytes.
 *
 * After you are done with the arena, use the standard release mechanism (if
 * there is one) for the buffer provided (i.e. munmap for mmap, free for malloc,
 * etc).
 *
 * The provided buffer must be at least as big as the result from
 * rtl_tlsf_minimum_arena_size()
 *
 * This function returns:
 *
 * 0 on Success
 * -1 if the arena pointer is null
 * -2 if the memory pointer isn't aligned properly
 * -3 If the provided size is smaller than rtl_tlsf_minimum_arena_size()
 * -4 If the provided size is greater than rtl_tlsf_maximum_arena_size()
 *
 * If this function fails, then the arena pointer won't be modified.
 *
 * \param arena the pointer to a pointer of the arena type
 * \param memory the memory buffer for the arena to use
 * \param sz the size of the memory buffer
 * \return 0 on success, otherwise -1, -2, -3, -4
 */
int rtl_tlsf_make_arena(struct rtl_tlsf_arena** arena, void* memory, size_t sz);

/*!
 * \brief rtl_tlsf_alloc allocates a chunk of memory that is at least sz big
 *
 * This function is analgous to malloc().  Given a request of size sz, it will
 * return a contiguous region of memory at least the size of sz.
 *
 * If it can not do this, then NULL is returned.
 *
 * This function assumes that arena is fully constructed.  Behavior is undefined
 * if this isn't the case.
 *
 * *** IMPORTANT***
 * *** DO NOT CALL free() ON MEMORY ALLOCATED FROM THIS FUNCTION.  ONLY CALL
 * *** rtl_tlsf_free()
 *
 * \param arena a constructed memory arena
 * \param sz the size of the request.
 * \return a pointer to contiguous memory if successful, otherwise NULL
 */
void* rtl_tlsf_alloc(struct rtl_tlsf_arena* arena, size_t sz);

/*!
 * \brief rtl_tlsf_free frees a piece of memory allocated by rtl_tlsf_alloc.
 *
 * *** IMPORTANT***
 * *** DO NOT CALL THIS FUNCTION ON MEMORY ALLOCATED FROM malloc().  ONLY CALL
 * *** THIS FUNCTION FROM MEMORY ALLOCATED FROM rtl_tlsf_alloc().
 *
 * It is ok to pass in NULL for the ptr parameter.
 *
 * This function assumes that arena is fully constructed.  Behavior is undefined
 * if this isn't the case.
 *
 * \param arena a constructed memory arena
 * \param ptr the pointer to memory needing to be freed
 */
void rtl_tlsf_free(struct rtl_tlsf_arena* arena, void* ptr);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // RTL_MEMORY_H
