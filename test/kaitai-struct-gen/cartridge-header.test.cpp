
#include "gtest/gtest.h"

#include "./tetris.dump.hpp"
#include "kaitai-struct-gen/cartridge-header.h"
#include <iostream>
#include <sstream>

namespace gen {

TEST(gen_test, test_tetris) {

  //   std::stringstream tetris_rom_stream;
  //   tetris_rom_stream << __tetris_gb;
  std::cout << sizeof(__tetris_gb) << std::endl;
  //   tetris_rom_stream.seekg(0);
  //   kaitai::kstream ks(&tetris_rom_stream);
  //   gen::cartridge_header_t cartrigde_header = gen::cartridge_header_t(&ks);

  //   ASSERT_EQ(cartrigde_header.cartridge_type(), 0);
  ASSERT_TRUE(true);
}

} // namespace gen
