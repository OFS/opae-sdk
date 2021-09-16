// Copyright(c) 2021, Intel Corporation
//
// Redistribution  and  use  in source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of  source code  must retain the  above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name  of Intel Corporation  nor the names of its contributors
//   may be used to  endorse or promote  products derived  from this  software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
// IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
// LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
// CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
// SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
// INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
// CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include "gtest/gtest.h"

#include <opae/mem_alloc.h>

#define ALIGNED(__addr, __size) ((__addr + __size - 1) & ~(__size - 1))

extern "C" {
struct mem_link *mem_link_alloc(uint64_t address, uint64_t size);
void mem_alloc_coalesce(struct mem_link *head, struct mem_link *l);
int mem_alloc_allocate_node(struct mem_alloc *m,
                            struct mem_link *node,
                            uint64_t *address,
                            uint64_t size);
int mem_alloc_allocate_split_node(struct mem_alloc *m,
				  struct mem_link *node,
				  uint64_t aligned_addr,
				  uint64_t *address,
				  uint64_t size);
int mem_alloc_free_node(struct mem_alloc *m,
                        struct mem_link *node);
}

/**
 * @test    init
 * @brief   Test: mem_alloc_init()
 * @details mem_alloc_init() correctly<br>
 *          initializes the given struct mem_alloc.
 */
TEST(mem_alloc, init)
{
  struct mem_alloc m;

  mem_alloc_init(&m);

  EXPECT_EQ(m.free.prev, &m.free);
  EXPECT_EQ(m.free.next, &m.free);

  EXPECT_EQ(m.allocated.prev, &m.allocated);
  EXPECT_EQ(m.allocated.next, &m.allocated);
}

/**
 * @test    destroy
 * @brief   Test: mem_alloc_destroy()
 * @details mem_alloc_destroy() correctly<br>
 *          releases the given struct mem_alloc.
 */
TEST(mem_alloc, destroy)
{
  struct mem_alloc m;

  struct mem_link *free_link = (struct mem_link *)
	  malloc(sizeof(struct mem_link));
  struct mem_link *allocated_link = (struct mem_link *)
	  malloc(sizeof(struct mem_link));

  ASSERT_NE(free_link, nullptr);
  ASSERT_NE(allocated_link, nullptr);

  m.free.prev = free_link;
  m.free.next = free_link;
  free_link->prev = &m.free;
  free_link->next = &m.free;

  m.allocated.prev = allocated_link;
  m.allocated.next = allocated_link;
  allocated_link->prev = &m.allocated;
  allocated_link->next = &m.allocated;

  mem_alloc_destroy(&m);

  EXPECT_EQ(m.free.prev, &m.free);
  EXPECT_EQ(m.free.next, &m.free);

  EXPECT_EQ(m.allocated.prev, &m.allocated);
  EXPECT_EQ(m.allocated.next, &m.allocated);
}

/**
 * @test    link_alloc
 * @brief   Test: mem_link_alloc()
 * @details mem_link_alloc() correctly<br>
 *          allocates and initializes a new<br>
 *          struct mem_link.
 */
TEST(mem_alloc, link_alloc)
{
  const uint64_t addr = 1UL;
  const uint64_t size = 2UL;
  struct mem_link *link = mem_link_alloc(addr, size);

  ASSERT_NE(link, nullptr);
  EXPECT_EQ(link->address, addr);
  EXPECT_EQ(link->size, size);
  EXPECT_EQ(link->prev, link);
  EXPECT_EQ(link->next, link);

  free(link);
}

/**
 * @test    coalesce0
 * @brief   Test: mem_alloc_coalesce()
 * @details When the l (2nd) parameter<br>
 *          is equal to the head (1st)<br>
 *          parameter, the fn simply returns.
 */
TEST(mem_alloc, coalesce0)
{
  const uint64_t addr = 1UL;
  const uint64_t size = 2UL;
  struct mem_link *link = mem_link_alloc(addr, size);

  ASSERT_NE(link, nullptr);
  EXPECT_EQ(link->address, addr);
  EXPECT_EQ(link->size, size);
  EXPECT_EQ(link->prev, link);
  EXPECT_EQ(link->next, link);

  mem_alloc_coalesce(link, link);

  EXPECT_EQ(link->address, addr);
  EXPECT_EQ(link->size, size);
  EXPECT_EQ(link->prev, link);
  EXPECT_EQ(link->next, link);

  free(link);
}

