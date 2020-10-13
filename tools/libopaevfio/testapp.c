#include "opaevfio.h"

struct fme_dfh {
	uint64_t fme_version;
	uint64_t afu_id_low;
	uint64_t afu_id_high;
	uint64_t next_afu;
};

int main(int argc, char *argv[])
{
	struct opae_vfio_container c;
	int res;
	struct fme_dfh *mmio = NULL;
	uint8_t *p = NULL;

	if (argc < 3) {
		printf("usage: testapp /dev/vfio/X 0000:00:00.0\n");
		return 1;
	}

	res = opae_vfio_open(&c, argv[1], argv[2]);
	if (res) {
		return res;
	}

	opae_vfio_region_get(&c, 0, (uint8_t **)&mmio, NULL);

	printf("FME\n"
	       "dfh:         0x%016lx\n"
	       "afu_id low:  0x%016lx\n"
	       "afu_id high: 0x%016lx\n"
	       "next afu:    0x%016lx\n",
	       mmio->fme_version, mmio->afu_id_low,
	       mmio->afu_id_high, mmio->next_afu);

	opae_vfio_region_get(&c, 2, (uint8_t **)&mmio, NULL);

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

	opae_vfio_close(&c);

	return 0;
}
