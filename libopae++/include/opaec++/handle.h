/*
 * handle.h
 * Copyright (C) 2017 rrojo <rrojo@fsw-ub-02>
 *
 * Distributed under terms of the MIT license.
 */
#pragma once
#include <opae/types.h>
#include <opae/enum.h>

#include <vector>
#include <memory>
#include "token.h"

namespace opae
{
namespace fpga
{
namespace types
{


class handle
{
public:
    typedef std::shared_ptr<handle> ptr_t;

    ~handle();

    fpga_handle get(){
        return handle_;
    }


    static handle::ptr_t open(fpga_token token, int flags);

    static handle::ptr_t open(token::ptr_t token, int flags);

    fpga_result close();
private:
    handle(fpga_handle h);

    fpga_handle handle_;
};

} // end of namespace types
} // end of namespace fpga
} // end of namespace opae
