#include "state/save_state.hpp"
#include "state/serializable.hpp"

#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

namespace state {

SaveStateManager::SaveStateManager(cpu::Cpu &cpu,
                                   std::shared_ptr<mmu::Mmunit> mmu,
                                   ppu::Ppu &ppu, timer::Timer &tmr,
                                   apu::Apu &apu,
                                   std::shared_ptr<input::Joypad> joypad,
                                   const std::string &rom_path)
    : cpu_(cpu), mmu_(mmu), ppu_(ppu), tmr_(tmr), apu_(apu), joypad_(joypad),
      rom_path_(rom_path) {}

std::string SaveStateManager::getStatePath(int slot) const {
  return rom_path_ + ".state" + std::to_string(slot);
}

bool SaveStateManager::save(int slot) {
  std::vector<uint8_t> buf;
  buf.reserve(32 * 1024);

  // Header
  write_bytes(buf, MAGIC, 4);
  write_u16(buf, VERSION);
  write_u8(buf, static_cast<uint8_t>(mmu_->getCartridge()->getCartridgeType()));

  // Subsystem state
  cpu_.serialize(buf);
  mmu_->serialize(buf);
  mmu_->getCartridge()->serialize(buf);
  ppu_.serialize(buf);
  tmr_.serialize(buf);
  apu_.serialize(buf);
  joypad_->serialize(buf);

  // Write to file
  std::string path = getStatePath(slot);
  std::ofstream ofs(path, std::ios::binary | std::ios::out);
  if (!ofs.good()) {
    std::cerr << "Failed to open save state file: " << path << std::endl;
    return false;
  }

  ofs.write(reinterpret_cast<const char *>(buf.data()),
            static_cast<std::streamsize>(buf.size()));
  std::cout << "State saved to " << path << " (" << buf.size() << " bytes)"
            << std::endl;
  return true;
}

bool SaveStateManager::load(int slot) {
  std::string path = getStatePath(slot);
  std::ifstream ifs(path, std::ios::binary | std::ios::ate);
  if (!ifs.good()) {
    std::cerr << "Save state file not found: " << path << std::endl;
    return false;
  }

  auto file_size = ifs.tellg();
  ifs.seekg(0);

  std::vector<uint8_t> buf(static_cast<size_t>(file_size));
  ifs.read(reinterpret_cast<char *>(buf.data()), file_size);

  const uint8_t *data = buf.data();
  size_t pos = 0;

  // Validate header
  if (buf.size() < 7) {
    std::cerr << "Save state file too small" << std::endl;
    return false;
  }

  if (std::memcmp(data, MAGIC, 4) != 0) {
    std::cerr << "Invalid save state magic" << std::endl;
    return false;
  }
  pos += 4;

  uint16_t version = read_u16(data, pos);
  if (version != VERSION) {
    std::cerr << "Unsupported save state version: " << version << std::endl;
    return false;
  }

  uint8_t cart_type = read_u8(data, pos);
  if (cart_type !=
      static_cast<uint8_t>(mmu_->getCartridge()->getCartridgeType())) {
    std::cerr << "Cartridge type mismatch in save state" << std::endl;
    return false;
  }

  // Restore subsystem state
  cpu_.deserialize(data, pos);
  mmu_->deserialize(data, pos);
  mmu_->getCartridge()->deserialize(data, pos);
  ppu_.deserialize(data, pos);
  tmr_.deserialize(data, pos);
  apu_.deserialize(data, pos);
  joypad_->deserialize(data, pos);

  std::cout << "State loaded from " << path << std::endl;
  return true;
}

} // namespace state
