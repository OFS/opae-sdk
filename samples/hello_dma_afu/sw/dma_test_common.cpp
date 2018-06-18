#include <assert.h>
#include <uuid/uuid.h>
#include <unistd.h>

#include <poll.h>
#include <errno.h>
#include <string.h>

#include "dma_test_common.h"

static uint64_t msgdma_bbb_dfh_offset = -256*1024;

static void mmio_read64(fpga_handle afc_handle, uint64_t addr, uint64_t *data, const char *reg_name)
{
	fpgaReadMMIO64(afc_handle, 0, addr, data);
	printf("Reading %s (Byte Offset=%08lx) = %08lx\n", reg_name, addr, *data);
}

static void mmio_read64_silent(fpga_handle afc_handle, uint64_t addr, uint64_t *data)
{
	fpgaReadMMIO64(afc_handle, 0, addr, data);
}

void set_msgdma_bbb_dfh_offset(uint64_t offset)
{
	msgdma_bbb_dfh_offset = offset;
}

void copy_to_mmio(fpga_handle afc_handle, uint64_t mmio_dst, uint64_t *host_src, int len)
{
	//mmio requires 8 byte alignment
	assert(len % 8 == 0);
	assert(mmio_dst % 8 == 0);
	
	uint64_t dev_addr = mmio_dst;
	uint64_t *host_addr = host_src;
	
	for(int i = 0; i < len/8; i++)
	{
		fpgaWriteMMIO64(afc_handle, 0, dev_addr, *host_addr);
		
		host_addr += 1;
		dev_addr += 8;
	}
}

void copy_to_dev_with_mmio(fpga_handle afc_handle, uint64_t *host_src, uint64_t dev_dest, int len)
{
	//mmio requires 8 byte alignment
	assert(len % 8 == 0);
	assert(dev_dest % 8 == 0);
	
	uint64_t dev_addr = dev_dest;
	uint64_t *host_addr = host_src;
	
	uint64_t cur_mem_page = dev_addr & ~MSGDMA_BBB_MEM_WINDOW_SPAN_MASK;
	fpgaWriteMMIO64(afc_handle, 0, MEM_WINDOW_CRTL(msgdma_bbb_dfh_offset), cur_mem_page);
	
	for(int i = 0; i < len/8; i++)
	{
		uint64_t mem_page = dev_addr & ~MSGDMA_BBB_MEM_WINDOW_SPAN_MASK;
		if(mem_page != cur_mem_page)
		{
			cur_mem_page = mem_page;
			fpgaWriteMMIO64(afc_handle, 0, MEM_WINDOW_CRTL(msgdma_bbb_dfh_offset), cur_mem_page);
		}
		fpgaWriteMMIO64(afc_handle, 0, MEM_WINDOW_MEM(msgdma_bbb_dfh_offset)+(dev_addr&MSGDMA_BBB_MEM_WINDOW_SPAN_MASK), *host_addr);
		
		host_addr += 1;
		dev_addr += 8;
	}
}

void copy_from_dev_with_mmio(fpga_handle afc_handle, uint64_t *host_dst, uint64_t dev_src, int len)
{
	//mmio requires 8 byte alignment
	assert(len % 8 == 0);
	assert(dev_src % 8 == 0);
	
	uint64_t dev_addr = dev_src;
	uint64_t *host_addr = host_dst;
	
	uint64_t cur_mem_page = dev_addr & ~MSGDMA_BBB_MEM_WINDOW_SPAN_MASK;
	fpgaWriteMMIO64(afc_handle, 0, MEM_WINDOW_CRTL(msgdma_bbb_dfh_offset), cur_mem_page);
	
	for(int i = 0; i < len/8; i++)
	{
		uint64_t mem_page = dev_addr & ~MSGDMA_BBB_MEM_WINDOW_SPAN_MASK;
		if(mem_page != cur_mem_page)
		{
			cur_mem_page = mem_page;
			fpgaWriteMMIO64(afc_handle, 0, MEM_WINDOW_CRTL(msgdma_bbb_dfh_offset), cur_mem_page);
		}
		fpgaReadMMIO64(afc_handle, 0, MEM_WINDOW_MEM(msgdma_bbb_dfh_offset)+(dev_addr&MSGDMA_BBB_MEM_WINDOW_SPAN_MASK), host_addr);
		
		host_addr += 1;
		dev_addr += 8;
	}
}

