#ifndef __FPGA_GLOBALS_H__
#define __FPGA_GLOBALS_H__

#include "fpga/types.h"
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


void token_for_fme0(struct _fpga_token *_tok);
void token_for_afu0(struct _fpga_token *_tok);
void token_for_invalid(struct _fpga_token *_tok);
bool token_is_fme0(fpga_token t);
bool token_is_afu0(fpga_token t);

#endif
