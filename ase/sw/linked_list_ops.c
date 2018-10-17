// Copyright(c) 2014-2017, Intel Corporation
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
// **************************************************************************
/*
 * Module Info: Linked List memory buffer controls
 * Language   : C/C++
 * Owner      : Rahul R Sharma
 *              rahul.r.sharma@intel.com
 *              Intel Corporation
 *
 */

#include "ase_common.h"

struct buffer_t *head;
struct buffer_t *end;

/*
 * ll_print_info: Print linked list node info
 * Thu Oct  2 15:50:06 PDT 2014 : Modified for cleanliness
 * Prints buffer info
 */
void ll_print_info(struct buffer_t *print_ptr)
{
	FUNC_CALL_ENTRY;

	ASE_MSG("%d  ", print_ptr->index);
	ASE_MSG("%5s \t", print_ptr->memname);
	ASE_MSG("%5s  ",
		(print_ptr->valid ==
		 ASE_BUFFER_VALID) ? "VALID" : "INVLD");
	ASE_MSG("0x%" PRIx64 "  ", print_ptr->vbase);
	ASE_MSG("0x%" PRIx64 "  ", print_ptr->pbase);
	ASE_MSG("0x%" PRIx64 "  ", print_ptr->fake_paddr);
	ASE_MSG("0x%" PRIx32 "  ", print_ptr->memsize);
	ASE_MSG("\n");

	FUNC_CALL_EXIT;
}


// ---------------------------------------------------------------
// ll_traverse_print: Traverse and print linked list data
// ---------------------------------------------------------------
void ll_traverse_print(void)
{
	FUNC_CALL_ENTRY;
	struct buffer_t *traverse_ptr;

	ASE_MSG("Starting linked list traversal from 'head'..\n");
	traverse_ptr = head;
	while (traverse_ptr != NULL) {
		ll_print_info(traverse_ptr);
		traverse_ptr = traverse_ptr->next;
	}

	FUNC_CALL_EXIT;
}


// --------------------------------------------------------------------
// ll_append_buffer :  Append a buffer to linked list
// A buffer must be allocated before this function is called
// --------------------------------------------------------------------
void ll_append_buffer(struct buffer_t *pbuf)
{
	FUNC_CALL_ENTRY;

	// If there are no nodes in the list, set the new buffer as head
	if (head == NULL) {
		head = pbuf;
		end = pbuf;
	}
	// Link the new new node to the end of the list
	end->next = pbuf;
	// Set the next field as NULL
	pbuf->next = NULL;
	// Adjust end to point to last node
	end = pbuf;

	FUNC_CALL_EXIT;
}


// --------------------------------------------------------------------
// ll_remove_buffer : Remove a buffer (relink remaining)
// Use ll_search_buffer() to pin-point the deletion target first.
// --------------------------------------------------------------------
void ll_remove_buffer(struct buffer_t *ptr)
{
	FUNC_CALL_ENTRY;

	struct buffer_t *temp, *prev;
	// node to be deleted
	temp = ptr;

	// Reset linked list traversal
	prev = head;

	// If first node is to be deleted
	if (temp == head) {
		// Move head to next node
		head = head->next;
		// If there is only one node in the linked list
		if (end == temp)
			end = end->next;
	} else {            // If not the first node
		// Traverse until node is found
		while (prev->next != temp) {
			prev = prev->next;
		}
		// Link previous node to next node
		prev->next = temp->next;
		// If this is the end node, reset the end pointer
		if (end == temp)
			end = prev;
	}

	FUNC_CALL_EXIT;
}


// --------------------------------------------------------------------
// search_buffer_ll : Search buffer by ID
// Pass the head of the linked list along when calling
// --------------------------------------------------------------------
struct buffer_t *ll_search_buffer(int search_index)
{
	FUNC_CALL_ENTRY;

	struct buffer_t *search_ptr;

	// Start searching from the head
	search_ptr = head;

	// Traverse linked list starting from head
	if (search_ptr != NULL) {
		while (search_ptr->index != search_index) {
			search_ptr = search_ptr->next;
			if (search_ptr == NULL)
				break;
		}
	}
	// When found, return pointer to buffer
	if (search_ptr != NULL) {
		if (search_index == (int) search_ptr->index)
			return search_ptr;
		else
			return (struct buffer_t *) NULL;
	} else
		return (struct buffer_t *) NULL;

	FUNC_CALL_EXIT;
}
