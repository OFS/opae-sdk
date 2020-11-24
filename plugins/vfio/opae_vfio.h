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
	bdf_t bdf;
	uint32_t vendor;
	uint32_t device;
	uint32_t numa_node;
	struct _pci_device *next;
} pci_device_t;

typedef struct _vfio_ops
{
	fpga_result (*reset)(const pci_device_t *p, volatile uint8_t *mmio);
} vfio_ops;

#define USER_MMIO_MAX 8
typedef struct _vfio_token
{
	uint32_t magic;
	fpga_guid guid;
	fpga_guid compat_id;
	const pci_device_t *device;
	uint32_t region;
	uint32_t offset;
	uint32_t mmio_size;
	uint32_t pr_control;
	uint32_t user_mmio_count;
	uint32_t user_mmio[USER_MMIO_MAX];
	uint64_t bitstream_id;
	uint64_t bitstream_mdata;
	uint32_t type;
	struct _vfio_token *parent;
	struct _vfio_token *next;
	vfio_ops ops;
} vfio_token;

typedef struct _vfio_handle
{
	uint32_t magic;
	struct _vfio_token *token;
	struct opae_vfio vfio_device;
	volatile uint8_t *mmio_base;
	size_t mmio_size;
} vfio_handle;


int pci_discover();
int features_discover();
pci_device_t *get_pci_device(char addr[PCIADDR_MAX]);
void free_device_list();
void free_token_list();
vfio_token *get_token(const pci_device_t *p, uint32_t region, int type);
fpga_result get_guid(uint64_t* h, fpga_guid guid);
#endif
