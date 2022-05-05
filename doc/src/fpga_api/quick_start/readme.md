# Quick Start Guide

```{toctree}
```

## Overview

The OPAE C library is a lightweight user-space library that provides
an abstraction for FPGA resources in a compute environment. Built on top of the
OPAE Intel&reg; FPGA driver stack that supports Intel&reg; FPGA platforms, the library
abstracts away hardware-specific and OS-specific details and exposes the
underlying FPGA resources as a set of features accessible from within
software programs running on the host.

These features include the acceleration logic preconfigured on the
device, as well as functions to manage and reconfigure the
device. Hence, the library can enable user applications to
transparently and seamlessly leverage FPGA-based acceleration.

In this document, we will explore the initial steps on how to set up
the required libraries and utilities to use the FPGA devices.

If you do not have access to an Intel&reg; Xeon&reg; processor with integrated
FPGA, or a programmable FPGA acceleration card for Intel&reg; Xeon&reg;
processors, you will not be able to run the examples below. However, you can
still make use of the AFU simulation environment (ASE) to develop and test
accelerator RTL with OPAE applications.

The source for the OPAE SDK Linux device drivers is available at the
[OPAE Linux DFL drivers repository](https://github.com/OPAE/linux-dfl).

## Build the OPAE Linux device drivers from the source

For building the OPAE kernel and kernel driver, the kernel development environment is required. So before you build the kernel, you must install the required packages. Run the following commands (we are using Fedora 32 as an example):

```bash
sudo yum install gcc gcc-c++ make kernel-headers kernel-devel elfutils-libelf-devel ncurses-devel openssl-devel bison flex
```

Download the OPAE upstream kernel tree from GitHub:

```bash
git clone https://github.com/OPAE/linux-dfl.git -b fpga-upstream-dev-5.8.0
```

Configure the kernel:

```bash
cd linux-dfl
cp /boot/config-`uname -r` .config
cat configs/n3000_d5005_defconfig >> .config 
echo 'CONFIG_LOCALVERSION="-dfl"' >> .config
echo 'CONFIG_LOCALVERSION_AUTO=y' >> .config
make olddefconfig
```

Compile and install the new kernel:

```bash
make -j
sudo make modules_install
sudo make install
```

After the installation finishes, reboot your system.
Log back into the system, and confirm the correct version for the kernel:

```bash
$ uname -a
Linux localhost.localdomain 5.8.0-rc1-dfl-g73e16386cda0 #6 SMP Wed Aug 19 08:38:32 EDT 2020 x86_64 x86_64 x86_64 GNU/Linux
```

## Building and installing the OPAE SDK from the source

Download the OPAE SDK source package:

1. Go to the section corresponding to the desired release on
[GitHub](https://github.com/OPAE/opae-sdk/releases):
2. Click the `Source code (tar.gz)` link under the section's `Assets`.
3. On the command line, go through the following steps to install it:

   ```bash
   # Unpack
   tar xfvz opae-sdk-<release>.tar.gz
   # Configure
   cd opae-sdk-<release>
   mkdir build
   cd build
   # The default installation prefix is `/usr/local`;
   # You have the option to configure for a different location
   cmake [-DCMAKE_INSTALL_PREFIX=<prefix>] ..
   # Compile
   make
   # Install: you need system administration privileges (`sudo`)
   # if you have elected to install in the default location
   [sudo] make install
   ```

The remainder of this guide assumes you installed into `/usr/local`.

## Configuring the FPGA (loading an FPGA AFU)

The `fpgaconf` tool exercises the AFU reconfiguration
functionality. It shows how to read a bitstream from a disk file,
check its validity and compatibility, and then injects it into FPGA to
be configured as a new AFU, which can then be discovered and used by
user applications.

For this step, you require a valid green bitstream (GBS) file. To
reconfigure the FPGA slot, you can issue the following command as system
administrator:

```bash
sudo fpgaconf -b 0x5e <filename>.gbs
```

The `-b` option to `fpgaconf` indicates the *target bus number* of the
FPGA slot to be reconfigured. Alternatively, you can also specify the
*target socket number* of the FPGA using the `-s` option.

```bash
$ sudo fpgaconf --help
Usage:
        fpgaconf [-hvn] [-b <bus>] [-d <device>] [-f <function>] [-s <socket>] <gbs>

                -h,--help           Print this help
                -v,--verbose        Increase verbosity
                -n,--dry-run        Don't actually perform actions
                -b,--bus            Set target bus number
                -d,--device         Set target device number
                -f,--function       Set target function number
                -s,--socket         Set target socket number
```

````{note}
The sample application on the Building a Sample Application
section requires loading of an AFU called "Native Loopback
Adapter" (NLB) on the FPGA. Please refer to the NLB documentation
for the location of the NLB's green bitstream. You also can verify
if the NLB green bitstream has already been loaded into the FPGA
slot by typing the following command and checking the output
matches the following:

```bash
$ cat /sys/class/fpga_region/region0/dfl-port.0/afu_id
d8424dc4a4a3c413f89e433683f9040b
```
````

```{note}
The fpgaconf tool is not available for the Intel PAC N3000. The NLB is
already included in the AFU.
```

## Building a sample application

The library source includes code samples. Use these samples to learn
how to call functions in the library. Build and run these samples as
quick sanity checks to determine if your installation and environment
are set up properly.

In this guide, we will build `hello_fpga.c`. This is the "Hello
World!" example of using the library.  This code searches for a
predefined and known AFU called "Native Loopback Adapter" on the
FPGA. If found, it acquires ownership and then interacts with the AFU
by sending it a 2MB message and waiting for the message to be echoed
back. This code exercises all major components of the API except for
AFU reconfiguration: AFU search, enumeration, access, MMIO, and memory
management.

You can also find the source for `hello_fpga` in the `samples` directory of the
OPAE SDK repository on GitHub.

```c
    int main(int argc, char *argv[])
    {
        fpga_properties    filter = NULL;
        fpga_token         afu_token;
        fpga_handle        afu_handle;
        fpga_guid          guid;
        uint32_t           num_matches;

        volatile uint64_t *dsm_ptr    = NULL;
        volatile uint64_t *status_ptr = NULL;
        volatile uint64_t *input_ptr  = NULL;
        volatile uint64_t *output_ptr = NULL;

        uint64_t        dsm_wsid;
        uint64_t        input_wsid;
        uint64_t        output_wsid;
        fpga_result     res = FPGA_OK;

        if (uuid_parse(NLB0_AFUID, guid) < 0) {
            fprintf(stderr, "Error parsing guid '%s'\n", NLB0_AFUID);
            goto out_exit;
        }

        /* Look for accelerator by its "afu_id" */
        res = fpgaGetProperties(NULL, &filter);
        ON_ERR_GOTO(res, out_exit, "creating properties object");

        res = fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR);
        ON_ERR_GOTO(res, out_destroy_prop, "setting object type");

        res = fpgaPropertiesSetGuid(filter, guid);
        ON_ERR_GOTO(res, out_destroy_prop, "setting GUID");

        /* TODO: Add selection via BDF / device ID */

        res = fpgaEnumerate(&filter, 1, &afu_token, 1, &num_matches);
        ON_ERR_GOTO(res, out_destroy_prop, "enumerating accelerators");

        if (num_matches < 1) {
            fprintf(stderr, "accelerator not found.\n");
            res = fpgaDestroyProperties(&filter);
            return FPGA_INVALID_PARAM;
        }

        /* Open accelerator and map MMIO */
        res = fpgaOpen(afu_token, &afu_handle, 0);
        ON_ERR_GOTO(res, out_destroy_tok, "opening accelerator");

        res = fpgaMapMMIO(afu_handle, 0, NULL);
        ON_ERR_GOTO(res, out_close, "mapping MMIO space");

        /* Allocate buffers */
        res = fpgaPrepareBuffer(afu_handle, LPBK1_DSM_SIZE,
                    (void **)&dsm_ptr, &dsm_wsid, 0);
        ON_ERR_GOTO(res, out_close, "allocating DSM buffer");

        res = fpgaPrepareBuffer(afu_handle, LPBK1_BUFFER_ALLOCATION_SIZE,
                   (void **)&input_ptr, &input_wsid, 0);
        ON_ERR_GOTO(res, out_free_dsm, "allocating input buffer");

        res = fpgaPrepareBuffer(afu_handle, LPBK1_BUFFER_ALLOCATION_SIZE,
                   (void **)&output_ptr, &output_wsid, 0);
        ON_ERR_GOTO(res, out_free_input, "allocating output buffer");

        printf("Running Test\n");

        /* Initialize buffers */
        memset((void *)dsm_ptr,    0,    LPBK1_DSM_SIZE);
        memset((void *)input_ptr,  0xAF, LPBK1_BUFFER_SIZE);
        memset((void *)output_ptr, 0xBE, LPBK1_BUFFER_SIZE);

        cache_line *cl_ptr = (cache_line *)input_ptr;
        for (uint32_t i = 0; i < LPBK1_BUFFER_SIZE / CL(1); ++i) {
            cl_ptr[i].uint[15] = i+1; /* set the last uint in every cacheline */
        }

        /* Reset accelerator */
        res = fpgaReset(afu_handle);
        ON_ERR_GOTO(res, out_free_output, "resetting accelerator");

        /* Program DMA addresses */
        uint64_t iova;
        res = fpgaGetIOAddress(afu_handle, dsm_wsid, &iova);
        ON_ERR_GOTO(res, out_free_output, "getting DSM IOVA");

        res = fpgaWriteMMIO64(afu_handle, 0, CSR_AFU_DSM_BASEL, iova);
        ON_ERR_GOTO(res, out_free_output, "writing CSR_AFU_DSM_BASEL");

        res = fpgaWriteMMIO32(afu_handle, 0, CSR_CTL, 0);
        ON_ERR_GOTO(res, out_free_output, "writing CSR_CFG");
        res = fpgaWriteMMIO32(afu_handle, 0, CSR_CTL, 1);
        ON_ERR_GOTO(res, out_free_output, "writing CSR_CFG");

        res = fpgaGetIOAddress(afu_handle, input_wsid, &iova);
        ON_ERR_GOTO(res, out_free_output, "getting input IOVA");
        res = fpgaWriteMMIO64(afu_handle, 0, CSR_SRC_ADDR, CACHELINE_ALIGNED_ADDR(iova));
        ON_ERR_GOTO(res, out_free_output, "writing CSR_SRC_ADDR");

        res = fpgaGetIOAddress(afu_handle, output_wsid, &iova);
        ON_ERR_GOTO(res, out_free_output, "getting output IOVA");
        res = fpgaWriteMMIO64(afu_handle, 0, CSR_DST_ADDR, CACHELINE_ALIGNED_ADDR(iova));
        ON_ERR_GOTO(res, out_free_output, "writing CSR_DST_ADDR");

        res = fpgaWriteMMIO32(afu_handle, 0, CSR_NUM_LINES, LPBK1_BUFFER_SIZE / CL(1));
        ON_ERR_GOTO(res, out_free_output, "writing CSR_NUM_LINES");
        res = fpgaWriteMMIO32(afu_handle, 0, CSR_CFG, 0x42000);
        ON_ERR_GOTO(res, out_free_output, "writing CSR_CFG");

        status_ptr = dsm_ptr + DSM_STATUS_TEST_COMPLETE/8;

        /* Start the test */
        res = fpgaWriteMMIO32(afu_handle, 0, CSR_CTL, 3);
        ON_ERR_GOTO(res, out_free_output, "writing CSR_CFG");

        /* Wait for test completion */
        while (0 == ((*status_ptr) & 0x1)) {
            usleep(100);
        }

        /* Stop the device */
        res = fpgaWriteMMIO32(afu_handle, 0, CSR_CTL, 7);
        ON_ERR_GOTO(res, out_free_output, "writing CSR_CFG");

        /* Check output buffer contents */
        for (uint32_t i = 0; i < LPBK1_BUFFER_SIZE; i++) {
            if (((uint8_t*)output_ptr)[i] != ((uint8_t*)input_ptr)[i]) {
                fprintf(stderr, "Output does NOT match input "
                    "at offset %i!\n", i);
                break;
            }
        }

        printf("Done Running Test\n");

        /* Release buffers */
    out_free_output:
        res = fpgaReleaseBuffer(afu_handle, output_wsid);
        ON_ERR_GOTO(res, out_free_input, "releasing output buffer");
    out_free_input:
        res = fpgaReleaseBuffer(afu_handle, input_wsid);
        ON_ERR_GOTO(res, out_free_dsm, "releasing input buffer");
    out_free_dsm:
        res = fpgaReleaseBuffer(afu_handle, dsm_wsid);
        ON_ERR_GOTO(res, out_unmap, "releasing DSM buffer");

        /* Unmap MMIO space */
    out_unmap:
        res = fpgaUnmapMMIO(afu_handle, 0);
        ON_ERR_GOTO(res, out_close, "unmapping MMIO space");

        /* Release accelerator */
    out_close:
        res = fpgaClose(afu_handle);
        ON_ERR_GOTO(res, out_destroy_tok, "closing accelerator");

        /* Destroy token */
    out_destroy_tok:
        res = fpgaDestroyToken(&afu_token);
        ON_ERR_GOTO(res, out_destroy_prop, "destroying token");

        /* Destroy properties object */
    out_destroy_prop:
        res = fpgaDestroyProperties(&filter);
        ON_ERR_GOTO(res, out_exit, "destroying properties object");

    out_exit:
        return res;

    }
```

Linking with the OPAE library is straightforward.  Code using this library
should include the header file `fpga.h`. Taking the GCC compiler on
Linux as an example, the minimalist compile and link line should look
like:

```bash
gcc -std=c99 hello_fpga.c -I/usr/local/include -L/usr/local/lib -lopae-c -luuid -ljson-c -lpthread -o hello_fpga
```

```{note}
The API uses some features from the C99 language standard. The
`-std=c99` switch is required if the compiler does not support C99 by
default.
```

```{note}
Third-party library dependency: The library internally uses
`libuuid` and `libjson-c`. But they are not distributed as part of the
library. Make sure you have these libraries properly installed.
```

````{note}
The layout of AFU is different between the N3000 card and Rush Creek/Darby Creek.
In the N3000 card, the NLB and DMA are contained in the AFU, so we need to do
enumeration again in AFU to discover the NLB.
To run the hello_fpga application on the N3000 card, it should use the `-c`
option to support the N3000 card:

```bash
$ sudo ./hello_fpga -c
Running Test
Running on bus 0x08.
AFU NLB0 found @ 28000
Done Running Test
```
````

To run the `hello_fpga` application; just issue:

```bash
$ sudo ./hello_fpga
Running Test
Done
```

## Setup IOFS Release1 Bitstream on FPGA PCIe card

Program IOFS Release1 bitstream on the FPGA D5005 or N6000 cards and reboot the system.

Run this command:

```bash
$ lspci | grep acc
3b:00.0 Processing accelerators: Intel Corporation Device af00 (rev 01)
```

Number of virtual functions supported by bitstream:

```bash
$ cat /sys/bus/pci/devices/0000:3b:00.0/sriov_totalvfs 
output: 3
```

Enable FPGA virtual functions:

```bash
sudo sh -c "echo 3 > /sys/bus/pci/devices/0000:3b:00.0/sriov_numvfs"
```

List of FPGA PF and VF's:

```txt
Physical Functions (PFs):
  3b:00.0 Processing accelerators: Intel Corporation Device af00 (rev 01)

Virtual Functions (VFs).
  3b:00.1 Processing accelerators: Intel Corporation Device af01 (rev 01)
  3b:00.2 Processing accelerators: Intel Corporation Device af01 (rev 01)
  3b:00.3 Processing accelerators: Intel Corporation Device af01 (rev 01)
```

Bind vfio-pcie driver to FPGA virtual functions:

```bash
sudo opaevfio  -i 0000:3b:00.1 -u userid -g groupid
sudo opaevfio  -i 0000:3b:00.2 -u userid -g groupid
sudo opaevfio  -i 0000:3b:00.3 -u userid -g groupid
```

List of fpga accelerators:

```bash
$ fpgainfo port

  //****** PORT ******//
  Object Id                        : 0x600D000000000000
  PCIe s:b:d.f                     : 0000:3b:00.3
  Device Id                        : 0xAF00
  Socket Id                        : 0xFF
  Accelerator Id                   : 43425ee6-92b2-4742-b03a-bd8d4a533812
  Accelerator GUID                 : 43425ee6-92b2-4742-b03a-bd8d4a533812
  //****** PORT ******//
  Object Id                        : 0x400D000000000000
  PCIe s:b:d.f                     : 0000:3b:00.2
  Device Id                        : 0xAF00
  Socket Id                        : 0xFF
  Accelerator Id                   : 8568AB4E-6bA5-4616-BB65-2A578330A8EB
  Accelerator GUID                 : 8568AB4E-6bA5-4616-BB65-2A578330A8EB
  //****** PORT ******//
  Object Id                        : 0x200D000000000000
  PCIe s:b:d.f                     : 0000:3b:00.1
  Device Id                        : 0xAF00
  Socket Id                        : 0xFF
  Accelerator Id                   : 56e203e9-864f-49a7-b94b-12284c31e02b
  Accelerator GUID                 : 56e203e9-864f-49a7-b94b-12284c31e02b

FPGA VF1/3b:00.1/Host Exerciser Loopback Accelerator GUID: 56E203E9-864F-49A7-B94B-12284C31E02B
FPGA VF2/3b:00.2/Host Exerciser Memory Accelerator GUID: 8568AB4E-6bA5-4616-BB65-2A578330A8EB
FPGA VF3/3b:00.3/Host Exerciser HSSI Accelerator GUID: 43425ee6-92b2-4742-b03a-bd8d4a533812
```

Unbind pcie-vfio dirver to FPGA virtual functions:

```bash
sudo opaevfio  -r 0000:3b:00.1
```

Host Exerciser Loopback (HE-LBK) AFU can move data between host memory and FPGA:

```bash
$ host_exerciser lpbk
  
  [lpbk] [info] starting test run, count of 1
  Input Config:0
  Allocate SRC Buffer
  Allocate DST Buffer
  Allocate DSM Buffer
  Start Test
  Test Completed
  Host Exerciser swtest msg:0
  Host Exerciser numReads:32
  Host Exerciser numWrites:32
  Host Exerciser numPendReads:0
  Host Exerciser numPendWrites:0
  [lpbk] [info] Test lpbk(1): PASS
```

````{note}
  In order to successfully run hello\_fpga, the user needs to configure
  system hugepage to reserve 2M-hugepages.
  For example, the command below reserves 20 2M-hugepages:

  ```bash
  echo 20 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
  ```

  For x86_64 architecture CPU, user can use the following command to find out available huge page sizes:

  ```bash
  $ grep pse /proc/cpuinfo | uniq
  flags : ... pse ...
  ```

  If this command returns a non-empty string, 2MB pages are supported:

  ```bash
  $ grep pse /proc/cpuinfo | uniq
  flags : ... pdpe1gb ...
  ```

  If this commands returns a non-empty string, 1GB pages are supported.
  ````

````{note}
The default configuration for many Linux distributions currently sets a
relatively low limit for pinned memory allocations per process 
(RLIMIT_MEMLOCK, often set to a default of 64kiB).

To run an OPAE application that attempts to share more memory than specified
by this limit between software and an accelerator, you can either:

* Run the application as root, or
* Increase the limit for locked memory via `ulimit`:

```bash
ulimit -l unlimited
```

See the Installation Guide for how to permanently adjust the memlock limit.
````
  