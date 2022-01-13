#include <iostream>
#include "fpga_hw.h"

using namespace opae::testing;

int main(int argc, char* argv[])
{
  (void)argc;
  (void)argv;
  auto db = fpga_db::instance();
  for (auto k : db->keys()) {
      std::cout << "platform: " << k << "\n";
  }

}
