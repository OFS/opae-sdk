// Copyright (c) 2017, Intel Corporation
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// Neither the name of the Intel Corporation nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <opae/fpga.h>
#include <opae/enum.h>
#include <opae/access.h>
#include <opae/utils.h>
#include <getopt.h>


#ifndef CL
# define CL(x)                       ((x) * 64)
#endif // CL
#ifndef LOG2_CL
# define LOG2_CL                     6
#endif // LOG2_CL
#ifndef MB
# define MB(x)                       ((x) * 1024 * 1024)
#endif // MB

#define CACHELINE_ALIGNED_ADDR(p) ((p) >> LOG2_CL)

// NLB0 AFU_ID
#define NLB0_AFUID            "D8424DC4-A4A3-C413-F89E-433683F9040B"
// HELLO AFU AFU_ID
#define HELLO_AFU_AFUID       "850ADCC2-6CEB-4B22-9722-D43375B61C66"

// #defines for Hello FPGA Example
#define LPBK1_BUFFER_SIZE            MB(1)
#define LPBK1_BUFFER_ALLOCATION_SIZE MB(2)
#define LPBK1_DSM_SIZE               MB(2)
#define CSR_SRC_ADDR                 0x0120
#define CSR_DST_ADDR                 0x0128
#define CSR_CTL                      0x0138
#define CSR_CFG                      0x0140
#define CSR_NUM_LINES                0x0130
#define DSM_STATUS_TEST_COMPLETE     0x40
#define CSR_AFU_DSM_BASEL            0x0110
#define CSR_AFU_DSM_BASEH            0x0114

// #defines for Hello AFU example
#define SCRATCH_REG                  0x80ul
#define SCRATCH_VALUE                0x0123456789ABCDEF
#define SCRATCH_RESET                0ul
#define BYTE_OFFSET                  8

#define AFU_DFH_REG                  0x0
#define AFU_ID_LO                    0x8
#define AFU_ID_HI                    0x10
#define AFU_NEXT                     0x18
#define AFU_RESERVED                 0x20

#define OUT_FREE_OUTPUT              0
#define OUT_FREE_INPUT               1
#define OUT_FREE_DSM                 2
#define OUT_UNMAP                    3
#define OUT_CLOSE                    4
#define OUT_DESTROY_TOK              5
#define OUT_DESTROY_PROP             6
#define OUT_EXIT                     7

// Assert for Hello FPGA sample
#define ASSERT(condition, desc)					\
	do {							\
	    if (condition == 0){				\
		    fprintf(stderr, "Error %s\n", desc);	\
		    s_error_count += 1;				\
	    }							\
	} while(0)


/* Type definitions */
typedef struct {
	uint32_t uint[16];
} cache_line;

typedef struct {
        fpga_properties		filter;
        fpga_token*		accelerator_token;
        fpga_handle		accelerator_handle;
        fpga_guid		guid;
        uint64_t		res;
        int			open_flags;
} fpga_struct;

typedef struct {
        uint64_t		dsm_wsid;
        uint64_t		input_wsid;
        uint64_t		output_wsid;
} wsid_struct;


/*Function Definitions */
void build_filter(fpga_struct* fpga, char* ID);
int enumerate_fpga(fpga_struct* fpga, uint32_t *num_matches);
void open_afu(fpga_struct* fpga, const uint32_t num_matches);
void reset_afu(fpga_struct * fpga);
void hello_afu_test(fpga_struct *fpga);
void hello_fpga_test(fpga_struct *fpga, wsid_struct *wsid,
		    volatile uint64_t **status,  volatile uint64_t **dsm);
void check_result(uint64_t res, char* err_str);
void cleanup(fpga_struct * fpga, wsid_struct * wsid, pid_t pid);

void print_err(const char *s, fpga_result res)
{
        printf("Error %s: %s\n", s, fpgaErrStr(res));
}

static int s_error_count = 0;

