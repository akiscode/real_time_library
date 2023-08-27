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

#include <sys/mman.h>

#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <intrin.h>
#elif __x86_64__
#include <x86intrin.h>
#elif __arm__
static inline unsigned __rdtsc(void) {
  unsigned cc;
  asm volatile("mrc p15, 0, %0, c15, c12, 1" : "=r"(cc));
  return cc;
}
#else
#error "unknown platform"
#endif

#include <random>

#include "rtl/rtl.h"

struct results {
  uint64_t idx;
  const char* allocator;
  const char* op;
  size_t blk_size;
  uint64_t time;
};

void print_result(results const& r) {
  std::cout << r.idx << "," << r.allocator << "," << r.op << "," << r.blk_size
            << "," << r.time << std::endl;
}

void run_bench_rtl(uint64_t loops, size_t blk_min, size_t blk_max,
                   size_t num_blocks, size_t seed) {
  size_t buf_size = 1024 * 1024 * 100;  // 100 MB

  void* buf = mmap(0, buf_size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  for (size_t i = 0; i < buf_size; i++) {
    *((unsigned char*)buf + i) = 0x33;
    //      *((unsigned char*)buf + i) = 0x00;
  }

  struct rtl_tlsf_arena* arena;

  int err = rtl_tlsf_make_arena(&arena, buf, buf_size);

  if (err < 0) {
    std::cerr << "Could not make arena: " << err << std::endl;
    return;
  }

  size_t num_blks{num_blocks};
  std::vector<std::pair<void*, size_t>> blks{num_blks};

  uint64_t start, end;
  std::minstd_rand gen;

  gen.seed(seed);

  size_t blk_size;

  for (uint64_t i = 0; i < loops; i++) {
    size_t idx = gen() % num_blks;
    blk_size = blk_min + (gen() % (blk_max - blk_min));

    if (blks[idx].first) {
      start = __rdtsc();
      rtl_tlsf_free(arena, blks[idx].first);
      end = __rdtsc();
      print_result({i, "RTL", "free", blks[idx].second, end - start});

      blks[idx].first = NULL;
      blks[idx].second = 0;
    }

    start = __rdtsc();
    blks[idx].first = rtl_tlsf_alloc(arena, blk_size);
    end = __rdtsc();

    if (blks[idx].first) {
      blks[idx].second = blk_size;

      unsigned char* tmp = (unsigned char*)blks[idx].first;
      for (size_t i = 0; i < blk_size; i++) {
        *(tmp + i) = 0x33;
      }

      print_result({i, "RTL", "malloc", blk_size, end - start});
    }
  }

  for (auto& p : blks) {
    if (p.first) rtl_tlsf_free(arena, p.first);
  }

  munmap(buf, buf_size);
}

void run_bench_malloc(uint64_t loops, size_t blk_min, size_t blk_max,
                      size_t num_blocks, size_t seed) {
  size_t num_blks{num_blocks};
  std::vector<std::pair<void*, size_t>> blks{num_blks};

  uint64_t start, end;
  std::minstd_rand gen;

  gen.seed(seed);

  size_t blk_size;

  for (uint64_t i = 0; i < loops; i++) {
    size_t idx = gen() % num_blks;
    blk_size = blk_min + (gen() % (blk_max - blk_min));

    if (blks[idx].first) {
      start = __rdtsc();
      free(blks[idx].first);
      end = __rdtsc();
      print_result({i, "SYSTEM", "free", blks[idx].second, end - start});

      blks[idx].first = NULL;
      blks[idx].second = 0;
    }

    start = __rdtsc();
    blks[idx].first = malloc(blk_size);
    end = __rdtsc();

    if (blks[idx].first) {
      blks[idx].second = blk_size;
      print_result({i, "SYSTEM", "malloc", blk_size, end - start});
    }
  }

  for (auto& p : blks) {
    if (p.first) free(p.first);
  }
}

void print_results(std::vector<results> const& rtla) {
  for (results const& r : rtla) {
    print_result(r);
  }
}

int main() {
  uint64_t loops = 1000000;
  size_t blk_min = 32;
  size_t blk_max = 4 * 1024;

  std::random_device seeder;
  std::mt19937 engine(seeder());
  std::uniform_int_distribution<int> dist(3, 10000);

#ifdef NDEBUG
  std::cerr << "RELEASE BUILD" << std::endl;
#else
  std::cerr << "DEBUG BUILD" << std::endl;
#endif

  std::cout << "Index,Allocator,Operation,Block_Size,Time" << std::endl;
  //  for (uint32_t i = 0; i < 4294967295; i++) {
  for (uint32_t i = 0; i < 5; i++) {
    int num_blocks = dist(engine);
    int seed = dist(engine);

    run_bench_rtl(loops, blk_min, blk_max, num_blocks, seed);
    run_bench_malloc(loops, blk_min, blk_max, num_blocks, seed);
  }

  return 0;
}
