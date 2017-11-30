# Quick Start Guide #

.. toctree::

.. highlight:: c

.. highlight:: console

## Overview ##
The OPAE C library is a lightweight user-space library that provides
abstraction for FPGA resources in a compute environment. Built on top of the
OPAE Intel&reg; FPGA driver stack that supports Intel&reg; FPGA platforms, the library
abstracts away hardware specific and OS specific details and exposes the
underlying FPGA resources as a set of features accessible from within
software programs running on the host.

These features include the acceleration logic preconfigured on the
device, as well as functions to manage and reconfigure the
device. Hence, the library is able to enalbe user applications to
transparently and seamlessly leverage FPGA-based acceleration.

In this document, we will explore the initial steps on how to setup
the required libraries and utilities to use the FPGA devices.

## Installing the OPAE Intel&reg; FPGA drivers ##

If you do not have access to an Intel&reg; Xeon&reg; processor with integrated
FPGA, or a programmable FPGA acceleration card for Intel&reg; Xeon&reg;
processors, you will not be able to run the examples below. However, you can
still make use of the AFU simulation environment (ASE) to develop and test
accelerator RTL with OPAE applications.

For more information about ASE, see the [OPAE AFU Simulation Environment
(ASE) User Guide](../../ase_userguide/ase_userguide.html).