int main(int argc, char *argv[])
{

	static struct option long_options[] = {
		{"same_test", optional_argument, 0, 's'},
		{0,0,0,0}	};
	int				sameTest = 0;
        fpga_struct			myfpga;
	myfpga.filter			= NULL;
        myfpga.res			= FPGA_OK;
	myfpga.accelerator_token	= (fpga_token*)malloc(2 * sizeof(fpga_token));
        myfpga.open_flags		= 0;
        volatile uint64_t   *dsm_ptr     = NULL;
        volatile uint64_t   *status_ptr  = NULL;
        volatile uint64_t   *input_ptr   = NULL;
        volatile uint64_t   *output_ptr  = NULL;
        wsid_struct         mywsid;
        uint32_t            num_matches;
        int                 err;

	int getopt_ret, option_index;
	while(1)
	{
		getopt_ret = getopt_long(argc, argv, "s::", long_options,
					 &option_index);
		if (getopt_ret == -1) break;
		switch(getopt_ret)
		{
			case 's':
				// Assume we have the same gbs
				sameTest = 1;
				break;
			default:
				sameTest = 0;
				break;
		}
	}

        pid_t pid;
	pid = fork();

        if(pid == 0)
	{
		// Child process enumerate hello_afu
		printf("Starting child process...\n");

		// build filter for AFU with the given ID
		build_filter(&myfpga, HELLO_AFU_AFUID);
		check_result(myfpga.res, "looking up afu\n");

		// Enumerate FPGA
		err = enumerate_fpga(&myfpga, &num_matches);
		if(err == FPGA_INVALID_PARAM){
			free(myfpga.accelerator_token);
			return FPGA_INVALID_PARAM;
		}
		check_result(myfpga.res, "enumerating accelerators");

		// Open FPGA
		open_afu(&myfpga, num_matches);
		check_result(myfpga.res, "opening afu\n");

		printf("Running Test in child process.\n");

		// Run test
		hello_afu_test(&myfpga);
		check_result(myfpga.res, " running Hello_afu test");

		printf("Done Running Test in child process.\n");

        }
	else if (pid > 0)
	{
		//Parent process enumerate nlb0/hello_fpga
		printf("parent process running...\n");

		if (sameTest){
			printf("Running Test in parent process\n");
			// build filter for afu
			build_filter(&myfpga, HELLO_AFU_AFUID);
			check_result(myfpga.res, "looking up afu\n");

			// Enumerate FPGA
			err = enumerate_fpga(&myfpga, &num_matches);
			if(err == FPGA_INVALID_PARAM){
				free(myfpga.accelerator_token);
				return FPGA_INVALID_PARAM;
			}
			check_result(myfpga.res, "enumerating accelerators");

			// Open FPGA
			open_afu(&myfpga, num_matches);
			check_result(myfpga.res, "opening afu\n");

			// Run test
			hello_afu_test(&myfpga);
			check_result(myfpga.res, " running Hello_afu test");
			printf("Done Running Test in parent process.\n");
		}
		else
		{
			printf("Running Test in parent process\n");
			// build filter for afu
			build_filter(&myfpga, NLB0_AFUID);
			check_result(myfpga.res, "looking up afu\n");

			/* TODO: Add selection via BDF / device ID */
			// enumerate fpga
			err = enumerate_fpga(&myfpga, &num_matches);
			if(err == FPGA_INVALID_PARAM) {
				free(myfpga.accelerator_token);
				return FPGA_INVALID_PARAM;
			}
			check_result(myfpga.res, "enumerating accelerators");

			open_afu(&myfpga, num_matches);
			check_result(myfpga.res, "opening afu\n");

			/* Allocate buffers */
			myfpga.res = fpgaPrepareBuffer(myfpga.accelerator_handle,
						   LPBK1_DSM_SIZE, (void **)&dsm_ptr,
						   &(mywsid.dsm_wsid), 0);
			check_result(myfpga.res, "allocating DSM buffer");

			myfpga.res = fpgaPrepareBuffer(myfpga.accelerator_handle,
						   LPBK1_BUFFER_ALLOCATION_SIZE,
						   (void **)&input_ptr,
						   &(mywsid.input_wsid), 0);
			check_result(myfpga.res, "allocate input buffer");
			myfpga.res = fpgaPrepareBuffer(myfpga.accelerator_handle,
						   LPBK1_BUFFER_ALLOCATION_SIZE,
						   (void **)&output_ptr,
						   &(mywsid.output_wsid), 0);
			check_result(myfpga.res, "allocating output buffer");

			// Initialize buffers
			memset((void *)dsm_ptr,    0,    LPBK1_DSM_SIZE);
			memset((void *)input_ptr,  0xAF, LPBK1_BUFFER_SIZE);
			memset((void *)output_ptr, 0xBE, LPBK1_BUFFER_SIZE);

			cache_line *cl_ptr = (cache_line *)input_ptr;
			for (uint32_t i = 0; i < LPBK1_BUFFER_SIZE / CL(1); ++i) {
			    // set the last uint in every cacheline
			    cl_ptr[i].uint[15] = i+1;
			}

			hello_fpga_test(&myfpga, &mywsid, &status_ptr, &dsm_ptr);
			check_result(myfpga.res, "running Hello Fpga Test");

			// Check output buffer contents
			for (uint32_t i = 0; i < LPBK1_BUFFER_SIZE; i++) {
			    if (((uint8_t*)output_ptr)[i] != ((uint8_t*)input_ptr)[i]) {
				    fprintf(stderr, "Output does NOT match input "
					    "at offset %i!\n", i);
				    break;
			    }
			}
			printf("Done Running Test in parent process.\n");
			cleanup(&myfpga, &mywsid, pid);
		}
        }
	else
	{
            //failed fork
            printf("fork() failed");
            free(myfpga.accelerator_token);
            return 1;
        }
	free(myfpga.accelerator_token);
	return EXIT_SUCCESS;
}


