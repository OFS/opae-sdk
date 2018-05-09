# Open Programmable Accelerator Engine (OPAE) Linux Device Driver Architecture #

.. toctree::

.. highlight:: c

.. highlight:: sh

.. highlight:: console

The OPAE Intel&reg; FPGA Linux Device Driver provides interfaces for userspace applications to
configure, enumerate, open, and access FPGA accelerators on platforms equipped
with Intel FPGA solutions. The OPAE FPGA driver also enables system-level management functions such
as FPGA reconfiguration and virtualization.

## Hardware Architecture ##

The Linux Operating System treats the FPGA hardware as a PCIe\* device. A predefined data structure
Device Feature List (DFL) defines PCIe memory.

![FPGA PCIe Device](FPGA_PCIe_Device.png "FPGA PCIe Device")

The Linux Device Driver uses PCIe Single Root I/O Virtualization (SR-IOV) to create Virtual Functions (VFs). The device driver
can assign individual accelerators to virtual machines (VMs).

![Virtualized FPGA PCIe Device](FPGA_PCIe_Device_SRIOV.png "Virtualized FPGA PCIe Device")

## FPGA Management Engine (FME) ##

The FPGA Management Engine provides error reporting, reconfiguration, performance reporting, and other 
infrastructure functions. Each FPGA has one FME which is always accessed through the Physical
Function (PF). The Intel Xeon&reg; Processor with Integrated FPGA also performs power and thermal management.
These functions are not available on the Intel Programmable Acceleration Card (PAC).  

User-space applications can acquire exclusive access to the FME using `open()`,
and release it using `close()` as a privileged user (root).

.. Note::

```
    If an application terminates without freeing the FME or Port resources, Linux closes all 
    file descriptors owned by the terminating process, freeing those resources.
```

## Port ##

A Port represents the interface between two components: 
* The FPGA Interface Manager (FIM) which is the static FPGA fabric 
* The Accelerator Function (AF) which is the partially reconfigurable region

The Port controls the communication from software to the AF and makes features such as reset and debug available.
A PCIe device may have several Ports. Assign multiple ports to VFs using the `FPGA_FME_PORT_ASSIGN` ioctl on the FME device.

## Accelerator Function (AF) ##

An AF attaches to a Port. The AF provides a 256 KB memory mapped I/O (MMIO) region for accelerator-specific control registers.

* Use `open()` on the Port device to acquire exclusive access to an AFU associated with the Port device.
* Use `close()`on the Port device to release the AFU associated with the Port device.
* Use `mmap()` on the Port device to map accelerator MMIO regions.

## Partial Reconfiguration (PR) ##

Use PR to reconfigure an AF file. Successful reconfiguration has two requirements:

* You must generate the reconfiguration AF for the exact FIM. The AF and FIM are compatible if their IDs match. You can
verify this match by comparing the interface ID in the AF header against the interface ID the available through 
```sysfs intel-fpga-dev.*i*/intel-fpga-fme.*j*/pr/interface_ID```. PR always performs this check before reconfiguring
the AF. 
* The AF must also target the reconfigurable region (Port) of the FPGA. 

In all other cases PR fails and may cause system instability.

.. note::

```
    Platforms that support 512-bit Partial Reconfiguration require
    binutils >= version 2.25.
```

 Close any software program accessing the FPGA, including software programs running in a virtualized host before
initiating PR. Here is the recommended sequence: 

1. Unload the driver from the guest
2. Unplug the VF from the guest

.. note::

```
    NOTE: Unplugging the VF from the guest while an application on the guest is
    still accessing its resources may lead to VM instabilities. We recommend
    closing all applications accessing the VF in the guest before unplugging the
    VF.
```
3. Disable SR-IOV
4. Perform PR
5. Enable SR-IOV
6. Plug the VF to the guest
7. Load the driver in the guest

## FPGA Virtualization ##

To enable accelerator access from applications running on a VM, assign the AF Port to a VF using the following process:

1. Release the Port from the PF using the `FPGA_FME_PORT_RELEASE` ioctl on the FME device.

2. Use the following command to enable SR-IOV and VFs. Each VF can own a single Port with an AF. In the following command,
N is the number of Port released from the PF.

```console
    echo N > $PCI_DEVICE_PATH/sriov_numvfs
```

3. Pass through the VFs to VMs. 

4. You access the AF on a VF from applications running on the VM using the same driver inside the VF.

.. Note::

```
You cannot assign an FME to a VF. Consequently, PR and other management functions are only available through
the PFs.
```
## Driver Organization ##

### PCIe Module Device Driver ###

!## Driver Organization ##

### PCIe Module Device Driver ###

![Driver Organization](Driver_Organization.png "Driver Organization")




