#include "mmu/memory.hpp"

using namespace mmu;

uint16_t mmu::Memory::getWord(const uint16_t &address) const {
    return static_cast<uint16_t>(get(address)) | static_cast<uint16_t>(get(address + 1) << 8);
}

void mmu::Memory::setWord(const uint16_t &address, uint16_t value) {
    set(address, static_cast<uint8_t>(value & 0xFF));
    set(address + 1, static_cast<uint8_t>(value >> 8));
}