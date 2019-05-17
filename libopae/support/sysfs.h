// Copyright(c) 2019, Intel Corporation
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
#ifndef _GENSYSFS_H
#define _GENSYSFS_H
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#define SYSFS_PATH_MAX 256

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _pci_node {
  char pci_bus_path[PATH_MAX];
  uint32_t segment;
  uint8_t bus;
  uint8_t device;
  uint8_t function;
  uint32_t device_id;
  uint32_t vendor_id;
  struct _pci_node *parent;
  struct _pci_node *children;
  struct _pci_node *next;
} pci_node;

typedef struct _sysfs_object {
	char sysfs_path[SYSFS_PATH_MAX];
  const pci_node *pci_object;
} sysfs_object;


size_t sysfs_enum_class(const char *path, sysfs_object *objects, size_t max_objects);
const pci_node *sysfs_root_port(const sysfs_object *obj);

#ifdef __cplusplus
}
#endif

#endif /* !SYSFS_H */
