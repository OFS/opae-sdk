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



#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "eth_group.h"

// Eth Group CSR
#define ETH_GROUP_INFO		0x8
#define ETH_GROUP_CTRL		0x10
#define CMD_NOP			0
#define CMD_RD			1
#define CMD_WR			2
#define SELECT_IP		0
#define SELECT_FEAT		1
#define ETH_GROUP_STAT		0x18
#define ETH_GROUP_SELECT_FEAT	(1 << 0)
#define ETH_GROUP_PHY		1
#define ETH_GROUP_MAC		2
#define ETH_GROUP_ETHER		3

#define MAC_CONFIG	0x310

#define ETH_GROUP_TIMEOUT          100
#define ETH_GROUP_TIMEOUT_COUNT    50
#define ETH_GROUP_RET_VALUE        0xffff
#define ETH_GROUP_FEATUREID        0x10

// open eth group
int eth_group::eth_group_open(const std::string& fpga_uid_str)
{
	int res       = 0;
	uint8_t *mem  = NULL;

	res = opae_uio_open(&uio, fpga_uid_str.c_str());
	if (res) {
		return res;
	}

	res = opae_uio_region_get(&uio, 0, (uint8_t **)&mem, NULL);
	if (res) {
		return res;
	}

	mmap_ptr = mem;
	ptr_ = (uint64_t*)mem;

	eth_dfh.csr = *(ptr_);
	// Check ETH group FeatureID
	if (eth_dfh.id != ETH_GROUP_FEATUREID) {
		printf("Wrong Eth group Feature ID \n");
		return -1;
	}

	eth_info.csr = *(ptr_ + 1);
	direction = eth_info.direction;
	phy_num = eth_info.no_phys;
	group_id = eth_info.group_num;
	speed = eth_info.speed_gbs;
	df_id = eth_dfh.id;
	eth_lwmac = eth_info.light_wight_mac;

	// Reset Mac
	if (!mac_reset()) {
		printf("Failed to reset MAC \n");
		return -1;
	}

	return 0;
}

// reset mac
bool eth_group::mac_reset()
{
	uint32_t i       = 0;

	for (i = 0; i < phy_num; i++) {
		if (read_reg(ETH_GROUP_MAC, i, 0, MAC_CONFIG) == ETH_GROUP_RET_VALUE)
			return false;
		if (write_reg(ETH_GROUP_MAC, i, 0, MAC_CONFIG, 0x0))
			return false;
	}

	return true;
}

// Close eth group
int eth_group::eth_group_close(void)
{
	opae_uio_close(&uio);
	return 0;
}

// read eth group reg
uint32_t eth_group::read_reg(uint32_t type,
							uint32_t index,
							uint32_t flags,
							uint32_t addr)
{
	struct eth_group_ctl eth_ctl;
	struct eth_group_stat eth_stat;
	uint32_t data           = 0;
	int timer_count         = 0;
	eth_ctl.csr             = 0;
	eth_stat.csr            = 0;


	if (flags & ETH_GROUP_SELECT_FEAT && type != ETH_GROUP_PHY)
		return -1;

	if (type == ETH_GROUP_PHY)
		eth_ctl.ctl_dev_select = index * 2 + 2;
	else if (type == ETH_GROUP_MAC)
		eth_ctl.ctl_dev_select = index * 2 + 3;
	else if (type == ETH_GROUP_ETHER)
		eth_ctl.ctl_dev_select = 0;

	eth_ctl.eth_cmd = CMD_RD;
	eth_ctl.ctl_addr = addr;
	eth_ctl.ctl_fev_select = flags & ETH_GROUP_SELECT_FEAT;

	// write to ctrl reg
	*((volatile uint64_t *)(mmap_ptr + ETH_GROUP_CTRL))
		= (uint64_t)eth_ctl.csr;

	//read until status reg bit valid
	while (1)
	{
		eth_stat.csr = *((volatile uint64_t *)(mmap_ptr + ETH_GROUP_STAT));
		if (eth_stat.stat_valid) {
			data = eth_stat.stat_data;
			return data;
		}
		timer_count++;
		usleep(ETH_GROUP_TIMEOUT);
		if (timer_count > ETH_GROUP_TIMEOUT_COUNT)
			break;

	};

	return ETH_GROUP_RET_VALUE;
}

// Write eth group reg
int eth_group::write_reg(uint32_t type,
						uint32_t index,
						uint32_t flags,
						uint32_t addr,
						uint32_t data)
{
	struct eth_group_ctl eth_ctl;
	struct eth_group_stat eth_stat;
	int timer_count = 0;
	eth_ctl.csr = 0;
	eth_stat.csr = 0;


	if (flags & ETH_GROUP_SELECT_FEAT && type != ETH_GROUP_PHY)
		return -1;

	if (type == ETH_GROUP_PHY)
		eth_ctl.ctl_dev_select = index * 2 + 2;
	else if (type == ETH_GROUP_MAC)
		eth_ctl.ctl_dev_select = index * 2 + 3;
	else if (type == ETH_GROUP_ETHER)
		eth_ctl.ctl_dev_select = 0;

	eth_ctl.eth_cmd = CMD_WR;
	eth_ctl.ctl_addr = addr;
	eth_ctl.ctl_data = data;
	eth_ctl.ctl_fev_select = flags & ETH_GROUP_SELECT_FEAT;

	// write to ctrl reg
	*((volatile uint64_t *)(mmap_ptr + ETH_GROUP_CTRL))
		= (uint64_t)eth_ctl.csr;

	//read until status reg bit valid
	while (1)
	{
		eth_stat.csr = *((volatile uint64_t *)(mmap_ptr + ETH_GROUP_STAT));
		if (eth_stat.stat_valid) {
			return 0;
		}
		timer_count++;
		timer_count++;
		usleep(ETH_GROUP_TIMEOUT);
		if (timer_count > ETH_GROUP_TIMEOUT_COUNT)
			break;
	};

	return -1;
}


namespace py = pybind11;
PYBIND11_MODULE(eth_group, m) {
	// optional module docstring
	m.doc() = "pybind11 eth_group plugin";

	py::class_<eth_group>(m, "eth_group")
		.def(py::init<>())
		.def("eth_group_open", (int(eth_group::*)(const std::string&))&eth_group::eth_group_open)
		.def("eth_group_close",(int(eth_group::*)(void))&eth_group::eth_group_close)
		.def("read_reg", (uint32_t(eth_group::*)(uint32_t type, uint32_t index, uint32_t flags, uint32_t addrr))&eth_group::read_reg)
		.def("write_reg", (int(eth_group::*)(uint32_t type, uint32_t index, uint32_t flags, uint32_t addrr, uint32_t data))&eth_group::write_reg)
		.def_readonly("direction", &eth_group::direction)
		.def_readonly("phy_num", &eth_group::phy_num)
		.def_readonly("group_id", &eth_group::group_id)
		.def_readonly("speed", &eth_group::speed)
		.def_readonly("df_id", &eth_group::df_id)
		.def_readonly("eth_lwmac", &eth_group::eth_lwmac);
}