void check_result(uint64_t res, char * err_str)
{
	if(res == FPGA_BUSY)
	{
		printf("Error. Fpga Busy\n");
		return;
	}
	if(res != FPGA_OK)
	{
		const char* err = fpgaErrStr(res);
		printf("%s: %s\n", err, err_str);
		exit(EXIT_FAILURE);
	}
}

void build_filter(fpga_struct* fpga, char* ID)
{
	// Look for  AFU with given ID
	if (uuid_parse(ID, fpga->guid) < 0){
		fprintf(stderr, "Error parsing guid '%s'\n", ID);
		fpga->res = FPGA_INVALID_PARAM;
	}
	fpga->res = fpgaGetProperties(NULL, &(fpga->filter));
	check_result(fpga->res, "creating properties object\n");

	fpga->res = fpgaPropertiesSetObjectType(fpga->filter, FPGA_ACCELERATOR);
	check_result(fpga->res, "setting object type\n");

	fpga->res = fpgaPropertiesSetGUID(fpga->filter, fpga->guid);
	check_result(fpga->res, "setting GUID");
}

int enumerate_fpga(fpga_struct* fpga, uint32_t* num_matches)
{
	fpga->res = fpgaEnumerate(&(fpga->filter), 1, fpga->accelerator_token,
				  2, num_matches);
	check_result(fpga->res,"enumerating AFUs\n");
	if(*num_matches < 1)
	{
		fprintf(stderr, "Accelerator not found!\n");
		fpga->res = fpgaDestroyProperties(&(fpga->filter));
		return FPGA_INVALID_PARAM;
	}
	return FPGA_OK;
}

void open_afu(fpga_struct * fpga, const uint32_t num_matches)
{
	// Cycle through token array and see if FPGA is busy or not
	uint32_t i = 0;
	while (i < num_matches){
		fpga->res = fpgaOpen(fpga->accelerator_token[i],
				     &(fpga->accelerator_handle),
				     fpga->open_flags);
		if(fpga->res == FPGA_BUSY){ ++i; }
		else {break;}
	}
	check_result(fpga->res, "opening Accelerator\n");

	fpga->res = fpgaMapMMIO(fpga->accelerator_handle, 0, NULL);
	check_result(fpga->res, "mapping MMIO space\n");
}

