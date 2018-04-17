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

/**
 * @test Umsg_drv_01
 * When the parameters are valid and the drivers are loaded,
 * fpgaUmsgGetNumber returns number of umsgs supported by slot.
 */
TEST(Umsg, Umsg_drv_01)
{
	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;
	uint64_t     Umsg_num =0;

	// Open  port device
	token_for_afu0(&_tok);
	EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	// get umsg number
	EXPECT_EQ(FPGA_OK, fpgaGetNumUmsg(h, &Umsg_num));
	EXPECT_GT(Umsg_num,0);

	// close
	EXPECT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test Umsg_drv_02
 * When the parameters are invalid and the drivers are loaded,
 * fpgaUmsgGetNumber returns error.
 */
TEST(Umsg, Umsg_drv_02)
{
	struct _fpga_token   _tok;
	fpga_handle h;
	fpga_token tok    = &_tok;
	uint64_t Umsg_num = 0;
	int fddev         = -1;

	token_for_afu0(&_tok);

	// NULL Driver hnadle
	EXPECT_NE(FPGA_OK, fpgaGetNumUmsg(NULL, &Umsg_num));

	// Invalid Magic Number
	EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	struct _fpga_handle *_handle = (struct _fpga_handle*)h;
	_handle->magic = 0x123;

	EXPECT_NE(FPGA_OK, fpgaGetNumUmsg(h, &Umsg_num));

	_handle->magic = FPGA_HANDLE_MAGIC;
	EXPECT_EQ(FPGA_OK, fpgaClose(h));


	// Invalid Driver handle
	EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
	_handle = (struct _fpga_handle*)h;

	fddev = _handle->fddev;
	_handle->fddev = -1;

	EXPECT_NE(FPGA_OK, fpgaGetNumUmsg(h, &Umsg_num));

	_handle->fddev = fddev;
	EXPECT_EQ(FPGA_OK, fpgaClose(h));


	// Invlaid Input Paramter
	EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	EXPECT_NE(FPGA_OK, fpgaGetNumUmsg(h, NULL));

	EXPECT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test Umsg_drv_03
 * When the parameters are valid and the drivers are loaded,
 * fpgaUmsgSetAttributes sets umsg hit  Enable / Disable.
 */
TEST(Umsg, Umsg_drv_03)
{
	struct _fpga_token _tok;
	fpga_token  tok = &_tok;
	fpga_handle h;
	uint64_t     Umsghit_Enable =0xffff;
	uint64_t     Umsghit_Disble =0;

	// Open  port device
	token_for_afu0(&_tok);
	EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	// Set umsg hint
	EXPECT_NE(FPGA_OK, fpgaSetUmsgAttributes(h, Umsghit_Enable));
	EXPECT_EQ(FPGA_OK, fpgaSetUmsgAttributes(h, Umsghit_Disble));

	// close
	EXPECT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test Umsg_drv_04
 * When the parameters are Invalid and the drivers are loaded,
 * fpgaUmsgSetAttributes retuns error.
 */
TEST(Umsg, Umsg_drv_04)
{
	struct _fpga_token _tok;
	fpga_token  tok = &_tok;
	fpga_handle h;
	uint64_t     Umsghit_Enable = 0xffff;
	uint64_t     Umsghit_Disble = 0;
	int          fddev          = -1;

	// Open  port device
	token_for_afu0(&_tok);


	// NULL Driver hnadle
	EXPECT_NE(FPGA_OK, fpgaSetUmsgAttributes(NULL, Umsghit_Disble));


	// Invalid Magic Number
	EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	struct _fpga_handle *_handle = (struct _fpga_handle*)h;
	_handle->magic = 0x123;

	EXPECT_NE(FPGA_OK, fpgaSetUmsgAttributes(h, Umsghit_Disble));

	_handle->magic = FPGA_HANDLE_MAGIC;
	EXPECT_EQ(FPGA_OK, fpgaClose(h));

	// Invalid Driver handle
	EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
	_handle = (struct _fpga_handle*)h;

	fddev = _handle->fddev ;
	_handle->fddev = -1;

	EXPECT_NE(FPGA_OK, fpgaSetUmsgAttributes(h, Umsghit_Disble));

	_handle->fddev = fddev ;
	EXPECT_EQ(FPGA_OK, fpgaClose(h));


	// Invlaid Input Paramter
	EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	EXPECT_NE(FPGA_OK, fpgaSetUmsgAttributes(h, 0xFFFFFFFF));

	EXPECT_EQ(FPGA_OK, fpgaClose(h));

}

/**
 * @test Umsg_drv_05
 * When the parameters are valid and the drivers are loaded,
 * fpgaGetUmsgPtr returns umsg address.
 */
TEST(Umsg, Umsg_drv_05)
{
	struct _fpga_token _tok;
	fpga_token  tok = &_tok;
	fpga_handle h;
	uint64_t    *umsg_ptr  = NULL;

	// Open  port device
	token_for_afu0(&_tok);
	EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	// Get umsg buffer
	EXPECT_EQ(FPGA_OK, fpgaGetUmsgPtr(h, &umsg_ptr));
	EXPECT_TRUE(umsg_ptr != NULL);
	printf("umsg_ptr %p",umsg_ptr);

	// Close
	EXPECT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test Umsg_drv_06
 * When the parameters are invalid and the drivers are loaded,
 * fpgaGetUmsgPtr returns uerror.
 */
TEST(Umsg, Umsg_drv_06)
{
	struct _fpga_token _tok;
	fpga_token  tok = &_tok;
	fpga_handle h;
	uint64_t    *umsg_ptr  = NULL;
	int          fddev     = -1;

	// Open  port device
	token_for_afu0(&_tok);

	// NULL Driver hnadle
	EXPECT_NE(FPGA_OK, fpgaGetUmsgPtr(NULL, &umsg_ptr));

	// Invalid Magic Number
	EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	struct _fpga_handle *_handle = (struct _fpga_handle*)h;
	_handle->magic = 0x123;

	EXPECT_NE(FPGA_OK, fpgaGetUmsgPtr(h, &umsg_ptr));

	_handle->magic = FPGA_HANDLE_MAGIC;
	EXPECT_EQ(FPGA_OK, fpgaClose(h));

	// Invalid Driver handle
	EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
	_handle = (struct _fpga_handle*)h;

	fddev = _handle->fddev ;
	_handle->fddev = -1;

	EXPECT_NE(FPGA_OK, fpgaGetUmsgPtr(h, &umsg_ptr));

	_handle->fddev  = fddev ;
	EXPECT_EQ(FPGA_OK, fpgaClose(h));

	// Invalid Input Parameter
	EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	EXPECT_NE(FPGA_OK, fpgaGetUmsgPtr(h, NULL));

	EXPECT_EQ(FPGA_OK, fpgaClose(h));


}