/**
 * @test    coalesce1
 * @brief   Test: mem_alloc_coalesce()
 * @details When the node prior to the l<br>
 *          (2nd) parameter in the list is not<br>
 *          the list head, and when the prior node<br>
 *          meets the criteria for coalescing, the fn<br>
 *          coalesces the previous node into the l node.
 */
TEST(mem_alloc, coalesce1)
{
  struct mem_link head;
  struct mem_link *zero = mem_link_alloc(0, 1024);
  struct mem_link *one = mem_link_alloc(1024, 1024);

  ASSERT_NE(zero, nullptr);
  ASSERT_NE(one, nullptr);

  head.next = zero;
  zero->prev = &head;
  
  zero->next = one;
  one->prev = zero;

  one->next = &head;
  head.prev = one;

  // head -> zero -> one
  mem_alloc_coalesce(&head, one);

  // head -> one
  // (zero has been freed)
  EXPECT_EQ(head.prev, one);
  EXPECT_EQ(head.next, one);
  EXPECT_EQ(one->prev, &head);
  EXPECT_EQ(one->next, &head);
  EXPECT_EQ(one->address, 0);
  EXPECT_EQ(one->size, 2048);

  free(one);
}

/**
 * @test    coalesce2
 * @brief   Test: mem_alloc_coalesce()
 * @details When the node just after the l<br>
 *          (2nd) parameter in the list is not<br>
 *          the list head, and when the next node<br>
 *          meets the criteria for coalescing, the fn<br>
 *          coalesces l into the next node.
 */
TEST(mem_alloc, coalesce2)
{
  struct mem_link head;
  struct mem_link *zero = mem_link_alloc(0, 1024);
  struct mem_link *one = mem_link_alloc(1024, 1024);

  ASSERT_NE(zero, nullptr);
  ASSERT_NE(one, nullptr);

  head.next = zero;
  zero->prev = &head;
  
  zero->next = one;
  one->prev = zero;

  one->next = &head;
  head.prev = one;

  // head -> zero -> one
  mem_alloc_coalesce(&head, zero);

  // head -> one
  // (zero has been freed)
 
  EXPECT_EQ(head.prev, one);
  EXPECT_EQ(head.next, one);
  EXPECT_EQ(one->prev, &head);
  EXPECT_EQ(one->next, &head);
  EXPECT_EQ(one->address, 0);
  EXPECT_EQ(one->size, 2048);

  free(one);
}

/**
 * @test    add_free0
 * @brief   Test: mem_alloc_add_free()
 * @details When the allocator's free list is empty,<br>
 *          the fn adds a single mem_link for the<br>
 *          given address and size, returning 0.
 */
TEST(mem_alloc, add_free0)
{
  struct mem_alloc allocator;
  struct mem_link *l;

  mem_alloc_init(&allocator);

  ASSERT_EQ(mem_alloc_add_free(&allocator, 0, 1024), 0);

  EXPECT_NE(allocator.free.prev, &allocator.free);
  EXPECT_NE(allocator.free.next, &allocator.free);

  l = allocator.free.next;
  EXPECT_EQ(l->prev, &allocator.free);
  EXPECT_EQ(l->next, &allocator.free);
  EXPECT_EQ(l->address, 0);
  EXPECT_EQ(l->size, 1024);

  free(l);
}

/**
 * @test    add_free1
 * @brief   Test: mem_alloc_add_free()
 * @details The fn maintains the free list in<br>
 *          ascending order, based on the address.<br>
 */
TEST(mem_alloc, add_free1)
{
  struct mem_alloc allocator;
  struct mem_link *l, *trash;
  const uint64_t size = 1024UL;

  mem_alloc_init(&allocator);

  // address: 0, 1024, 2048, 3072, 4096
  //          x           x           x
  ASSERT_EQ(mem_alloc_add_free(&allocator, 4096, size), 0);
  ASSERT_EQ(mem_alloc_add_free(&allocator, 0, size), 0);
  ASSERT_EQ(mem_alloc_add_free(&allocator, 2048, size), 0);

  EXPECT_NE(allocator.free.prev, &allocator.free);
  EXPECT_NE(allocator.free.next, &allocator.free);

  l = allocator.free.next;

  EXPECT_EQ(l->prev, &allocator.free);
  EXPECT_EQ(l->address, 0);
  EXPECT_EQ(l->size, size);

  trash = l;
  l = l->next;
  free(trash);

  EXPECT_EQ(l->prev, trash);
  EXPECT_EQ(l->address, 2048);
  EXPECT_EQ(l->size, size);

  trash =l;
  l = l->next;
  free(trash);

  EXPECT_EQ(l->prev, trash);
  EXPECT_EQ(l->next, &allocator.free);
  EXPECT_EQ(l->address, 4096);
  EXPECT_EQ(l->size, size);

  free(l);
}