int compare_dev_and_host(fpga_handle afc_handle, uint64_t *host_dst, uint64_t dev_src, int len)
{
	//mmio requires 8 byte alignment
	assert(len % 8 == 0);
	assert(dev_src % 8 == 0);
	
	uint64_t dev_addr = dev_src;
	uint64_t *host_addr = host_dst;
	
	uint64_t cur_mem_page = dev_addr & ~MSGDMA_BBB_MEM_WINDOW_SPAN_MASK;
	fpgaWriteMMIO64(afc_handle, 0, MEM_WINDOW_CRTL(msgdma_bbb_dfh_offset), cur_mem_page);
	
	int num_errors = 0;
	uint64_t cmp_data;
	for(int i = 0; i < len/8; i++)
	{
		uint64_t mem_page = dev_addr & ~MSGDMA_BBB_MEM_WINDOW_SPAN_MASK;
		if(mem_page != cur_mem_page)
		{
			cur_mem_page = mem_page;
			fpgaWriteMMIO64(afc_handle, 0, MEM_WINDOW_CRTL(msgdma_bbb_dfh_offset), cur_mem_page);
		}
		fpgaReadMMIO64(afc_handle, 0, MEM_WINDOW_MEM(msgdma_bbb_dfh_offset)+(dev_addr&MSGDMA_BBB_MEM_WINDOW_SPAN_MASK), &cmp_data);
		if(*host_addr != cmp_data)
			num_errors++;
		
		host_addr += 1;
		dev_addr += 8;
	}
	
	return num_errors;
}

typedef struct __attribute__((__packed__)) 
{
	//0x0
	uint32_t rd_address;
	//0x4
	uint32_t wr_address;
	//0x8
	uint32_t len;
	//0xC
	uint8_t wr_burst_count;
	uint8_t rd_burst_count;
	uint16_t seq_num;
	//0x10
	uint16_t wr_stride;
	uint16_t rd_stride;
	//0x14
	uint32_t rd_address_ext;
	//0x18
	uint32_t wr_address_ext;
	//0x1c
	uint32_t control;
} msgdma_ext_descriptor_t;

void copy_dev_to_dev_with_mmio(fpga_handle afc_handle, uint64_t dev_src, uint64_t dev_dest, int len)
{
	//dma requires 64 byte alignment
	assert(len % 64 == 0);
	assert(dev_src % 64 == 0);
	assert(dev_dest % 64 == 0);
	
	uint64_t *tmp = (uint64_t *)malloc(len);
	assert(tmp);
	
	copy_from_dev_with_mmio(afc_handle, tmp, dev_src, len);
	copy_to_dev_with_mmio(afc_handle, tmp, dev_dest, len);
	
	free(tmp);
}

