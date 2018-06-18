#include <string.h>
#include <uuid/uuid.h>
#include <opae/fpga.h>
extern "C" {
#include "fpga_dma_internal.h"
#include "fpga_dma.h"
}
#include "gtest/gtest.h"
#define HELLO_AFU_ID              "331DB30C-9885-41EA-9081-F88B8F655CAA"
#define TEST_BUF_SIZE (10*1024*1024)
#define ASE_TEST_BUF_SIZE (4*1024)
#define FPGA_DMA_ADDR_SPAN_EXT_CNTL 0x200
#define FPGA_DMA_ADDR_SPAN_EXT_DATA 0x1000

#define RAND() (rand() % 0x7fffffff)
#define RAND_CNT() (rand() %  0x7fffff)
#define random_test_cnt 5

// Uncomment to enable bandwidth measurement
//#define FPGA_DMA_BANDWIDTH_TEST 1
//#define DMA_DEBUG 1

// Convenience macros
#ifdef DMA_DEBUG
   #define debug_printk(fmt, ...) \
      do { if (DMA_DEBUG) fprintf(stderr, fmt, ##__VA_ARGS__); } while (0)
#else
   #define debug_printk(...)
#endif

struct timeval start, stop;
double secs = 0;

static int err_cnt=0;
#define ON_ERR(res,label, desc)\
  do {\
    if ((res) != FPGA_OK) {\
      err_cnt++;\
      fprintf(stderr, "Error %s: %s\n", (desc), fpgaErrStr(res));\
      goto label;\
    }\
  } while (0)

/*

TEST CASES:

host to fpga
src         dst         count                         test
aligned     aligned     aligned                       h2f_aligned
aligned     aligned     unaligned                     h2f_unaligned_cnt
aligned     unaligned   aligned                       h2f_unaligned_dst
aligned     unaligned   unaligned                     h2f_unaligned_dst_unaligned_cnt
unaligned   aligned     aligned                       -
unaligned   aligned     unaligned                     -
unaligned   unaligned   aligned                       -
unaligned   unaligned   unaligned                     -

fpga to host
src         dst         count
aligned     aligned     aligned                       f2h_aligned
aligned     aligned     unaligned                     f2h_unaligned_cnt
aligned     unaligned   aligned                       -
aligned     unaligned   unaligned                     -
unaligned   aligned     aligned                       f2h_unaligned_src
unaligned   aligned     unaligned                     f2h_unaligned_src_unaligned_cnt
unaligned   unaligned   aligned                       -
unaligned   unaligned   unaligned                     -

fpga to fpga
src         dst         count
aligned     aligned     aligned                       f2f_aligned
aligned     aligned     unaligned                     f2f_unaligned_cnt
aligned     unaligned   aligned                       f2f_unaligned_dst
aligned     unaligned   unaligned                     f2f_unaligned_dst_unaligned_cnt
unaligned   aligned     aligned                       f2f_unaligned_src
unaligned   aligned     unaligned                     f2f_unaligned_src_unaligned_cnt
unaligned   unaligned   aligned                       f2f_unaligned_src_unaligned_dst
unaligned   unaligned   unaligned                     f2f_unaligned

Additional Tests:
*)valid count test(Count =0)                          h2f_valid_count, f2h_valid_count, f2f_valid_count
*)addr overlap test(src<dst && src+count. dst)        h2f_overlap_addr, f2h_overlap_addr, f2f_overlap_addr
*)addr span exp test(count<64)                        h2f_addr_span_exp_test, f2h_addr_span_exp_test, f2f_addr_span_exp_test
*)Random test(src,dst,count are randomly generated)   h2f_random_test, f2h_random_test, f2f_random_test

*/

struct test_parms_s{
   uint64_t fpga_addr1;
   uint64_t fpga_addr2;
   uint64_t tx_count;
   bool pass;
};

uint64_t* generate_addr(int range) {
   size_t i;
   uint64_t *n1 = (uint64_t*)malloc(random_test_cnt*sizeof(uint64_t));

   srand(time(0));

   for (i = 0; i < random_test_cnt; i++) {
      if(!range)
         n1[i] = ((uint64_t)RAND_CNT());
      else
         n1[i] = ((uint64_t)RAND()/(i*range+1));
   }
   return n1;
}

test_parms_s* build (uint64_t addr1, uint64_t addr2, uint64_t count)
{
   test_parms_s* pTestData =  new test_parms_s;
   pTestData->fpga_addr1 = addr1;
   pTestData->fpga_addr2 = addr2;
   pTestData->tx_count = count;
   if((addr1 < addr2) && (addr1 + count >= addr2))
      pTestData->pass = false;
   else
      pTestData->pass = true;
   return pTestData;
}

std::vector<test_parms_s*> BuildTestData() {
   uint64_t *addr1 = generate_addr(256);
   uint64_t *addr2 = generate_addr(512);
   uint64_t *count = generate_addr(0);
   uint64_t i;
   std::vector<test_parms_s*> values;
   for(i = 0; i<random_test_cnt; i++){
      values.push_back(build(*(addr1+i), *(addr2+i), *(count+i)));
   }
   return values;
}

//class  DmaAfuTest : public ::testing::TestWithParam<test_param> {
class  DmaAfuTest : public ::testing::Test {
 public:
   fpga_result InitRoutine(){
      if(uuid_parse(HELLO_AFU_ID, guid) < 0) {
         return (fpga_result)1;
      }
      res = fpgaGetProperties(NULL, &filter);
      ON_ERR(res, out, "fpgaGetProperties");
      res = fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR);
      ON_ERR(res, out, "fpgaPropertiesSetObjectType");
      res = fpgaPropertiesSetGUID(filter, guid);
      ON_ERR(res, out, "fpgaPropertiesSetGUID");
      res = fpgaEnumerate(&filter, 1, &afc_token, 1, &num_matches);
      ON_ERR(res, out, "fpgaEnumerate");
      if(num_matches < 1) {
         ON_ERR(FPGA_INVALID_PARAM, out, "num_matches<1");
         return (fpga_result)err_cnt;
      }
      res = fpgaOpen(afc_token, &afc_h, 0);
      ON_ERR(res, out, "fpgaOpen");

      if(!use_ase) {
         res = fpgaMapMMIO(afc_h, 0, (uint64_t**)&mmio_ptr);
         ON_ERR(res, out, "fpgaMapMMIO");
      }

      // reset AFC
      res = fpgaReset(afc_h);
      ON_ERR(res, out,"fpgaReset");
      res = fpgaDmaOpen(afc_h, &dma_h);
      ON_ERR(res, out,"fpgaDmaOpen");

   out:
		return (fpga_result)err_cnt;
   }

   virtual void SetUp() {
      ASSERT_EQ(0,InitRoutine());
   }

   fpga_result exitRoutine(){
      res = fpgaDmaClose(dma_h);
      ON_ERR(res, out, "fpgaDmaClose");
      if(!use_ase) {
         res = fpgaUnmapMMIO(afc_h, 0);
         ON_ERR(res, out, "fpgaUnmapMMIO");
      }
      res = fpgaClose(afc_h);
      ON_ERR(res, out, "fpgaClose");
      res = fpgaDestroyToken(&afc_token);
      ON_ERR(res, out, "fpgaDestroyToken");
      res = fpgaDestroyProperties(&filter);
      ON_ERR(res, out, "fpgaDestroyProperties");
   out:
      return (fpga_result)err_cnt;
   }

   virtual void TearDown() {
      ASSERT_EQ(0,exitRoutine());
   }

   void fill_buffer(char *buf, uint32_t size) {
      uint32_t i=0;
      // use a deterministic seed to generate pseudo-random numbers
      srand(99);

      for(i=0; i<size; i++) {
         *buf = rand()%256;
         buf++;
      }
   }

   fpga_result verify_buffer(char *buf, uint32_t size) {
      uint32_t i, rnum=0;
      srand(99);

      for(i=0; i<size; i++) {
         rnum = rand()%256;
         if((*buf&0xFF) != rnum) {
            debug_printk("Invalid data at %d Expected = %x Actual = %x\n",i,rnum,(*buf&0xFF));
            return FPGA_INVALID_PARAM;
         }
         buf++;
      }
      debug_printk("Buffer Verification Success!\n");
      return FPGA_OK;
   }

   void clear_buffer(char *buf, uint32_t size) {
      memset(buf, 0, size);
   }

   fpga_result H2F_F2H(fpga_dma_handle dma_h,uint64_t dev_addr1, uint64_t count){
      uint64_t *dma_buf_ptr  = NULL;
      dma_buf_ptr = (uint64_t*)malloc(count);
      fill_buffer((char*)dma_buf_ptr, count);

      // copy from host to fpga
      gettimeofday(&start, NULL);
      res = fpgaDmaTransferSync(dma_h, dev_addr1 /*dst*/, (uint64_t)dma_buf_ptr /*src*/, count, HOST_TO_FPGA_MM);
      ON_ERR(res, out, "fpgaDmaTransferSync HOST_TO_FPGA_MM");
      clear_buffer((char*)dma_buf_ptr, count);
      gettimeofday(&stop, NULL);
      secs = ((double)(stop.tv_usec - start.tv_usec) / 1000000) + (double)(stop.tv_sec - start.tv_sec);
      if(secs>0){
         debug_printk("Time taken Host To FPGA - %f s, BandWidth = %f MB/s \n",secs, ((unsigned long long)count/(float)secs/1000000));
      }
      // copy from fpga to host
      gettimeofday(&start, NULL);
      res = fpgaDmaTransferSync(dma_h, (uint64_t)dma_buf_ptr /*dst*/, dev_addr1 /*src*/, count, FPGA_TO_HOST_MM);
      ON_ERR(res, out, "fpgaDmaTransferSync FPGA_TO_HOST_MM");
      gettimeofday(&stop, NULL);
      secs = ((double)(stop.tv_usec - start.tv_usec) / 1000000) + (double)(stop.tv_sec - start.tv_sec);
      if(secs > 0)
         debug_printk("Time taken FPGA To Host - %f s, BandWidth = %f MB/s \n",secs, ((unsigned long long)count/(float)secs/1000000));

      res = verify_buffer((char*)dma_buf_ptr, count);
      ON_ERR(res, out, "verify_buffer");
      clear_buffer((char*)dma_buf_ptr, count);
   out:
      free(dma_buf_ptr);
      return (fpga_result)err_cnt;
   }

   fpga_result H2F_F2F_F2H(fpga_dma_handle dma_h,uint64_t dev_addr1, uint64_t dev_addr2, uint64_t count){
      uint64_t *dma_buf_ptr  = NULL;
      dma_buf_ptr = (uint64_t*)malloc(count);
      fill_buffer((char*)dma_buf_ptr, count);

      gettimeofday(&start, NULL);
      res = fpgaDmaTransferSync(dma_h, dev_addr1 /*dst*/, (uint64_t)dma_buf_ptr /*src*/, count, HOST_TO_FPGA_MM);
      ON_ERR(res, out, "fpgaDmaTransferSync HOST_TO_FPGA_MM");
      gettimeofday(&stop, NULL);
      secs = ((double)(stop.tv_usec - start.tv_usec) / 1000000) + (double)(stop.tv_sec - start.tv_sec);
      if(secs > 0)
         debug_printk("Time taken Host To FPGA - %f s, BandWidth = %f MB/s \n",secs, ((unsigned long long)count/(float)secs/1000000));

      // copy from fpga to fpga
      gettimeofday(&start, NULL);
      res = fpgaDmaTransferSync(dma_h, dev_addr2 /*dst*/, dev_addr1 /*src*/, count, FPGA_TO_FPGA_MM);
      ON_ERR(res, out, "fpgaDmaTransferSync FPGA_TO_FPGA_MM");
      clear_buffer((char*)dma_buf_ptr, count);
      gettimeofday(&stop, NULL);
      secs = ((double)(stop.tv_usec - start.tv_usec) / 1000000) + (double)(stop.tv_sec - start.tv_sec);
      if(secs > 0)
         debug_printk("Time taken FPGA To FPGA - %f s, BandWidth = %f MB/s\n",secs, ((unsigned long long)count/(float)secs/1000000));

      // copy from fpga to host
      gettimeofday(&start, NULL);
      res = fpgaDmaTransferSync(dma_h, (uint64_t)dma_buf_ptr /*dst*/, dev_addr2 /*src*/, count, FPGA_TO_HOST_MM);
      ON_ERR(res, out, "fpgaDmaTransferSync FPGA_TO_HOST_MM");
      gettimeofday(&stop, NULL);
      secs = ((double)(stop.tv_usec - start.tv_usec) / 1000000) + (double)(stop.tv_sec - start.tv_sec);
      if(secs > 0)
         debug_printk("Time taken FPGA To Host - %f s, BandWidth = %f MB/s\n",secs, ((unsigned long long)count/(float)secs/1000000));

      res = verify_buffer((char*)dma_buf_ptr, count);
      ON_ERR(res, out, "verify_buffer");
      clear_buffer((char*)dma_buf_ptr, count);
   out:
      free(dma_buf_ptr);
      return (fpga_result)err_cnt;
   }

   fpga_result res = FPGA_OK;
   fpga_dma_handle dma_h;
   fpga_properties filter = NULL;
   fpga_token afc_token;
   fpga_handle afc_h;
   fpga_guid guid;
   uint32_t num_matches;
   volatile uint64_t *mmio_ptr = NULL;
   uint32_t use_ase=0;
   uint64_t dev_addr1;
   uint64_t dev_addr2;
   uint64_t count;
   bool pass;
   uint64_t *dma_buf_ptr  = NULL;
   struct _fpga_properties *prop;

};

class  WithParamDmaAfuTest : public DmaAfuTest, public ::testing::WithParamInterface<test_parms_s*> {

};

INSTANTIATE_TEST_CASE_P(RandomTest, WithParamDmaAfuTest, ::testing::ValuesIn(BuildTestData()));

// HOST to FPGA Tests!
TEST_F(DmaAfuTest, h2f_aligned)
{
   count = 512;
   dev_addr1 = 0x40;
   debug_printk("dev_addr1 = %08lx, count = %08lx \n", dev_addr1, count);
   EXPECT_EQ(0,H2F_F2H(dma_h,dev_addr1,count));
   err_cnt = 0;
}

TEST_F(DmaAfuTest, h2f_unaligned_count)
{
   count = 1023*1023*2  ;
   dev_addr1 = 0x40;
   debug_printk("dev_addr1 = %08lx, count = %08lx \n", dev_addr1, count);
   EXPECT_EQ(0,H2F_F2H(dma_h,dev_addr1,count));
   err_cnt = 0;
}

TEST_F(DmaAfuTest, h2f_unaligned_dst)
{
   count = 1023*1024;
   dev_addr1 = 0x1D8;
   debug_printk("dev_addr1 = %08lx, count = %08lx \n", dev_addr1, count);
   EXPECT_EQ(0,H2F_F2H(dma_h,dev_addr1,count));
   err_cnt = 0;
}

TEST_F(DmaAfuTest, h2f_unaligned_dst_unaligned_cnt)
{
   count = 5*1023*1025;
   dev_addr1 = 0x348;
   debug_printk("dev_addr1 = %08lx, count = %08lx \n", dev_addr1, count);
   EXPECT_EQ(0,H2F_F2H(dma_h,dev_addr1,count));
   err_cnt = 0;
}

// FPGA to HOST Tests!
TEST_F(DmaAfuTest, f2h_aligned)
{
   count = 1023*1024*10;
   dev_addr1 = 0x2C0;
   debug_printk("dev_addr1 = %08lx, count = %08lx \n", dev_addr1, count);
   EXPECT_EQ(0,H2F_F2H(dma_h,dev_addr1,count));
   err_cnt = 0;
}

TEST_F(DmaAfuTest, f2h_unaligned_count)
{
   count = 1023*980*10;
   dev_addr1 = 0x2C0;
   debug_printk("dev_addr1 = %08lx, count = %08lx \n", dev_addr1, count);
   EXPECT_EQ(0,H2F_F2H(dma_h,dev_addr1,count));
   err_cnt = 0;
}

TEST_F(DmaAfuTest, f2h_unaligned_src)
{
   count = 1023*1024*10;
   dev_addr1 = 0x7FFF3;
   debug_printk("dev_addr1 = %08lx, count = %08lx \n", dev_addr1, count);
   EXPECT_EQ(0,H2F_F2H(dma_h,dev_addr1,count));
   err_cnt = 0;
}

TEST_F(DmaAfuTest, f2h_unaligned_src_unaligned_cnt)
{
   count = 1058*860*3;
   dev_addr1 = 0x3467893;
   debug_printk("dev_addr1 = %08lx, count = %08lx \n", dev_addr1, count);
   EXPECT_EQ(0,H2F_F2H(dma_h,dev_addr1,count));
   err_cnt = 0;
}

// FPGA to FPGA Tests!
TEST_F(DmaAfuTest, f2f_aligned)
{
   count = 1023*1024;
   dev_addr1 = 0x40;
   dev_addr2 = 0x100000;
   debug_printk("dev_addr1 = %08lx , dev_addr2 = %08lx , count = %08lx \n", dev_addr1, dev_addr2 , count);
   EXPECT_EQ(0, H2F_F2F_F2H(dma_h, dev_addr1, dev_addr2, count));
   err_cnt = 0;
}

TEST_F(DmaAfuTest, f2f_unaligned_cnt)
{
   count = 1023*1023*4;
   dev_addr1 = 0x3C0;
   dev_addr2 = 0x400000;
   debug_printk("dev_addr1 = %08lx , dev_addr2 = %08lx , count = %08lx \n", dev_addr1, dev_addr2 , count);
   EXPECT_EQ(0, H2F_F2F_F2H(dma_h, dev_addr1, dev_addr2, count));
   err_cnt = 0;
}

TEST_F(DmaAfuTest, f2f_unaligned_dst)
{
   count = 1023*1024*6;
   dev_addr1 = 0x600;
   dev_addr2 = 0x5FEFF4;
   debug_printk("dev_addr1 = %08lx , dev_addr2 = %08lx , count = %08lx \n", dev_addr1, dev_addr2 , count);
   EXPECT_EQ(0, H2F_F2F_F2H(dma_h, dev_addr1, dev_addr2, count));
   err_cnt = 0;
}

TEST_F(DmaAfuTest, f2f_unaligned_dst_unaligned_cnt)
{
   count = 1023*1520;
   dev_addr1 = 0x1440;
   dev_addr2 = 0x21D7A0;
   debug_printk("dev_addr1 = %08lx , dev_addr2 = %08lx , count = %08lx \n", dev_addr1, dev_addr2 , count);
   EXPECT_EQ(0, H2F_F2F_F2H(dma_h, dev_addr1, dev_addr2, count));
   err_cnt = 0;
}

TEST_F(DmaAfuTest, f2f_unaligned_src)
{
   count = 1023*1024*15;
   dev_addr1 = 0x790;
   dev_addr2 = 0x9854200;
   debug_printk("dev_addr1 = %08lx , dev_addr2 = %08lx , count = %08lx \n", dev_addr1, dev_addr2 , count);
   EXPECT_EQ(0, H2F_F2F_F2H(dma_h, dev_addr1, dev_addr2, count));
   err_cnt = 0;
}

TEST_F(DmaAfuTest, f2f_unaligned_src_unaligned_cnt)
{
   count = 1028*5048;
   dev_addr1 = 0x75943;
   dev_addr2 = 0xC002040;
   debug_printk("dev_addr1 = %08lx , dev_addr2 = %08lx , count = %08lx \n", dev_addr1, dev_addr2 , count);
   EXPECT_EQ(0, H2F_F2F_F2H(dma_h, dev_addr1, dev_addr2, count));
   err_cnt = 0;
}

TEST_F(DmaAfuTest, f2f_unaligned_src_unaligned_dst)
{
   count = 1023*1024*3;
   dev_addr1 = 0x47823;
   dev_addr2 = 0xE43593;
   debug_printk("dev_addr1 = %08lx , dev_addr2 = %08lx , count = %08lx \n", dev_addr1, dev_addr2 , count);
   EXPECT_EQ(0, H2F_F2F_F2H(dma_h, dev_addr1, dev_addr2, count));
   err_cnt = 0;
}

TEST_F(DmaAfuTest, f2f_unaligned)
{
   count = 1123*7128;
   dev_addr1 = 0x39542;
   dev_addr2 = 0xCD5680;
   debug_printk("dev_addr1 = %08lx , dev_addr2 = %08lx , count = %08lx \n", dev_addr1, dev_addr2 , count);
   EXPECT_EQ(0, H2F_F2F_F2H(dma_h, dev_addr1, dev_addr2, count));
   err_cnt = 0;
}

// Additional Tests!
TEST_F(DmaAfuTest, h2f_valid_count)
{
   count = 0;
   dev_addr1 = 0x400;
   debug_printk("dev_addr1 = %08lx, count = %08lx \n", dev_addr1, count);
   EXPECT_EQ(0,H2F_F2H(dma_h,dev_addr1,count));
   err_cnt = 0;
}

TEST_F(DmaAfuTest, f2h_valid_count)
{
   count = 0;
   dev_addr1 = 0x6C0;
   debug_printk("dev_addr1 = %08lx, count = %08lx \n", dev_addr1, count);
   EXPECT_EQ(0,H2F_F2H(dma_h,dev_addr1,count));
   err_cnt = 0;
}

TEST_F(DmaAfuTest, f2f_overlap_addr)
{
   count = 10*1024*1024;
   dev_addr1 = 0x3C;
   dev_addr2 = 0xA00000;
   debug_printk("dev_addr1 = %08lx , dev_addr2 = %08lx , count = %08lx \n", dev_addr1, dev_addr2 , count);
   EXPECT_EQ(1, H2F_F2F_F2H(dma_h, dev_addr1, dev_addr2, count));
   err_cnt = 0;
}

TEST_F(DmaAfuTest, h2f_addr_span_exp_test)
{
   count = 48;
   dev_addr1 = 0x3C;
   debug_printk("dev_addr1 = %08lx , count = %08lx \n", dev_addr1, count);
   EXPECT_EQ(0, H2F_F2H(dma_h, dev_addr1, count));
   err_cnt = 0;
}

TEST_F(DmaAfuTest, f2h_addr_span_exp_test)
{
   count = 30;
   dev_addr1 = 0x420;
   debug_printk("dev_addr1 = %08lx , count = %08lx \n", dev_addr1, count);
   EXPECT_EQ(0, H2F_F2H(dma_h, dev_addr1, count));
   err_cnt = 0;
}

TEST_F(DmaAfuTest, f2f_addr_span_exp_test)
{
   count = 25;
   dev_addr1 = 0x1432;
   dev_addr2 = 0x9FFFF0;
   debug_printk("dev_addr1 = %08lx , dev_addr2 = %08lx , count = %08lx \n", dev_addr1, dev_addr2 , count);
   EXPECT_EQ(0, H2F_F2F_F2H(dma_h, dev_addr1, dev_addr2, count));
   err_cnt = 0;
}

// Bandwidth test
#ifdef FPGA_DMA_BANDWIDTH_TEST
TEST_F(DmaAfuTest, h2f_bandwidth_test)
{
   count = 8l*1024l*1024l*1024l/32l;
   dev_addr1 = 0x0;
   debug_printk("dev_addr1 = %08lx , count = %08lx \n", dev_addr1, count);
   EXPECT_EQ(0, H2F_F2H(dma_h, dev_addr1, count));
   err_cnt = 0;
}
#endif

// Random Test
TEST_P(WithParamDmaAfuTest, h2f_random_test)
{
   test_parms_s *tp = GetParam();
   count = tp->tx_count;
   dev_addr1 = tp->fpga_addr1;
   debug_printk("dev_addr1 = %08lx , count = %08lx \n", dev_addr1, count);
   EXPECT_EQ(0, H2F_F2H(dma_h, dev_addr1, count));
   err_cnt = 0;
}

TEST_P(WithParamDmaAfuTest, f2h_random_test)
{
   test_parms_s *tp = GetParam();
   count = tp->tx_count;
   dev_addr1 = tp->fpga_addr1;
   debug_printk("dev_addr1 = %08lx , count = %08lx \n", dev_addr1, count);
   EXPECT_EQ(0, H2F_F2H(dma_h, dev_addr1, count));
   err_cnt = 0;
}

TEST_P(WithParamDmaAfuTest, f2f_random_test)
{
   test_parms_s *tp = GetParam();
   count = tp->tx_count;
   dev_addr1 = tp->fpga_addr1;
   dev_addr2 = tp->fpga_addr2;
   debug_printk("dev_addr1 = %08lx , dev_addr2 = %08lx , count = %08lx \n", dev_addr1, dev_addr2 , count);
   H2F_F2F_F2H(dma_h, dev_addr1, dev_addr2, count);
   if(err_cnt){
     pass = false;
     err_cnt = 0;
   }
   else
      pass = true;
   debug_printk("test result = %d, exp_rest = %d \n",pass, tp->pass);
   EXPECT_EQ(tp->pass,pass);
}

int main(int argc, char** argv)
{
   testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}
