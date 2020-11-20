#ifndef _OPAE_VFIO_PLUGIN_H
#define _OPAE_VFIO_PLUGIN_H
#include <opae/vfio.h>
#include <opae/fpga.h>

typedef union _bdf
{
	struct
	{
		uint16_t segment;
		uint8_t bus;
		uint8_t device : 5;
		uint8_t function : 3;
	};
	uint32_t bdf;
} bdf_t;

#define PCIADDR_MAX 16
typedef struct _pci_device
{
	char addr[PCIADDR_MAX];
	struct opae_vfio vfio_device;
	bdf_t bdf;
	uint32_t vendor;
	uint32_t device;
	uint32_t numa_node;
	struct _pci_device *next;
} pci_device_t;

struct _vfio_token;

typedef struct _vfio_ops
{
	fpga_result (*reset)(struct _vfio_token *t);
	fpga_result (*clear_errors)(struct _vfio_token *t);
} vfio_ops;


typedef struct _user_mmio_t
{
  volatile uint8_t *base;
  size_t size;
} user_mmio_t;

#define USER_MMIO_MAX 8
typedef struct _vfio_token
{
	uint32_t magic;
	fpga_guid guid;
	const pci_device_t *device;
	uint32_t region;
	volatile uint8_t *address;
	user_mmio_t user_mmio[USER_MMIO_MAX];
	uint32_t user_mmio_count;
	int fd;
	uint32_t type;
	struct _vfio_token *parent;
	struct _vfio_token *next;
	vfio_ops ops;
} vfio_token;

typedef struct _vfio_handle
{
	uint32_t magic;
	struct _vfio_token *token;
} vfio_handle;


int pci_discover();
int features_discover();
pci_device_t *get_pci_device(char addr[PCIADDR_MAX]);
void free_device_list();
void free_token_list();
vfio_token *get_token(const pci_device_t *p, uint32_t region, int fd, volatile uint8_t *mmio, int type);
fpga_result get_guid(uint64_t* h, fpga_guid guid);
#endif
