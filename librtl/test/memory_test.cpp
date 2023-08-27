
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

#include "memory.incl"
#include <cstring>
#include <iostream>

// Remove unused function warning
void impl_rtl_memory_void() {
  (void)blk_get_size(NULL);
  (void)blk_set_size(NULL, 0);
  (void)blk_is_free(NULL);
  (void)blk_set_free(NULL);
  (void)blk_set_busy(NULL);
  (void)blk_is_last(NULL);
  (void)blk_set_last(NULL);
  (void)blk_set_not_last(NULL);
  (void)blk_hdr_to_ptr(NULL);
  (void)ptr_to_blk_hdr(NULL);
  RTL_UWORD fli, sli;
  fli = 10;
  sli = 2;
  (void)find_suitable_block(NULL, &fli, &sli);
  (void)mapping_search(0, NULL, NULL);
}

#include "gtest/gtest.h"

class UniquePointerTests : public ::testing::Test {
 protected:
  // You can remove any or all of the following functions if their bodies would
  // be empty.

  UniquePointerTests() {
    // You can do set-up work for each test here.
  }

  ~UniquePointerTests() override {
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

TEST_F(UniquePointerTests, FlsTest) {
    ASSERT_EQ(rtl_fls32(0), 0);
    ASSERT_EQ(rtl_fls32(1), 0);
    ASSERT_EQ(rtl_fls32(0x80000000), 31);
    ASSERT_EQ(rtl_fls32(0x7FFFFFFF), 30);
    ASSERT_EQ(rtl_fls32(0x80008000), 31);
    ASSERT_EQ(rtl_fls32(74), 6);

    ASSERT_EQ(rtl_fls64(0), 0);
    ASSERT_EQ(rtl_fls64(1), 0);
    ASSERT_EQ(rtl_fls64(0x80000000), 31);
    ASSERT_EQ(rtl_fls64((uint64_t)0x0000000080000000), 31);
    ASSERT_EQ(rtl_fls64(0x7FFFFFFF), 30);
    ASSERT_EQ(rtl_fls64(0x80008000), 31);
    ASSERT_EQ(rtl_fls64(74), 6);

    ASSERT_EQ(rtl_fls64(0x8000000080000000), 63);
    ASSERT_EQ(rtl_fls64(0x0800000080000000), 59);
    ASSERT_EQ(rtl_fls64(0x7FFFFFFF7FFFFFFF), 62);
    ASSERT_EQ(rtl_fls64(0x8000800080008000), 63);
}

TEST_F(UniquePointerTests, FfsTest) {

    ASSERT_EQ(rtl_ffs32((uint16_t)0x8000), 15);
    ASSERT_EQ(rtl_ffs32((uint16_t)0xFFFF), 0);

    ASSERT_EQ(rtl_ffs32(0), 0);
    ASSERT_EQ(rtl_ffs32(1), 0);
    ASSERT_EQ(rtl_ffs32(0x80000000), 31);
    ASSERT_EQ(rtl_ffs32(0x7FFFFFFF), 0);
    ASSERT_EQ(rtl_ffs32(0x80008000), 15);

    ASSERT_EQ(rtl_ffs64(0), 0);
    ASSERT_EQ(rtl_ffs64(1), 0);
    ASSERT_EQ(rtl_ffs64(0x80000000), 31);
    ASSERT_EQ(rtl_ffs64(0x7FFFFFFF), 0);
    ASSERT_EQ(rtl_ffs64(0x80008000), 15);

    ASSERT_EQ(rtl_ffs64(0x8000000080000000), 31);
    ASSERT_EQ(rtl_ffs64(0x7FFFFFFF7FFFFFFF), 0);
    ASSERT_EQ(rtl_ffs64(0x8000800080008000), 15);

    ASSERT_EQ(rtl_ffs64(0x8000000080000000), 31);
    ASSERT_EQ(rtl_ffs64(0x0800000080000000), 31);
}


TEST_F(UniquePointerTests, BlockHeaderSetSizeTest) {
  std::cout << "Word Size in Bytes: " << WORD_SIZE_BYTES << std::endl;
  tlsf_blk_hdr blk_hdr;
  blk_set_size(&blk_hdr, 136);

  blk_set_free(&blk_hdr);
  blk_set_busy(&blk_hdr);
  blk_set_free(&blk_hdr);

  blk_set_last(&blk_hdr);

  ASSERT_EQ(blk_get_size(&blk_hdr), 136);
  blk_set_size(&blk_hdr, 48);
  ASSERT_EQ(blk_get_size(&blk_hdr), 48);

  blk_set_size(&blk_hdr, MAXIMUM_BLOCK_SIZE);
  ASSERT_EQ(blk_get_size(&blk_hdr), MAXIMUM_BLOCK_SIZE);

  ASSERT_TRUE(blk_is_free(&blk_hdr));
  ASSERT_TRUE(blk_is_last(&blk_hdr));

  blk_set_busy(&blk_hdr);
  blk_set_not_last(&blk_hdr);

  ASSERT_FALSE(blk_is_free(&blk_hdr));
  ASSERT_FALSE(blk_is_last(&blk_hdr));

#if RTL_WORD_SIZE_BYTES == 2
  ASSERT_EQ(MAXIMUM_FLI, 14);
  ASSERT_EQ(MINIMUM_FLI, 6);
  ASSERT_EQ(MINIMUM_FLI_ALLOCATION, 64);
  ASSERT_EQ(FLI_COUNT, 9);
  ASSERT_EQ(FLI_SHIFT_VAL, 5);
  ASSERT_EQ(WORD_SIZE_BYTES, 4);  // Not a typo, should be 4

#elif RTL_WORD_SIZE_BYTES == 4
  ASSERT_EQ(MAXIMUM_FLI, 30);
  ASSERT_EQ(MINIMUM_FLI, 7);
  ASSERT_EQ(MINIMUM_FLI_ALLOCATION, 128);
  ASSERT_EQ(FLI_COUNT, 24);
  ASSERT_EQ(FLI_SHIFT_VAL, 6);
  ASSERT_EQ( WORD_SIZE_BYTES, 4);

#elif RTL_WORD_SIZE_BYTES == 8
  ASSERT_EQ(MAXIMUM_FLI, 62);
  ASSERT_EQ(MINIMUM_FLI, 8);
  ASSERT_EQ(MINIMUM_FLI_ALLOCATION, 256);
  ASSERT_EQ(FLI_COUNT, 55);
  ASSERT_EQ(FLI_SHIFT_VAL, 7);
  ASSERT_EQ(WORD_SIZE_BYTES, 8);
#endif
}

TEST_F(UniquePointerTests, MappingTests) {
  RTL_UWORD fli, sli;

  mapping_insert(2056, &fli, &sli);
  ASSERT_EQ(fli, 11);
  ASSERT_EQ(sli, 0);

  mapping_search(2056, &fli, &sli);
  ASSERT_EQ(fli, 11);
  ASSERT_EQ(sli, 1);

  mapping_insert(8, &fli, &sli);
  ASSERT_EQ(fli, MINIMUM_FLI - 1);
  ASSERT_EQ(sli, 8 / WORD_SIZE_BYTES);
}

TEST_F(UniquePointerTests, SplitMergeTest) {

    const RTL_UWORD sz = 1024;
    char buf[sz];

    for (RTL_UWORD i = 0; i < sz; i++) {
        buf[i] = (char)0x33;
    }

    struct rtl_tlsf_arena arena;

    tlsf_blk_hdr * blk_hdr = (tlsf_blk_hdr *)buf;
    blk_set_size(blk_hdr, 200);
    blk_hdr->prev_physical_block = NULL;
    blk_hdr->next_free = NULL;
    blk_hdr->prev_free = NULL;

    blk_set_free(blk_hdr);
    blk_set_last(blk_hdr);

    tlsf_blk_hdr * next_hdr = split_blk(blk_hdr, 136);
    blk_set_free(next_hdr);

    ASSERT_EQ(next_hdr->prev_physical_block, blk_hdr);
    ASSERT_EQ(blk_get_size(blk_hdr), 136);
    ASSERT_EQ(blk_get_size(next_hdr), 64);
    ASSERT_TRUE(blk_is_last(next_hdr));
    ASSERT_FALSE(blk_is_last(blk_hdr));

    tlsf_blk_hdr * next_next_hdr = split_blk(next_hdr, 16);
    blk_set_free(next_next_hdr);

    ASSERT_EQ(blk_get_size(next_hdr), 16);
    ASSERT_EQ(blk_get_size(next_next_hdr), 48);
    ASSERT_TRUE(blk_is_last(next_next_hdr));
    ASSERT_FALSE(blk_is_last(next_hdr));
    ASSERT_FALSE(blk_is_last(blk_hdr));

    ASSERT_TRUE(blk_is_free(blk_hdr));
    ASSERT_TRUE(blk_is_free(next_hdr));
    ASSERT_TRUE(blk_is_free(next_next_hdr));

    ASSERT_TRUE(next_next_hdr->prev_physical_block == next_hdr);
    ASSERT_TRUE(next_hdr->prev_physical_block == blk_hdr);
    ASSERT_TRUE(NEXT_BLK(blk_hdr) == next_hdr);
    ASSERT_TRUE(NEXT_BLK(next_hdr) == next_next_hdr);


    // Check forward and back

   tlsf_blk_hdr * merge1 = merge_prev(&arena, next_hdr);

   ASSERT_EQ(blk_get_size(merge1), 152);
   ASSERT_TRUE(merge1 == blk_hdr);
   ASSERT_FALSE(blk_is_last(merge1));
   ASSERT_TRUE(blk_is_last(next_next_hdr));

   ASSERT_TRUE(NEXT_BLK(merge1) == next_next_hdr);
   ASSERT_TRUE(NEXT_BLK(merge1)->prev_physical_block == merge1);
   ASSERT_TRUE(next_next_hdr->prev_physical_block == merge1);

   //

   ASSERT_TRUE(merge1 == merge_prev(&arena, merge1));
   ASSERT_TRUE(next_next_hdr == merge_next(&arena, next_next_hdr));

   merge1 = merge_next(&arena, merge1);
    ASSERT_EQ(blk_get_size(merge1), 200);
    ASSERT_TRUE(merge1 == blk_hdr);
    ASSERT_TRUE(blk_is_last(merge1));
    ASSERT_FALSE(blk_is_last(next_next_hdr));


}

TEST_F(UniquePointerTests, ArenaSmokeTest) {
  struct rtl_tlsf_arena* arena{nullptr};

  char arena_buf[sizeof(rtl_tlsf_arena)];

  const RTL_UWORD sz = 16384;

  char* buf = new char[sz];
  //  char buf[sz];

  for (RTL_UWORD i = 0; i < sz; i++) {
    buf[i] = (char)0x33;
  }

  ASSERT_EQ(rtl_tlsf_make_arena(&arena, buf, sz), 0);

  std::memcpy(arena_buf, arena, sizeof(rtl_tlsf_arena));

  int* ptr = (int*)rtl_tlsf_alloc(arena, sizeof(int));
  tlsf_blk_hdr* pptr = ptr_to_blk_hdr(ptr);

  *ptr = 0x66666666;

  int* ptr1 = (int*)rtl_tlsf_alloc(arena, sizeof(int));
  tlsf_blk_hdr* pptr1 = ptr_to_blk_hdr(ptr1);

  *ptr1 = 0x77777777;

  struct test_val {
    char arr[81];
  };

  int* ptr2 = (int*)rtl_tlsf_alloc(arena, sizeof(int));
  tlsf_blk_hdr* pptr2 = ptr_to_blk_hdr(ptr2);

  *ptr2 = 0x88888888;

  test_val* ptr3 = (test_val*)rtl_tlsf_alloc(arena, sizeof(test_val));
  tlsf_blk_hdr* pptr3 = ptr_to_blk_hdr(ptr3);

  for (int i = 0; i < 81; i++) {
    ptr3->arr[i] = 0x44;
  }

  ASSERT_EQ(pptr->prev_physical_block, (tlsf_blk_hdr*)NULL);
  ASSERT_EQ(pptr1->prev_physical_block, pptr);
  ASSERT_EQ(pptr3->prev_physical_block, pptr2);

  ASSERT_EQ((bool)blk_is_free(pptr3), false);
  ASSERT_EQ((bool)blk_is_last(pptr3), false);

  RTL_UWORD pptr_sz = blk_get_size(pptr3);

  tlsf_blk_hdr* last_ptr = (tlsf_blk_hdr*)((unsigned char*)pptr3 + pptr_sz);

  ASSERT_EQ((bool)blk_is_free(last_ptr), true);
  ASSERT_EQ((bool)blk_is_last(last_ptr), true);
  ASSERT_EQ(last_ptr->prev_physical_block, pptr3);

  ASSERT_EQ(pptr3->prev_physical_block, pptr2);

  rtl_tlsf_free(arena, ptr2);

  ASSERT_EQ(pptr3->prev_physical_block, pptr2);

  rtl_tlsf_free(arena, ptr1);

  ASSERT_EQ(pptr3->prev_physical_block, pptr1);

  rtl_tlsf_free(arena, ptr);

  ASSERT_EQ(pptr3->prev_physical_block, pptr);

  rtl_tlsf_free(arena, ptr3);

  for (size_t i = 0; i < sizeof(rtl_tlsf_arena); i++) {
    ASSERT_EQ((char)arena_buf[i], *((char*)arena + i));
  }

  delete[] buf;
}

TEST_F(UniquePointerTests, temptest) {
    ASSERT_TRUE(safe_to_cast_to_rtl_uword(4294967295));
}
