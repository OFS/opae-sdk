// Copyright(c) 2019-2020, Intel Corporation
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


#ifndef ETH_VFIO_H
#define ETH_VFIO_H

#include <map>
#include <memory>
#include <vector>
#include <linux/vfio.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <mutex>
#include <stdexcept>


struct eth_group_info {
	union {
		uint64_t csr;
		struct {
			uint8_t  group_num : 8;
			uint8_t no_phys : 8;
			uint8_t speed_gbs : 8;
			uint8_t direction : 1;
			uint8_t light_wight_mac : 1;
			uint64_t reserved : 38;
		};
	};
};

struct dfh {
	union {
		uint64_t csr;
		struct {
			uint16_t id : 12;
			uint8_t  revision : 4;
			uint32_t next_header_offset : 24;
			uint8_t eol : 1;
			uint32_t reserved : 19;
			uint8_t  type : 4;
		};
	};
};
struct eth_group_ctl {
	union {
		uint64_t csr;
		struct {
			uint32_t ctl_data : 32;
			uint16_t ctl_addr : 16;
			uint8_t ctl_fev_select : 1;
			uint8_t ctl_dev_select : 5;
			uint16_t reserved : 8;
			uint8_t  eth_cmd : 2;
		};
	};
};

struct eth_group_stat {
	union {
		uint64_t csr;
		struct {
			uint32_t stat_data : 32;
			uint16_t stat_valid : 1;
			uint32_t reserved : 31;
		};
	};
};

struct eth_group_mac {
	union {
		uint64_t csr;
		struct {
			uint8_t mac_mask : 3;
			uint64_t reserved : 61;
		};
	};
};


class eth_group {
public:
	eth_group() { }
	~eth_group() {}

	int eth_group_open(int vfio_id, std::string fpga_mdev_str);
	int eth_group_close();
	uint32_t read_reg(uint32_t type, uint32_t index,
		uint32_t flags, uint32_t addr);
	int write_reg(uint32_t type, uint32_t index,
		uint32_t flags, uint32_t addr, uint32_t data);
	bool mac_reset();

	uint32_t direction;
	uint32_t phy_num;
	uint32_t group_id;
	uint32_t speed;
	uint32_t df_id;
	uint32_t eth_lwmac;

private:
	uint64_t* ptr_;
	int container;
	int group;
	int device;
	int reg_size;
	int reg_offset;

	struct vfio_device_info device_info;
	struct eth_group_info eth_info;
	struct dfh eth_dfh;

	uint8_t *mmap_ptr;
};


#endif //ETH_VFIO_H
