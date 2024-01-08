// Copyright(c) 2024, Intel Corporation
//
// Redistribution  and	use  in source	and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of	 source code  must retain the  above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name  of Intel Corporation  nor the names of its contributors
//   may be used to  endorse or promote	 products derived  from this  software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
// IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT	 SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
// LIABLE  FOR	ANY  DIRECT,  INDIRECT,	 INCIDENTAL,  SPECIAL,	EXEMPLARY,  OR
// CONSEQUENTIAL  DAMAGES  (INCLUDING,	BUT  NOT LIMITED  TO,  PROCUREMENT  OF
// SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,	DATA, OR PROFITS;  OR BUSINESS
// INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
// CONTRACT,  STRICT LIABILITY,	 OR TORT  (INCLUDING NEGLIGENCE	 OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,	EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

//
// Multi-ported AFUs have a parent AFU that names child AFUs by GUID. These
// functions apply operations to all childen of a parent AFU.
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif // _GNU_SOURCE

#include <uuid/uuid.h>

#include <opae/properties.h>
#include <opae/types_enum.h>
#include <opae/access.h>
#include <opae/enum.h>
#include <opae/mmio.h>

#include "pluginmgr.h"
#include "opae_int.h"
#include "props.h"
#include "mock/opae_std.h"

STATIC void api_guid_to_fpga(uint64_t guidh, uint64_t guidl, fpga_guid guid)
{
	uint32_t i;
	uint32_t s;

	// The API expects the MSB of the GUID at [0] and the LSB at [15].
	s = 64;
	for (i = 0; i < 8; ++i) {
		s -= 8;
		guid[i] = (uint8_t) ((guidh >> s) & 0xff);
	}

	s = 64;
	for (i = 0; i < 8; ++i) {
		s -= 8;
		guid[8 + i] = (uint8_t) ((guidl >> s) & 0xff);
	}
}

fpga_result afu_open_children(opae_wrapped_handle *wrapped_parent_handle)
{
	fpga_result result;
	uint64_t v;

	const opae_api_adapter_table *adapter =
		wrapped_parent_handle->wrapped_token->adapter_table;
	fpga_handle handle = wrapped_parent_handle->opae_handle;

	//
	// Does this AFU have children? Return FPGA_OK if it does not.
	//

	if (!adapter->fpgaReadMMIO64)
		return FPGA_OK;

	// DFH must be a v1 AFU with ID 0
	result = adapter->fpgaReadMMIO64(handle, 0, 0, &v);
	if (result != FPGA_OK)
		return result;

	// An AFU?
	if ((v >> 60) != 1)
	       return FPGA_OK;
	// At least v1?
	if (!((v >> 52) & 0xff))
	       return FPGA_OK;
	// ID is 0 (normal AFU)?
	if (v & 0xfff)
	       return FPGA_OK;

	// Is there a parameter list?
	result = adapter->fpgaReadMMIO64(handle, 0, 0x20, &v);
	if (result != FPGA_OK)
		return result;
	if (((v >> 31) & 1) == 0)
		return FPGA_OK;

	// Look for a parameter with ID 2 (list of child GUIDs)
	bool found_children = false;
	uint64_t offset = 0x28;
	do {
		result = adapter->fpgaReadMMIO64(handle, 0, offset, &v);
		if (result != FPGA_OK)
			return result;

		if ((v & 0xffff) == 2) {
			found_children = true;
			offset += 8;
			break;
		}

		// Next parameter
		offset += (v >> 35) * 8;
	} while (((v >> 32) & 1) == 0); // Continue until EOP

	if (!found_children)
		return FPGA_OK;

	// Number of children, inferred from the size of the parameter block
	uint32_t num_children = v >> 36;
	opae_wrapped_handle *child_prev = NULL;

	// Walk the list of child AFU GUIDs and load them. The resulting
	// list of child FPGA handles matches the order of the parameter
	// block.
	//
	// *** For most errors, no cleanup is required. Once a child is
	// *** on the parent's list it will be cleaned up along with
	// *** the parent.
	for (uint32_t c = 0; c < num_children; ++c) {
		fpga_guid guid;
		uint64_t guidh, guidl;

		result = adapter->fpgaReadMMIO64(handle, 0, offset, &guidl);
		if (result != FPGA_OK)
			return result;
		result = adapter->fpgaReadMMIO64(handle, 0, offset+8, &guidh);
		if (result != FPGA_OK)
			return result;

		// Call the shell's public API methods, not the adapter
		// instance. Children will have their own wrapped handles.

		fpga_properties filter = NULL;
		result = fpgaGetProperties(NULL, &filter);
		if (result != FPGA_OK)
			return result;
		fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR);
		api_guid_to_fpga(guidh, guidl, guid);
		fpgaPropertiesSetGUID(filter, guid);

		fpga_token accel_token;
		uint32_t num_matches;
		result = fpgaEnumerate(&filter, 1, &accel_token, 1, &num_matches);
		fpgaDestroyProperties(&filter);
		if (result != FPGA_OK)
			return result;
		if (num_matches == 0) {
			char guid_str[64];
			uuid_unparse(guid, guid_str);
			OPAE_ERR("Child %s not found", guid_str);
			return FPGA_NOT_FOUND;
		}

		fpga_handle child_handle;
		result = fpgaOpen(accel_token, &child_handle, 0);
		fpgaDestroyToken(&accel_token);
		if (result != FPGA_OK)
			return result;

		opae_wrapped_handle *wrapped_child_handle =
			opae_validate_wrapped_handle(child_handle);
		ASSERT_NOT_NULL(wrapped_child_handle);

		wrapped_child_handle->parent = wrapped_parent_handle;
		if (!child_prev)
			wrapped_parent_handle->child_next = wrapped_child_handle;
		else
			child_prev->child_next = wrapped_child_handle;
		child_prev = wrapped_child_handle;

		// Next child GUID in the parameter block
		offset += 16;
	}

	return result;
}

