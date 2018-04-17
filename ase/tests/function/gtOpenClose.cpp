#ifdef __cplusplus
extern "C" {
#endif
#include "fpga/access.h"

#ifdef __cplusplus
}
#endif

#include "gtest/gtest.h"
#include "globals.h"
#include "types_int.h"
#define ASE_TOKEN_MAGIC       0x46504741544f4b40
#define ASE_AFU_ID           "D8424DC4-A4A3-C413-F89E-433683F9040B"

void token_for_afu0(struct _fpga_token *_tok)
{
   // slot 0 AFU
   _tok->afc_id = (uint64_t)ASE_AFU_ID;
_tok->magic = ASE_TOKEN_MAGIC;}

void token_for_invalid(struct _fpga_token *_tok)
{
   // slot 0 AFU
} 

/*bool token_is_afu0(fpga_token t)
{
   struct _fpga_token *_t = (struct _fpga_token *)t;

   return (0 == strcmp(_t->sysfspath, SYSFS_FPGA_CLASS_PATH "/intel-fpga-dev.0/intel-fpga-port.0")) &&
          (0 == strcmp(_t->devpath, FPGA_DEV_PATH "/intel-fpga-port.0"));
}*/

/**
 * @test open_01
 * When the fpga_handle * parameter to fpgaOpen is NULL,
 * the function returns FPGA_INVALID_PARAM.
 */
TEST(Open, open_01)
{
   fpga_token tok;

   EXPECT_EQ(FPGA_INVALID_PARAM, fpgaOpen(tok, NULL, 0));
}

/**
 * @test open_06
 * When the fpga_token parameter to fpgaOpen is NULL,
 * the function returns FPGA_INVALID_PARAM.
 */
TEST(Open, open_06)
{
   fpga_handle h;

   EXPECT_EQ(FPGA_INVALID_PARAM, fpgaOpen(NULL, &h, 0));
}

/**
 * @test open_drv_02
 * When the parameters are valid and the drivers are loaded,
 * fpgaOpen returns FPGA_OK.
 */
TEST(Open, open_drv_02)
{
   struct _fpga_token _tok;
   fpga_token tok = &_tok;
   fpga_handle h;

   token_for_afu0(&_tok);
   EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
   fpgaClose(h);
}

/**
 * @test open_drv_07
 * When the parameters are valid and the drivers are loaded,
 * but the user lacks sufficient privilege for the device,
 * fpgaOpen returns FPGA_NO_ACCESS.
 */
TEST(Open, open_drv_07)
{
   struct _fpga_token _tok;
   fpga_token tok = &_tok;
   fpga_handle h;

   // Only root can open FME
 //  token_for_fme0(&_tok);
 //  EXPECT_EQ(FPGA_NO_ACCESS, fpgaOpen(tok, &h, 0));
}

/**
 * @test open_nodrv_03
 * When the parameters are valid but the drivers are not loaded,
 * fpgaOpen returns FPGA_NO_DRIVER.
 */
/*TEST(Open, open_nodrv_03)
{
   struct _fpga_token _tok;
   fpga_token tok = &_tok;
   fpga_handle h;

   token_for_afu0(&_tok);
   EXPECT_EQ(FPGA_NO_DRIVER, fpgaOpen(tok, &h, 0));
}*/

/**
 * @test close_04
 * When the fpga_handle parameter to fpgaClose is NULL,
 * the function returns FPGA_INVALID_PARAM.
 */
TEST(Close, close_04)
{
   EXPECT_EQ(FPGA_INVALID_PARAM, fpgaClose(NULL));
}

/**
 * @test close_drv_05
 * When the parameters are valid, the drivers are loaded,
 * and a previous call to fpgaOpen has been made, fpgaClose
 * returns FPGA_OK.
 */
TEST(Close, close_drv_05)
{
   struct _fpga_token _tok;
   fpga_token tok = &_tok;
   fpga_handle h;

   token_for_afu0(&_tok);
   EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
   EXPECT_EQ(FPGA_OK, fpgaClose(h));
}