/**
 * @test    add_free2
 * @brief   Test: mem_alloc_add_free()
 * @details The fn detects double-free's and<br>
 *          returns non-zero.
 */
TEST(mem_alloc, add_free2)
{
  struct mem_alloc allocator;
  struct mem_link *l;
  const uint64_t size = 1024UL;

  mem_alloc_init(&allocator);

  ASSERT_EQ(mem_alloc_add_free(&allocator, 4096, size), 0);
  EXPECT_NE(mem_alloc_add_free(&allocator, 4096, size), 0);

  l = allocator.free.next;
  free(l);
}

/**
 * @test    allocate_node0
 * @brief   Test: mem_alloc_allocate_node()
 * @details When the given node has a size that matches<br>
 *          the requested size, the node is unlinked<br>
 *          from the free list and added to the<br>
 *          allocated list.
 */
TEST(mem_alloc, allocate_node0)
{
  struct mem_alloc allocator;
  struct mem_link *l;
  const uint64_t size = 1024UL;
  uint64_t addr = 8192;

  mem_alloc_init(&allocator);

  EXPECT_EQ(mem_alloc_add_free(&allocator, 0, size), 0);
  EXPECT_EQ(mem_alloc_allocate_node(&allocator,
                                    allocator.free.next,
                                    &addr,
				    size), 0);

  EXPECT_EQ(allocator.free.prev, &allocator.free);
  EXPECT_EQ(allocator.free.next, &allocator.free);

  EXPECT_NE(allocator.allocated.prev, &allocator.allocated);
  EXPECT_NE(allocator.allocated.next, &allocator.allocated);

  l = allocator.allocated.next;
  EXPECT_EQ(l->prev, &allocator.allocated);
  EXPECT_EQ(l->next, &allocator.allocated);
  EXPECT_EQ(l->address, 0);
  EXPECT_EQ(l->size, size);

  free(l);
}

/**
 * @test    allocate_node1
 * @brief   Test: mem_alloc_allocate_node()
 * @details When the given node has a size that is<br>
 *          greater or equal to the requested size,<br>
 *          that node is adjusted to account for the allocation,<br>
 *          and a new node is allocated and added to the<br>
 *          allocated list.
 */
TEST(mem_alloc, allocate_node1)
{
  struct mem_alloc allocator;
  struct mem_link *l;
  const uint64_t size = 1024UL;
  uint64_t addr = 8192;

  mem_alloc_init(&allocator);

  EXPECT_EQ(mem_alloc_add_free(&allocator, 0, size), 0);

  l = allocator.free.next;
  EXPECT_EQ(mem_alloc_allocate_node(&allocator,
                                    allocator.free.next,
                                    &addr,
                                    512), 0);

  EXPECT_EQ(allocator.free.prev, l);
  EXPECT_EQ(allocator.free.next, l);
  EXPECT_EQ(l->prev, &allocator.free);
  EXPECT_EQ(l->next, &allocator.free);
  EXPECT_EQ(l->address, 512);
  EXPECT_EQ(l->size, 512);

  free(l);
  l = allocator.allocated.next;

  EXPECT_EQ(allocator.allocated.prev, l);
  EXPECT_EQ(allocator.allocated.next, l);
  EXPECT_EQ(l->prev, &allocator.allocated);
  EXPECT_EQ(l->next, &allocator.allocated);
  EXPECT_EQ(l->address, 0);
  EXPECT_EQ(l->size, 512);

  free(l);
}

/**
 * @test    alloc_split_node0
 * @brief   Test: mem_alloc_allocate_split_node()
 * @details When the given node has an aligned size that matches<br>
 *          the requested size, the node is readjusted to meet<br>
 *          the alignment requirement, and a new node is added to<br>
 *          the allocated list.
 */
