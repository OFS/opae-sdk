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

#define DVE_VFIO_PATH  "/dev/vfio/vfio"
#define MAC_CONFIG	0x310

#define ETH_GROUP_TIMEOUT          100
#define ETH_GROUP_TIMEOUT_COUNT    50
#define ETH_GROUP_RET_VALUE        0xffff


// open eth group
int eth_group::eth_group_open(int vfio_id, std::string fpga_mdev_str)
{
	char dev_path[256] = { 0 };
	int i = 0;
	struct vfio_group_status group_status ;
	struct vfio_iommu_type1_info iommu_info;
	struct vfio_iommu_type1_dma_map dma_map;

	memset(&group_status, 0, sizeof(group_status));
	group_status.argsz = sizeof(group_status);

	memset(&iommu_info, 0, sizeof(iommu_info));
	iommu_info.argsz = sizeof(iommu_info);

	memset(&dma_map, 0, sizeof(dma_map));
	dma_map.argsz = sizeof(dma_map);

	memset(&device_info, 0, sizeof(device_info));
	device_info.argsz = sizeof(device_info);

	memset(&device_info, 0, sizeof(device_info));
	device_info.argsz = sizeof(device_info);

	container = open(DVE_VFIO_PATH, O_RDWR);
	if (container < 0) {
		fprintf(stderr, "error opening container: %s\n",
			strerror(errno));
		return -1;
	}

	if (ioctl(container, VFIO_GET_API_VERSION) != VFIO_API_VERSION) {
		printf("Wrong VFIO_API_VERSION \n");
		return -1;
	}
	
	if (!ioctl(container, VFIO_CHECK_EXTENSION, VFIO_TYPE1_IOMMU)) {
		printf("Doen't support VFIO_TYPE1_IOMMU \n");
		return -1;
	}

	if (snprintf(dev_path, sizeof(dev_path),
		"%s/%d", "/dev/vfio", vfio_id) < 0) {
		printf("snprint fails \n");
		return -1;
	}

	group = open(dev_path, O_RDWR);
	if (group < 0) {
		fprintf(stderr, "error opening group: %s\n",
			strerror(errno));
		return -1;
	}

	// checks VFIO group status
	ioctl(group, VFIO_GROUP_GET_STATUS, &group_status);
	if (!(group_status.flags & VFIO_GROUP_FLAGS_VIABLE)) {
		printf("Wrong VFIO_GROUP_FLAGS_VIABLE \n");
		return -1;
	}

	// Add the group to the container
	ioctl(group, VFIO_GROUP_SET_CONTAINER, &container);

	// Enable the IOMMU model
	ioctl(container, VFIO_SET_IOMMU, VFIO_TYPE1_IOMMU);

	// Get IOMMU info
	ioctl(container, VFIO_IOMMU_GET_INFO, &iommu_info);

	// Allocate some space and setup a DMA mapping 
	dma_map.vaddr = (unsigned long int) mmap(0, 1024 * 1024, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	dma_map.size = 1024 * 1024;
	dma_map.iova = 0;
	dma_map.flags = VFIO_DMA_MAP_FLAG_READ | VFIO_DMA_MAP_FLAG_WRITE;

	ioctl(container, VFIO_IOMMU_MAP_DMA, &dma_map);

	// Get FIO a file descriptor for the device
	device = ioctl(group, VFIO_GROUP_GET_DEVICE_FD, fpga_mdev_str.c_str()	);
	if (device < 0) {
		fprintf(stderr, "error getting device fd: %s\n",
			strerror(errno));
		return -1;
	}
	// Get dvice info
	ioctl(device, VFIO_DEVICE_GET_INFO, &device_info);

	for (i = 0; i < (int)device_info.num_regions; i++) {
		struct vfio_region_info reg;
		memset(&reg, 0, sizeof(reg));
		reg.argsz = sizeof(reg);
		reg.index = i;
		ioctl(device, VFIO_DEVICE_GET_REGION_INFO, &reg);
		uint8_t *mem;
		mem = (uint8_t*) mmap(NULL, reg.size, PROT_READ | PROT_WRITE,
			MAP_SHARED, device, reg.offset);

		if (mem == MAP_FAILED) {
			printf("Failed to MMAP \n");
			eth_group_close();
			return -1;
		}

		reg_size = reg.size;
		reg_offset = reg.offset;

		mmap_ptr = mem;
		ptr_ = (uint64_t*)mem;

		eth_info.csr = *(ptr_ + 1);
		eth_dfh.csr = *(ptr_);
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
	}
	return 0;
}

// reset mac
bool eth_group::mac_reset()
{
	uint32_t i;
	uint32_t value;
	int ret = 0;
	struct eth_group_mac mac_value;

	for (i = 0; i < phy_num; i++) {
		value = read_reg(ETH_GROUP_MAC, i, 0, MAC_CONFIG);
		if (value == ETH_GROUP_RET_VALUE)
			return false;

		mac_value.csr = value;
		value |= mac_value.mac_mask;
		ret = write_reg(ETH_GROUP_MAC, i, 0, MAC_CONFIG, 0x0);
		if (ret != 0)
			return false;
	}

	return true;
}

// Close eth group
int eth_group::eth_group_close(void)
{
	struct vfio_iommu_spapr_register_memory reg;
	struct vfio_iommu_type1_dma_unmap dma_unmap;

	memset(&reg, 0, sizeof(reg));
	reg.argsz = sizeof(reg);

	memset(&dma_unmap, 0, sizeof(dma_unmap));
	dma_unmap.argsz = sizeof(struct vfio_iommu_type1_dma_unmap);
	dma_unmap.size = 1024 * 1024;
	dma_unmap.iova = 0;

	munmap(ptr_, reg_size);

	if (device > 0)
		close(device);

	ioctl(container, VFIO_IOMMU_UNMAP_DMA,&dma_unmap);

	if (group > 0)
		close(group);

	if (container > 0)
		close(container);

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
	uint32_t data;
	int timer_count = 0;
	eth_ctl.csr = 0;
	eth_stat.csr = 0;

	//printf("read_reg addr: %x  type: %x  index: %x flags: %x \n", addr, type, index, flags);

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

	//read untill status reg bit valid
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

	//printf("write_reg addr: %x  type: %x  index: %x flags: %x \n", addr, type, index, flags);

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

	//read untill status reg bit valid
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
		.def("eth_group_open", (int(eth_group::*)(int, std::string))&eth_group::eth_group_open)
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