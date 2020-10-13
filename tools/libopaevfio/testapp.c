#include "opaevfio.h"

struct fme_dfh {
	uint64_t fme_version;
	uint64_t afu_id_low;
	uint64_t afu_id_high;
	uint64_t next_afu;
};

void print_dfhs(struct opae_vfio_container *c)
{
	struct fme_dfh *mmio = NULL;
	uint8_t *p = NULL;

	opae_vfio_region_get(c, 0, (uint8_t **)&mmio, NULL);

	printf("FME\n"
	       "dfh:         0x%016lx\n"
	       "afu_id low:  0x%016lx\n"
	       "afu_id high: 0x%016lx\n"
	       "next afu:    0x%016lx\n",
	       mmio->fme_version, mmio->afu_id_low,
	       mmio->afu_id_high, mmio->next_afu);

	opae_vfio_region_get(c, 2, (uint8_t **)&mmio, NULL);

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

void allocate_bufs(struct opae_vfio_container *c)
{
	size_t sz_2m;
	size_t sz_1g;
	uint8_t *buf_2m_virt;
	uint8_t *buf_1g_virt;
	uint64_t iova_2m;
	uint64_t iova_1g;

	sz_2m = 2 * 1024 * 1024;
	buf_2m_virt = NULL;
	iova_2m = 0;

	if (opae_vfio_buffer_allocate(c, &sz_2m,
				      &buf_2m_virt, &iova_2m)) {
		printf("whoops 2M!\n");
	}

	sz_1g = 1024 * 1024 * 1024;
	buf_1g_virt = NULL;
	iova_1g = 0;

	if (opae_vfio_buffer_allocate(c, &sz_1g,
				      &buf_1g_virt, &iova_1g)) {
		printf("whoops 1G!\n");
	}

	if (opae_vfio_buffer_free(c, buf_2m_virt)) {
		printf("whoops 2M free!\n");
	}
	if (opae_vfio_buffer_free(c, buf_1g_virt)) {
		printf("whoops 1G free!\n");
	}
}

int main(int argc, char *argv[])
{
	struct opae_vfio_container c;
	int res;

	if (argc < 3) {
		printf("usage: testapp /dev/vfio/X 0000:00:00.0\n");
		return 1;
	}

	res = opae_vfio_open(&c, argv[1], argv[2]);
	if (res) {
		return res;
	}

	//print_dfhs(&c);
	allocate_bufs(&c);

	opae_vfio_close(&c);

	return 0;
}
