// Copyright(c) 2017-2018, Intel Corporation
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

#pragma once
#include <memory>
#include <algorithm>
#include "mmio.h"

namespace intel
{
namespace fpga
{

class dfh
{
public:
    typedef std::shared_ptr<dfh> ptr_t;

    enum dfh_type
    {
        priv = 0x3
    };

    dfh()
    : offset_(0)
    , u_(0)
    {
    }

    dfh(uint32_t addr, uint64_t v)
    : offset_(addr)
    , u_(v)
    {
    }

    void reset()
    {
        offset_ = 0;
        u_.value = 0;
    }

    void update(uint32_t offset, uint64_t value)
    {
        offset_ = offset;
        u_.value = value;
    }

    dfh(const dfh & other)
    : offset_(other.offset_)
    , u_(other.u_)
    {
    }

    dfh & operator=(const dfh & other)
    {
        offset_ = other.offset_;
        u_ = other.u_;
        return *this;
    }

    bool operator==(const dfh & other) const
    {
        return u_.value == other.u_.value;
    }

    bool operator!=(const dfh & other) const
    {
        return u_.value != other.u_.value;
    }

    uint32_t offset() const
    {
        return offset_;
    }

    uint64_t value() const
    {
        return u_.value;
    }

    uint32_t id() const
    {
        return u_.s.id;
    }

    uint32_t rev() const
    {
        return u_.s.rev;
    }

    uint32_t next() const
    {
        return offset_ + u_.s.next;
    }

    uint32_t end() const
    {
        return u_.s.end;
    }

    uint32_t type() const
    {
        return u_.s.type;
    }

private:
    union dfh_u
    {
        dfh_u(): value(0){}
        dfh_u(uint64_t v): value(v){}
        uint64_t value;
        struct
        {
            uint64_t id        : 12,
                     rev       : 4,
                     next      : 24,
                     end       : 1,
                     reserved2 : 7,
                     rev_minor : 4,
                     reserved1 : 8,
                     type      : 4;
        } s;
    };
    uint32_t offset_;
    dfh_u    u_;
};

class dfh_list
{
public:
    class iterator : public std::iterator<std::forward_iterator_tag,
                                          dfh,
                                          uint32_t,
                                          const dfh*,
                                          dfh>
    {
    public:
        iterator()
        : mmio_(0)
        , value_(new dfh())
        {
        }

        iterator(const iterator & other)
        : mmio_(other.mmio_)
        , value_(0)
        {
            if (other.value_ != nullptr)
            {
                value_ = new dfh(*other.value_);
            }
        }

        iterator & operator=(const iterator & other)
        {
            if (this != &other)
            {
                mmio_ = other.mmio_;
                if (value_ != nullptr)
                {
                    delete value_;
                }

                if (other.value_ != nullptr)
                {
                    value_ = new dfh(*other.value_);
                }
                else
                {
                    value_ = nullptr;
                }
            }
            return *this;
        }


        explicit iterator(mmio::ptr_t mmio)
        : mmio_(mmio)
        , value_(0)
        {
            uint64_t value = 0;
            if (mmio_->read_mmio64(0, value))
            {
                value_ = new dfh(0, value);
            }
        }


        ~iterator()
        {
            if (value_ != nullptr)
            {
                delete value_;
            }
            value_ = 0;
        }

        iterator & operator++()
        {
            if (value_->next() == value_->offset() || value_->end())
            {
                value_->reset();
            }
            else
            {
                uint64_t value = 0;
                mmio_->read_mmio64(value_->next(), value);
                value_->update(value_->next(), value);
            }
            return *this;
        }

        operator bool() const
        {
            return value_ != nullptr;
        }

        bool operator==(iterator other) const
        {
            return *value_ == *other.value_;
        }

        bool operator!=(iterator other) const
        {
            return *value_ != *other.value_;
        }

        reference operator*() const
        {
            return *value_;
        }

        pointer operator->() const
        {
            return value_;
        }


    private:
        mmio::ptr_t  mmio_;
        dfh*         value_;
    };

    typedef std::shared_ptr<dfh_list> ptr_t;

    dfh_list(mmio::ptr_t mmio)
    : mmio_(mmio)
    {
    }

    iterator begin()
    {
        return iterator(mmio_);
    }

    iterator end()
    {
        return iterator();
    }

    iterator find(uint64_t id, uint64_t rev, uint64_t type)
    {
        auto it = begin();
        while (it && it != end())
        {
            if (it->id() == id &&
                it->rev() == rev &&
                it->type() == type)
            {
                return it;
            }
            ++it;
        }
        return iterator();
    }



private:
    mmio::ptr_t mmio_;

};

} // end of namespace fpga
} // end of namespace intel
