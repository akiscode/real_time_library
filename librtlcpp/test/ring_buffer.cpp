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

#include <gtest/gtest.h>

#include <random>
#include <thread>

class RingBufferTest : public ::testing::Test {
 protected:
  RingBufferTest() {
    // You can do set-up work for each test here.
  }

  ~RingBufferTest() override {
    // You can do clean-up work that doesn't throw exceptions here.
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }
};

void producer_contig(rtl::spsc_ringbuffer& rb, std::atomic<bool>& go) {
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_int_distribution<int> dist(10, 200);
  std::uniform_int_distribution<int> write_length(1, 50);

  while (!go) {
  }

  uint32_t sz;
  bool end_of_buffer;
  unsigned char* w_ptr;

  uint8_t i{1};

  while (i < 254) {
    sz = write_length(mt);

    w_ptr = rb.alloc_contig(&sz, &end_of_buffer);

    for (uint32_t j = 0; j < sz; j++) {
      w_ptr[j] = (unsigned char)i;
      if (++i == 255) {
        sz = j;
        break;
      }
    }

    std::this_thread::sleep_for(std::chrono::microseconds{dist(mt)});
    rb.commit_write(sz);
  }
}

void consumer_contig(rtl::spsc_ringbuffer& rb, std::atomic<bool>& go) {
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_int_distribution<int> read_length(1, 50);
  rtl::SlumberViaProgressive s;

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

      ASSERT_TRUE(i == (new_i - 1));
      i = new_i;
    }

    s.wait();
    rb.commit_read(sz);
  }
}

TEST_F(RingBufferTest, ThreadedTest) {
  int number_of_loops = 10;

  // Some length between 1-50 to provide oddity write patterns
  const uint32_t buf_sz{48};
  unsigned char buf[buf_sz];
  rtl::spsc_ringbuffer rb(buf, buf_sz);

  std::atomic<bool> go{false};

  std::vector<std::thread> threads;

  for (int i = 0; i < number_of_loops; i++) {
    threads.clear();
    go = false;
    threads.emplace_back(producer_contig, std::ref(rb), std::ref(go));
    threads.emplace_back(consumer_contig, std::ref(rb), std::ref(go));
    // Start threads
    go = true;

    for (std::thread& t : threads) t.join();
  }
}

void producer_block(rtl::spsc_ringbuffer& rb, std::atomic<bool>& go) {
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_int_distribution<int> dist(10, 200);
  std::uniform_int_distribution<int> write_length(1, 7);

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
    }

    std::this_thread::sleep_for(std::chrono::microseconds{dist(mt)});
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

    std::this_thread::sleep_for(std::chrono::microseconds{dist(mt)});
  }

  for (uint32_t i = 0; i < 255 - 1; i++) {
    uint8_t cur = output[i];
    uint8_t next = output[i + 1];

    ASSERT_TRUE(cur == (next - 1));
  }
}

TEST_F(RingBufferTest, BlockThreadedTest) {
  int number_of_loops = 10;

  const uint32_t buf_sz{13};
  unsigned char buf[buf_sz];
  rtl::spsc_ringbuffer rb(buf, buf_sz);

  std::atomic<bool> go{false};

  std::vector<std::thread> threads;

  for (int i = 0; i < number_of_loops; i++) {
    threads.clear();
    go = false;
    threads.emplace_back(producer_block, std::ref(rb), std::ref(go));
    threads.emplace_back(consumer_block, std::ref(rb), std::ref(go));
    // Start threads
    go = true;

    for (std::thread& t : threads) t.join();
  }
}

