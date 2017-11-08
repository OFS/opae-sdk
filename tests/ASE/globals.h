#ifndef __FPGA_GLOBALS_H__
#define __FPGA_GLOBALS_H__

#include <opae/types.h>
#include "types_int.h"

class GlobalOptions
{
public:
   static GlobalOptions & Instance();

   void     NumSockets(unsigned s) { m_NumSockets = s;    }
   unsigned NumSockets() const     { return m_NumSockets; }

protected:
   static GlobalOptions sm_Instance;

   GlobalOptions() :
      m_NumSockets(1)
   {}

   unsigned m_NumSockets;
};
#define ASE_TOKEN_MAGIC    0x46504741544f4b40
static const fpga_guid FPGA_FME_ID = {
			0xbf, 0xaf, 0x2a, 0xe9, 0x4a, 0x52, 0x46, 0xe3, 0x82, 0xfe,
						0x38, 0xf0, 0xf9, 0xe1, 0x77, 0x64
};
static const fpga_guid ASE_GUID = {
			0xd8, 0x42, 0x4d, 0xc4, 0xa4,  0xa3, 0xc4, 0x13, 0xf8,0x9e, 
						0x43, 0x36, 0x83, 0xf9, 0x04, 0x0b
};

static const fpga_guid 	WRONG_ASE_GUID = {
			0xd8, 0xd8, 0xd8, 0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
void token_init(struct _fpga_token *_tok);
void token_for_fme0(struct _fpga_token *_tok);
void token_for_afu0(struct _fpga_token *_tok);
void token_for_invalid(struct _fpga_token *_tok);
bool token_is_fme0(fpga_token t);
bool token_is_afu0(fpga_token t);

#endif
