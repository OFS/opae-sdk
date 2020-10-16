#include <string.h>
#include <unistd.h>
#include "opaevfio.h"

struct fme_dfh {
	uint64_t fme_version;
	uint64_t afu_id_low;
	uint64_t afu_id_high;
	uint64_t next_afu;
};

void print_dfhs(struct opae_vfio *v)
{
	struct fme_dfh *mmio = NULL;
	uint8_t *p = NULL;

	opae_vfio_region_get(v, 0, (uint8_t **)&mmio, NULL);

	printf("FME\n"
	       "dfh:         0x%016lx\n"
	       "afu_id low:  0x%016lx\n"
	       "afu_id high: 0x%016lx\n"
	       "next afu:    0x%016lx\n",
	       mmio->fme_version, mmio->afu_id_low,
	       mmio->afu_id_high, mmio->next_afu);

	opae_vfio_region_get(v, 2, (uint8_t **)&mmio, NULL);

	printf("Port\n"
	       "dfh:         0x%016lx\n"
	       "afu_id low:  0x%016lx\n"
	       "afu_id high: 0x%016lx\n"
	       "next afu:    0x%016lx\n",
	       mmio->fme_version, mmio->afu_id_low,
	       mmio->afu_id_high, mmio->next_afu);

	p = (uint8_t *) mmio;
	p += 0x40000;
	mmio = (struct fme_dfh *) p;

	printf("AFU\n"
	       "dfh:         0x%016lx\n"
	       "afu_id low:  0x%016lx\n"
	       "afu_id high: 0x%016lx\n"
	       "next afu:    0x%016lx\n",
	       mmio->fme_version, mmio->afu_id_low,
	       mmio->afu_id_high, mmio->next_afu);
}

void allocate_bufs(struct opae_vfio *v)
{
	size_t sz_4k;
	size_t sz_2m;
	size_t sz_1g;
	uint8_t *buf_4k_virt;
	uint8_t *buf_2m_virt;
	uint8_t *buf_1g_virt;
	uint64_t iova_4k;
	uint64_t iova_2m;
	uint64_t iova_1g;

	sz_4k = 4096;
	buf_4k_virt = NULL;
	iova_4k = 0;

	if (opae_vfio_buffer_allocate(v, &sz_4k,
				      &buf_4k_virt, &iova_4k)) {
		printf("whoops 4K!\n");
	}

	sz_2m = 2 * 1024 * 1024;
	buf_2m_virt = NULL;
	iova_2m = 0;

	if (opae_vfio_buffer_allocate(v, &sz_2m,
				      &buf_2m_virt, &iova_2m)) {
		printf("whoops 2M!\n");
	}

	sz_1g = 1024 * 1024 * 1024;
	buf_1g_virt = NULL;
	iova_1g = 0;

	if (opae_vfio_buffer_allocate(v, &sz_1g,
				      &buf_1g_virt, &iova_1g)) {
		printf("whoops 1G!\n");
	}

	if (opae_vfio_buffer_free(v, buf_2m_virt)) {
		printf("whoops 2M free!\n");
	}
	if (opae_vfio_buffer_free(v, buf_4k_virt)) {
		printf("whoops 4K free!\n");
	}
	if (opae_vfio_buffer_free(v, buf_1g_virt)) {
		printf("whoops 1G free!\n");
	}
}

#define AFU_OFFSET 0x40000
#define CSR_SRC_ADDR      (AFU_OFFSET + 0x0120)
#define CSR_DST_ADDR      (AFU_OFFSET + 0x0128)
#define CSR_CTL           (AFU_OFFSET + 0x0138)
#define CSR_STATUS1       (AFU_OFFSET + 0x0168)
#define CSR_CFG           (AFU_OFFSET + 0x0140)
#define CSR_NUM_LINES     (AFU_OFFSET + 0x0130)
#define CSR_AFU_DSM_BASEL (AFU_OFFSET + 0x0110)

#define DSM_STATUS_TEST_COMPLETE 0x40

void nlb0(struct opae_vfio *v)
{
	volatile uint8_t *afu = NULL;
	volatile uint32_t *p32;
	volatile uint64_t *p64;
	volatile uint64_t *test_status;

#define wrcsr64(__offset, __value)                     \
do                                                     \
{                                                      \
	p64 = (volatile uint64_t *)(afu + (__offset)); \
	*p64 = (__value);                              \
} while(0)

#define wrcsr32(__offset, __value)                     \
do                                                     \
{                                                      \
	p32 = (volatile uint32_t *)(afu + (__offset)); \
	*p32 = (__value);                              \
} while(0)

	size_t size = 2 * 1024 * 1024;

	uint8_t *afu_dsm_virt = NULL;
	uint8_t *src_virt = NULL;
	uint8_t *dst_virt = NULL;

	uint64_t afu_dsm_iova = 0;
	uint64_t src_iova = 0;
	uint64_t dst_iova = 0;


	if (opae_vfio_region_get(v, 2, (uint8_t **)&afu, NULL))
		printf("whoops afu mmio\n");

	if (opae_vfio_buffer_allocate(v, &size,
				      &afu_dsm_virt, &afu_dsm_iova))
		printf("whoops alloc afu dsm\n");
	
	if (opae_vfio_buffer_allocate(v, &size,
				      &src_virt, &src_iova))
		printf("whoops alloc src buf\n");
	
	if (opae_vfio_buffer_allocate(v, &size,
				      &dst_virt, &dst_iova))
		printf("whoops alloc dst buf\n");

	memset(afu_dsm_virt, 0, size);
	memset(src_virt, 0xaf, size);
	memset(dst_virt, 0xbe, size);

	wrcsr64(CSR_AFU_DSM_BASEL, afu_dsm_iova);
	wrcsr32(CSR_CTL, 0);
	wrcsr32(CSR_CTL, 1);
	wrcsr64(CSR_SRC_ADDR, (src_iova >> 6));
	wrcsr64(CSR_DST_ADDR, (dst_iova >> 6));
	wrcsr32(CSR_NUM_LINES, (size >> 6));
	wrcsr32(CSR_CFG, 0x42000);

	test_status = (volatile uint64_t *) (afu_dsm_virt + DSM_STATUS_TEST_COMPLETE);
	wrcsr32(CSR_CTL, 3);

	while (0 == (*test_status & 0x1)) {
		usleep(1000);
	}

	wrcsr32(CSR_CTL, 7);

	if (memcmp(src_virt, dst_virt, size))
		printf("whoops buffer mismatch\n");
	else
		printf("NLB0 OK\n");

	if (opae_vfio_buffer_free(v, afu_dsm_virt))
		printf("whoops free afu dsm\n");
	if (opae_vfio_buffer_free(v, src_virt))
		printf("whoops free src\n");
	if (opae_vfio_buffer_free(v, dst_virt))
		printf("whoops free dst\n");
}

int main(int argc, char *argv[])
{
	struct opae_vfio v;
	int res;

	if (argc < 2) {
		printf("usage: testapp 0000:00:00.0\n");
		return 1;
	}

	res = opae_vfio_open(&v, argv[1]);
	if (res) {
		return res;
	}

	//print_dfhs(&v);
	//allocate_bufs(&v);
	nlb0(&v);

	opae_vfio_close(&v);

	return 0;
}