void copy_dev_to_dev_with_dma(fpga_handle afc_handle, uint64_t dev_src, uint64_t dev_dest, int len, bool intr)
{
	fpga_result     res = FPGA_OK;
	struct pollfd pfd;
	
	//dma requires 64 byte alignment
	assert(len % 64 == 0);
	assert(dev_src % 64 == 0);
	assert(dev_dest % 64 == 0);
	
	//only 32bit for now
	const uint64_t MASK_FOR_32BIT_ADDR = 0xFFFFFFFF;
	
	msgdma_ext_descriptor_t d;
	
	d.rd_address = dev_src & MASK_FOR_32BIT_ADDR;
	d.wr_address = dev_dest & MASK_FOR_32BIT_ADDR;  
	d.len = len;
	d.wr_burst_count = 1;
	d.rd_burst_count = 1;
	d.seq_num = 0;
	d.wr_stride = 1;
	d.rd_stride = 1;
	d.rd_address_ext = (dev_src >> 32) & MASK_FOR_32BIT_ADDR;
	d.wr_address_ext = (dev_dest >> 32)& MASK_FOR_32BIT_ADDR;  
	d.control = 0x80000000;
	d.control |= (intr == true) ? (1 << 14) : 0; //enable interupts on completion of this descriptor
	
	const uint64_t SGDMA_CSR_BASE = msgdma_bbb_dfh_offset+ACL_DMA_INST_DMA_MODULAR_SGDMA_DISPATCHER_0_CSR_BASE;
	const uint64_t SGDMA_DESC_BASE = msgdma_bbb_dfh_offset+ACL_DMA_INST_DMA_MODULAR_SGDMA_DISPATCHER_0_DESCRIPTOR_SLAVE_BASE;
	uint64_t mmio_data = 0;
	
	fpga_event_handle ehandle;
	if(intr)
	{
		res = fpgaCreateEventHandle(&ehandle);
		if(FPGA_OK != res)
		{
			printf("error creating event handle\n");
			exit(1);
		}
		
		/* Register user interrupt with event handle */
		res = fpgaRegisterEvent(afc_handle, FPGA_EVENT_INTERRUPT, ehandle, 0);
		if(FPGA_OK != res)
		{
			printf("error registering event\n");
			exit(1);
		}
	}
	
	//enable interrupts on sgdma
	if(intr)
		fpgaWriteMMIO32(afc_handle, 0, SGDMA_CSR_BASE+4, 0x10);
	
	//send descriptor
	copy_to_mmio(afc_handle, SGDMA_DESC_BASE, (uint64_t *)&d, sizeof(d));
	
	if(intr)
	{
		/* Poll event handle*/
		res = fpgaGetOSObjectFromEventHandle(ehandle, &pfd.fd);
		if(res != FPGA_OK)
		{
			fprintf(stderr, "error getting event file handle");
			exit(1);
		}
		pfd.events = POLLIN;            
		int poll_res = poll(&pfd, 1, -1);
		if(poll_res < 0) {
		 fprintf( stderr, "Poll error errno = %s\n",strerror(errno));
		 exit(1);
		} 
		else if(poll_res == 0) {
		 fprintf( stderr, "Poll(interrupt) timeout \n");
		 exit(1);
		} else {
		 uint64_t count;
		 read(pfd.fd, &count, sizeof(count));
		 printf("Poll success. Return = %d, count = %d\n",poll_res, count);
		}
		
		//disable interrupts
		fpgaWriteMMIO32(afc_handle, 0, SGDMA_CSR_BASE+4, 0x0);
		
		//clear interrupt
		fpgaWriteMMIO32(afc_handle, 0, SGDMA_CSR_BASE, 0x200);
	}
	else
	{
		//TODO: the status register is only 32 bits.  Need to update this.
		mmio_read64_silent(afc_handle, SGDMA_CSR_BASE, &mmio_data);
		while((mmio_data&0xFFFFFFFF) !=0x2)
		{
	#ifdef USE_ASE
			sleep(1);
			mmio_read64(afc_handle, SGDMA_CSR_BASE, &mmio_data, "sgdma_csr_base");
	#else
			mmio_read64_silent(afc_handle, SGDMA_CSR_BASE, &mmio_data);
	#endif
		}
	}
	
	if(intr)
	{
		/* cleanup */
		res = fpgaUnregisterEvent(afc_handle, FPGA_EVENT_INTERRUPT, ehandle);   
		if(FPGA_OK != res)
		{
			printf("error fpgaUnregisterEvent\n");
			exit(1);
		}
		
		res = fpgaDestroyEventHandle(&ehandle);
		if(FPGA_OK != res)
		{
			printf("error fpgaDestroyEventHandle\n");
			exit(1);
		}
	}
}

#define DFH_FEATURE_EOL(dfh) (((dfh >> 40) & 1) == 1)
#define DFH_FEATURE(dfh) ((dfh >> 60) & 0xf)
#define DFH_FEATURE_IS_PRIVATE(dfh) (DFH_FEATURE(dfh) == 3)
#define DFH_FEATURE_IS_BBB(dfh) (DFH_FEATURE(dfh) == 2)
#define DFH_FEATURE_IS_AFU(dfh) (DFH_FEATURE(dfh) == 1)
#define DFH_FEATURE_NEXT(dfh) ((dfh >> 16) & 0xffffff)