FPGA devices appear as a PCIe devices. Once enumeration detects a PCIe PF or VF, the Linux OS loads the FPGA PCIe
device driver, `intel-fpga-pci.ko`. The device driver performs the following functions:

1. Creates an FPGA container device as parent of the feature devices.
2. Walks through the Device Feature List in PCIe device base address register (BAR) memory to discover feature devices
and their sub-features. Creates platform devices for the features and sub-features under the container device.
3. Supports SR-IOV.
4. Introduces the feature device infrastructure, which abstracts operations for sub-features and provides common functions 
to feature device drivers.

### PCIe Module Device Driver Functions ###

The PCIe Module Device Driver performs the following functions:

1. PCIe discovery, device enumeration, and feature discovery. 
2. Creates sysfs directories for the parent device, FME, and Port.
3. Creates the platform driver instances, causing the Linux kernel to load their respective platform module drivers.

### FME Platform Module Device Driver ###

The FME Platform Module Device Driver, `intel-fpga-fme.ko`, loads automatically after the PCIe driver creates the
FME Platform Module. It provides the following features for FPGA management:

1. Power and thermal management, error reporting, performance reporting, and other infrastructure functions. You can access
these functions via sysfs interfaces the FME driver provides. 

.. Note::

```
The Power and thermal management function are only available on the Intel Xeon Processor with Integrated FPGA.
```

2. Partial Reconfiguration. During PR sub-feature initialization, the FME driver registers the FPGA Manager framework
to support PR. When the FME receives an `FPGA_FME_PORT_PR` ioctl from user-space, it invokes the common interface
function from the FPGA Manager to reconfigure the AF using PR.

3. Port management for virtualization. The FME driver introduces two ioctls:
* `FPGA_FME_PORT_RELEASE` releases a Port from  the PF
* `FPGA_FME_PORT_ASSIGN` assigns a Port back to PF

After `FPGA_FME_PORT_RELEASE` completes, you can use the PCIe driver SR-IOV interfaces to reassign the Port to a VF. 

For more information, refer to "FPGA Virtualization".

### FME Platform Module Device Driver Functions ###

The FME Platform Module Device Driver performs the the following functions:

* Creates the FME character device node.
* Creates the FME sysfs files and implements the FME sysfs file accessors.
* Implements the FME private feature sub-drivers.
* FME private feature sub-drivers:
    * FME Header
    * Thermal Management - available only on the  Intel Xeon Processor with Integrated FPGA
    * Power Management - available only on the  Intel Xeon Processor with Integrated FPGA
    * Global Error
    * Partial Reconfiguration
    * Global Performance

### Port Platform Module Device Driver ###

After the PCIe Module Device Driver creates the Port Platform Module device, the FPGA Port and AF driver,
`intel-fpga-afu.ko`, are available. This module provides an interface for user-space applications to access the 
individual accelerators, including basic reset control on the Port, AF MMIO region export, DMA buffer mapping 
service, UMsg notification, and remote debug functions. UMsg is only supported on the Intel Xeon Processor 
with Integrated FPGA.

### Port Platform Module Device Driver Functions ###

The Port Platform Module Device Driver performs the the following functions:

* Creates the Port character device node.
* Creates the Port sysfs files and implements the Port sysfs file accessors.
* Implements the following Port private feature sub-drivers.
    * Port Header
    * AFU
    * Port Error
    * UMsg - UMsg is only supported through the Intel Xeon Processor with Integrated FPGA.
    * Signal Tap


## Application FPGA Device Enumeration ##

Applications enumerate the FPGA device from the sysfs hierarchy under `/sys/class/fpga`.

In the example below the host includes two Intel FPGA devices. Each FPGA device has one FME and two Ports (AFUs).

Each FPGA device has a device directory under `/sys/class/fpga`:

```c
    /sys/class/fpga/intel-fpga-dev.0
    /sys/class/fpga/intel-fpga-dev.1
```

Each node has one FME and two Ports (AFUs) as child devices:

```c
    /sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0
    /sys/class/fpga/intel-fpga-dev.0/intel-fpga-port.0
    /sys/class/fpga/intel-fpga-dev.0/intel-fpga-port.1

    /sys/class/fpga/intel-fpga-dev.1/intel-fpga-fme.1
    /sys/class/fpga/intel-fpga-dev.1/intel-fpga-port.2
    /sys/class/fpga/intel-fpga-dev.1/intel-fpga-port.3
```

In general, the FME/Port sysfs interfaces use the following naming convention:

```c
    /sys/class/fpga/intel-fpga-dev.i/intel-fpga-fme.j/
    /sys/class/fpga/intel-fpga-dev.i/intel-fpga-port.k/
```

* `i` consecutively numbers all of the container devices
* `j` consecutively numbers the FMEs
* `k` consecutively numbers all Ports

