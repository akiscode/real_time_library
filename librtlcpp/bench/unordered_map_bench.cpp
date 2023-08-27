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

#include <climits>
#include <iostream>
#include <random>
#include <unordered_map>
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

#include "rtlcpp/rtlcpp.hpp"

struct results {
  const char* map_type;
  const char* op;
  uint64_t num_count;
  double time;
};

void print_result(results const& r) {
  std::cout << r.map_type << "," << r.op << "," << r.num_count << "," << r.time
            << std::endl;
}

void run_put(std::vector<int>& input,
             rtl::unordered_map<int, int, rtl::RTAllocatorST>& m) {
  auto start = std::chrono::high_resolution_clock::now();
  auto end = std::chrono::high_resolution_clock::now();
  for (int i : input) {
    start = std::chrono::high_resolution_clock::now();
    bool rval = m.put(i, i);
    end = std::chrono::high_resolution_clock::now();
    double elapsed_time =
        std::chrono::duration<double, std::micro>(end - start).count();
    if (rval) print_result({"RTL", "Put", input.size(), elapsed_time});
  }
}

void run_put(std::vector<int>& input,
             std::unordered_map<int, int, rtl::hash<int>>& m) {
  auto start = std::chrono::high_resolution_clock::now();
  auto end = std::chrono::high_resolution_clock::now();
  for (int i : input) {
    start = std::chrono::high_resolution_clock::now();
    m[i] = i;
    end = std::chrono::high_resolution_clock::now();
    double elapsed_time =
        std::chrono::duration<double, std::micro>(end - start).count();
    print_result({"STD", "Put", input.size(), elapsed_time});
  }
}

void run_get(std::vector<int>& input,
             rtl::unordered_map<int, int, rtl::RTAllocatorST>& m) {
  auto start = std::chrono::high_resolution_clock::now();
  auto end = std::chrono::high_resolution_clock::now();
  int* tmp;
  for (int i : input) {
    start = std::chrono::high_resolution_clock::now();
    tmp = m.get(i);
    end = std::chrono::high_resolution_clock::now();

    if (!tmp) continue;

    *tmp = 77;

    double elapsed_time =
        std::chrono::duration<double, std::micro>(end - start).count();
    print_result({"RTL", "Get", input.size(), elapsed_time});
  }
}

void run_get(std::vector<int>& input,
             std::unordered_map<int, int, rtl::hash<int>>& m) {
  auto start = std::chrono::high_resolution_clock::now();
  auto end = std::chrono::high_resolution_clock::now();
  int* tmp;
  for (int i : input) {
    start = std::chrono::high_resolution_clock::now();
    tmp = &m[i];
    end = std::chrono::high_resolution_clock::now();

    *tmp = 77;

    double elapsed_time =
        std::chrono::duration<double, std::micro>(end - start).count();
    print_result({"STD", "Get", input.size(), elapsed_time});
  }
}

void run_del(std::vector<int>& input,
             rtl::unordered_map<int, int, rtl::RTAllocatorST>& m) {
  auto start = std::chrono::high_resolution_clock::now();
  auto end = std::chrono::high_resolution_clock::now();
  for (int i : input) {
    start = std::chrono::high_resolution_clock::now();
    m.del(i);
    end = std::chrono::high_resolution_clock::now();

    double elapsed_time =
        std::chrono::duration<double, std::micro>(end - start).count();
    print_result({"RTL", "Del", input.size(), elapsed_time});
  }
}

void run_del(std::vector<int>& input,
             std::unordered_map<int, int, rtl::hash<int>>& m) {
  auto start = std::chrono::high_resolution_clock::now();
  auto end = std::chrono::high_resolution_clock::now();
  for (int i : input) {
    start = std::chrono::high_resolution_clock::now();
    m.erase(i);
    end = std::chrono::high_resolution_clock::now();

    double elapsed_time =
        std::chrono::duration<double, std::micro>(end - start).count();
    print_result({"STD", "Del", input.size(), elapsed_time});
  }
}

struct SystemAllocator {
  void* allocate(size_t sz) { return malloc(sz); }

  void deallocate(void* p) { free(p); }
};

int main() {
  std::vector<uint64_t> number_count = {100, 1000, 10000, 100000, 150000};
  std::vector<std::vector<int>> rand_nums;
  rand_nums.resize(number_count.size());

  std::random_device seeder;
  std::mt19937 engine(seeder());
  std::uniform_int_distribution<int> dist(INT_MIN, INT_MAX);
  int count = 0;
  for (uint64_t n : number_count) {
    for (uint64_t i = 0; i < n; i++) {
      rand_nums[count].push_back(dist(engine));
    }
    count++;
  }

#ifdef NDEBUG
  std::cerr << "RELEASE BUILD" << std::endl;
#else
  std::cerr << "DEBUG BUILD" << std::endl;
#endif

  std::cout << "MapType,Operation,NumCount,TimeMicro" << std::endl;

  for (std::vector<int> in : rand_nums) {
    std::unordered_map<int, int, rtl::hash<int>> stdm;
    stdm.max_load_factor(7);

    run_put(in, stdm);
    run_get(in, stdm);
    run_del(in, stdm);
  }

  const size_t buf_size{500 * 1024 * 1024};
  {
    rtl::MMapMemoryResource mr;
    rtl::RTAllocatorST allocST;

    if (!mr.init(buf_size)) {
      std::cerr << "Could not initialize buffer" << std::endl;
      return EXIT_FAILURE;
    }
    if (!allocST.init(mr.get_buf(), mr.get_capacity())) return EXIT_FAILURE;
    for (std::vector<int> in : rand_nums) {
      rtl::unordered_map<int, int, rtl::RTAllocatorST> rtlm(&allocST, 7);

      run_put(in, rtlm);
      run_get(in, rtlm);
      run_del(in, rtlm);
    }
  }

  return EXIT_SUCCESS;
}