int dump_dfh_list(fpga_handle afc_handle)
{
	uint64_t offset = 0;
	uint64_t dfh = 0;
	
	do
	{
		mmio_read64_silent(afc_handle, offset, &dfh);
		int is_bbb = DFH_FEATURE_IS_BBB(dfh);
		int is_afu = DFH_FEATURE_IS_AFU(dfh);
		int is_private = DFH_FEATURE_IS_PRIVATE(dfh);
		uint64_t id_l = 0;
		uint64_t id_h = 0;
		
		if(is_afu || is_bbb)
		{
			mmio_read64_silent(afc_handle, offset+8, &id_l);
			mmio_read64_silent(afc_handle, offset+16, &id_h);
		}
		
		printf("DFH=0x%08lx offset=%lx next_offset=%lx ", dfh, offset, DFH_FEATURE_NEXT(dfh));

		if(is_afu)
			printf("type=afu ");
		else if(is_bbb)
			printf("type=bbb ");
		else if(is_private)
			printf("type=private ");
		else
			printf("type=unknown ");			
		if(is_afu || is_bbb)
			printf("id_l=0x%08lx id_h=0x%08lx", id_l, id_h);
		
		offset += DFH_FEATURE_NEXT(dfh);
		printf("\n");
	} while(!DFH_FEATURE_EOL(dfh));
	
	return 1;
}

bool find_dfh_by_guid(fpga_handle afc_handle, 
	uint64_t find_id_l, uint64_t find_id_h, 
	uint64_t *result_offset, uint64_t *result_next_offset)
{
	if(result_offset)
		*result_offset = 0;
	if(result_next_offset)
		*result_next_offset = 0;
	
	if(find_id_l == 0)
		return 0;
	if(find_id_l == 0)
		return 0;

	uint64_t offset = 0;
	uint64_t dfh = 0;
	
	do
	{
		mmio_read64_silent(afc_handle, offset, &dfh);
		int is_bbb = DFH_FEATURE_IS_BBB(dfh);
		int is_afu = DFH_FEATURE_IS_AFU(dfh);
		
		if(is_afu || is_bbb)
		{
			uint64_t id_l = 0;
			uint64_t id_h = 0;
			mmio_read64_silent(afc_handle, offset+8, &id_l);
			mmio_read64_silent(afc_handle, offset+16, &id_h);
			if(find_id_l == id_l && find_id_h == id_h)
			{
				if(result_offset)
					*result_offset = offset;
				if(result_next_offset)
					*result_next_offset = DFH_FEATURE_NEXT(dfh);
				return 1;
			}
		}
		
		offset += DFH_FEATURE_NEXT(dfh);
	} while(!DFH_FEATURE_EOL(dfh));
	
	return 0;
}

bool find_dfh_by_guid(fpga_handle afc_handle, 
	const char *guid_str,
	uint64_t *result_offset, uint64_t *result_next_offset)
{
	fpga_guid          guid;

	fpga_result     res = FPGA_OK;

	if (uuid_parse(guid_str, guid) < 0)
		return 0;
	
	uint32_t i;
	uint32_t s;
	
	uint64_t find_id_l = 0;
	uint64_t find_id_h = 0;
	
	// The API expects the MSB of the GUID at [0] and the LSB at [15].
	s = 64;
	for (i = 0; i < 8; ++i) {
		s -= 8;
		find_id_h = ((find_id_h << 8) | (0xff & guid[i]));
	}
	
	s = 64;
	for (i = 0; i < 8; ++i) {
		s -= 8;
		find_id_l = ((find_id_l << 8) | (0xff & guid[8 + i]));
	}
	
	return find_dfh_by_guid(afc_handle, find_id_l, find_id_h, 
		result_offset, result_next_offset);
}

void check_guid(fpga_handle afc_handle, uint64_t id_l, uint64_t id_h, const char *name)
{
	uint64_t offset = 0;
	uint64_t size = 0;
	int has_guid = find_dfh_by_guid(afc_handle, id_l, id_h, &offset, &size);
	
	const char *has_guid_str = has_guid == 1 ? "found" : "not found";
	
	if(name)
		printf("%s is %s", name, has_guid_str);
	else
		printf("GUID %08lx:%08lx is %s", id_l, id_h, has_guid_str);
	
	if(has_guid)
		printf(" offset=%08lx size=%ld", offset, size);
	
	printf("\n");
}
