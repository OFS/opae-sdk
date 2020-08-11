// Copyright(c) 2020, Intel Corporation
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
#include <chrono>
#include "test_afu.h"
#include "dummy_afu.h"

using namespace opae::app;

template<typename T>
inline void timeit_wr(test_afu *afu, uint32_t count)
{
  using namespace std::chrono;
  auto begin = high_resolution_clock::now();
  for (uint32_t i = 0; i < count; ++i) {
    afu->write<T>(dummy_afu::SCRATCHPAD, i);
  }
  auto end = high_resolution_clock::now();
  auto delta = duration_cast<nanoseconds>(end - begin).count();
  auto width = sizeof(T)*8;
  std::cout << "count: " << count << ", "
            << "op: wr, "
            << "width: " << width << ", "
            << "mean: " << delta/count << " nsec\n";
}

template<typename T>
inline void timeit_rd(test_afu *afu, uint32_t count)
{
  using namespace std::chrono;
  auto begin = high_resolution_clock::now();
  for (uint32_t i = 0; i < count; ++i) {
    afu->read<T>(dummy_afu::SCRATCHPAD);
  }
  auto end = high_resolution_clock::now();
  auto delta = duration_cast<nanoseconds>(end - begin).count();
  auto width = sizeof(T)*8;
  std::cout << "count: " << count << ", "
            << "op: rd, "
            << "width: " << width << ", "
            << "mean: " << delta/count << " nsec\n";
}


template<typename T>
inline void write_verify(test_afu *afu, uint32_t addr, T value)
{
    afu->write<T>(addr, 0);
    if (afu->read<T>(addr) != 0)
      throw std::logic_error("mmio write failed");
    afu->write<T>(addr, value);
    auto rd_value = afu->read<T>(addr);
    if (rd_value != value)
      throw std::logic_error("mmio write failed");
    afu->write<T>(addr, 0);
    if (afu->read<T>(addr) != 0)
      throw std::logic_error("mmio write failed");
}

template<typename T>
inline void write_verify(test_afu *afu, uint32_t addr, uint32_t i, uint64_t value)
{
    afu->write<T>(addr, i, 0);
    if (afu->read<T>(addr, i) != 0)
      throw std::logic_error("mmio write failed");
    afu->write<T>(addr, i, value);
    auto rd_value = afu->read<T>(addr, i);
    if (rd_value != value)
      throw std::logic_error("mmio write failed");
    afu->write<T>(addr, i, 0);
    if (afu->read<T>(addr, i) != 0)
      throw std::logic_error("mmio write failed");
}

class mmio_test : public test_command
{
public:
  mmio_test()
  : count_(1)
  , sp_index_(0)
  , perf_(false)
  , width_(64)
  , op_("rd")
  {

  }
  virtual ~mmio_test(){}
  virtual const char *name() const
  {
    return "mmio";
  }

  virtual const char *description() const
  {
    return "run mmio test";
  }

  virtual void add_options(CLI::App *app)
  {
    app->add_option("-c,--count",
                    count_,
                    "number of repititions")->default_val(1);
    app->add_option("-s,--scratchpad-index",
                    sp_index_,
                    "index in the scratchpad array")->check(CLI::Range(0, 63));
    app->add_flag("--perf", perf_, "get mmio performace numbers");
    auto opt = app->add_option("-w,--width", width_, "mmio width for mmio performance stats");
    opt->check(CLI::IsMember({8, 16, 32, 64}))->default_val(64);
    opt = app->add_option("--op", op_, "operation for mmio performance stats");
    opt->check(CLI::IsMember({"rd", "wr"}))->default_val("rd");
  }

  virtual int run(test_afu *afu, CLI::App *app)
  {
    if (perf_)
      return run_perf(afu, app);
    auto sp_index = app->get_option("--scratchpad-index");
    uint32_t start = *sp_index ? sp_index_ : 0;
    uint32_t end = *sp_index ? start + 1 : 64;
    uint32_t dummy_value32 = 0xc0c0cafe;
    uint64_t dummy_value64 = 0xc0c0cafeUL << 32;
    for (uint32_t i = 0; i < count_; ++i) {
      write_verify<uint32_t>(afu, dummy_afu::SCRATCHPAD, dummy_value32);
      write_verify<uint64_t>(afu, dummy_afu::SCRATCHPAD, dummy_value64);

      for (uint32_t scratch_i = start; scratch_i < end; ++scratch_i) {
        write_verify<uint32_t>(afu, dummy_afu::MMIO_TEST_SCRATCHPAD, scratch_i, dummy_value32 | scratch_i);
        write_verify<uint64_t>(afu, dummy_afu::MMIO_TEST_SCRATCHPAD, scratch_i, dummy_value64 | scratch_i);
      }

    }
    return 0;
  }


  int run_perf(test_afu *afu, CLI::App *app)
  {
    (void)app;
    typedef void(*perf_test)(test_afu*, uint32_t);
    std::map<uint32_t, perf_test> rd_tests
    {
      {8, timeit_rd<uint8_t>},
      {16, timeit_rd<uint16_t>},
      {32, timeit_rd<uint32_t>},
      {64, timeit_rd<uint64_t>},
    };
    std::map<uint32_t, perf_test> wr_tests
    {
      {8, timeit_wr<uint8_t>},
      {16, timeit_wr<uint16_t>},
      {32, timeit_wr<uint32_t>},
      {64, timeit_wr<uint64_t>},
    };
    if (op_ == "wr")
      wr_tests[width_](afu, count_);
    else
      rd_tests[width_](afu, count_);
    return 0;
  }
private:
  uint32_t count_;
  uint32_t sp_index_;
  bool perf_;
  uint32_t width_;
  std::string op_;
};