TEST_F(RingBufferTest, SmallBlockSmokeTest) {
  unsigned char buf[7];
  {
    rtl::spsc_ringbuffer rb(buf, 7);
    ASSERT_EQ(rb.writable_capacity(), 6);
    ASSERT_TRUE(rb.empty());

    uint8_t input[500];
    for (int i = 0; i < 500; i++) input[i] = i;

    uint8_t output[6];
    for (int i = 0; i < 6; i++) output[i] = 11;

    ASSERT_TRUE(rb.write(input, 6));
    uint32_t bytes_read = rb.read(output, 4);
    ASSERT_EQ(bytes_read, 4);
    ASSERT_TRUE(rb.write(input + 6, 4));

    for (int i = 0; i < 6; i++) output[i] = 11;
    bytes_read = rb.read(output, 10);
    ASSERT_EQ(bytes_read, 6);

    uint8_t to_test[6] = {4, 5, 6, 7, 8, 9};
    for (int i = 0; i < 6; i++) {
      ASSERT_EQ(output[i], to_test[i]);
    }

    ASSERT_TRUE(rb.empty());
    ASSERT_EQ(rb.approx_size(), 0);
  }

  {
    rtl::spsc_ringbuffer rb(buf, 7);
    ASSERT_EQ(rb.writable_capacity(), 6);
    ASSERT_TRUE(rb.empty());

    uint8_t input[500];
    for (int i = 0; i < 500; i++) input[i] = i;

    uint8_t output[6];
    for (int i = 0; i < 6; i++) output[i] = 11;

    ASSERT_TRUE(rb.write(input, 6));
    uint32_t bytes_read = rb.read(output, 4);
    ASSERT_EQ(bytes_read, 4);
    ASSERT_TRUE(rb.write(input + 6, 3));
    ASSERT_TRUE(rb.write(input + 9, 1));

    bytes_read = rb.read(output, 10);
    ASSERT_EQ(bytes_read, 6);

    uint8_t to_test[6] = {4, 5, 6, 7, 8, 9};

    for (int i = 0; i < 6; i++) {
      ASSERT_EQ(output[i], to_test[i]);
    }
  }
}

TEST_F(RingBufferTest, BlockBytesSmokeTest) {
  unsigned char buf[5];
  unsigned char buf2[501];
  rtl::spsc_ringbuffer rb(buf, 5);
  ASSERT_EQ(rb.writable_capacity(), 4);
  ASSERT_TRUE(rb.empty());

  uint8_t input[500];
  for (int i = 0; i < 500; i++) input[i] = i % 144;

  ASSERT_TRUE(rb.empty());
  ASSERT_EQ(rb.write_bytes(input, 2), 2);
  ASSERT_FALSE(rb.empty());
  ASSERT_EQ(rb.write_bytes(input + 2, 2), 2);

  uint32_t bytes_read{0};
  uint8_t output[500];

  bytes_read = rb.read(output, 700);
  ASSERT_EQ(bytes_read, 4);

  for (int i = 0; i < 4; i++) {
    ASSERT_EQ(output[i], i % 144);
  }

  for (int i = 0; i < 501; i++) buf2[i] = 2;

  rtl::spsc_ringbuffer rb2(buf2, 201);
  ASSERT_TRUE(rb2.empty());

  for (int i = 3; i < 5; i++) {
    size_t offset = i * 100;
    ASSERT_EQ(rb2.write_bytes(input + offset, 100), 100);
  }
  bytes_read = rb2.read(output, 100);
  ASSERT_EQ(bytes_read, 100);
  ASSERT_EQ(rb2.approx_size(), 100);
  ASSERT_EQ(rb2.approx_free_bytes(), 100);

  for (int i = 0; i < 100; i++) {
    ASSERT_EQ(output[i], input[300 + i]);
  }

  ASSERT_EQ(rb2.write_bytes(input, 100), 100);

  for (int i = 0; i < 500; i++) output[i] = 11;
  bytes_read = rb2.read(output, 700);
  ASSERT_EQ(bytes_read, 200);

  unsigned char to_test[200];
  for (int i = 0; i < 200; i++) to_test[i] = 3;
  std::memcpy(to_test, input + 400, 100);
  std::memcpy(to_test + 100, input, 100);

  for (uint32_t i = 0; i < bytes_read; i++) {
    ASSERT_EQ(output[i], to_test[i]);
  }
}