TEST(mem_alloc, alloc_split_node0)
{
  struct mem_alloc allocator;
  struct mem_link *l;
  const uint64_t fourK = 4096UL;
  const uint64_t twoM = 2 * 1024UL * 1024UL;
  uint64_t addr = 0;

  mem_alloc_init(&allocator);

  EXPECT_EQ(mem_alloc_add_free(&allocator, 0x1000, (2 * twoM) - fourK), 0);

  l = allocator.free.next;
  EXPECT_EQ(mem_alloc_allocate_split_node(&allocator,
                                          allocator.free.next,
                                          ALIGNED(0x1000, twoM),
                                          &addr,
                                          twoM), 0);

  EXPECT_EQ(allocator.free.prev, l);
  EXPECT_EQ(allocator.free.next, l);
  EXPECT_EQ(l->prev, &allocator.free);
  EXPECT_EQ(l->next, &allocator.free);
  EXPECT_EQ(l->address, 0x1000);
  EXPECT_EQ(l->size, twoM - fourK);
  EXPECT_EQ(addr, twoM);

  free(l);
  l = allocator.allocated.next;

  EXPECT_EQ(allocator.allocated.prev, l);
  EXPECT_EQ(allocator.allocated.next, l);
  EXPECT_EQ(l->prev, &allocator.allocated);
  EXPECT_EQ(l->next, &allocator.allocated);
  EXPECT_EQ(l->address, twoM);
  EXPECT_EQ(l->size, twoM);

  free(l);
}

/**
 * @test    alloc_split_node1
 * @brief   Test: mem_alloc_allocate_split_node()
 * @details When the given node has an aligned size that is greater than<br>
 *          the requested size, the node is split into two allocated nodes<br>
 *          and the allocation is made from the aligned address.
 */
TEST(mem_alloc, alloc_split_node1)
{
  struct mem_alloc allocator;
  struct mem_link *l;
  struct mem_link *m;
  const uint64_t fourK = 4096UL;
  const uint64_t twoM = 2 * 1024UL * 1024UL;
  uint64_t addr = 0;

  mem_alloc_init(&allocator);

  EXPECT_EQ(mem_alloc_add_free(&allocator, 0x1000, (3 * twoM) - fourK), 0);

  l = allocator.free.next;
  EXPECT_EQ(mem_alloc_allocate_split_node(&allocator,
                                          allocator.free.next,
                                          ALIGNED(0x1000, twoM),
                                          &addr,
                                          twoM), 0);
  m = l->next;

  EXPECT_EQ(allocator.free.prev, m);
  EXPECT_EQ(allocator.free.next, l);
  EXPECT_EQ(l->prev, &allocator.free);
  EXPECT_EQ(l->next, m);
  EXPECT_EQ(m->prev, l);
  EXPECT_EQ(m->next, &allocator.free);
  EXPECT_EQ(l->address, 0x1000);
  EXPECT_EQ(l->size, twoM - fourK);
  EXPECT_EQ(m->address, 2 * twoM);
  EXPECT_EQ(m->size, twoM);
  EXPECT_EQ(addr, twoM);

  free(l);
  free(m);
  l = allocator.allocated.next;

  EXPECT_EQ(allocator.allocated.prev, l);
  EXPECT_EQ(allocator.allocated.next, l);
  EXPECT_EQ(l->prev, &allocator.allocated);
  EXPECT_EQ(l->next, &allocator.allocated);
  EXPECT_EQ(l->address, twoM);
  EXPECT_EQ(l->size, twoM);

  free(l);
}

/**
 * @test    get0
 * @brief   Test: mem_alloc_get()
 * @details The fn allocates from the free list<br>
 *          using a first fit algorithm.
 */
TEST(mem_alloc, get0)
{
  struct mem_alloc allocator;
  struct mem_link *l, *trash;
  const uint64_t size = 1024UL;
  uint64_t addr = 8192;

  mem_alloc_init(&allocator);

  EXPECT_EQ(mem_alloc_add_free(&allocator, 0, size), 0);
  EXPECT_EQ(mem_alloc_add_free(&allocator, 2048, size), 0);

  // address: 0, 1024, 2048
  //          x           x

  EXPECT_EQ(mem_alloc_get(&allocator, &addr, 512), 0);
  EXPECT_EQ(addr, 0);

  l = allocator.free.next;
  EXPECT_EQ(l->address, 512);
  EXPECT_EQ(l->size, 512);

  trash = l;
  l = l->next;
  free(trash);

  EXPECT_EQ(l->address, 2048);
  EXPECT_EQ(l->size, size);

  free(l);
  l = allocator.allocated.next;

  EXPECT_EQ(l->address, 0);
  EXPECT_EQ(l->size, 512);

  free(l);
}