Use the following device nodes to make `ioctl()` and `mmap()` calls:

```c
    /dev/intel-fpga-fme.j
    /dev/intel-fpga-port.k
```

## PCIe Driver Enumeration ##

`intel-fpga-pci.ko` performs device enumeration. This section highlights the main data structures and 
functions of `intel-fpga-pci.ko`. For more detailed information refer to the source code, `pcie.c`.

### Enumeration Data Structures ###

```c
    enum fpga_id_type {
        PARENT_ID,
        FME_ID,
        PORT_ID,
        FPGA_ID_MAX
    };

    static struct idr fpga_ids[FPGA_ID_MAX];
```


```c
    struct fpga_chardev_info {
        const char *name;
        dev_t devt;
    };

    struct fpga_chardev_info fpga_chrdevs[] = {
        { .name = FPGA_FEATURE_DEV_FME },
        { .name = FPGA_FEATURE_DEV_PORT },
    };
```

```c
    static struct class *fpga_class;
```

```c
    static struct pci_device_id cci_pcie_id_tbl[] = {
    	{PCI_DEVICE(PCI_VENDOR_ID_INTEL, PCIe_DEVICE_ID_RCiEP0_MCP),}, 
	{PCI_DEVICE(PCI_VENDOR_ID_INTEL, PCIe_DEVICE_ID_VF_MCP),}, 
        {PCI_DEVICE(PCI_VENDOR_ID_INTEL, PCIe_DEVICE_ID_RCiEP0_SKX_P),},
        {PCI_DEVICE(PCI_VENDOR_ID_INTEL, PCIe_DEVICE_ID_VF_SKX_P),},
        {PCI_DEVICE(PCI_VENDOR_ID_INTEL, PCIe_DEVICE_ID_RCiEP0_DCP),},
        {PCI_DEVICE(PCI_VENDOR_ID_INTEL, PCIe_DEVICE_ID_VF_DCP),},
        {0,}
    };

    static struct pci_driver cci_pci_driver = {
        .name = DRV_NAME,
        .id_table = cci_pcie_id_tbl,
        .probe = cci_pci_probe,
        .remove = cci_pci_remove,
        .sriov_configure = cci_pci_sriov_configure
    };
```

```c
    struct cci_drvdata {
        int device_id;
        struct device *fme_dev;
        struct mutex lock;
        struct list_head port_dev_list;
        int released_port_num;
        struct list_head regions;
    };
```

```c
    struct build_feature_devs_info {
        struct pci_dev *pdev;
        void __iomem *ioaddr;
        void __iomem *ioend;
        int current_bar;
        void __iomem *pfme_hdr;
        struct device *parent_dev;
        struct platform_device *feature_dev;
    };
```

### Enumeration Flow ###

* `ccidrv_init()`
    * Initialize `fpga_ids` using `idr_init()`.
    * Initialize `fpga_chrdevs[i].devt` using `alloc_chrdev_region()`.
    * Initialize `fpga_class` using `class_create()`.
    * `pci_register_driver(&cci_pci_driver);`

* `cci_pci_probe()`
    * Enable the PCI device, request access to its regions, set PCI master mode, configure DMA.

* `cci_pci_create_feature_devs()` `build_info_alloc_and_init()`
    * Allocate a `struct build_feature_devs_info`, initialize it..
    * `.parent_dev` is set to a parent sysfs directory (intel-fpga-dev.*id*) that contains the FME and Port sysfs directories.

* `parse_feature_list()`
    * Walk the BAR0 Device Feature List to discover the FME, the Port, and their private features.

* `parse_feature()` `parse_feature_afus()` `parse_feature_fme()`
    * When enumeration discovers an FME:
        * `build_info_create_dev()`
             * Allocate a platform device for the FME, storing in `build_feature_devs_info.feature_dev`.
             * `feature_dev.id` is initialized to the result of `idr_alloc(fpga_ids[FME_ID],`
             * `feature_dev.parent` is set to `build_feature_devs_info.parent_dev`.
             * Allocate an array of `struct resource` in `feature_dev.resource`.
        * Allocate a `struct feature_platform_data`, initialize it, and store a pointer in `feature_dev.dev.platform_data`
            * `create_feature_instance()` `build_info_add_sub_feature()`
            * Initialize `feature_dev.resource[FME_FEATURE_ID_HEADER]`.
            * `feature_platform_data_add()`
            * Initialize `feature_platform_data.features[FME_FEATURE_ID_HEADER]`, everything but .fops.

