#pragma once
#include <chrono>
#include <deque>
#include "test_afu.h"
#include "dummy_afu.h"

using namespace opae::app;

template<class T>
int print_stat(const char *name, T stat)
{
  std::cout << "[" << name << "]: ";
  std::cout << "PASS=" << bool(stat->TrafficGenTestPass) << ", ";
  std::cout << "FAIL=" << bool(stat->TrafficGenTestFail) << ", ";
  std::cout << "TIMEOUT=" << bool(stat->TrafficGenTestTimeout) << "\n";
  return stat->value & 0b110;
}

class dma_test : public test_command
{
public:
  dma_test(){}
  virtual ~dma_test(){}
  virtual const char *name() const
  {
    return "ddr";
  }

  virtual const char *description() const
  {
    return "run ddr test";
  }

  virtual int run(test_afu *afu, CLI::App *app)
  {
    (void)app;
    using namespace std::chrono;
    auto bank0 = afu->register_ptr<uint64_t>(dummy_afu::ddr_test_bank0_stat::offset);
    auto bank1 = afu->register_ptr<uint64_t>(dummy_afu::ddr_test_bank1_stat::offset);
    auto bank2 = afu->register_ptr<uint64_t>(dummy_afu::ddr_test_bank2_stat::offset);
    auto bank3 = afu->register_ptr<uint64_t>(dummy_afu::ddr_test_bank3_stat::offset);
    std::deque<volatile uint64_t*> ptrs;
    ptrs.push_back(bank0);
    ptrs.push_back(bank1);
    ptrs.push_back(bank2);
    ptrs.push_back(bank3);
    for (auto p : ptrs)
      if (*p & 0b111)
        throw std::runtime_error("banks not zero");

    // start the test
    afu->write64(dummy_afu::DDR_TEST_CTRL, 0x00000000);
    afu->write64(dummy_afu::DDR_TEST_CTRL, 0x0000000f);

    auto begin = high_resolution_clock::now();
    while (!ptrs.empty()) {
      if (*ptrs.front() & 0b111) {
        ptrs.pop_front();
      }
      auto now = high_resolution_clock::now();
      if (duration_cast<duration<double>>(now - begin).count() > 1.0) {
        throw std::runtime_error("timeout error");
      }
    }
    int res = 0;
    res += print_stat("Bank0",
      reinterpret_cast<volatile dummy_afu::ddr_test_bank0_stat*>(bank0));
    res += print_stat("Bank1",
      reinterpret_cast<volatile dummy_afu::ddr_test_bank1_stat*>(bank1));
    res += print_stat("Bank2",
      reinterpret_cast<volatile dummy_afu::ddr_test_bank2_stat*>(bank2));
    res += print_stat("Bank3",
      reinterpret_cast<volatile dummy_afu::ddr_test_bank3_stat*>(bank3));
    return res;
  }

private:

};