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

#ifndef RTLCPP_UTILITY_HPP
#define RTLCPP_UTILITY_HPP

#include <cstdint>
#include <utility>

#include "rtl/pounds.h"

#ifndef RTL_TARGET_WORD_SIZE_BITS
#error \
    "Please define RTL_TARGET_WORD_SIZE_BITS (the word size of your **TARGET** platform in bits) to compile this library"
#endif

namespace rtl {

// Replaces the value of obj with new_value and returns the old value of obj.
template <typename T, typename U>
T exchange(T& obj, U&& new_value) {
  T old_value = std::move(obj);
  obj = std::forward<U>(new_value);
  return old_value;
}

// Assumes that the index is the power of 2 you want.
// e.g. if you are looking for a prime at least as big as 2^3
// then search for power_of_two_primes[3]
// Like wise for 2^4 then power_of_two_primes[4]
static const uint32_t power_of_two_primes[32] = {
    2U,          2U,         5U,        11U,        17U,        37U,
    67U,         131U,       257U,      521U,       1031U,      2053U,
    4099U,       8209U,      16411U,    32771U,     65537U,     131101U,
    262147U,     524309U,    1048583U,  2097169U,   4194319U,   8388617U,
    16777259U,   33554467U,  67108879U, 134217757U, 268435459U, 536870923U,
    1073741827U, 2147483659U

};

/*!
 * Given a power of 2 "n" with constraints 0 <= n <= 31 return a prime
 * number that is at least as big as 2^n.
 *
 * Useful for hash table implementations.
 *
 * If a value of greater than or equal to 32 is provided, this function
 * returns 0.
 *
 * @param power_of_2 a power of two from 0 to 31 (inclusive for both ends)
 * @return a prime number that is at least the size of 2^n.
 */
inline constexpr uint32_t get_prime_power_of_2(unsigned int n) {
  return (n >= 32U) ? 0U : power_of_two_primes[n];
}

/*!
 * Activates the platform specific pause functionality, mainly used
 * for spinlocks
 */
inline void asm_cpu_relax() {
#if defined(RTL_ARCH_ARM)

#if defined(_MSC_VER)
  YieldProcessor();
#elif defined(RTL_ARCH_ARM) && !(__ARM_ARCH < 7)
  // yield is supported on ARMv7 and above
  asm volatile("yield" ::: "memory");
#else
  asm volatile("nop" ::: "memory");
#endif

#elif defined(RTL_ARCH_X86)
#if defined(_MSC_VER)
  YieldProcessor();
#else
  asm volatile("pause" ::: "memory");
#endif

#elif defined(RTL_ARCH_PPC)
  asm volatile("or 27,27,27" ::: "memory");

#elif defined(RTL_ARCH_MIPS)
  asm volatile("pause" ::: "memory");
#else
  std::this_thread::sleep_for(std::chrono::microseconds(0));
#endif
}

}  // namespace rtl

#endif  // RTLCPP_UTILITY_HPP
