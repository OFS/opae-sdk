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
 * Simulator/application shared buffer management.  These methods are
 * called only by the simulator side.
 */

#include "ase_common.h"

// ---------------------------------------------------------------
// ASE graceful shutdown - Called if: error() occurs
// Deallocate & Unlink all shared memories and message queues
// ---------------------------------------------------------------
void ase_shmem_perror_teardown(char *msg, int ase_err)
{
	FUNC_CALL_ENTRY;

	if (msg != NULL)
		ase_error_report(msg, errno, ase_err);
	self_destruct_in_progress = 1;

	ase_shmem_destroy();

	start_simkill_countdown();
	FUNC_CALL_EXIT;
}


// --------------------------------------------------------------------
// DPI ALLOC buffer action - Allocate buffer action inside DPI
// Receive buffer_t pointer with memsize, memname and index populated
// Calculate fd, pbase and fake_paddr
// --------------------------------------------------------------------
void ase_shmem_alloc_action(struct buffer_t *mem)
{
	FUNC_CALL_ENTRY;

	struct buffer_t *new_buf;
	int fd_alloc;

	ASE_DBG("SIM-C : Adding a new buffer \"%s\"...\n", mem->memname);

	// Obtain a file descriptor
	fd_alloc = shm_open(mem->memname, O_RDWR, S_IRUSR | S_IWUSR);
	if (fd_alloc < 0) {
		ase_shmem_perror_teardown("shm_open", ASE_OS_SHM_ERR);
	} else {
		// Add to IPC list
		add_to_ipc_list("SHM", mem->memname);

		// Mmap to pbase, find one with unique low 38 bit
		mem->pbase =
		    (uintptr_t) mmap(NULL, mem->memsize,
				     PROT_READ | PROT_WRITE, MAP_SHARED,
				     fd_alloc, 0);
		if (mem->pbase == 0)
			ase_shmem_perror_teardown("mmap", ASE_OS_MEMMAP_ERR);

		if (ftruncate(fd_alloc, (off_t) mem->memsize) != 0) {
			ase_error_report("ftruncate", errno,
					 ASE_OS_SHM_ERR);
			ASE_MSG("Running ftruncate to %d bytes\n",
				(off_t) mem->memsize);
		}
		close(fd_alloc);

		// Received buffer is valid
		mem->valid = ASE_BUFFER_VALID;

		// Create a buffer and store the information
		new_buf = (struct buffer_t *) ase_malloc(BUFSIZE);
		ase_memcpy(new_buf, mem, BUFSIZE);

		// Append to linked list
		ll_append_buffer(new_buf);
#ifdef ASE_LL_VIEW
		ll_traverse_print();
#endif

		// Convert buffer_t to string
		mqueue_send(sim2app_alloc_tx, (char *) mem,
			    sizeof(struct buffer_t));

		// If memtest is enabled
#ifdef ASE_MEMTEST_ENABLE
		ase_dbg_memtest(mem);
#endif

#ifdef ASE_DEBUG
		if (fp_pagetable_log != NULL) {
			if (mem->index % 20 == 0) {
				fprintf(fp_pagetable_log,
					"Index\tAppVBase\tASEVBase\tBufsize\tBufname\t\tPhysBase\n");
			}

			fprintf(fp_pagetable_log,
				"%d\t0x%" PRIx64 "\t0x%" PRIx64
				"\t%x\t%s\t\t0x%" PRIx64 "\n", mem->index,
				mem->vbase, mem->pbase, mem->memsize,
				mem->memname, mem->fake_paddr);
		}
#endif
	}

	FUNC_CALL_EXIT;
}

// --------------------------------------------------------------------
// DPI dealloc buffer action - Deallocate buffer action inside DPI
// Receive index and invalidates buffer
// --------------------------------------------------------------------
void ase_shmem_dealloc_action(struct buffer_t *buf, int mq_enable)
{
	FUNC_CALL_ENTRY;

	char buf_str[ASE_MQ_MSGSIZE];
	ase_memset(buf_str, 0, ASE_MQ_MSGSIZE);

	// Traversal pointer
	struct buffer_t *dealloc_ptr;

	// Search buffer and Invalidate
	dealloc_ptr = ll_search_buffer(buf->index);

	//  If deallocate returns a NULL, dont get hosed
	if (dealloc_ptr == NULL) {
		ASE_INFO_2
		    ("NULL deallocation request received ... ignoring.\n");
	} else {
		ASE_INFO_2("Request to deallocate \"%s\" ...\n",
			   dealloc_ptr->memname);
		// Mark buffer as invalid & deallocate
		dealloc_ptr->valid = ASE_BUFFER_INVALID;
		munmap((void *) (uintptr_t) dealloc_ptr->pbase,
		       (size_t) dealloc_ptr->memsize);
		shm_unlink(dealloc_ptr->memname);
		// Respond back
		ll_remove_buffer(dealloc_ptr);
		ase_memcpy(buf_str, dealloc_ptr, sizeof(struct buffer_t));
		// If Buffer removal is requested by APP, send back notice, else no response
		if (mq_enable == 1) {
			mqueue_send(sim2app_dealloc_tx, buf_str,
				    ASE_MQ_MSGSIZE);
		}
#ifdef ASE_LL_VIEW
		ll_traverse_print();
#endif
	}

	FUNC_CALL_EXIT;
}


// --------------------------------------------------------------------
// ase_shmem_destroy : Destroy everything, called before exiting OR to
// reset simulation environment
//
// OPERATION:
// Traverse trough linked list
// - Remove each shared memory region
// - Remove each buffer_t
// --------------------------------------------------------------------
void ase_shmem_destroy(void)
{
	FUNC_CALL_ENTRY;

#ifdef ASE_DEBUG
	char str[256];
	snprintf(str, 256, "ASE destroy called");
	buffer_msg_inject(1, str);
#endif

	struct buffer_t *ptr;

	ptr = head;
	if (head != NULL) {
		while (ptr != (struct buffer_t *) NULL) {
			ase_shmem_dealloc_action(ptr, 0);
			ptr = ptr->next;
		}
	}

	FUNC_CALL_EXIT;
}
