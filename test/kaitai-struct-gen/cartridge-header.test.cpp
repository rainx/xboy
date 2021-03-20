
#include "gtest/gtest.h"

#include "./tetris.dump.hpp"
#include "kaitai-struct-gen/cartridge-header.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
namespace gen {

TEST(gen_test, test_tetris) {

  std::stringstream tetris_rom_data;
  // load rom data
  tetris_rom_data << std::string((char *)(__tetris_gb), __tetris_gb_len);
  kaitai::kstream ks(&tetris_rom_data);

  gen::cartridge_header_t cartrigde_header = gen::cartridge_header_t(&ks);

  ASSERT_EQ(cartrigde_header.cartridge_type(), 0);
  ASSERT_STREQ("TETRIS", std::string(cartrigde_header.title()).c_str());
  ASSERT_EQ(cartrigde_header.rom_size(), 0);
  ASSERT_EQ(cartrigde_header.ram_size(), 0);
  ASSERT_EQ(cartrigde_header.global_checksum(), 0xbf16);
}

} // namespace gen
