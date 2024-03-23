/**
 * Copyright [2015] Tianfu Ma (matianfu@gmail.com)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * File: buddy.h
 *
 * Created on: Jun 5, 2015
 * Author: Tianfu Ma (matianfu@gmail.com)
 */

#include "os.h"
extern uint32_t HEAP_START;
extern uint32_t HEAP_SIZE;
/******************************************************************************
 *
 * Definitions
 *
 ******************************************************************************/
#define MAX_ORDER       10
#define MIN_ORDER       4   // 2 ** 4 == 16 bytes
#define NULL    0

/* the order ranges 0..MAX_ORDER, the largest memblock is 2**(MAX_ORDER) */
// #define POOLSIZE        ((1 << MAX_ORDER) + 64)
#define POOLSIZE  HEAP_SIZE  // todo: 还要减去freelist占用的空间

/* blocks are of size 2**i. */
#define BLOCKSIZE(i)    (1 << (i))

/* the address of the buddy of a block from freelists[i]. */
typedef unsigned long int	uintptr_t;
typedef unsigned char uint8_t;

#define _MEMBASE        ((uintptr_t)BUDDY->pool)
#define _OFFSET(b)      ((uintptr_t)b - _MEMBASE)
#define _BUDDYOF(b, i)  (_OFFSET(b) ^ (1 << (i)))
#define BUDDYOF(b, i)   ((pointer*)( _BUDDYOF(b, i) + _MEMBASE))

// not used yet, for higher order memory alignment
#define ROUND4(x)       ((x % 4) ? (x / 4 + 1) * 4 : x)

/******************************************************************************
 *
 * Types & Globals
 *
 ******************************************************************************/

// typedef void * pointer; /* used for untyped pointers */
typedef struct pointer{
  struct pointer* next;
}pointer;

typedef struct buddy {
  pointer* freelist[MAX_ORDER + 2];  // one more slot for first block in pool
  uint8_t pool;
} buddy_t;


buddy_t * BUDDY = 0;

pointer* bmalloc(int size) {

  int i, order;
  pointer* block;
  pointer* buddy;

  // calculate minimal order for this size
  i = 0;
  while (BLOCKSIZE(i) < size + 1) // one more byte for storing order
    i++;

  order = i = (i < MIN_ORDER) ? MIN_ORDER : i;

  // level up until non-null list found
  for (;; i++) {
    if (i > MAX_ORDER)
      return NULL;
    if (BUDDY->freelist[i])
      break;
  }

  // remove the block out of list
  block = BUDDY->freelist[i];
  BUDDY->freelist[i] = BUDDY->freelist[i] -> next;

  // split until i == order
  while (i-- > order) {
    buddy = BUDDYOF(block, i);
    BUDDY->freelist[i] = buddy;
  }

  // store order in previous byte
  *((uint8_t*) (block - 1)) = order;
  return block;
}

void bfree(pointer* block) {

  int i;
  pointer* buddy;
  pointer** p;

  // fetch order in previous byte
  i = *((uint8_t*) (block - 1));

  for (;; i++) {
    // calculate buddy
    buddy = BUDDYOF(block, i);
    p = &(BUDDY->freelist[i]);

    // find buddy in list
    while ((*p != NULL) && (*p != buddy))
      p = (pointer **) *p;

    // not found, insert into list
    if (*p != buddy) {
      ((pointer*) block) -> next = BUDDY->freelist[i];
      BUDDY->freelist[i] = block;
      return;
    }
    // found, merged block starts from the lower one
    block = (block < buddy) ? block : buddy;
    // remove buddy out of list
    *p = ((pointer*) *p)->next;
  }
}


int is_power_of_two(unsigned int n) {
    return n != 0 && (n & (n - 1)) == 0;
}

// Helper function to find the largest power of 2 less than n
static int find_largest_power_of_two_less_than(int n) {
  int k = 1;
  while ((1 << k) < n) k++;
  return k - 1;
}


void buddy_init(buddy_t * buddy) {
  buddy->pool = sizeof(buddy->freelist[MAX_ORDER + 2]) + HEAP_START;
  BUDDY = buddy;
  // memset(buddy, 0, sizeof(buddy_t)); // Adjust the size dynamically based on POOLSIZE
  if(is_power_of_two(POOLSIZE)){
    BUDDY->freelist[MAX_ORDER] = (pointer *)BUDDY->pool;
    return;
  }
  int i;

  // Find the largest block size that is less than or equal to POOLSIZE
  int largest_block_order = find_largest_power_of_two_less_than(POOLSIZE);
  int largest_block_size = 1 << largest_block_order;

  // Initialize the freelist with the largest block
  buddy->freelist[largest_block_order] = (pointer *)buddy->pool;

  // Calculate the remaining memory after the largest block is allocated
  int remaining_memory = POOLSIZE - largest_block_size;

  // Split the remaining memory into smaller blocks and add them to the freelist
  for (i = largest_block_order - 1; i >= MIN_ORDER && remaining_memory > 0; --i) {
    int current_block_size = 1 << i;
    while (remaining_memory >= current_block_size) {
      pointer* block = (pointer*)((uintptr_t)buddy->pool + POOLSIZE - remaining_memory);
      block->next = buddy->freelist[i];
      buddy->freelist[i] = block;
      remaining_memory -= current_block_size;
    }
  }
}


void buddy_deinit() {

  BUDDY = 0;
}

/*
 * The following functions are for simple tests.
 */

static int count_blocks(int i) {

  int count = 0;
  pointer** p = &(BUDDY->freelist[i]);

  while (*p != NULL) {
    count++;
    p = (pointer**) *p;
  }
  return count;
}

static int total_free() {

  int i, bytecount = 0;

  for (i = 0; i <= MAX_ORDER; i++) {
    bytecount += count_blocks(i) * BLOCKSIZE(i);
  }
  return bytecount;
}

static void print_list(int i) {

  printf("freelist[%d]: \n", i);

  pointer**p = &BUDDY->freelist[i];
  while (*p != NULL) {
    printf("    绝对地址：0x%08lx, 相对地址：0x%08lx\n", (uintptr_t) *p, (uintptr_t) *p - (uintptr_t) BUDDY->pool);
    p = (pointer**) *p;
  }
}

void print_buddy() {

  int i;

  printf("========================================\n");
  printf("MEMPOOL size: %d\n", POOLSIZE);
  printf("MEMPOOL start @ 0x%08x\n", (unsigned int) (uintptr_t) BUDDY->pool);
  printf("total free: %d\n", total_free());

  for (i = 0; i <= MAX_ORDER; i++) {
    print_list(i);
  }
}

void bmalloc_test() {
  // buddy_t * buddy = (buddy_t*) malloc(sizeof(buddy_t));
  buddy_t* buddy = (void *)(HEAP_START);
  buddy_init(buddy);
  print_buddy();
  void * p1, *p2;
  p1 = bmalloc(64);
  p2 = bmalloc(13);

//   bfree(p2);
  bfree(p1);

//   p2 = bmalloc(45);
//   p2 = bmalloc(13);
//   p2 = bmalloc(13);
  print_buddy();
}