/**
 * @test    get1
 * @brief   Test: mem_alloc_get()
 * @details When the free list has no block large<br>
 *          enough to satisfy the request,<br>
 *          the fn returns a non-zero value<br>
 *          to indicate the out-of-memory condition.
 */
TEST(mem_alloc, get1)
{
  struct mem_alloc allocator;
  const uint64_t size = 1024UL;
  uint64_t addr = 8192;

  mem_alloc_init(&allocator);

  EXPECT_EQ(mem_alloc_add_free(&allocator, 0, size), 0);

  EXPECT_NE(mem_alloc_get(&allocator, &addr, size * 2), 0);

  free(allocator.free.next);
}

/**
 * @test    free_node
 * @brief   Test: mem_alloc_free_node()
 * @details When the allocated list contains the<br>
 *          target node, that node is freed, and<br>
 *          the address and size are added back<br>
 *          to the free list in a new node.
 */
TEST(mem_alloc, free_node)
{
  struct mem_alloc allocator;
  const uint64_t addr = 0;
  const uint64_t size = 1024;
  struct mem_link *node = mem_link_alloc(addr, size);

  ASSERT_NE(node, nullptr);

  mem_alloc_init(&allocator);

  allocator.allocated.next = node;
  allocator.allocated.prev = node;
  node->prev = &allocator.allocated;
  node->next = &allocator.allocated;

  EXPECT_EQ(mem_alloc_free_node(&allocator, node), 0);
  EXPECT_EQ(allocator.allocated.prev, &allocator.allocated);
  EXPECT_EQ(allocator.allocated.next, &allocator.allocated);

  EXPECT_NE(allocator.free.prev, &allocator.free);
  EXPECT_NE(allocator.free.next, &allocator.free);

  node = allocator.free.next;
  EXPECT_EQ(node->prev, &allocator.free);
  EXPECT_EQ(node->next, &allocator.free);
  EXPECT_EQ(node->address, addr);
  EXPECT_EQ(node->size, size);

  free(node);
}

/**
 * @test    put0
 * @brief   Test: mem_alloc_put()
 * @details When the allocated list contains the<br>
 *          target address, that node is freed, and<br>
 *          the address and size are added back<br>
 *          to the free list in a new node.
 */
TEST(mem_alloc, put0)
{
  struct mem_alloc allocator;
  const uint64_t addr = 0;
  const uint64_t size = 1024;
  struct mem_link *node = mem_link_alloc(addr, size);

  ASSERT_NE(node, nullptr);

  mem_alloc_init(&allocator);

  allocator.allocated.next = node;
  allocator.allocated.prev = node;
  node->prev = &allocator.allocated;
  node->next = &allocator.allocated;

  EXPECT_EQ(mem_alloc_put(&allocator, addr), 0);

  EXPECT_EQ(allocator.allocated.prev, &allocator.allocated);
  EXPECT_EQ(allocator.allocated.next, &allocator.allocated);

  EXPECT_NE(allocator.free.prev, &allocator.free);
  EXPECT_NE(allocator.free.next, &allocator.free);

  node = allocator.free.next;

  EXPECT_EQ(node->prev, &allocator.free);
  EXPECT_EQ(node->next, &allocator.free);
  EXPECT_EQ(node->address, addr);
  EXPECT_EQ(node->size, size);

  free(node);
}

/**
 * @test    put1
 * @brief   Test: mem_alloc_put()
 * @details When the given address is not found<br>
 *          in the allocated list,<br>
 *          the fn returns non-zero to indicate<br>
 *          an error.
 */
TEST(mem_alloc, put1)
{
  struct mem_alloc allocator;
  const uint64_t addr = 0;
  const uint64_t size = 1024;
  struct mem_link *node = mem_link_alloc(addr, size);

  ASSERT_NE(node, nullptr);

  mem_alloc_init(&allocator);

  allocator.allocated.prev = node;
  allocator.allocated.next = node;
  node->prev = &allocator.allocated;
  node->next = &allocator.allocated;

  EXPECT_NE(mem_alloc_put(&allocator, 4096), 0);

  EXPECT_EQ(allocator.free.prev, &allocator.free);
  EXPECT_EQ(allocator.free.next, &allocator.free);

  EXPECT_EQ(allocator.allocated.prev, node);
  EXPECT_EQ(allocator.allocated.next, node);
  EXPECT_EQ(node->prev, &allocator.allocated);
  EXPECT_EQ(node->next, &allocator.allocated);

  free(node);
}