TEST_F(RingBufferTest, BlockSmokeTest) {
  unsigned char buf[5];
  unsigned char buf2[501];
  rtl::spsc_ringbuffer rb(buf, 5);
  ASSERT_EQ(rb.writable_capacity(), 4);
  ASSERT_TRUE(rb.empty());

  uint8_t input[500];
  for (int i = 0; i < 500; i++) input[i] = i % 144;

  ASSERT_TRUE(rb.empty());
  ASSERT_TRUE(rb.write(input, 2));
  ASSERT_FALSE(rb.empty());
  ASSERT_FALSE(rb.write(input + 2, 10));
  ASSERT_TRUE(rb.write(input + 2, 2));

  uint32_t bytes_read{0};
  uint8_t output[500];

  bytes_read = rb.read(output, 700);
  ASSERT_EQ(bytes_read, 4);

  for (int i = 0; i < 4; i++) {
    ASSERT_EQ(output[i], i % 144);
  }

  for (int i = 0; i < 501; i++) buf2[i] = 2;

  rtl::spsc_ringbuffer rb2(buf2, 201);
  ASSERT_TRUE(rb2.empty());

  for (int i = 3; i < 5; i++) {
    size_t offset = i * 100;
    ASSERT_TRUE(rb2.write(input + offset, 100));
  }
  bytes_read = rb2.read(output, 100);
  ASSERT_EQ(bytes_read, 100);
  ASSERT_EQ(rb2.approx_size(), 100);

  for (int i = 0; i < 100; i++) {
    ASSERT_EQ(output[i], input[300 + i]);
  }

  ASSERT_TRUE(rb2.write(input, 100));
  ASSERT_FALSE(rb2.write(input, 100));

  for (int i = 0; i < 500; i++) output[i] = 11;
  bytes_read = rb2.read(output, 700);
  ASSERT_EQ(bytes_read, 200);

  unsigned char to_test[200];
  for (int i = 0; i < 200; i++) to_test[i] = 3;
  std::memcpy(to_test, input + 400, 100);
  std::memcpy(to_test + 100, input, 100);

  for (uint32_t i = 0; i < bytes_read; i++) {
    ASSERT_EQ(output[i], to_test[i]);
  }
}

TEST_F(RingBufferTest, CompoundAlloc) {
  unsigned char input[10] = {0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1};
  unsigned char buf[8];
  unsigned char output[10];
  {
    // Full scenario
    rtl::spsc_ringbuffer rb(buf, 8);
    ASSERT_EQ(rb.writable_capacity(), 7);
    ASSERT_TRUE(rb.empty());
    ASSERT_TRUE(rb.write(input, 7));
    ASSERT_EQ(rb.approx_size(), 7);
    ASSERT_EQ(rb.approx_free_bytes(), 0);

    rtl::spsc_ringbuffer::alloc_result rval = rb.compound_alloc_contig();
    ASSERT_EQ(rval.first_buf_sz, 0);
    ASSERT_EQ(rval.second_buf_sz, 0);
    ASSERT_TRUE(rval.write_ahead_of_read);
  }
  {
    // Wrap Around Not Available
    rtl::spsc_ringbuffer rb(buf, 8);
    ASSERT_TRUE(rb.write(input, 5));

    rtl::spsc_ringbuffer::alloc_result rval = rb.compound_alloc_contig();
    ASSERT_EQ(rval.first_buf, &buf[5]);
    ASSERT_EQ(rval.first_buf_sz, 2);
    ASSERT_EQ(rval.second_buf_sz, 0);
    ASSERT_TRUE(rval.write_ahead_of_read);

    ASSERT_TRUE(rb.read(output, 5));
    ASSERT_TRUE(rb.write(input, 4));

    rval = rb.compound_alloc_contig();
    ASSERT_EQ(rval.first_buf, &buf[1]);
    ASSERT_EQ(rval.first_buf_sz, 3);
    ASSERT_EQ(rval.second_buf_sz, 0);
    ASSERT_FALSE(rval.write_ahead_of_read);
  }

  {
    // Wrap Around Available
    rtl::spsc_ringbuffer rb(buf, 8);
    ASSERT_TRUE(rb.write(input, 5));

    rtl::spsc_ringbuffer::alloc_result rval = rb.compound_alloc_contig();
    ASSERT_EQ(rval.first_buf, &buf[5]);
    ASSERT_EQ(rval.first_buf_sz, 2);
    ASSERT_EQ(rval.second_buf_sz, 0);
    ASSERT_TRUE(rval.write_ahead_of_read);

    ASSERT_TRUE(rb.read(output, 3));

    rval = rb.compound_alloc_contig();
    ASSERT_EQ(rval.first_buf, &buf[5]);
    ASSERT_EQ(rval.first_buf_sz, 3);
    ASSERT_EQ(rval.second_buf, &buf[0]);
    ASSERT_EQ(rval.second_buf_sz, 2);
    ASSERT_TRUE(rval.write_ahead_of_read);
  }

  {
    // Off by one test
    rtl::spsc_ringbuffer rb(buf, 8);
    ASSERT_EQ(rb.writable_capacity(), 7);
    ASSERT_TRUE(rb.empty());
    ASSERT_TRUE(rb.write(input, 7));
    ASSERT_EQ(rb.approx_size(), 7);
    ASSERT_EQ(rb.approx_free_bytes(), 0);

    ASSERT_TRUE(rb.read(output, 1));

    rtl::spsc_ringbuffer::alloc_result rval = rb.compound_alloc_contig();

    ASSERT_EQ(rval.first_buf_sz, 1);
    ASSERT_EQ(rval.second_buf_sz, 0);
    ASSERT_TRUE(rval.write_ahead_of_read);
  }
}

