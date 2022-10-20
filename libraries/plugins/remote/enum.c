// Copyright(c) 2022, Intel Corporation
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

#include <stdlib.h>
#include <string.h>
//#include <errno.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <dirent.h>
//#include <fcntl.h>
//#include <unistd.h>

//#include "xfpga.h"
//#include "common_int.h"
//#include "error_int.h"
//#include "props.h"
//#include "opae_drv.h"

#include <opae/types.h>
#include <opae/log.h>

#include "mock/opae_std.h"


fpga_result __REMOTE_API__ remote_fpgaEnumerate(const fpga_properties *filters,
				       uint32_t num_filters, fpga_token *tokens,
				       uint32_t max_tokens,
				       uint32_t *num_matches)
{
	fpga_result result = FPGA_NOT_FOUND;

	if (NULL == num_matches) {
		OPAE_MSG("num_matches is NULL");
		return FPGA_INVALID_PARAM;
	}

	/* requiring a max number of tokens, but not providing a pointer to
	 * return them through is invalid */
	if ((max_tokens > 0) && (NULL == tokens)) {
		OPAE_MSG("max_tokens > 0 with NULL tokens");
		return FPGA_INVALID_PARAM;
	}

	if ((num_filters > 0) && (NULL == filters)) {
		OPAE_MSG("num_filters > 0 with NULL filters");
		return FPGA_INVALID_PARAM;
	}

	if (!num_filters && (NULL != filters)) {
		OPAE_MSG("num_filters == 0 with non-NULL filters");
		return FPGA_INVALID_PARAM;
	}

	*num_matches = 0;


#if 0
	memset(&head, 0, sizeof(head));

	/* create and populate token data structures */
	for (lptr = head.next; NULL != lptr; lptr = lptr->next) {
		// Skip the "container" device list nodes.
		if (!lptr->devpath[0])
			continue;

		if (lptr->hdr.objtype == FPGA_DEVICE &&
		    sync_fme(lptr) != FPGA_OK) {
			continue;
		} else if (lptr->hdr.objtype == FPGA_ACCELERATOR &&
			   sync_afu(lptr) != FPGA_OK) {
			continue;
		}

		if (matches_filters(lptr, filters, num_filters)) {
			if (*num_matches < max_tokens) {

				tokens[*num_matches] = token_add(lptr);

				if (!tokens[*num_matches]) {
					uint32_t i;
					OPAE_ERR("Failed to allocate memory for token");
					result = FPGA_NO_MEMORY;

					for (i = 0 ; i < *num_matches ; ++i)
						opae_free(tokens[i]);
					*num_matches = 0;

					goto out_free_trash;
				}

			}
			++(*num_matches);
		}
	}

out_free_trash:
	for (lptr = head.next; NULL != lptr;) {
		struct dev_list *trash = lptr;
		lptr = lptr->next;
		opae_free(trash);
	}

#endif

	return result;
}

fpga_result __REMOTE_API__ remote_fpgaCloneToken(fpga_token src, fpga_token *dst)
{
	if (NULL == src || NULL == dst) {
		OPAE_MSG("src or dst in NULL");
		return FPGA_INVALID_PARAM;
	}






	return FPGA_OK;
}

fpga_result __REMOTE_API__ remote_fpgaDestroyToken(fpga_token *token)
{

	if (!token || !(*token)) {
		OPAE_MSG("Invalid token pointer");
		return FPGA_INVALID_PARAM;
	}



	*token = NULL;

	return FPGA_OK;
}
