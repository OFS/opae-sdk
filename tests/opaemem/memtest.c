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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include <opae/mem_alloc.h>

void test_insert_basic(void)
{
	struct mem_alloc m;
	struct mem_link *a;
	struct mem_link *b;

	mem_alloc_init(&m);

	mem_alloc_add_free(&m, 0x1000, 4096);
	a = m.free.next;
	assert(a);
	assert(a->address == 0x1000);
	assert(a->size == 4096);
	assert(a->prev == &m.free);
	assert(a->next == &m.free);
	assert(m.free.next == a);
	assert(m.free.prev == a);

	mem_alloc_add_free(&m, 0x0000, 4096);
	a = m.free.next;
	assert(a);
	assert(a->address == 0x0000);
	assert(a->size == 2 * 4096);
	assert(a->prev == &m.free);
	assert(a->next == &m.free);
	assert(m.free.next == a);
	assert(m.free.prev == a);

	mem_alloc_add_free(&m, 0x2000, 4096);
	a = m.free.next;
	assert(a);
	assert(a->address == 0x0000);
	assert(a->size == 3 * 4096);
	assert(a->prev == &m.free);
	assert(a->next == &m.free);
	assert(m.free.next == a);
	assert(m.free.prev == a);

	mem_alloc_destroy(&m);


	mem_alloc_add_free(&m, 0x0000, 4096);
	a = m.free.next;
	assert(a);
	assert(a->address == 0x0000);
	assert(a->size == 4096);
	assert(a->prev == &m.free);
	assert(a->next == &m.free);
	assert(m.free.next == a);
	assert(m.free.prev == a);

	mem_alloc_add_free(&m, 0x2000, 4096);
	a = m.free.next;
	b = m.free.prev;
	assert(a);
	assert(a->address == 0x0000);
	assert(a->size == 4096);
	assert(b->address == 0x2000);
	assert(b->size == 4096);
	assert(a->prev == &m.free);
	assert(a->next == b);
	assert(b->prev == a);
	assert(b->next == &m.free);
	assert(m.free.prev == b);
	assert(m.free.next == a);

	mem_alloc_add_free(&m, 0x1000, 4096);
	a = m.free.next;
	assert(a);
	assert(a->address == 0x0000);
	assert(a->size == 3 * 4096);
	assert(a->prev == &m.free);
	assert(a->next == &m.free);
	assert(m.free.next == a);
	assert(m.free.prev == a);

	mem_alloc_destroy(&m);


	mem_alloc_add_free(&m, 0x0000, 4096);
	a = m.free.next;
	assert(a);
	assert(a->address == 0x0000);
	assert(a->size == 4096);
	assert(a->prev == &m.free);
	assert(a->next == &m.free);
	assert(m.free.next == a);
	assert(m.free.prev == a);

	mem_alloc_add_free(&m, 0x1000, 4096);
	a = m.free.next;
	assert(a);
	assert(a->address == 0x0000);
	assert(a->size == 2 * 4096);
	assert(a->prev == &m.free);
	assert(a->next == &m.free);
	assert(m.free.next == a);
	assert(m.free.prev == a);

	mem_alloc_add_free(&m, 0x2000, 4096);
	a = m.free.next;
	assert(a);
	assert(a->address == 0x0000);
	assert(a->size == 3 * 4096);
	assert(a->prev == &m.free);
	assert(a->next == &m.free);
	assert(m.free.next == a);
	assert(m.free.prev == a);

	mem_alloc_destroy(&m);
}

