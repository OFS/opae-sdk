#include <opae/access.h>

#include "gtest/gtest.h"
#include "globals.h"
#include "types_int.h"

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
   ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
   fpgaClose(h);
}

#if 0
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
   token_for_fme0(&_tok);
   EXPECT_EQ(FPGA_NO_ACCESS, fpgaOpen(tok, &h, 0));
}
#endif
/**
 * @test open_nodrv_03
 * When the parameters are valid but the drivers are not loaded,
 * fpgaOpen returns FPGA_NO_DRIVER.
 */
TEST(Open, open_nodrv_03)
{
   struct _fpga_token _tok;
   fpga_token tok = &_tok;
   fpga_handle h;

   token_for_afu0(&_tok);
   EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
}

/**             
 * @test open_drv_09
 * When the parameters are valid and the drivers are loaded,
 * and the flag FPGA_OPEN_SHARED is not given,
 * fpgaOpen on an already opened token returns FPGA_BUSY.
 */
TEST(Open, open_drv_09)
{
	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h1, h2;

	token_for_afu0(&_tok);
	EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h1, 0));
// PENDING---------------------------------------------------------
#if 0
// ASK RAHUL.
	EXPECT_EQ(FPGA_BUSY, fpgaOpen(tok, &h2, 0));
#endif
	fpgaClose(h1);
}

/** --------------------------------------------------------- 
 * @test open_drv_10
 * When the parameters are valid and the drivers are loaded,
 * and the flag FPGA_OPEN_SHARED is given,
 * fpgaOpen on an already opened token returns FPGA_OK.
 */

TEST(Open, open_drv_10)
{
	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h1, h2;

	token_for_afu0(&_tok);
	EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaOpen(tok, &h1, FPGA_OPEN_SHARED));
	EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaOpen(tok, &h2, FPGA_OPEN_SHARED));
// NOT SUPPORTED FOR ASE______________
//	fpgaClose(h1);
//	fpgaClose(h2);
}

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
   ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
   EXPECT_EQ(FPGA_OK, fpgaClose(h));
}

