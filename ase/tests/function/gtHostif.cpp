#ifdef __cplusplus
extern "C" {
#endif
#include "fpga/access.h"
#include "fpga/manage.h"

#ifdef __cplusplus
}
#endif

#include "gtest/gtest.h"
#include <iostream>
#include "globals.h"
#include "types_int.h"

/**
 * @test Hostif_drv_01
 * When the parameters are valid and the drivers are loaded,
 * fpgaAssignPortToInterface Assign and Release  port to a host interface.
 */
TEST(HostIf, Hostif_drv_01)
{
	struct _fpga_token _tok;
	fpga_token  tok = &_tok;
	fpga_handle h;

	// open FME device
	token_for_fme0(&_tok);
	EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	// Release Port Interface
	EXPECT_EQ(FPGA_OK, fpgaAssignPortToInterface(h, 1, 0, 0));

	// Assign Port Interface
	EXPECT_EQ(FPGA_OK, fpgaAssignPortToInterface(h, 0, 0, 0));

	EXPECT_EQ(FPGA_OK, fpgaClose(h));


}