void test_insert_stress(int iters)
{
	struct {
		uint64_t address;
		uint64_t size;
	} addresses[] = {
		{ 0x0000, 4096 },
		{ 0x1000, 4096 },
		{ 0x2000, 4096 },
		{ 0x3000, 4096 },
		{ 0x4000, 4096 },
		{ 0x5000, 4096 },
		{ 0x6000, 4096 },
		{ 0x7000, 4096 },
		{ 0x8000, 4096 },
		{ 0x9000, 4096 }
	};
	int addrs = sizeof(addresses) / sizeof(addresses[0]);

	int i;
	int j;
	int r;
	uint64_t temp;

	struct mem_alloc m;
	struct mem_link *a;

	mem_alloc_init(&m);

	for (i = 0 ; i < iters ; ++i) {

		for (j = addrs - 1 ; j > 0 ; --j) {
			r = rand() % j;
			temp = addresses[r].address;
			addresses[r].address = addresses[j].address;
			addresses[j].address = temp;
		}

		/*
		printf("----\n");
		for (j = 0 ; j < addrs ; ++j) {
			printf("0x%lx\n", addresses[j].address);
		}
		*/

		for (j = 0 ; j < addrs ; ++j) {
			//printf("addr: 0x%lx\n", addresses[j].address);
			assert(0 == mem_alloc_add_free(&m, addresses[j].address, addresses[j].size));
		}

		a = m.free.next;
		assert(a);
		assert(a->address == 0x0000);
		assert(a->size == 10 * 4096);
		assert(a->prev == &m.free);
		assert(a->next == &m.free);
		assert(m.free.next == a);
		assert(m.free.prev == a);

		mem_alloc_destroy(&m);
	}

}

void test_alloc_free_stress(int iters)
{
	struct {
		uint64_t address;
		uint64_t size;
	} addresses[] = {
		{ 0x0000, 4096 },
		{ 0x2000, 4096 },
		{ 0x4000, 4096 },
		{ 0x6000, 4096 },
		{ 0x8000, 4096 }
	};
	int addrs = sizeof(addresses) / sizeof(addresses[0]);

	int i;
	int j;
	int k;
	int r;

	struct mem_alloc m;
	struct mem_link *a;
	struct mem_link *b;

	uint64_t size = 1024;
	uint64_t allocated[addrs * 4];
	uint64_t temp;

	for (i = 0 ; i < iters ; ++i) {
		mem_alloc_init(&m);

		for (j = 0 ; j < addrs ; ++j) {
			assert(0 == mem_alloc_add_free(&m, addresses[j].address, addresses[j].size));
		}

		a = m.free.next;
		assert(a);
		assert(a->address == 0x0000);
		assert(a->size == 4096);
		assert(a->prev == &m.free);

		b = a->next;
		assert(b);
		assert(b->address == 0x2000);
		assert(b->size == 4096);
		assert(b->prev == a);

		a = b;
		b = b->next;
		assert(b);
		assert(b->address == 0x4000);
		assert(b->size == 4096);
		assert(b->prev == a);

		a = b;
		b = b->next;
		assert(b);
		assert(b->address == 0x6000);
		assert(b->size == 4096);
		assert(b->prev == a);

		a = b;
		b = b->next;
		assert(b);
		assert(b->address == 0x8000);
		assert(b->size == 4096);
		assert(b->prev == a);

		assert(b->next == &m.free);
		assert(m.free.prev == b);


		k = 0;
		while (0 == mem_alloc_get(&m, &allocated[k], size)) {
			++k;
		}

		assert(k == addrs * 4);

		assert(m.free.next == &m.free);
		assert(m.free.prev == &m.free);

		for (j = k - 1 ; j > 0 ; --j) {
			r = rand() % j;
			temp = allocated[r];
			allocated[r] = allocated[j];
			allocated[j] = temp;

			assert(0 == mem_alloc_put(&m, allocated[j]));
		}
		assert(0 == mem_alloc_put(&m, allocated[0]));

		assert(m.allocated.next == &m.allocated);
		assert(m.allocated.prev == &m.allocated);

		a = m.free.next;
		assert(a);
		assert(a->address == 0x0000);
		assert(a->size == 4096);
		assert(a->prev == &m.free);

		b = a->next;
		assert(b);
		assert(b->address == 0x2000);
		assert(b->size == 4096);
		assert(b->prev == a);

		a = b;
		b = b->next;
		assert(b);
		assert(b->address == 0x4000);
		assert(b->size == 4096);
		assert(b->prev == a);

		a = b;
		b = b->next;
		assert(b);
		assert(b->address == 0x6000);
		assert(b->size == 4096);
		assert(b->prev == a);

		a = b;
		b = b->next;
		assert(b);
		assert(b->address == 0x8000);
		assert(b->size == 4096);
		assert(b->prev == a);

		assert(b->next == &m.free);
		assert(m.free.prev == b);

		mem_alloc_destroy(&m);
	}
}

int main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	srand(time(NULL));

	test_insert_basic();
	test_insert_stress(10000000);
	test_alloc_free_stress(10000000);

	return 0;
}
