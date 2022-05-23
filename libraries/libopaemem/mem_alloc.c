// Copyright(c) 2020-2022, Intel Corporation
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <opae/mem_alloc.h>
#include "mock/opae_std.h"

#define __SHORT_FILE__                                    \
({                                                        \
	const char *file = __FILE__;                      \
	const char *p = file;                             \
	while (*p)                                        \
		++p;                                      \
	while ((p > file) && ('/' != *p) && ('\\' != *p)) \
		--p;                                      \
	if (p > file)                                     \
		++p;                                      \
	p;                                                \
})

#define ERR(format, ...)                               \
fprintf(stderr, "%s:%u:%s() **ERROR** [%s] : " format, \
	__SHORT_FILE__, __LINE__, __func__, strerror(errno), ##__VA_ARGS__)

#define ALIGNED(__addr, __size) ((__addr + __size - 1) & ~(__size - 1))

void mem_alloc_init(struct mem_alloc *m)
{
	m->free.address = 0;
	m->free.size = 0;
	m->free.prev = &m->free;
	m->free.next = &m->free;
	m->allocated.address = 0;
	m->allocated.size = 0;
	m->allocated.prev = &m->allocated;
	m->allocated.next = &m->allocated;
}

void mem_alloc_destroy(struct mem_alloc *m)
{
	struct mem_link *p;
	struct mem_link *trash;

	for (p = m->free.next ; p != &m->free ; ) {
		trash = p;
		p = p->next;
		opae_free(trash);
	}

	for (p = m->allocated.next ; p != &m->allocated ; ) {
		trash = p;
		p = p->next;
		opae_free(trash);
	}

	mem_alloc_init(m);
}

STATIC struct mem_link *mem_link_alloc(uint64_t address, uint64_t size)
{
	struct mem_link *m;
	m = opae_malloc(sizeof(struct mem_link));
	if (m) {
		m->address = address;
		m->size = size;
		m->prev = m;
		m->next = m;
	}
	return m;
}

static inline void link_before(struct mem_link *a, struct mem_link *b)
{
	a->prev = b->prev;
	a->next = b;
	b->prev = a;
	a->prev->next = a;
}

static inline void link_after(struct mem_link *a, struct mem_link *b)
{
	a->next = b->next;
	a->prev = b;
	b->next->prev = a;
	b->next = a;
}

static inline void link_unlink(struct mem_link *x)
{
	x->next->prev = x->prev;
	x->prev->next = x->next;
}

STATIC void mem_alloc_coalesce(struct mem_link *head,
			       struct mem_link *l)
{
	struct mem_link *prev = l->prev;
	struct mem_link *next = l->next;

	if (prev == head && next == head)
		return;

	if (prev != head) {
		if (prev->address + prev->size == l->address) {
			l->address = prev->address;
			l->size += prev->size;
			link_unlink(prev);
			opae_free(prev);
		}
	}

	next = l->next;
	if (next != head) {
		if (l->address + l->size == next->address) {
			next->address = l->address;
			next->size += l->size;
			link_unlink(l);
			opae_free(l);
		}
	}
}

int mem_alloc_add_free(struct mem_alloc *m, uint64_t address, uint64_t size)
{
	struct mem_link *node;
	struct mem_link *p;

	node = mem_link_alloc(address, size);
	if (!node) {
		ERR("malloc() failed\n");
		return 1;
	}

	for (p = m->free.next ; p != &m->free ; p = p->next) {
		if (address < p->address) {
			link_before(node, p);
			mem_alloc_coalesce(&m->free, node);
			return 0;
		} else if (address == p->address) {
			// double free
			opae_free(node);
			ERR("double free detected 0x%lx\n", address);
			return 2;
		} else {
			// address > p->address
			break;
		}

	}

	while ((address > p->address) && (p != &m->free)) {
		p = p->next;
	}

	link_before(node, p);
	mem_alloc_coalesce(&m->free, node);

	return 0;
}

STATIC int mem_alloc_allocate_node(struct mem_alloc *m,
				   struct mem_link *node,
				   uint64_t *address,
				   uint64_t size)
{
	struct mem_link *p;

	if (node->size == size) {
		// If we have an exact fit, recycle the node struct.
		link_unlink(node);
		link_before(node, &m->allocated);
		*address = node->address;
		return 0;
	}

	// node->size > size

	p = mem_link_alloc(node->address, size);
	if (!p) {
		ERR("malloc() failed\n");
		return 1;
	}

	node->address += size;
	node->size -= size;

	link_before(p, &m->allocated);
	*address = p->address;

	return 0;
}

STATIC int mem_alloc_allocate_split_node(struct mem_alloc *m,
					 struct mem_link *node,
					 uint64_t aligned_addr,
					 uint64_t *address,
					 uint64_t size)
{
	struct mem_link *p;
	struct mem_link *p2;
	uint64_t first_size;
	uint64_t second_size;
	uint64_t second_addr;

	if ((aligned_addr + size) == (node->address + node->size)) {
		// We're consuming from aligned_addr to the end of node.
		p = mem_link_alloc(aligned_addr, size);
		if (!p) {
			ERR("malloc() failed\n");
			return 1;
		}

		link_before(p, &m->allocated);
		*address = p->address;

		node->size -= size;

		return 0;
	}

	first_size = aligned_addr - node->address;
	second_size = node->size - (first_size + size);
	second_addr = aligned_addr + size;

	// We need to split node into three nodes:
	//
	// first_size         size              second_size
	// -----------------  ----------------  -----------------------
	// | node->address |  | aligned_addr |  | aligned_addr + size |
	// -----------------  ----------------  -----------------------

	p = mem_link_alloc(aligned_addr, size);
	if (!p) {
		ERR("malloc() failed\n");
		return 2;
	}

	p2 = mem_link_alloc(second_addr, second_size);
	if (!p2) {
		ERR("malloc() failed\n");
		opae_free(p);
		return 3;
	}

	node->size = first_size;

	link_before(p, &m->allocated);
	*address = p->address;

	link_after(p2, node);

	return 0;
}

int mem_alloc_get(struct mem_alloc *m, uint64_t *address, uint64_t size)
{
	struct mem_link *p;

	for (p = m->free.next ; p != &m->free ; p = p->next) {
		uint64_t aligned_addr = ALIGNED(p->address, size);
		if ((aligned_addr + size) <= (p->address + p->size)) { // First fit.
			if (aligned_addr == p->address)
				return mem_alloc_allocate_node(m,
							       p,
							       address,
							       size);
			else
				return mem_alloc_allocate_split_node(m,
								     p,
								     aligned_addr,
								     address,
								     size);
		}
	}

	ERR("no free block of sufficient size found\n");
	return 1; // Out of memory.
}

STATIC int mem_alloc_free_node(struct mem_alloc *m,
			       struct mem_link *node)
{
	uint64_t address;
	uint64_t size;

	address = node->address;
	size = node->size;

	link_unlink(node);
	opae_free(node);

	return mem_alloc_add_free(m, address, size);
}

int mem_alloc_put(struct mem_alloc *m, uint64_t address)
{
	struct mem_link *p;

	for (p = m->allocated.next ; p != &m->allocated ; p = p->next) {
		if (address == p->address) {
			return mem_alloc_free_node(m, p);
		}
	}

	ERR("attempt to free non-allocated 0x%lx\n", address);
	return 1; // Address not found.
}
