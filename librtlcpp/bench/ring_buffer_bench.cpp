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

#include <random>
#include <thread>

#include "rtlcpp/rtlcpp.hpp"

void producer_contig(rtl::spsc_ringbuffer& rb, std::atomic<bool>& go) {
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_int_distribution<int> dist(10, 200);
  std::uniform_int_distribution<int> write_length(1, 50);
  rtl::SlumberViaProgressive s;

  while (!go) {
  }

  uint32_t sz;
  bool end_of_buffer;
  unsigned char* w_ptr;

  uint8_t i{1};

  while (i < 254) {
    sz = write_length(mt);

    w_ptr = rb.alloc_contig(&sz, &end_of_buffer);

    if (sz == 0) s.wait();

    for (uint32_t j = 0; j < sz; j++) {
      w_ptr[j] = (unsigned char)i;
      if (++i == 255) {
        sz = j;
        break;
      }
    }

    rb.commit_write(sz);
  }
}

void consumer_contig(rtl::spsc_ringbuffer& rb, std::atomic<bool>& go) {
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_int_distribution<int> dist(10, 200);
  std::uniform_int_distribution<int> read_length(1, 50);

  while (!go) {
  }

  uint32_t sz;
  bool end_of_buffer;
  unsigned char* r_ptr;
  uint8_t i{0};

  while (i < 253) {
    sz = read_length(mt);
    r_ptr = rb.read_contig(&sz, &end_of_buffer);

    for (uint32_t j = 0; j < sz; j++) {
      uint8_t new_i = (uint8_t)r_ptr[j];

      if (i != (new_i - 1)) {
        std::cerr << "INVALID.  I: " << +i << " NEW I: " << +new_i << std::endl;
        std::exit(-1);
      }
      i = new_i;
    }

    rb.commit_read(sz);
  }
}

void producer_block(rtl::spsc_ringbuffer& rb, std::atomic<bool>& go) {
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_int_distribution<int> dist(10, 200);
  std::uniform_int_distribution<int> write_length(1, 7);
  rtl::SlumberViaProgressive s;

  unsigned char buf[255];
  for (int i = 0; i < 255; i++) buf[i] = i;

  while (!go) {
  }

  int32_t bytes_to_write{255};
  uint32_t sz;

  uint32_t offset{0};

  while (bytes_to_write > 0) {
    sz = write_length(mt);

    if ((bytes_to_write - (int32_t)sz) < 0) sz = bytes_to_write;

    if (rb.write(buf + offset, sz)) {
      offset += sz;
      bytes_to_write -= (int32_t)sz;
    } else {
      s.wait();
    }
  }
}

void consumer_block(rtl::spsc_ringbuffer& rb, std::atomic<bool>& go) {
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_int_distribution<int> dist(10, 200);
  std::uniform_int_distribution<int> read_length(1, 50);

  while (!go) {
  }

  uint32_t sz;
  uint32_t bytes_read{0};
  uint8_t output[255];
  for (int i = 0; i < 255; i++) output[i] = '\0';
  while (bytes_read < 255) {
    sz = read_length(mt);

    sz = rb.read(output + bytes_read, sz);
    bytes_read += sz;
  }
}

int main(int argc, char** argv) {
  int number_of_loops;

  if (argc != 2) {
    number_of_loops = 10;
  } else {
    number_of_loops = std::atoi(argv[1]);
  }

  // Some length between 1-50 to provide oddity write patterns
  const uint32_t buf_sz{48};
  unsigned char buf[buf_sz];
  rtl::spsc_ringbuffer rb(buf, buf_sz);

  std::atomic<bool> go{false};

  std::vector<std::thread> threads;

  auto start = std::chrono::high_resolution_clock::now();
  auto end = std::chrono::high_resolution_clock::now();

  std::vector<double> times;

  for (int i = 0; i < number_of_loops; i++) {
    threads.clear();
    go = false;
    threads.emplace_back(producer_contig, std::ref(rb), std::ref(go));
    threads.emplace_back(consumer_contig, std::ref(rb), std::ref(go));
    // Start threads
    go = true;
    start = std::chrono::high_resolution_clock::now();
    for (std::thread& t : threads) t.join();
    end = std::chrono::high_resolution_clock::now();

    double elapsed_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
            .count();
    times.push_back(elapsed_time);
  }

  double sum{0.0};
  for (double& d : times) {
    sum += d;
  }

  double average = sum / (double)times.size();

  std::cout << "Completed " << number_of_loops << " loops each taking "
            << average << " milliseconds." << std::endl;

  return 0;
}