* `parse_feature()` `parse_feature_afus()` `parse_feature_port()`
    * When enumeration discovers a Port:
        * `build_info_create_dev()`
            * Allocate a platform device for the Port, storing in `build_feature_devs_info.feature_dev`.
            * `feature_dev.id` is initialized to the result of `idr_alloc(fpga_ids[PORT_ID],`
            * `feature_dev.parent` is set to `build_feature_devs_info.parent_dev`.
            * Allocate an array of `struct resource` in `feature_dev.resource`.
            * Allocate a `struct feature_platform_data`, initialize it, and store a pointer in `feature_dev.dev.platform_data`
        * `build_info_commit_dev()`
            * Add the `struct feature_platform_data.node` for the Port to the list of Ports in `struct cci_drvdata.port_dev_list`
        * `create_feature_instance()` `build_info_add_sub_feature()`
            * Initialize `feature_dev.resource[PORT_FEATURE_ID_HEADER]`.
        * `feature_platform_data_add()`
            * Initialize `feature_platform_data.features[PORT_FEATURE_ID_HEADER]`, everything but .fops.

* `parse_feature()` `parse_feature_afus()` `parse_feature_port_uafu()`
    * When enumeration discovers an AFU:
        * `create_feature_instance()` `build_info_add_sub_feature()`
            * Initialize `feature_dev.resource[PORT_FEATURE_ID_UAFU]`.
        * `feature_platform_data_add()`
            * Initialize `feature_platform_data.features[PORT_FEATURE_ID_UAFU]`, everything but .fops.

* `parse_feature()` `parse_feature_private()` `parse_feature_fme_private()`
    * When enumeration discovers an FME private feature:
        * `create_feature_instance()` `build_info_add_sub_feature()`
            * Initialize `feature_dev.resource[id]`.
        * `feature_platform_data_add()`
            * Initialize `feature_platform_data.features[id]`, everything but .fops.

* `parse_feature()` `parse_feature_private()` `parse_feature_port_private()`
   * When a Port private feature is encountered:
        * `create_feature_instance()` `build_info_add_sub_feature()`
            * Initialize `feature_dev.resource[id]`.
        * `feature_platform_data_add()`
            * Initialize `feature_platform_data.features[id]`, everything but .fops.

* `parse_ports_from_fme()`
    * If the driver is loaded on the Physical Function (PF), then..
        * Run the `parse_feature_list()` flow on each port described in the FME header.
        * Use the BAR mentioned in each Port entry in the header.

## FME Platform Device Initialization ##

`intel-fpga-fme.ko` performs FME device initialization. This section highlights the main data structures and
and functions. For more information refer to the source code, `fme-main.c`. 

### FME Platform Device Data Structures ###

```c
    struct feature_ops {
        int (*init)(struct platform_device *pdev, struct feature *feature);
        int (*uinit)(struct platform_device *pdev, struct feature *feature);
        long (*ioctl)(struct platform_device *pdev, struct feature *feature,
                    unsigned int cmd, unsigned long arg);
        int (*test)(struct platform_device *pdev, struct feature *feature);
    };
```

```c
    struct feature {
        const char *name;
        int resource_index;
        void __iomem *ioaddr;
        struct feature_ops *ops;
    };
```

```c
    struct feature_platform_data {
        struct list_head node;
        struct mutex lock;
        unsigned long dev_status;
        struct cdev cdev;
        struct platform_device *dev;
        unsigned int disable_count;
        void *private;
        int num;
        int (*config_port)(struct platform_device *, u32, bool);
        struct platform_device *(*fpga_for_each_port)(struct platform_device *,
                void *, int (*match)(struct platform_device *, void *));
        struct feature features[0];
    };
```

```c
    struct perf_object {
        int id;
        const struct attribute_group **attr_groups;
        struct device *fme_dev;
        struct list_head node;
        struct list_head children;
        struct kobject kobj;
    };
```

```c
    struct fpga_fme {
        u8 port_id;
        u64 pr_err;
        struct device *dev_err;
        struct perf_object *perf_dev;
        struct feature_platform_data *pdata;
    };
```

### FME Platform Device Initialization Flow ###

![FME Initialization Flow](fme_init_flow.png "FME Initialization Flow")

* `fme_probe()` `fme_dev_init()`
    * Initialize a `struct fpga_fme` and store it in the `feature_platform_data.private` field.

* `fme_probe()` `fpga_dev_feature_init()` `feature_instance_init()`
    * Save a `struct feature_ops` into the `feature_platform_data.features` for each populated feature.
    * Call the `test` function, if any, from the struct.
    * Call the `init` function from the struct.

* `fme_probe()` `fpga_register_dev_ops()`
    * Create the FME character device node, registering a `struct file_operations`.

## Port Platform Device Initialization ##

`intel-fpga-afu.ko` performs Port device initialization.  This section highlights the main data structures and
and functions. For more detailed information refer to the source code, `afu.c`.

### Port Platform Device Data Structures ###