TEST_F(RingBufferTest, SmokeTest) {
  unsigned char buf[5];
  rtl::spsc_ringbuffer rb(buf, 5);
  ASSERT_EQ(rb.writable_capacity(), 4);
  ASSERT_TRUE(rb.empty());

  uint32_t sz{0};
  bool end_of_buffer;

  unsigned char* w_ptr = rb.alloc_contig(&sz, &end_of_buffer);
  ASSERT_TRUE(w_ptr);
  sz = 2;
  w_ptr = rb.alloc_contig(&sz, &end_of_buffer);
  ASSERT_TRUE(w_ptr);
  ASSERT_EQ(sz, 2);

  for (uint32_t i = 0; i < sz; i++) w_ptr[i] = (uint8_t)i;
  ASSERT_TRUE(rb.empty());
  rb.commit_write(sz);
  ASSERT_FALSE(rb.empty());

  sz = 10;
  w_ptr = rb.alloc_contig(&sz, &end_of_buffer);
  ASSERT_TRUE(w_ptr);
  ASSERT_EQ(sz, 2);
  ASSERT_FALSE(end_of_buffer);
  for (uint32_t i = 0; i < sz; i++) w_ptr[i] = (uint8_t)(i + 2);
  rb.commit_write(sz);

  end_of_buffer = true;
  sz = 10000;
  unsigned char* r_ptr = rb.read_contig(&sz, &end_of_buffer);
  ASSERT_FALSE(end_of_buffer);
  ASSERT_TRUE(r_ptr);
  ASSERT_EQ(sz, 4);

  for (uint32_t i = 0; i < sz; i++) {
    ASSERT_EQ((uint8_t)r_ptr[i], (uint8_t)i);
  }

  ASSERT_FALSE(rb.empty());
  rb.commit_read(sz);
  ASSERT_TRUE(rb.empty());

  // --

  end_of_buffer = false;
  sz = 4;
  w_ptr = rb.alloc_contig(&sz, &end_of_buffer);
  ASSERT_TRUE(w_ptr);
  ASSERT_EQ(sz, 1);
  ASSERT_TRUE(end_of_buffer);

  w_ptr[0] = (uint8_t)9;
  rb.commit_write(sz);

  sz = 10;
  w_ptr = rb.alloc_contig(&sz, &end_of_buffer);
  ASSERT_TRUE(w_ptr);
  ASSERT_EQ(sz, 3);
  ASSERT_FALSE(end_of_buffer);
  for (int i = 0; i < 3; i++) {
    w_ptr[i] = (uint8_t)9;
  }

  rb.commit_write(sz);

  sz = 10;
  end_of_buffer = false;
  r_ptr = rb.read_contig(&sz, &end_of_buffer);
  ASSERT_TRUE(r_ptr);
  ASSERT_EQ(sz, 1);
  ASSERT_TRUE(end_of_buffer);

  ASSERT_EQ((uint8_t)r_ptr[0], (uint8_t)9);

  rb.commit_read(sz);

  sz = 10;
  end_of_buffer = true;
  r_ptr = rb.read_contig(&sz, &end_of_buffer);
  ASSERT_TRUE(r_ptr);
  ASSERT_EQ(sz, 3);
  ASSERT_FALSE(end_of_buffer);

  for (int i = 0; i < 3; i++) {
    ASSERT_EQ((uint8_t)r_ptr[i], (uint8_t)9);
  }

  ASSERT_FALSE(rb.empty());
  rb.commit_read(sz);
  ASSERT_TRUE(rb.empty());
}
