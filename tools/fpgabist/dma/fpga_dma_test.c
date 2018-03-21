// Copyright(c) 2017, Intel Corporation
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

#include <string.h>
#include <uuid/uuid.h>
#include <opae/fpga.h>
#include <time.h>
#include "fpga_dma.h"
/**
 * \fpga_dma_test.c
 * \brief User-mode DMA test
 */

#include <stdlib.h>
#include <assert.h>

#define HELLO_AFU_ID              "331DB30C-9885-41EA-9081-F88B8F655CAA"
#define TEST_BUF_SIZE (10*1024*1024)
#define ASE_TEST_BUF_SIZE (4*1024)


static int err_cnt = 0;
/*
 * macro for checking return codes
 */
#define ON_ERR_GOTO(res, label, desc)\
  do {\
    if ((res) != FPGA_OK) {\
      err_cnt++;\
      fprintf(stderr, "Error %s: %s\n", (desc), fpgaErrStr(res));\
      goto label;\
    }\
  } while (0)


void fill_buffer(char *buf, size_t size) {
   size_t i=0;
   // use a deterministic seed to generate pseudo-random numbers
   srand(99);

   for(i=0; i<size; i++) {
      *buf = rand()%256;
      buf++;
   }
}

fpga_result verify_buffer(char *buf, size_t size) {
   size_t i, rnum=0;
   srand(99);

   for(i=0; i<size; i++) {
      rnum = rand()%256;
      if((*buf&0xFF) != rnum) {
         printf("Invalid data at %zx Expected = %zx Actual = %x\n",i,rnum,(*buf&0xFF));
         return FPGA_INVALID_PARAM;
      }
      buf++;
   }
   printf("Buffer Verification Success!\n");
   return FPGA_OK;
}

void clear_buffer(char *buf, size_t size) {
   memset(buf, 0, size);
}

void report_bandwidth(size_t size, double seconds) {
   double throughput = (double)size/((double)seconds*1000*1000);
   printf("\rMeasured bandwidth = %lf Megabytes/sec\n", throughput);
}

// return elapsed time
double getTime(struct timespec start, struct timespec end) {
   uint64_t diff = 1000000000L * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;
   return (double) diff/(double)1000000000L;
}


fpga_result ddr_sweep(fpga_dma_handle dma_h) {
   int res;
	struct timespec start, end;
   
   ssize_t total_mem_size = (uint64_t)(4*1024)*(uint64_t)(1024*1024);
   
   uint64_t *dma_buf_ptr = malloc(total_mem_size);
   if(dma_buf_ptr == NULL) {
      printf("Unable to allocate %ld bytes of memory", total_mem_size);
      return FPGA_NO_MEMORY;
   }
   printf("Allocated test buffer\n");
   printf("Fill test buffer\n");
   fill_buffer((char*)dma_buf_ptr, total_mem_size);

   uint64_t src = (uint64_t)dma_buf_ptr;
   uint64_t dst = 0x0;
   
   printf("DDR Sweep Host to FPGA\n");   
     
	clock_gettime(CLOCK_MONOTONIC, &start);
   res = fpgaDmaTransferSync(dma_h, dst, src, total_mem_size, HOST_TO_FPGA_MM);
	clock_gettime(CLOCK_MONOTONIC, &end);
   if(res != FPGA_OK) {
      printf(" fpgaDmaTransferSync Host to FPGA failed with error %s", fpgaErrStr(res));
      free(dma_buf_ptr);
      return FPGA_EXCEPTION;
   }
	
   report_bandwidth(total_mem_size, getTime(start,end));

   printf("\rClear buffer\n");
   clear_buffer((char*)dma_buf_ptr, total_mem_size);

   src = 0x0;
   dst = (uint64_t)dma_buf_ptr;
	
   printf("DDR Sweep FPGA to Host\n");   
	clock_gettime(CLOCK_MONOTONIC, &start);
   res = fpgaDmaTransferSync(dma_h, dst, src, total_mem_size, FPGA_TO_HOST_MM);
	clock_gettime(CLOCK_MONOTONIC, &end);

   if(res != FPGA_OK) {
      printf(" fpgaDmaTransferSync FPGA to Host failed with error %s", fpgaErrStr(res));
      free(dma_buf_ptr);
      return FPGA_EXCEPTION;
   }
   report_bandwidth(total_mem_size, getTime(start,end));
   
   printf("Verifying buffer..\n");   
   verify_buffer((char*)dma_buf_ptr, total_mem_size);

   free(dma_buf_ptr);
   return FPGA_OK;
}

