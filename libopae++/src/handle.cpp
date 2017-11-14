/*
 * handle.h
 * Copyright (C) 2017 rrojo <rrojo@fsw-ub-02>
 *
 * Distributed under terms of the MIT license.
 */
#include "opaec++/handle.h"


namespace opae
{
namespace fpga
{
namespace types
{

handle::handle(fpga_handle h)
: handle_(h)
{
}

handle::~handle(){
    close();
}


handle::ptr_t handle::open(fpga_token token, int flags){
    fpga_handle c_handle;
    auto res = fpgaOpen(token, &c_handle, flags);
    if (res == FPGA_OK){
        return handle::ptr_t(new handle(c_handle));
    }
    // TODO : Log or throw error
    return handle::ptr_t();
}

handle::ptr_t handle::open(token::ptr_t token, int flags){
    return handle::open(token->get(), flags);
}

fpga_result handle::close(){
    if (handle_ != nullptr){
        auto res = fpgaClose(handle_);
        if (res == FPGA_OK){
            handle_ = nullptr;
        }else{
            // TODO : Log or throw error
        }
        return res;
    }

}

} // end of namespace types
} // end of namespace fpga
} // end of namespace opae
