/*
 * token.h
 * Copyright (C) 2017 rrojo <rrojo@fsw-ub-02>
 *
 * Distributed under terms of the MIT license.
 */
#pragma once
#include <opae/types.h>
#include <opae/access.h>
#include <opae/enum.h>

#include <vector>
#include <memory>
#include "opaec++/properties.h"

namespace opae
{
namespace fpga
{
namespace types
{

class token
{
public:
    typedef std::shared_ptr<token> ptr_t;
    static std::vector<token::ptr_t> enumerate(const std::vector<properties> & props);

    ~token();

    fpga_token get(){
        return token_;
    }

private:
    fpga_token token_;
    token(fpga_token tok);
};

} // end of namespace types
} // end of namespace fpga
} // end of namespace opae