int main(int argc, char *argv[]) {
   fpga_result res = FPGA_OK;
   fpga_dma_handle dma_h;
   uint64_t count;
   fpga_properties filter = NULL;
   fpga_token afc_token;
   fpga_handle afc_h;
   fpga_guid guid;
   uint32_t num_matches;
   volatile uint64_t *mmio_ptr = NULL;
   uint64_t *dma_buf_ptr  = NULL;   
   uint32_t use_ase;

   if(argc < 2) {
      printf("Usage: fpga_dma_test <use_ase = 1 (simulation only), 0 (hardware)>");
      return 1;
   }
   use_ase = atoi(argv[1]);
   if(use_ase) {
      printf("Running test in ASE mode\n");
   } else {
      printf("Running test in HW mode\n");
   }

   // enumerate the afc
   if(uuid_parse(HELLO_AFU_ID, guid) < 0) {
      return 1;
   }

   res = fpgaGetProperties(NULL, &filter);
   ON_ERR_GOTO(res, out, "fpgaGetProperties");

   res = fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR);
   ON_ERR_GOTO(res, out_destroy_prop, "fpgaPropertiesSetObjectType");

   res = fpgaPropertiesSetGUID(filter, guid);
   ON_ERR_GOTO(res, out_destroy_prop, "fpgaPropertiesSetGUID");

   res = fpgaEnumerate(&filter, 1, &afc_token, 1, &num_matches);
   ON_ERR_GOTO(res, out_destroy_prop, "fpgaEnumerate");

   if(num_matches < 1) {
      printf("Error: Number of matches < 1");
      ON_ERR_GOTO(FPGA_INVALID_PARAM, out_destroy_prop, "num_matches<1");
   }

   // open the AFC
   res = fpgaOpen(afc_token, &afc_h, 0);
   ON_ERR_GOTO(res, out_destroy_tok, "fpgaOpen");

   if(!use_ase) {
      res = fpgaMapMMIO(afc_h, 0, (uint64_t**)&mmio_ptr);
      ON_ERR_GOTO(res, out_close, "fpgaMapMMIO");
   }

   // reset AFC
   res = fpgaReset(afc_h);
   ON_ERR_GOTO(res, out_unmap, "fpgaReset");

   res = fpgaDmaOpen(afc_h, &dma_h);
   ON_ERR_GOTO(res, out_dma_close, "fpgaDmaOpen");
   if(!dma_h) {
      res = FPGA_EXCEPTION;
      ON_ERR_GOTO(res, out_dma_close, "Invaid DMA Handle");
   }

   if(use_ase)
      count = ASE_TEST_BUF_SIZE;
   else
      count = TEST_BUF_SIZE;

   dma_buf_ptr = (uint64_t*)malloc(count);
   if(!dma_buf_ptr) {
      res = FPGA_NO_MEMORY;
      ON_ERR_GOTO(res, out_dma_close, "Error allocating memory");
   }   

   fill_buffer((char*)dma_buf_ptr, count);

   // Test procedure
   // - Fill host buffer with pseudo-random data
   // - Copy from host buffer to FPGA buffer at address 0x0
   // - Clear host buffer
   // - Copy from FPGA buffer to host buffer
   // - Verify host buffer data
   // - Clear host buffer
   // - Copy FPGA buffer at address 0x0 to FPGA buffer at addr "count"
   // - Copy data from FPGA buffer at addr "count" to host buffer
   // - Verify host buffer data

   // copy from host to fpga
   res = fpgaDmaTransferSync(dma_h, 0x0 /*dst*/, (uint64_t)dma_buf_ptr /*src*/, count, HOST_TO_FPGA_MM);
   ON_ERR_GOTO(res, out_dma_close, "fpgaDmaTransferSync HOST_TO_FPGA_MM");
   clear_buffer((char*)dma_buf_ptr, count);

   // copy from fpga to host
   res = fpgaDmaTransferSync(dma_h, (uint64_t)dma_buf_ptr /*dst*/, 0x0 /*src*/, count, FPGA_TO_HOST_MM);
   ON_ERR_GOTO(res, out_dma_close, "fpgaDmaTransferSync FPGA_TO_HOST_MM");
   res = verify_buffer((char*)dma_buf_ptr, count);
   ON_ERR_GOTO(res, out_dma_close, "verify_buffer");

   clear_buffer((char*)dma_buf_ptr, count);

   // copy from fpga to fpga
   res = fpgaDmaTransferSync(dma_h, count /*dst*/, 0x0 /*src*/, count, FPGA_TO_FPGA_MM);
   ON_ERR_GOTO(res, out_dma_close, "fpgaDmaTransferSync FPGA_TO_FPGA_MM");


   // copy from fpga to host
   res = fpgaDmaTransferSync(dma_h, (uint64_t)dma_buf_ptr /*dst*/, count /*src*/, count, FPGA_TO_HOST_MM);
   ON_ERR_GOTO(res, out_dma_close, "fpgaDmaTransferSync FPGA_TO_HOST_MM");

   res = verify_buffer((char*)dma_buf_ptr, count);
   ON_ERR_GOTO(res, out_dma_close, "verify_buffer");

   if(!use_ase) {
      printf("Running DDR sweep test\n");
      res = ddr_sweep(dma_h);
      ON_ERR_GOTO(res, out_dma_close, "ddr_sweep");
   }

out_dma_close:
   free(dma_buf_ptr);
   if(dma_h)
      res = fpgaDmaClose(dma_h);
   ON_ERR_GOTO(res, out_unmap, "fpgaDmaClose");

out_unmap:
   if(!use_ase) {
      res = fpgaUnmapMMIO(afc_h, 0);
      ON_ERR_GOTO(res, out_close, "fpgaUnmapMMIO");
	}
out_close:
   res = fpgaClose(afc_h);
   ON_ERR_GOTO(res, out_destroy_tok, "fpgaClose");

out_destroy_tok:
   res = fpgaDestroyToken(&afc_token);
   ON_ERR_GOTO(res, out_destroy_prop, "fpgaDestroyToken");

out_destroy_prop:
   res = fpgaDestroyProperties(&filter);
   ON_ERR_GOTO(res, out, "fpgaDestroyProperties");

out:
   return err_cnt;
}