fpga_result afu_close_children(opae_wrapped_handle *wrapped_parent_handle)
{
	opae_wrapped_handle *wrapped_child_next;

	ASSERT_NOT_NULL(wrapped_parent_handle);

	// Is handle actually a child? Avoid recursion.
	if (wrapped_parent_handle->parent)
		return FPGA_OK;

	wrapped_child_next = wrapped_parent_handle->child_next;
	while (wrapped_child_next) {
		opae_wrapped_handle *wrapped_child = wrapped_child_next;
		wrapped_child_next = wrapped_child->child_next;

		// Use the public API, which will clean up the wrapper.
		fpgaClose(wrapped_child);
	}

	return FPGA_OK;
}

fpga_result afu_pin_buffer(opae_wrapped_handle *wrapped_parent_handle,
			   void *buf_addr, uint64_t len, uint64_t wsid)
{
	fpga_result res;
	opae_wrapped_handle *wrapped_child = wrapped_parent_handle->child_next;
	opae_wrapped_handle *wrapped_undo;

	if (!wrapped_child)
		return FPGA_OK;

	ASSERT_NOT_NULL_RESULT(wrapped_parent_handle->adapter_table->fpgaGetIOAddress,
			       FPGA_NOT_SUPPORTED);

	uint64_t ioaddr;
	res = wrapped_parent_handle->adapter_table->fpgaGetIOAddress(
		wrapped_parent_handle->opae_handle, wsid, &ioaddr);
	if (res != FPGA_OK)
		return res;

	while (wrapped_child) {
		ASSERT_NOT_NULL_RESULT(wrapped_child->adapter_table->fpgaPinBuffer,
				       FPGA_NOT_SUPPORTED);
		res = wrapped_child->adapter_table->fpgaPinBuffer(
			wrapped_child->opae_handle, buf_addr, len, ioaddr);
		if (res != FPGA_OK)
			goto error_child;
		wrapped_child = wrapped_child->child_next;
	}

	return FPGA_OK;

error_child:
	// Undo pinning of any children completed before the error
	wrapped_undo = wrapped_parent_handle->child_next;
	while (wrapped_undo != wrapped_child) {
		if (wrapped_undo->adapter_table->fpgaUnpinBuffer)
			wrapped_undo->adapter_table->fpgaUnpinBuffer(
				wrapped_undo->opae_handle, buf_addr, len, ioaddr);

		wrapped_undo = wrapped_undo->child_next;
	}

	return res;
}

fpga_result afu_unpin_buffer(opae_wrapped_handle *wrapped_parent_handle,
			     uint64_t wsid)
{
	fpga_result res;
	opae_wrapped_handle *wrapped_child = wrapped_parent_handle->child_next;
	void *buf_addr;
	uint64_t ioaddr;
	uint64_t len;

	if (!wrapped_child)
		return FPGA_OK;

	ASSERT_NOT_NULL_RESULT(wrapped_parent_handle->adapter_table->fpgaGetWSInfo,
			       FPGA_NOT_SUPPORTED);
	res = wrapped_parent_handle->adapter_table->fpgaGetWSInfo(
		wrapped_parent_handle->opae_handle, wsid, &ioaddr, &buf_addr, &len);
	if (res != FPGA_OK)
	    return res;

	while (wrapped_child) {
		ASSERT_NOT_NULL_RESULT(wrapped_child->adapter_table->fpgaUnpinBuffer,
				       FPGA_NOT_SUPPORTED);
		res = wrapped_child->adapter_table->fpgaUnpinBuffer(
			wrapped_child->opae_handle, buf_addr, len, ioaddr);
		if (res != FPGA_OK)
			return res;

		wrapped_child = wrapped_child->child_next;
	}

	return FPGA_OK;
}
