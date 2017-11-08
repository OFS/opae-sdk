#ifdef __cplusplus
extern "C" {
#endif
#include <opae/access.h>

#ifdef __cplusplus
}
#endif

#include "gtest/gtest.h"
#include "globals.h"
#include "types_int.h"

void token_init(struct _fpga_token *_tok)
{
	   // slot 0 FME
	      memcpy(_tok->accelerator_id,0, sizeof(fpga_guid));
	          _tok->magic = 0;
}

void token_for_fme0(struct _fpga_token *_tok)
{
   // slot 0 FME
   memcpy(_tok->accelerator_id,FPGA_FME_ID, sizeof(fpga_guid));
    _tok->magic = ASE_TOKEN_MAGIC;
    _tok->ase_objtype=FPGA_DEVICE;
}

void token_for_afu0(struct _fpga_token *_tok)
{
   // slot 0 AFU
   memcpy(_tok->accelerator_id,ASE_GUID, sizeof(fpga_guid));
   _tok->magic = ASE_TOKEN_MAGIC;
   _tok->ase_objtype=FPGA_ACCELERATOR;
}

void token_for_invalid(struct _fpga_token *_tok)
{
   // slot 0 AFU
   memcpy(_tok->accelerator_id,WRONG_ASE_GUID, sizeof(fpga_guid));
   _tok->magic = FPGA_TOKEN_MAGIC;
}

bool token_is_fme0(fpga_token t)
{
   struct _fpga_token *_t = (struct _fpga_token *)t;
   if(_t->magic != ASE_TOKEN_MAGIC)
	   return 0;
   else
   return ((0 == memcmp(_t->accelerator_id,FPGA_FME_ID, sizeof(fpga_guid))));
}

bool token_is_afu0(fpga_token t)
{
   struct _fpga_token *_t = (struct _fpga_token *)t;
   if(_t->magic != ASE_TOKEN_MAGIC)
	return 0;
   else
        return ((0 == memcmp(_t->accelerator_id,ASE_GUID, sizeof(fpga_guid))));
}

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
 * @test open_08
 * When the flags parameter to fpgaOpen is invalid,
 * the function returns FPGA_INVALID_PARAM.
 */
TEST(Open, open_08)
{
	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;

	token_for_afu0(&_tok);
	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaOpen(tok, &h, 42));
}

