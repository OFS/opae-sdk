#pragma once
#include <chrono>
#include "test_afu.h"
#include "dummy_afu.h"

using namespace opae::app;

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
    app->add_flag("--perf", perf_, "get mmio performance stats");
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

      for (uint32_t i = start; i < end; ++i) {
        write_verify<uint32_t>(afu, dummy_afu::MMIO_TEST_SCRATCHPAD, i, dummy_value32 | i);
        write_verify<uint64_t>(afu, dummy_afu::MMIO_TEST_SCRATCHPAD, i, dummy_value64 | i);
      }

    }
    return 0;
  }

  template<typename T>
  inline void timeit_rd(test_afu *afu, const std::string &op)
  {
    using namespace std::chrono;
    auto begin = high_resolution_clock::now();
    for (uint32_t i = 0; i < count_; ++i) {
      afu->read<T>(dummy_afu::SCRATCHPAD);
    }
    auto end = high_resolution_clock::now();
    auto delta = duration_cast<nanoseconds>(end - begin).count();
    std::cout << "mean " << op << ": " << delta/count_ << " nanoseconds\n";
  }

  template<typename T>
  inline void timeit_wr(test_afu *afu, const std::string &op)
  {
    using namespace std::chrono;
    auto begin = high_resolution_clock::now();
    for (uint32_t i = 0; i < count_; ++i) {
      afu->write<T>(dummy_afu::SCRATCHPAD, i);
    }
    auto end = high_resolution_clock::now();
    auto delta = duration_cast<nanoseconds>(end - begin).count();
    std::cout << "mean " << op << ": " << delta/count_ << " nanoseconds\n";
  }

  int run_perf(test_afu *afu, CLI::App *app)
  {
    (void)app;
    timeit_rd<uint32_t>(afu, "read32");
    timeit_rd<uint64_t>(afu, "read64");
    timeit_wr<uint32_t>(afu, "write32");
    timeit_wr<uint64_t>(afu, "write64");
    return 0;
  }
private:
  uint32_t count_;
  uint32_t sp_index_;
  bool perf_;
};