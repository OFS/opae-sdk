#include "opaevfio.h"

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






	opae_vfio_close(&c);

	return 0;
}