```c
    struct feature_ops {
        int (*init)(struct platform_device *pdev, struct feature *feature);
        int (*uinit)(struct platform_device *pdev, struct feature *feature);
        long (*ioctl)(struct platform_device *pdev, struct feature *feature,
                    unsigned int cmd, unsigned long arg);
        int (*test)(struct platform_device *pdev, struct feature *feature);
    };
```

```c
    struct feature {
        const char *name;
        int resource_index;
        void __iomem *ioaddr;
        struct feature_ops *ops;
    };
```

```c
    struct feature_platform_data {
        struct list_head node;
        struct mutex lock;
        unsigned long dev_status;
        struct cdev cdev;
        struct platform_device *dev;
        unsigned int disable_count;
        void *private;
        int num;
        int (*config_port)(struct platform_device *, u32, bool);
        struct platform_device *(*fpga_for_each_port)(struct platform_device *,
                void *, int (*match)(struct platform_device *, void *));
        struct feature features[0];
    };
```

```c
    struct fpga_afu_region {
        u32 index;
        u32 flags;
        u64 size;
        u64 offset;
        u64 phys;
        struct list_head node;
    };
```

```c
    struct fpga_afu_dma_region {
        u64 user_addr;
        u64 length;
        u64 iova;
        struct page **pages;
        struct rb_node node;
        bool in_use;
    };
```

```c
    struct fpga_afu {
        u64 region_cur_offset;
        int num_regions;
        u8 num_umsgs;
        struct list_head regions;
        struct rb_root dma_regions;
        struct feature_platform_data *pdata;
    };
```

### Port Platform Device Initialization Flow ###

![Port Initialization Flow](port_init_flow.png "Port Initialization Flow")

* `afu_probe()` `afu_dev_init()`
    * Initialize a `struct fpga_afu` and store it in the `feature_platform_data.private` field.

* `afu_probe()` `fpga_dev_feature_init()` `feature_instance_init()`
    * Save a `struct feature_ops` into the `feature_platform_data`.features for each populated feature.
    * Call the `test` function, if any, from the struct.
    * Call the `init` function from the struct.

* `afu_probe()` `fpga_register_dev_ops()`
    * Create the Port character device node, registering a `struct file_operations`.

## FME IOCTLs ##

Call the following ioctls on an open file descriptor for /dev/intel-fpga-fme.*j*

`FPGA_GET_API_VERSION`

* return the current version as an integer, starting from 0.

`FPGA_CHECK_EXTENSION`

* (not currently supported).

`FPGA_FME_PORT_RELEASE`

* arg is a pointer to a:

```c
    struct fpga_fme_port_release {
        __u32 argsz;   // in: sizeof(struct fpga_fme_port_release)
        __u32 flags;   // in: must be 0
        __u32 port_id; // in: port ID (from 0) to release.
    };
```

`FPGA_FME_PORT_ASSIGN`

* arg is a pointer to a:

```c
    struct fpga_fme_port_assign {
        __u32 argsz;   // in: sizeof(struct fpga_fme_port_assign)
        __u32 flags;   // in: must be 0
        __u32 port_id; // in: port ID (from 0) to assign. (must have been previously released by FPGA_FME_PORT_RELEASE)
    };
```

`FPGA_FME_PORT_PR`

* arg is a pointer to a:

```c
    struct fpga_fme_port_pr {
        __u32 argsz;          // in: sizeof(struct fpga_fme_port_pr)
        __u32 flags;          // in: must be 0
        __u32 port_id;        // in: port ID (from 0)
        __u32 buffer_size;    // in: size of bitstream buffer in bytes. Must be 4-byte aligned.
        __u64 buffer_address; // in: process address of bitstream buffer
        __u64 status;         // out: error status (bitmask)
    };
```


## Port IOCTLs ##

Call the following ioctls on an open file descriptor for /dev/intel-fpga-port.*k* .

`FPGA_GET_API_VERSION`

* return the current version as an integer, starting from 0.

`FPGA_CHECK_EXTENSION`

* (not currently supported).

`FPGA_PORT_GET_INFO`

* arg is a pointer to a:

```c
    struct fpga_port_info {
        __u32 argsz;       // in: sizeof(struct fpga_port_info)
        __u32 flags;       // out: returns 0
        __u32 num_regions; // out: number of MMIO regions, 2 (1 for AFU and 1 for STP)
        __u32 num_umsgs;   // out: number of UMsg's supported by the hardware
    };
```

`FPGA_PORT_GET_REGION_INFO`

* arg is a pointer to a:

```c
    struct fpga_port_region_info {
        __u32 argsz;   // in: sizeof(struct fpga_port_region_info)
        __u32 flags;   // out: (bitmask) { FPGA_REGION_READ, FPGA_REGION_WRITE, FPGA_REGION_MMAP }
        __u32 index;   // in: FPGA_PORT_INDEX_UAFU or FPGA_PORT_INDEX_STP
        __u32 padding; // in: must be 0
        __u64 size;    // out: size of MMIO region in bytes
        __u64 offset;  // out: offset of MMIO region from start of device fd
    };
```

