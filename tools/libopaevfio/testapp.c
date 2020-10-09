#include "opaevfio.h"

int main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	struct opae_vfio_container c;
	int res;

	res = opae_vfio_open_container(&c);
	if (res) {
		return res;
	}




	return 0;
}
