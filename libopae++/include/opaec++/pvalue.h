#pragma once
#include <opae/properties.h>
#include <type_traits>
#include <iostream>

namespace opae
{
namespace fpga
{
namespace types
{

template<typename T>
struct pvalue
{
    typedef typename std::conditional<std::is_same<T, char*>::value,
                                      fpga_result(*)(fpga_properties, T),
                                      fpga_result(*)(fpga_properties, T*)>::type getter_t;
    typedef fpga_result (*setter_t)(fpga_properties, T);
    pvalue()
    : props_(0)
    {
    }

    pvalue(fpga_properties *p, getter_t g, setter_t s)
    : props_(p)
    , get_(g)
    , set_(s)
    {
    }

    template<typename U>
    pvalue<T>& operator=(U v){
        T value = static_cast<T>(v);
        auto res = set_(*props_, value);
        if (res != FPGA_OK){
            // TODO: Print error: std::cerr << "Could not set property";
        }
        return *this;
    }

    pvalue<fpga_guid>& operator=(fpga_guid v){
        auto res = set_(*props_, v);
        if (res != FPGA_OK){
            // TODO : log error
        }
        return *this;
    }

    fpga_result get_value(T & value) const {
        return get_(*props_, &value);
    }

    friend std::ostream & operator<<(std::ostream & ostr, const pvalue<T> & p){
        T value;
        if (p.get_value(value) == FPGA_OK){
            ostr << +(value);
        }else{
            ostr << "null";
        }
        return ostr;
    }

private:
    fpga_properties *props_;
    getter_t get_;
    setter_t set_;
};

} // end of namespace types
} // end of namespace fpga
} // end of namespace opae