`FPGA_PORT_DMA_MAP`

* arg is a pointer to a:

```c
    struct fpga_port_dma_map {
        __u32 argsz;     // in: sizeof(struct fpga_port_dma_map)
        __u32 flags;     // in: must be 0
        __u64 user_addr; // in: process virtual address. Must be page aligned.
        __u64 length;    // in: length of mapping in bytes. Must be a multiple of page size.
        __u64 iova;      // out: IO virtual address
    };
```

`FPGA_PORT_DMA_UNMAP`

* arg is a pointer to a:

```c
    struct fpga_port_dma_unmap {
        __u32 argsz; // in: sizeof(struct fpga_port_dma_unmap)
        __u32 flags; // in: must be 0
        __u64 iova;  // in: IO virtual address returned by a previous FPGA_PORT_DMA_MAP
    };
```

`FPGA_PORT_RESET`

* arg must be NULL.

`FPGA_PORT_UMSG_ENABLE`

* arg must be NULL.

`FPGA_PORT_UMSG_DISABLE`

* args must be NULL.

`FPGA_PORT_UMSG_SET_MODE`

* arg is a pointer to a:

```c
    struct fpga_port_umsg_cfg {
        __u32 argsz;       // in: sizeof(struct fpga_port_umsg_cfg)
        __u32 flags;       // in: must be 0
        __u32 hint_bitmap; // in: UMsg hint mode bitmap. Signifies which UMsg's are enabled.
    };
```

`FPGA_PORT_UMSG_SET_BASE_ADDR`

* Disable UMsg before issuing this ioctl.
* The buffer for the iova field must large enough for all UMsg's (num_umsgs * PAGE_SIZE).
    * The the driver's buffer management marks this buffer "in use".
    * If iova is NULL, the driver's buffer management marks any previous region "in use".
* arg is a pointer to a:

```c
    struct fpga_port_umsg_base_addr {
        __u32 argsz; // in: sizeof(struct fpga_port_umsg_base_addr)
        __u32 flags; // in: must be 0
        __u64 iova;  // in: IO virtual address from FPGA_PORT_DMA_MAP.
    };
```

.. Note::

```
    To clear the port errors, write the exact bitmask of the current errors, for example:

    $ cat errors > clear
```

# sysfs files #

The registers available on the Intel Programmable Acceleration Card (PAC) are a subset of the registers
available on the Intel Xeon Processor with Integrated FPGA. When the registers available for the two
platforms differ, the tables below include a fifth column to specify platform support. The tables use the
following abbreviations:

* Integrated FPGA - Intel Xeon Processor with Integrated FPGA
* PAC - Intel Programmable Acceleration Card (PAC)
## FME Header sysfs files ##

intel-fpga-dev.*i*/intel-fpga-fme.*j*/

| sysfs file         | mmio field                         | type         | access    |
|--------------------|------------------------------------|--------------|-----------|
| ports_num          | fme_header.capability.num_ports    | decimal int  | Read-only |
| cache_size         | fme_header.capability.cache_size   | decimal int  | Read-only |
| version            | fme_header.capability.fabric_verid | decimal int  | Read-only |
| socket_id          | fme_header.capability.socket_id    | decimal int  | Read-only |
| bitstream_id       | fme_header.bitstream_id            | hex uint64_t | Read-only |
| bitstream_metadata | fme_header.bitstream_md            | hex uint64_t | Read-only |

## FME Thermal Management sysfs files ##

intel-fpga-dev.*i*/intel-fpga-fme.*j*/thermal_mgmt/

| sysfs file         | mmio field                           | type         | access                           |platform support | 
|--------------------|--------------------------------------|--------------|----------------------------------|---------------- |
| threshold1         | thermal.threshold.tmp_thshold1       | decimal int  | User: Read-only Root: Read-write | Integrated FPGA |
| threshold2         | thermal.threshold.tmp_thshold2       | decimal int  | User: Read-only Root: Read-write | Integrated FPGA |
| threshold_trip     | thermal.threshold.therm_trip_thshold | decimal int  | Read-only                        | Integrated FPGA |
| threshold1_reached | thermal.threshold.thshold1_status    | decimal int  | Read-only                        | Integrated FPGA |
| threshold2_reached | thermal.threshold.thshold2_status    | decimal int  | Read-only                        | Integrated FPGA |
| threshold1_policy  | thermal.threshold.thshold_policy     | decimal int  | User: Read-only Root: Read-write | Integrated FPGA |
| temperature        | thermal.rdsensor_fm1.fpga_temp       | decimal int  | Read-only                        | Integrated FPGA, PAC |           |