void hello_afu_test(fpga_struct *fpga)
{
	// Reset AFU
	fpga->res = fpgaReset(fpga->accelerator_handle);
	check_result(fpga->res, "resetting accelerator\n");

	// Access mandatory AFU registers
	uint64_t data = 0;
	fpga->res = fpgaReadMMIO64(fpga->accelerator_handle, 0, AFU_DFH_REG,
				   &data);
	check_result(fpga->res,"reading from MMIO\n");
	printf("AFU DFH REG = %08lx\n", data);
	fpga->res = fpgaReadMMIO64(fpga->accelerator_handle, 0, AFU_ID_LO,
				   &data);
	check_result(fpga->res, "reading from MMIO\n");
	printf("AFU ID LO = %08lx\n", data);

	fpga->res = fpgaReadMMIO64(fpga->accelerator_handle, 0,AFU_ID_HI,
				   &data);
	check_result(fpga->res, "reading from MMIO\n");
	printf("AFU ID HI = %08lx\n", data);

	fpga->res = fpgaReadMMIO64(fpga->accelerator_handle, 0, AFU_NEXT,&data);
	check_result(fpga->res, "reading from MMIO\n");
	printf("AFU NEXT = %08lx\n", data);

	fpga->res = fpgaReadMMIO64(fpga->accelerator_handle,0, AFU_RESERVED,
				   &data);
	check_result(fpga->res, "reading from MMIO\n");
	printf("AFU RESERVED = %08lx\n", data);

	// Access AFU user scratch-pad register
	fpga->res = fpgaReadMMIO64(fpga->accelerator_handle, 0, SCRATCH_REG,
				   &data);
	check_result(fpga->res, "reading from MMIO\n");
	printf("Reading Scratch Register (Byte Offset=%08lx) = %08lx\n",
	   SCRATCH_REG, data);

	printf("MMIO Write to Scratch Register (Byte Offset=%08lx) = %08lx\n",
	   SCRATCH_REG, SCRATCH_VALUE);
	fpga->res = fpgaWriteMMIO64(fpga->accelerator_handle, 0,
				SCRATCH_REG, SCRATCH_VALUE);
	check_result(fpga->res, "writing to MMIO\n");

	fpga->res = fpgaReadMMIO64(fpga->accelerator_handle, 0, SCRATCH_REG,
				   &data);
	check_result(fpga->res, "reading from MMIO\n");

	printf("Reading Scratch Register (Byte Offset=%08lx) = %08lx\n",
	       SCRATCH_REG, data);

	ASSERT(data == SCRATCH_VALUE, "MMIO mismatched expected result");
	// Set Scratch Register to 0
	printf("Setting Scratch Register (Byte Offset=%08lx) = %08lx\n",
	       SCRATCH_REG, SCRATCH_RESET);
	fpga->res = fpgaWriteMMIO64(fpga->accelerator_handle, 0, SCRATCH_REG,
				SCRATCH_RESET);
	check_result(fpga->res, "writing to MMIO\n");

	fpga->res = fpgaReadMMIO64(fpga->accelerator_handle, 0, SCRATCH_REG,
				   &data);
	check_result(fpga->res, "reading from MMIO\n");


	printf("Reading Scratch Register (Byte Offset=%08lx) = %08lx\n",
	       SCRATCH_REG,data);

	ASSERT(data == SCRATCH_RESET, "MMIO mismatched expected result");
}