As part of the OPAE SDK release, we provide a DKMS-based RPM package for
distributions using RPM (e.g. Redhat, Fedora, Centos) package managers.
Download this package from the respective [release page on
GitHub](https://github.com/OPAE/opae-sdk/releases) - it is named
`opae-intel-fpga-drv-x.y.z-1.x86_64.rpm`, with `x.y.z` being the respective OPAE
release's version number.

.. note::

```
The RPM package requires that the DKMS (Dynamic Kernel Module System)
package, version greater than 2.2, is already installed.
```

For Redhat and Centos:
```console
$ sudo yum install opae-intel-fpga-drv-<release>-1.x86_64.rpm
```

## Installing the OPAE SDK from rpm packages ##
See the [OPAE Installation Guide](/fpga-doc/docs/fpga_api/install_guide/installation_guide.html)
for information about OPAE RPM packages.
Assuming RPM packages are already downloaded and exist in the current folder,
then use the commands below to install the OPAE library, tools, and development
headers.

```console
$ sudo yum install opae-<release>-1.x86_64-libs.rpm
$ sudo yum install opae-<release>-1.x86_64-tools.rpm
$ sudo yum install opae-<release>-1.x86_64-devel.rpm
```

To use OPAE in the simulation environment, you also need to install the AFU
Simulation Environment (ASE) package:

```console
$ sudo yum install opae-<release>-1.x86_64-ase.rpm
```

## Building and installing the OPAE SDK from source ##
Download the OPAE SDK source package from the respective [release page on
GitHub](https://github.com/OPAE/opae-sdk/releases) - click the `Source code
(tar.gz)` link under "Downloads".

After downloading the source, unpack, configure, and compile it:

```console
    tar xfvz opae-sdk-<release>.tar.gz
    cd opae-sdk-<release>
    mkdir mybuild
    cd mybuild
    cmake .. -DBUILD_ASE=1
    make
```

By default, the OPAE SDK will install into `/usr/local` if you also issue the following:

```console
    make install
```

You can change this installation prefix from `/usr/local` into something else
by adding `-DCMAKE_INSTALL_PREFIX=<new prefix>` to the `cmake` command above.
The remainder of this guide assumes you installed into `/usr/local`.

## Configuring the FPGA (loading an FPGA AFU)##

The *fpgaconf* tool exercises the AFU reconfiguration
functionality. It shows how to read a bitstream from a disk file,
check its validity and compatability, and then injects it into FPGA to
be configured as a new AFU, which can then be discovered and used by
user applications.

For this step you require a valid green bitstream (GBS) file. To
reconfigure the FPGA slot, you can issue following command as system
administrator (*root*):

```console
$ sudo fpgaconf -b 0x5e <filename>.gbs
```

The `-b` parameter to *fpgaconf* indicates the *target bus number* of the
FPGA slot to be reconfigured. Alternatively, you can also specify the
*target socket number* of the FPGA using the `-s` parameter.

```console
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

.. note::

```
    The sample application on the Building a Sample Application
    section requires loading of an AFU called "Native Loopback
    Adapter" (NLB) on the FPGA. Please refer to the NLB documentation
    for the location of the NLB's green bitstream. You also can verify
    if the NLB green bitstream has already been loaded into the FPGA
    slot by typing the following command and checking the output
    matches the following:

    $ cat /sys/class/fpga/intel-fpga-dev.0/intel-fpga-port.0/afu_id

    d8424dc4a4a3c413f89e433683f9040b
```

## Building a sample application ##
The library source include code samples. Use these samples to learn
how to call functions in the library. Build and run these samples as
quick sanity checks to determine if your installation and environment
are set up properly.

In this guide, we will build *hello\_fpga.c*. This is the "Hello
World!" example of using the library.  This code searches for a
predefined and known AFU called "Native Loopback Adapter" on the
FPGA. If found, it acquires ownership and then interacts with the AFU
by sending it a 2MB message and waiting for the message being echoed
back. This coe exercises all major components of the API except for
AFU reconfiguration: AFU search, enumeration, access, MMIO, and memory
management.

You can also find the source for `hello\_fpga` in the `samples` directory of the
OPAE SDK repository on github.

```c
    int main(int argc, char *argv[])
    {
        fpga_properties    filter = NULL;
        fpga_token         afc_token;
        fpga_handle        afc_handle;
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

        /* Look for AFC with MY_AFC_ID */
        res = fpgaGetProperties(NULL, &filter);
        ON_ERR_GOTO(res, out_exit, "creating properties object");

        res = fpgaPropertiesSetObjectType(filter, FPGA_AFC);
        ON_ERR_GOTO(res, out_destroy_prop, "setting object type");

        res = fpgaPropertiesSetGuid(filter, guid);
        ON_ERR_GOTO(res, out_destroy_prop, "setting GUID");

        /* TODO: Add selection via BDF / device ID */

        res = fpgaEnumerate(&filter, 1, &afc_token, 1, &num_matches);
        ON_ERR_GOTO(res, out_destroy_prop, "enumerating AFCs");

        if (num_matches < 1) {
            fprintf(stderr, "AFC not found.\n");
            res = fpgaDestroyProperties(&filter);
            return FPGA_INVALID_PARAM;
        }

        /* Open AFC and map MMIO */
        res = fpgaOpen(afc_token, &afc_handle, 0);
        ON_ERR_GOTO(res, out_destroy_tok, "opening AFC");

        res = fpgaMapMMIO(afc_handle, 0, NULL);
        ON_ERR_GOTO(res, out_close, "mapping MMIO space");

        /* Allocate buffers */
        res = fpgaPrepareBuffer(afc_handle, LPBK1_DSM_SIZE,
                    (void **)&dsm_ptr, &dsm_wsid, 0);
        ON_ERR_GOTO(res, out_close, "allocating DSM buffer");

        res = fpgaPrepareBuffer(afc_handle, LPBK1_BUFFER_ALLOCATION_SIZE,
                   (void **)&input_ptr, &input_wsid, 0);
        ON_ERR_GOTO(res, out_free_dsm, "allocating input buffer");

        res = fpgaPrepareBuffer(afc_handle, LPBK1_BUFFER_ALLOCATION_SIZE,
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

        /* Reset AFC */
        res = fpgaReset(afc_handle);
        ON_ERR_GOTO(res, out_free_output, "resetting AFC");

        /* Program DMA addresses */
        uint64_t iova;
        res = fpgaGetIOVA(afc_handle, dsm_wsid, &iova);
        ON_ERR_GOTO(res, out_free_output, "getting DSM IOVA");

        res = fpgaWriteMMIO64(afc_handle, 0, CSR_AFU_DSM_BASEL, iova);
        ON_ERR_GOTO(res, out_free_output, "writing CSR_AFU_DSM_BASEL");

        res = fpgaWriteMMIO32(afc_handle, 0, CSR_CTL, 0);
        ON_ERR_GOTO(res, out_free_output, "writing CSR_CFG");
        res = fpgaWriteMMIO32(afc_handle, 0, CSR_CTL, 1);
        ON_ERR_GOTO(res, out_free_output, "writing CSR_CFG");

        res = fpgaGetIOVA(afc_handle, input_wsid, &iova);
        ON_ERR_GOTO(res, out_free_output, "getting input IOVA");
        res = fpgaWriteMMIO64(afc_handle, 0, CSR_SRC_ADDR, CACHELINE_ALIGNED_ADDR(iova));
        ON_ERR_GOTO(res, out_free_output, "writing CSR_SRC_ADDR");

        res = fpgaGetIOVA(afc_handle, output_wsid, &iova);
        ON_ERR_GOTO(res, out_free_output, "getting output IOVA");
        res = fpgaWriteMMIO64(afc_handle, 0, CSR_DST_ADDR, CACHELINE_ALIGNED_ADDR(iova));
        ON_ERR_GOTO(res, out_free_output, "writing CSR_DST_ADDR");
        //fpgaProgramBufferAddressAndLength(afc_handle, dsm_wsid, 0, LPBK1_DSM_SIZE,
        //                 CSR_AFU_DSM_BASEL);
        //fpgaProgramBufferAddressAndLength(afc_handle, input_wsid, 0, LPBK1_BUFFER_SIZE,
        //                 CSR_SRC_ADDR);
        //fpgaProgramBufferAddressAndLength(afc_handle, output_wsid, 0, LPBK1_BUFFER_SIZE,
        //                 CSR_DST_ADDR);

        res = fpgaWriteMMIO32(afc_handle, 0, CSR_NUM_LINES, LPBK1_BUFFER_SIZE / CL(1));
        ON_ERR_GOTO(res, out_free_output, "writing CSR_NUM_LINES");
        res = fpgaWriteMMIO32(afc_handle, 0, CSR_CFG, 0x42000);
        ON_ERR_GOTO(res, out_free_output, "writing CSR_CFG");

        status_ptr = dsm_ptr + DSM_STATUS_TEST_COMPLETE/8;

        /* Start the test */
        res = fpgaWriteMMIO32(afc_handle, 0, CSR_CTL, 3);
        ON_ERR_GOTO(res, out_free_output, "writing CSR_CFG");

        /* Wait for test completion */
        while (0 == ((*status_ptr) & 0x1)) {
            usleep(100);
        }

        /* Stop the device */
        res = fpgaWriteMMIO32(afc_handle, 0, CSR_CTL, 7);
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
        res = fpgaReleaseBuffer(afc_handle, output_wsid);
        ON_ERR_GOTO(res, out_free_input, "releasing output buffer");
    out_free_input:
        res = fpgaReleaseBuffer(afc_handle, input_wsid);
        ON_ERR_GOTO(res, out_free_dsm, "releasing input buffer");
    out_free_dsm:
        res = fpgaReleaseBuffer(afc_handle, dsm_wsid);
        ON_ERR_GOTO(res, out_unmap, "releasing DSM buffer");

        /* Unmap MMIO space */
    out_unmap:
        res = fpgaUnmapMMIO(afc_handle, 0);
        ON_ERR_GOTO(res, out_close, "unmapping MMIO space");

        /* Release accelerator */
    out_close:
        res = fpgaClose(afc_handle);
        ON_ERR_GOTO(res, out_destroy_tok, "closing AFC");

        /* Destroy token */
    out_destroy_tok:
        res = fpgaDestroyToken(&afc_token);
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

```console
$ gcc -std=c99 hello_fpga.c -I/usr/local/include -L/usr/local/lib -lopae-c -luuid -ljson-c -lpthread -o hello_fpga
```

.. note:

```
    The API uses some features from the C99 language standard. The
    `-std=c99` switch is required if the compiler does not support C99 by
    default.
```

.. note::

```
    Third-party library dependency: The library internally uses
    `libuuid` and `libjson-c`. But they are not distributed as part of the
    library. Make sure you have these libraries properly installed.
```
To run the *hello_fpga* application; just issue:

```console
$ sudo ./hello_fpga

Running Test
Done

```

.. note::

```
  In order to successfully run hello\_fpga, user need to configure system hugepage to reserve 2M-hugepages. 
  For example, the command below reserves 20 2M-hugepages:

  $ echo 20 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages

  For x86_64 architecture CPU, user can use following command to find out avaiable huge page sizes:

  $ grep pse /proc/cpuinfo | uniq
  flags : ... pse ...

  If this commands returns a non-empty string, 2MB pages are supported:

  $ grep pse /proc/cpuinfo | uniq
  flags : ... pdpe1gb ...

  If this commands returns a non-empty string, 1GB pages are supported:

```

```
  The default configuration for many Linux distribution currently sets a relatively low limit for pinned memory allocations per process (RLIMIT_MEMLOCK, often set to a default of 64kiB). 
  To run an OPAE application which attempts to share more memory than specified by this limit between software and an accelerator, you can either:

     * Run the application as root, or
     * Increase the limit for locked memory via ulimit:

     $ ulimit -l unlimited

  See the Installation Guide for how to permanently adjust the memlock limit.

```