## FME Power Management sysfs files ##

Power management is available only for the Intel Xeon Processor with Integrated FPGA. 

intel-fpga-dev.*i*/intel-fpga-fme.*j*/power_mgmt/

| sysfs file        | mmio field                        | type             | access                           |
|-------------------|-----------------------------------|------------------|----------------------------------|
| consumed          | power.status.pwr_consumed         | hex uint64_t     | Read-only                        |
| threshold1        | power.threshold.threshold1        | hex uint64_t     | User: Read-only Root: Read-write |
| threshold2        | power.threshold.threshold2        | hex uint64_t     | User: Read-only Root: Read-write |
| threshold1_status | power.threshold.threshold1_status | decimal unsigned | Read-only                        |
| threshold2_status | power.threshold.threshold2_status | decimal unsigned | Read-only                        |
| rtl               | power.status.fpga_latency_report  | decimal unsigned | Read-only                        |

## FME Global Error sysfs files ##

intel-fpga-dev.*i*/intel-fpga-fme.*j*/errors/

| sysfs file         | mmio field                     | type             | access     |platform support      |
|--------------------|--------------------------------|------------------|------------|--------------------- |
| pcie0_errors       | gerror.pcie0_err               | hex uint64_t     | Read-write | Integrated FPGA, PAC |   
| pcie1_errors       | gerror.pcie1_err               | hex uint64_t     | Read-write | Integrated FPGA      |
| gbs_errors         | gerror.ras_gerr                | hex uint64_t     | Read-only  | Integrated FPGA, PAC |
| bbs_errors         | gerror.ras_berr                | hex uint64_t     | Read-only  | Integrated FPGA, PAC |
| warning_errors     | gerror.ras_werr.event_warn_err | hex int          | Read-write | Integrated FPGA, PAC |
| inject_error       | gerror.ras_error_inj           | hex uint64_t     | Read-write | Integrated FPGA, PAC |

intel-fpga-dev.*i*/intel-fpga-fme.*j*/errors/fme-errors/

| sysfs file         | mmio field                             | type             | access     |
|--------------------|----------------------------------------|------------------|------------|
| errors             | gerror.fme_err                         | hex uint64_t     | Read-only  |
| first_error        | gerror.fme_first_err.err_reg_status    | hex uint64_t     | Read-only  |
| next_error         | gerror.fme_next_err.err_reg_status     | hex uint64_t     | Read-only  |
| clear              | Clears errors, first_error, next_error | various uint64_t | Write-only |

.. note::

```
    To clear the FME errors, write the exact bitmask of the current errors, for example:
```

```sh
    cat errors > clear
```

## FME Partial Reconfiguration sysfs files ##

intel-fpga-dev.*i*/intel-fpga-fme.*j*/pr/ 

| sysfs file   | mmio field                                    | type        | access    |
|--------------|-----------------------------------------------|-------------|-----------|
| interface_id | pr.fme_pr_intfc_id0_h, pr.fme_pre_intfc_id0_l | hex 16-byte | Read-only |

## FME Global Performance sysfs files ##

intel-fpga-dev.*i*/intel-fpga-fme.*j*/dperf/clock

| sysfs file | mmio field                 | type         | access    |
|------------|----------------------------|--------------|-----------|
| clock      | gperf.clk.afu_interf_clock | hex uint64_t | Read-only |

intel-fpga-dev.*i*/intel-fpga-fme.*j*/perf/cache/     (Not valid for Acceleration Stack for Intel&reg; Xeon&reg; CPU with FPGAs)


| sysfs file                 | mmio field                      | type         | access     |
|----------------------------|---------------------------------|--------------|------------|
| freeze                     | gperf.ch_ctl.freeze             | decimal int  | Read-write |
| read_hit                   | gperf.CACHE_RD_HIT              | hex uint64_t | Read-only  |
| read_miss                  | gperf.CACHE_RD_MISS             | hex uint64_t | Read-only  |
| write_hit                  | gperf.CACHE_WR_HIT              | hex uint64_t | Read-only  |
| write_miss                 | gperf.CACHE_WR_MISS             | hex uint64_t | Read-only  |
| hold_request               | gperf.CACHE_HOLD_REQ            | hex uint64_t | Read-only  |
| tx_req_stall               | gperf.CACHE_TX_REQ_STALL        | hex uint64_t | Read-only  |
| rx_req_stall               | gperf.CACHE_RX_REQ_STALL        | hex uint64_t | Read-only  |
| data_write_port_contention | gperf.CACHE_DATA_WR_PORT_CONTEN | hex uint64_t | Read-only  |
| tag_write_port_contention  | gperf.CACHE_TAG_WR_PORT_CONTEN  | hex uint64_t | Read-only  |

