// Copyright(c) 2017, Intel Corporation
//
// Redistribution  and  use  in source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of  source code  must retain the  above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name  of Intel Corporation  nor the names of its contributors
//   may be used to  endorse or promote  products derived  from this  software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
// IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
// LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
// CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
// SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
// INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
// CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "fpga.h"
#include "property_map.h"
#include "utils.h"
#include <opae/fpga.h>
#include <fstream>

using namespace intel::utils;
using namespace std;

namespace intel
{
namespace fpga
{

fpga::fpga(shared_token token, fpga_properties props, const std::string & sysfspath)
: fpga_resource(token, props, nullptr)
, status_(fpga::unknown)
, sysfspath_(sysfspath)
{
}

fpga_resource::type_t fpga::type()
{
    return fpga_resource::fpga;
}


vector<fpga::ptr_t> fpga::enumerate(vector<option_map::ptr_t> options)
{
    vector<fpga::ptr_t> fpga_list;
    vector<shared_token> tokens;
    if (enumerate_tokens(FPGA_DEVICE, options, tokens))
    {
       for (const shared_token & tok : tokens)
       {
           fpga_properties props;

           if (FPGA_OK == fpgaGetProperties(*tok, &props))
           {
               fpga_list.push_back(fpga::ptr_t(new fpga(tok, props, sysfs_path_from_token(*tok))));
           }
       }
    }

    return fpga_list;
}

bool fpga::reconfigure(const std::string & bitstream_path, uint32_t slot)
{
    if (!intel::utils::path_exists(bitstream_path))
    {
        log_.error("reconfigure") << "Could not find bitstream path: " << bitstream_path << std::endl;
        return false;
    }

    fstream bitstream(bitstream_path, ios::binary | ios::in);
    bitstream.seekg(ios::end);
    size_t buffer_size = bitstream.tellg();
    bitstream.seekg(ios::beg);
    std::vector<uint8_t> buffer(buffer_size);
    bitstream.read(reinterpret_cast<char*>(buffer.data()), buffer_size);
    // TODO: check metadata

    if (fpgaReconfigureSlot(handle_, slot, buffer.data(), buffer_size, 0) == FPGA_OK)
    {
        return true;
    }

    return false;
}

} // end of namespace fpga
} // end of namespace intel