void hello_fpga_test(fpga_struct* fpga, wsid_struct* wsid,
		    volatile uint64_t ** status, volatile uint64_t ** dsm)
{

	// Reset accelerator
	fpga->res = fpgaReset(fpga->accelerator_handle);
	check_result(fpga->res, "reading from MMIO\n");


	// Program DMA addresses
	uint64_t iova;
	fpga->res = fpgaGetIOAddress(fpga->accelerator_handle, wsid->dsm_wsid,
				 &iova);
	check_result(fpga->res, "getting DSM IOVA\n");

	fpga->res = fpgaWriteMMIO64(fpga->accelerator_handle, 0,
				    CSR_AFU_DSM_BASEL, iova);
	check_result(fpga->res, "writing CSR_AFU_DSM_BASEL\n");

	fpga->res = fpgaWriteMMIO32(fpga->accelerator_handle, 0, CSR_CTL, 0);
	check_result(fpga->res, "writing CSR_CFG");

	fpga->res = fpgaWriteMMIO32(fpga->accelerator_handle, 0, CSR_CTL, 1);
	check_result(fpga->res, "writing CSR_CFG");

	fpga->res = fpgaGetIOAddress(fpga->accelerator_handle, wsid->input_wsid,
				 &iova);
	check_result(fpga->res, "getting input IOVA");

	fpga->res = fpgaWriteMMIO64(fpga->accelerator_handle, 0, CSR_SRC_ADDR,
				CACHELINE_ALIGNED_ADDR(iova));
	check_result(fpga->res, "writing CSR_SRC_ADDR");

	fpga->res = fpgaGetIOAddress(fpga->accelerator_handle,
				     wsid->output_wsid,  &iova);
	check_result(fpga->res,"getting output IOVA\n");

	fpga->res = fpgaWriteMMIO64(fpga->accelerator_handle, 0, CSR_DST_ADDR,
				CACHELINE_ALIGNED_ADDR(iova));
	check_result(fpga->res, "writing CSR_DST_ADDR");

	fpga->res = fpgaWriteMMIO32(fpga->accelerator_handle, 0, CSR_NUM_LINES,
				LPBK1_BUFFER_SIZE / CL(1));
	check_result(fpga->res, "writing CSR_NUM_LINES");

	fpga->res = fpgaWriteMMIO32(fpga->accelerator_handle, 0, CSR_CFG, 0x42000);
	check_result(fpga->res, "writing CSR_CFG");

	*status = *dsm + DSM_STATUS_TEST_COMPLETE/8;

	// Start the test
	fpga->res = fpgaWriteMMIO32(fpga->accelerator_handle, 0, CSR_CTL, 3);
	check_result(fpga->res, "writing CSR_CFG");


	// Wait for test completion
	while (0 == (*(*status) & 0x1)){
	    usleep(100);
	}

	// Stop the device
	fpga->res = fpgaWriteMMIO32(fpga->accelerator_handle, 0, CSR_CTL, 7);
	check_result(fpga->res, "writing CSR_CFG");

}

void cleanup(fpga_struct * fpga, wsid_struct * wsid, pid_t pid)
{
	// Release buffers
	printf("cleaning up registers\n");
	if (pid > 0){
		// free output
		fpga->res = fpgaReleaseBuffer(fpga->accelerator_handle,
				       wsid->output_wsid);
		check_result(fpga->res, "releasing output buffer");
		// Free input
		fpga->res = fpgaReleaseBuffer(fpga->accelerator_handle,
				       wsid->input_wsid);
		check_result(fpga->res, "releasing input buffer");
		// Free DSM
		fpga->res = fpgaReleaseBuffer(fpga->accelerator_handle,
				       wsid->dsm_wsid);
		check_result(fpga->res, "releasing DSM buffer");
	}
	// Unmap MMIO space
        fpga->res = fpgaUnmapMMIO(fpga->accelerator_handle, 0);
	check_result(fpga->res, "unmapping MMIO space");

	// close FPGA
        fpga->res = fpgaClose(fpga->accelerator_handle);
	check_result(fpga->res, "closing FPGA");

        // Destroy token
        fpga->res = fpgaDestroyToken(fpga->accelerator_token);
	check_result(fpga->res, "destroying token");

        // Destroy properties object
        fpga->res = fpgaDestroyProperties(&(fpga->filter));
	check_result(fpga->res, "destroying properties object\n");

	// Exit
        if(s_error_count > 0){
                printf("Test FAILED!\n");
		exit(EXIT_FAILURE);
	}
}
