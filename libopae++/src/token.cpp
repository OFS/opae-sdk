/*
 * token.h
 * Copyright (C) 2017 rrojo <rrojo@fsw-ub-02>
 *
 * Distributed under terms of the MIT license.
 */
#include "opaec++/token.h"
#include <algorithm>

namespace opae
{
namespace fpga
{
namespace types
{


std::vector<token::ptr_t> token::enumerate(const std::vector<properties> & props){
    std::vector<token::ptr_t> tokens;
    std::vector<fpga_properties> c_props(props.size());
    std::transform(props.begin(), props.end(), c_props.begin(),
                   [](const properties & p){
                       return p.get();
                   });
    uint32_t matches = 0;
    auto res = fpgaEnumerate(c_props.data(),
                             c_props.size(),
                             nullptr,
                             0,
                             &matches);
    if (res == FPGA_OK && matches > 0){
        std::vector<fpga_token> c_tokens(matches);
        tokens.resize(matches);
        res = fpgaEnumerate(c_props.data(),
                            c_props.size(),
                            c_tokens.data(),
                            c_tokens.size(),
                            &matches);

        // create a new c++ token object for each c token struct
        std::transform(c_tokens.begin(), c_tokens.end(), tokens.begin(),
                       [](fpga_token t){
                           return token::ptr_t(new token(t));
                       });

        // discard our c struct token objects
        std::for_each(c_tokens.begin(), c_tokens.end(),
                      [](fpga_token t){
                          auto res = fpgaDestroyToken(&t);
                      });
    }
    return tokens;
}

token::~token(){
    auto res = fpgaDestroyToken(&token_);
}

token::token(fpga_token tok){
    auto res = fpgaCloneToken(tok, &token_);
    // TODO: Throw exception on failure
}

} // end of namespace types
} // end of namespace fpga
} // end of namespace opae

