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

#ifndef RTL_POUNDS_H
#define RTL_POUNDS_H

#include "stdint.h" // For the UINTXXX_T type

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#define IMPL_RTL_CONCAT(x, y) x##y
#define RTL_CONCAT(x, y) IMPL_RTL_CONCAT(x, y)

#define IMPL_RTL_QUOTE(x) #x
#define RTL_QUOTE(x) IMPL_RTL_QUOTE(x)

#define RTL_PTR_IS_ALIGNED(PTR, BYTES) \
  (((uintptr_t)((const void *)(PTR))) % (BYTES) == 0)


// Use this static assert if it isn't available to you in C11/C++11
#define RTL_C_STATIC_ASSERT(x, msg) \
  typedef char RTL_CONCAT(msg, __LINE__)[(x) ? 1 : -1]


#if defined(__PPCGECKO__) || defined(__PPCBROADWAY__) || defined(_XENON) || \
    defined(__powerpc) || defined(__powerpc__) || defined(__POWERPC__) ||   \
    defined(__ppc__) || defined(_M_PPC) || defined(_ARCH_PPC)
#define RTL_ARCH_PPC
#endif

#if defined(__MIPS__) || defined(__mips__) || defined(__mips)
#define RTL_ARCH_MIPS
#endif

#if defined(__x86_64) || defined(__x86_64__) || defined(__amd64__) || \
    defined(__amd64) || defined(_M_X64)
#define RTL_ARCH_X86_64
#endif

#if defined(_M_IX86) || defined(_X86_) || defined(__THW_INTEL__) || \
    defined(i386) || defined(__i386__) || defined(__i486__) ||      \
    defined(__i586__) || defined(__i686__) || defined(__i386) ||    \
    defined(__I86__) || defined(__INTEL__)
#define RTL_ARCH_X86_32
#endif

#if defined(RTL_ARCH_X86_32) || defined(RTL_ARCH_X86_64)
#define RTL_ARCH_X86
#endif


#if defined(__arm__) || defined(__arm64) || defined(__thumb__) || \
    defined(__TARGET_ARCH_ARM) || defined(__TARGET_ARCH_THUMB) || \
    defined(_M_ARM) || defined(__aarch64__)
#define RTL_ARCH_ARM
#endif

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // RTL_POUNDS_H
