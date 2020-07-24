#pragma once
#include <chrono>
#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/shared_buffer.h>
#include <deque>
#include "dummy_afu.h"
#include "afu.h"

template<class T>
void print_stat(const char *name, T *stat)
{
  std::cout << "[" << name << "]: ";
  std::cout << "PASS=" << bool(stat->TrafficGenTestPass) << ", ";
  std::cout << "FAIL=" << bool(stat->TrafficGenTestFail) << ", ";
  std::cout << "TIMEOUT=" << bool(stat->TrafficGenTestTimeout) << "\n";
}


int run_dma(opae::fpga::types::handle::ptr_t h)
{
  using namespace std::chrono;
  dummy_afu::dummy_afu afu(h);
  afu.write64(dummy_afu::DDR_TEST_CTRL, 0x00000000);
  afu.write64(dummy_afu::DDR_TEST_CTRL, 0x0000000f);
  auto bank0 = afu.register_ptr<dummy_afu::ddr_test_bank0_stat>();
  auto bank1 = afu.register_ptr<dummy_afu::ddr_test_bank1_stat>();
  auto bank2 = afu.register_ptr<dummy_afu::ddr_test_bank2_stat>();
  auto bank3 = afu.register_ptr<dummy_afu::ddr_test_bank3_stat>();
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
  print_stat("Bank0", bank0);
  print_stat("Bank1", bank1);
  print_stat("Bank2", bank2);
  print_stat("Bank3", bank3);
  return 0;
}