intel-fpga-dev.*i*/intel-fpga-fme.*j*/perf/iommu/   

Power management is only available for the Intel Xeon Processor with Integrated FPGA.

| sysfs file | mmio field           | type        | access                           |
|------------|----------------------|-------------|----------------------------------|
| freeze     | gperf.vtd_ctl.freeze | decimal int | User: Read-only Root: Read-write |

intel-fpga-dev.*i*/intel-fpga-fme.*j*/perf/iommu/afu*k*/   
Power management is only available for the Intel Xeon Processor with Integrated FPGA.


| sysfs file        | mmio field                  | type         | access    |
|-------------------|-----------------------------|--------------|-----------|
| read_transaction  | gperf.VTD_AFU0_MEM_RD_TRANS | hex uint64_t | Read-only |
| write_transaction | gperf.VTD_AFU0_MEM_WR_TRANS | hex uint64_t | Read-only |
| tlb_read_hit      | gperf.VTD_AFU0_TLB_RD_HIT   | hex uint64_t | Read-only |
| tlb_write_hit     | gperf.VTD_AFU0_TLB_WR_HIT   | hex uint64_t | Read-only |

intel-fpga-dev.*i*/intel-fpga-fme.*j*/dperf/fabric/

| sysfs file  | mmio field              | type         | access                           | platform support     |
|-------------|-------------------------|--------------|----------------------------------|--------------------- |
| enable      | gperf.fab_ctl.(enabled) | decimal int  | User: Read-only Root: Read-write | Integrated FPGA, PAC |
| freeze      | gperf.fab_ctl.freeze    | decimal int  | User: Read-only Root: Read-write | Integrated FPGA, PAC |
| pcie0_read  | gperf.FAB_PCIE0_RD      | hex uint64_t | Read-only                        | Integrated FPGA, PAC |
| pcie0_write | gperf.FAB_PCIE0_WR      | hex uint64_t | Read-only                        | Integrated FPGA, PAC |
| pcie1_read  | gperf.FAB_PCIE1_RD      | hex uint64_t | Read-only                        | Integrated FPGA      |
| pcie1_write | gperf.FAB_PCIE1_WR      | hex uint64_t | Read-only                        | Integrated FPGA      |
| upi_read    | gperf.FAB_UPI_RD        | hex uint64_t | Read-only                        | Integrated FPGA      |
| upi_write   | gperf.FAB_UPI_WR        | hex uint64_t | Read-only                        | Integrated FPGA      |

intel-fpga-ev.*i*/intel-fpga/fme.*j*/dperf/fabric/port*k*/

| sysfs file  | mmio field         | type         | access    | Integrated FPGA      |
|-------------|--------------------|--------------|-----------|--------------------- |
| pcie0_read  | gperf.FAB_PCIE0_RD | hex uint64_t | Read-only | Integrated FPGA, PAC |
| pcie0_write | gperf.FAB_PCIE0_WR | hex uint64_t | Read-only | Integrated FPGA, PAC |
| pcie1_read  | gperf.FAB_PCIE1_RD | hex uint64_t | Read-only | Integrated FPGA      |
| pcie1_write | gperf.FAB_PCIE1_WR | hex uint64_t | Read-only | Integrated FPGA      |
| upi_read    | gperf.FAB_UPI_RD   | hex uint64_t | Read-only | Integrated FPGA      |
| upi_write   | gperf.FAB_UPI_WR   | hex uint64_t | Read-only | Integrated FPGA      |

## Port Header sysfs files ##

intel-fpga-dev.*i*/intel-fpga-port.*k*/

| sysfs file | mmio field                            | type        | access    |
|------------|---------------------------------------|-------------|-----------|
| id         | port_header.capability.port_number    | decimal int | Read-only |
| ltr        | port_header.control.latency_tolerance | decimal int | Read-only |

## Port AFU Header sysfs files ##

intel-fpga-dev.*i*/intel-fpga-port.*k*/

| sysfs file | mmio field      | type        | access    |
|------------|-----------------|-------------|-----------|
| afu_id     | afu_header.guid | hex 16-byte | Read-only |

## Port Error sysfs files ## 

intel-fpga-dev.*i*/intel-fpga-port.*k*/errors/

| sysfs file          | mmio field              | type             | access     |
|---------------------|-------------------------|------------------|------------|
| errors              | perror.port_error       | hex uint64_t     | Read-only  |
| first_error         | perror.port_first_error | hex uint64_t     | Read-only  |
| first_malformed_req | perror.malreq           | hex 16-byte      | Read-only  |
| clear               | perror.(all errors)     | various uint64_t | Write-only |

.. Note::

```
    To clear the Port errors, write the exact bitmask of the current errors, for example:
```

```sh
    cat errors > clear
```
