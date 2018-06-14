//
// Device feature lists describe capabilities of an AFU and the MMIO
// space used to control them from the host.  Feature lists are exported
// in the CCI-P interface as MMIO reads from the host.
//

#ifndef _CCIP_FEATURE_LIST_H_
#define _CCIP_FEATURE_LIST_H_

#include <stdint.h>

typedef enum
{
    eFTYP_AFU = 1,
    eFTYP_BBB = 2,
    eFTYP_PVT = 3
}
CCIP_FEATURE_TYPE;


//
// Decode a device feature header (all except AFU headers)
//
class CCIP_FEATURE_DFH
{
  private:
    // Encoded feature header
    bt64bitCSR dfh;

  public:
    CCIP_FEATURE_DFH(bt64bitCSR h)
    {
        dfh = h;
    }

    ~CCIP_FEATURE_DFH() {};
    
    CCIP_FEATURE_TYPE getFeatureType()
    {
        return CCIP_FEATURE_TYPE((dfh >> 60) & 0xf);
    }
    
    uint32_t getVersion()
    {
        return (dfh >> 12) & 0xf;
    }
    
    uint32_t getID()
    {
        return dfh & 0xfff;
    }
    
    uint32_t getNext()
    {
        return (dfh >> 16) & 0xffffff;
    }
    
    bool isEOL()
    {
        return ((dfh >> 40) & 1) == 1;
    }
};

#endif
