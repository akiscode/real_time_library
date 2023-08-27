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

#ifndef RTLCPP_HASH_HPP
#define RTLCPP_HASH_HPP

#include <cstdint>
#include <string>
#include <type_traits>

namespace rtl {

using FNV1A_32_PRIME = std::integral_constant<uint32_t, 16777619U>;
using FNV1A_32_OFFSET = std::integral_constant<uint32_t, 2166136261U>;

inline uint32_t fnv1a(const void* buffer, size_t sz) {
  const unsigned char* buf = static_cast<const unsigned char*>(buffer);

  uint32_t rval{FNV1A_32_OFFSET::value};

  for (size_t i = 0U; i < sz; i++) {
    rval = rval ^ buf[i];
    rval = rval * FNV1A_32_PRIME::value;
  }
  return rval;
}

inline uint32_t fnv1a(const char* str) {
  const char* c = str;
  uint32_t rval{FNV1A_32_OFFSET::value};
  while (*c != '\0') {
    rval = rval ^ static_cast<unsigned char>(*c);
    rval = rval * FNV1A_32_PRIME::value;
    c++;
  }

  return rval;
}

inline uint32_t fnv1a(std::string const& str) {
  return fnv1a(str.c_str(), str.length());
}

inline uint32_t fnv1a(float num) { return fnv1a(&num, sizeof(num)); }

inline uint32_t fnv1a(double num) { return fnv1a(&num, sizeof(num)); }

inline uint32_t fnv1a(uint8_t num) {
  return (num ^ FNV1A_32_OFFSET::value) * FNV1A_32_PRIME::value;
}

inline uint32_t fnv1a(uint16_t num) { return fnv1a(&num, sizeof(num)); }

inline uint32_t fnv1a(uint32_t num) { return fnv1a(&num, sizeof(num)); }

inline uint32_t fnv1a(uint64_t num) { return fnv1a(&num, sizeof(num)); }

inline uint32_t fnv1a(int8_t num) { return fnv1a(&num, sizeof(num)); }

inline uint32_t fnv1a(int16_t num) { return fnv1a(&num, sizeof(num)); }

inline uint32_t fnv1a(int32_t num) { return fnv1a(&num, sizeof(num)); }

inline uint32_t fnv1a(int64_t num) { return fnv1a(&num, sizeof(num)); }

template <typename T>
struct hash {
  uint32_t operator()(T const& obj) const noexcept {
    return fnv1a(&obj, sizeof(obj));
  }
};

template <>
struct hash<const char*> {
  uint32_t operator()(const char* const s) { return fnv1a(s); }
};

template <>
struct hash<std::string> {
  uint32_t operator()(std::string const& s) { return fnv1a(s); }
};

}  // namespace rtl

#endif  // RTLCPP_HASH_HPP
