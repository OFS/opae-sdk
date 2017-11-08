#include <opae/access.h>
#include <opae/manage.h>

#include "gtest/gtest.h"
#include "globals.h"
#include "types_int.h"

/**
 * @test Port_drv_reset_01
 * When the parameters are valid and the drivers are loaded,
 * fpgaReset Resets fpga slot.
 */
TEST(Reset, Port_drv_reset_01)
{
	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;

	// Open port device
	token_for_afu0(&_tok);
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	// Reset slot
	EXPECT_EQ(FPGA_OK, fpgaReset(h));

	// close
	EXPECT_EQ(FPGA_OK, fpgaClose(h));

}

/**
 * @test Port_drv_reset_02
 * When the parameters are invalid and the drivers are loaded,
 * fpgaReset return error.
 */
TEST(Reset, Port_drv_reset_02)
{
	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;
	int fddev = -1;

	// Open port device
	token_for_afu0(&_tok);
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	// Reset slot
	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaReset(NULL));

	// close
	EXPECT_EQ(FPGA_OK, fpgaClose(h));


	// Invalid Magic Number
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	struct _fpga_handle *_handle = (struct _fpga_handle*)h;
	_handle->magic = 0x123;

	EXPECT_NE(FPGA_OK, fpgaReset(h));

	_handle->magic = FPGA_HANDLE_MAGIC;
	EXPECT_EQ(FPGA_OK, fpgaClose(h));


	// Invalid Driver handle
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
	_handle = (struct _fpga_handle*)h;

	fddev = _handle->fddev;
	_handle->fddev = -1;

	EXPECT_EQ(FPGA_OK, fpgaReset(h));

	_handle->fddev = fddev;
	EXPECT_EQ(FPGA_OK, fpgaClose(h));

}
