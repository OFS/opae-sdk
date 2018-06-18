
#ifndef __DMA_TEST_COMMON_H__
#define __DMA_TEST_COMMON_H__

#include <opae/fpga.h>
#include <stdlib.h>

#define MSGDMA_BBB_GUID		"ef82def7-f6ec-40fc-a914-9a35bace01ea"
#define MSGDMA_BBB_SIZE		8192

#define MSGDMA_BBB_HOST_MASK		0x2000000000000
#define MSGDMA_BBB_MAGIC_ROM_ADDR	0x1000000000000
#define MSGDNA_BBB_MAGIC_ROM_VAL	0x5772745F53796E63

#define ACL_DMA_INST_ACL_DMA_AFU_ID_AVMM_SLAVE_0_BASE 0x0
#define ACL_DMA_INST_DMA_MODULAR_SGDMA_DISPATCHER_0_CSR_BASE 0x40
#define ACL_DMA_INST_DMA_MODULAR_SGDMA_DISPATCHER_0_DESCRIPTOR_SLAVE_BASE 0x60
#define ACL_DMA_INST_ADDRESS_SPAN_EXTENDER_0_CNTL_BASE 0x200
#define ACL_DMA_INST_ADDRESS_SPAN_EXTENDER_0_WINDOWED_SLAVE_BASE 0x1000

#define MSGDMA_BBB_MEM_WINDOW_SPAN (4*1024)
#define MSGDMA_BBB_MEM_WINDOW_SPAN_MASK ((uint64_t)(MSGDMA_BBB_MEM_WINDOW_SPAN-1))

#define MEM_WINDOW_CRTL(dfh) (ACL_DMA_INST_ADDRESS_SPAN_EXTENDER_0_CNTL_BASE+dfh)
#define MEM_WINDOW_MEM(dfh) (ACL_DMA_INST_ADDRESS_SPAN_EXTENDER_0_WINDOWED_SLAVE_BASE+dfh)

void set_msgdma_bbb_dfh_offset(uint64_t offset);
void copy_to_mmio(fpga_handle afc_handle, uint64_t mmio_dst, uint64_t *host_src, int len);
void copy_to_dev_with_mmio(fpga_handle afc_handle, uint64_t *host_src, uint64_t dev_dest, int len);
void copy_from_dev_with_mmio(fpga_handle afc_handle, uint64_t *host_dst, uint64_t dev_src, int len);
int compare_dev_and_host(fpga_handle afc_handle, uint64_t *host_dst, uint64_t dev_src, int len);
void copy_dev_to_dev_with_mmio(fpga_handle afc_handle, uint64_t dev_src, uint64_t dev_dest, int len);
void copy_dev_to_dev_with_dma(fpga_handle afc_handle, uint64_t dev_src, uint64_t dev_dest, int len, bool intr=false);

int dump_dfh_list(fpga_handle afc_handle);
bool find_dfh_by_guid(fpga_handle afc_handle, 
	uint64_t find_id_l, uint64_t find_id_h, 
	uint64_t *result_offset = NULL, uint64_t *result_next_offset = NULL);
bool find_dfh_by_guid(fpga_handle afc_handle, 
	const char *guid_str,
	uint64_t *result_offset = NULL, uint64_t *result_next_offset = NULL);
void check_guid(fpga_handle afc_handle, uint64_t id_l, uint64_t id_h, const char *name = NULL);

#endif // __DMA_TEST_COMMON_H__