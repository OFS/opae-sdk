#pragma once
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

int run_mmio(test_afu *afu, CLI::App *app)
{
  auto count = app->get_option("--count")->as<uint32_t>();
  auto sp_index = app->get_option("--scratchpad-index");
  uint32_t start = *sp_index ? sp_index->as<uint32_t>() : 0;
  uint32_t end = *sp_index ? start + 1 : 64;
  uint32_t dummy_value32 = 0xc0c0cafe;
  uint64_t dummy_value64 = 0xc0c0cafeUL << 32;
  for (uint32_t i = 0; i < count; ++i) {
    write_verify<uint32_t>(afu, dummy_afu::SCRATCHPAD, dummy_value32);
    write_verify<uint64_t>(afu, dummy_afu::SCRATCHPAD, dummy_value64);

    for (uint32_t i = start; i < end; ++i) {
      write_verify<uint32_t>(afu, dummy_afu::MMIO_TEST_SCRATCHPAD, i, dummy_value32 | i);
      write_verify<uint64_t>(afu, dummy_afu::MMIO_TEST_SCRATCHPAD, i, dummy_value64 | i);
    }

  }
  return 0;
}