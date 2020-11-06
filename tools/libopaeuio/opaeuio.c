// Copyright(c) 2020, Intel Corporation
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <glob.h>
#include <dirent.h>

#include <opae/uio.h>

#define __SHORT_FILE__                                    \
({                                                        \
	const char *file = __FILE__;                      \
	const char *p = file;                             \
	while (*p)                                        \
		++p;                                      \
	while ((p > file) && ('/' != *p) && ('\\' != *p)) \
		--p;                                      \
	if (p > file)                                     \
		++p;                                      \
	p;                                                \
})

#define ERR(format, ...)                                \
fprintf(stderr, "%s:%u:%s() **ERROR** [%s] : " format , \
	__SHORT_FILE__, __LINE__, __func__, strerror(errno), ##__VA_ARGS__)

STATIC int opae_uio_read_sysfs_uint64(const char *path, uint64_t *puint)
{
	FILE *fp;

	fp = fopen(path, "r");
	if (!fp) {
		ERR("failed to fopen(\"%s\", \"r\")\n", path);
		return 1;
	}

	if (fscanf(fp, "%lx", puint) != 1) {
		ERR("fscanf() failed\n");
		fclose(fp);
		return 2;
	}

	fclose(fp);

	return 0;
}

STATIC struct opae_uio_device_region *
opae_uio_device_region_create(uint32_t index,
			      uint8_t *ptr,
			      size_t page_offset,
			      size_t size)
{
	struct opae_uio_device_region *r;
	r = malloc(sizeof(*r));
	if (r) {
		r->region_index = index;
		r->region_ptr = ptr;
		r->region_page_offset = page_offset;
		r->region_size = size;
		r->next = NULL;
	}
	return r;
}

STATIC void
opae_uio_device_region_destroy(struct opae_uio_device_region *r)
{
	while (r) {
		struct opae_uio_device_region *trash = r;
		r = r->next;
		if (trash->region_ptr != MAP_FAILED) {
			if (munmap(trash->region_ptr, trash->region_size) < 0)
				ERR("munmap() failed\n");
		}
		free(trash);
	}
}

STATIC void opae_uio_destroy(struct opae_uio *u)
{
	opae_uio_device_region_destroy(u->regions);
	u->regions = NULL;

	if (u->device_fd >= 0) {
		close(u->device_fd);
		u->device_fd = -1;
	}
}

STATIC struct opae_uio_device_region *
opae_uio_enum_region(int fd, char *sysfs_path)
{
	uint32_t index = 0;
	uint8_t *ptr = NULL;
	uint64_t page_offset = 0;
	uint64_t size = 0;
	char *p;
	char *endptr;
	char path[OPAE_UIO_PATH_MAX];
	uint32_t page_size = 0;

	// The region index is encoded in the file name component.
	p = strrchr(sysfs_path, '/');
	if (!p) {
		ERR("invalid map path: \"%s\"\n", sysfs_path);
		return NULL;
	}

	endptr = NULL;
	index = strtoul(p + 4, &endptr, 10);
	if (*endptr) {
		ERR("invalid map index: \"%s\"\n", p + 4);
		return NULL;
	}

	// The size is available in the mapX/size sysfs file.
	if (snprintf(path, sizeof(path),
		     "%s/size", sysfs_path) < 0) {
		ERR("snprintf() failed\n");
		return NULL;
	}

	if (opae_uio_read_sysfs_uint64(path, &size)) {
		ERR("failed to query region size\n");
		return NULL;
	}

	// The page offset is available in the mapX/offset sysfs file.
	if (snprintf(path, sizeof(path),
		     "%s/offset", sysfs_path) < 0) {
		ERR("snprintf() failed\n");
		return NULL;
	}

	if (opae_uio_read_sysfs_uint64(path, &page_offset)) {
		ERR("failed to query region page offset\n");
		return NULL;
	}

	// Map the region into our address space. To map region X,
	// we pass (X * <page size>) to the offset parameter of mmap().
	page_size = (uint32_t) sysconf(_SC_PAGE_SIZE);

	ptr = mmap(NULL,
		   (size_t) size,
		   PROT_READ|PROT_WRITE,
		   MAP_SHARED,
		   fd,
		   (off_t) (index * page_size));
	if (ptr == MAP_FAILED) {
		ERR("mmap() failed\n");
		return NULL;
	}

	return opae_uio_device_region_create(index,
					     ptr,
					     (size_t) page_offset,
					     (size_t) size);
}

STATIC struct opae_uio_device_region *
opae_uio_enum_regions(int fd, const char *path)
{
	struct opae_uio_device_region *region_list = NULL;
	struct opae_uio_device_region **lptr = &region_list;
	DIR *dir;
	struct dirent *dirent;
	char sysfs_path[OPAE_UIO_PATH_MAX];

	dir = opendir(path);
	if (!dir) {
		ERR("opendir(\"%s\") failed\n", path);
		return NULL;
	}

	dirent = readdir(dir);
	while (dirent) {

		if (!strcmp(dirent->d_name, ".") ||
		    !strcmp(dirent->d_name, "..")) {
			dirent = readdir(dir);
			continue;
		}

		if (snprintf(sysfs_path, sizeof(sysfs_path),
			     "%s/%s", path, dirent->d_name) < 0) {
			ERR("snprintf() failed\n");
			goto out_close;
		}

		*lptr = opae_uio_enum_region(fd, sysfs_path);
		if (*lptr)
			lptr = &(*lptr)->next;

		dirent = readdir(dir);
	}

out_close:
	if (closedir(dir) < 0)
		ERR("closedir() failed\n");

	return region_list;
}

STATIC int opae_uio_init(struct opae_uio *u, const char *dfl_device)
{
	char path_expr[OPAE_UIO_PATH_MAX];
	glob_t globbuf;
	int res = 0;
	size_t len;
	char *p;

	memset(u, 0, sizeof(*u));
	u->device_fd = -1;

	// Use glob to discover the uio device name.
	if (snprintf(path_expr, sizeof(path_expr),
		     "/sys/bus/dfl/devices/%s/uio_pdrv_genirq.*.auto/uio/uio*",
		     dfl_device) < 0) {
		ERR("snprintf() failed\n");
		return 1;
	}

	if (glob(path_expr, GLOB_NOSORT, NULL, &globbuf)) {
		ERR("glob(%s) failed\n", path_expr);
		res = 2;
		goto out_glob_free;
	}

	if (globbuf.gl_pathc > 1) {
		ERR("Found more than one possible UIO device!\n");
		res = 3;
		goto out_glob_free;
	}

	len = strlen(globbuf.gl_pathv[0]);
	if (len >= OPAE_UIO_PATH_MAX) {
		ERR("len: %lu is too large. Increase the size of "
		    "OPAE_UIO_PATH_MAX\n", len);
		res = 4;
		goto out_glob_free;
	}

	p = strrchr(globbuf.gl_pathv[0], '/');
	if (!p) {
		ERR("bad glob path: \"%s\"\n", globbuf.gl_pathv[0]);
		res = 5;
		goto out_glob_free;
	}

	if (snprintf(u->device_path, sizeof(u->device_path),
		     "/dev/%s", p + 1) < 0) {
		ERR("snprintf() failed\n");
		res = 6;
		goto out_glob_free;
	}

	u->device_fd = open(u->device_path, O_RDWR);
	if (u->device_fd < 0) {
		ERR("failed to open(\"%s\")\n", u->device_path);
		res = 7;
		goto out_destroy;
	}

	// Walk /sys/class/uio/uioX/maps to discover the regions.
	if (snprintf(path_expr, sizeof(path_expr),
		     "/sys/class/uio/%s/maps", p + 1) < 0) {
		ERR("snprintf() failed\n");
		res = 8;
		goto out_destroy;
	}

	u->regions = opae_uio_enum_regions(u->device_fd, path_expr);

	goto out_glob_free;

out_destroy:
	opae_uio_destroy(u);
out_glob_free:
	if (globbuf.gl_pathv)
		globfree(&globbuf);
	return res;
}

int opae_uio_open(struct opae_uio *u, const char *dfl_device)
{
	if (!u || !dfl_device) {
		ERR("NULL param\n");
		return 1;
	}

	return opae_uio_init(u, dfl_device);
}

int opae_uio_region_get(struct opae_uio *u, uint32_t index,
			uint8_t **ptr, size_t *size)
{
	struct opae_uio_device_region *r;

	for (r = u->regions ; r ; r = r->next) {
		if (index == r->region_index) {
			if (ptr)
				*ptr = r->region_ptr + r->region_page_offset;
			if (size)
				*size = r->region_size;
			return 0;
		}
	}

	return 1;
}

void opae_uio_close(struct opae_uio *u)
{
	if (!u) {
		ERR("NULL param\n");
		return;
	}

	opae_uio_destroy(u);
}
