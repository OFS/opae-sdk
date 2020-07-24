#pragma once
#include <chrono>
#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/shared_buffer.h>
#include <deque>
#include "dummy_afu.h"
#include "afu.h"

template<class T>
int print_stat(const char *name, T stat)
{
  std::cout << "[" << name << "]: ";
  std::cout << "PASS=" << bool(stat->TrafficGenTestPass) << ", ";
  std::cout << "FAIL=" << bool(stat->TrafficGenTestFail) << ", ";
  std::cout << "TIMEOUT=" << bool(stat->TrafficGenTestTimeout) << "\n";
  return stat->value & 0b110;
}


int run_dma(opae::fpga::types::handle::ptr_t h)
{
  using namespace std::chrono;
  dummy_afu::dummy_afu afu(h);
  afu.write64(dummy_afu::DDR_TEST_CTRL, 0x00000000);
  afu.write64(dummy_afu::DDR_TEST_CTRL, 0x0000000f);
  auto bank0 = afu.register_ptr<uint64_t>(dummy_afu::ddr_test_bank0_stat::offset);
  auto bank1 = afu.register_ptr<uint64_t>(dummy_afu::ddr_test_bank1_stat::offset);
  auto bank2 = afu.register_ptr<uint64_t>(dummy_afu::ddr_test_bank2_stat::offset);
  auto bank3 = afu.register_ptr<uint64_t>(dummy_afu::ddr_test_bank3_stat::offset);
  std::deque<volatile uint64_t*> ptrs;
  ptrs.push_back(reinterpret_cast<volatile uint64_t*>(bank0));
  ptrs.push_back(reinterpret_cast<volatile uint64_t*>(bank1));
  ptrs.push_back(reinterpret_cast<volatile uint64_t*>(bank2));
  ptrs.push_back(reinterpret_cast<volatile uint64_t*>(bank3));
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