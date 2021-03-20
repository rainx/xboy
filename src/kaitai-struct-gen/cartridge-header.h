#pragma once

// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "kaitai/kaitaistruct.h"
#include <stdint.h>
#include <memory>

#if KAITAI_STRUCT_VERSION < 9000L
#error "Incompatible Kaitai Struct C++/STL API: version 0.9 or later is required"
#endif
namespace gen {

    class cartridge_header_t : public kaitai::kstruct {

    public:

        cartridge_header_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent = nullptr, cartridge_header_t* p__root = nullptr);

    private:
        void _read();
        void _clean_up();

    public:
        ~cartridge_header_t();

    private:
        std::string m_unknown_space1;
        std::string m_entry_point;
        std::string m_nintendo_logo;
        std::string m_title;
        uint16_t m_new_licensee_code;
        uint8_t m_sgb_flag;
        uint8_t m_cartridge_type;
        uint8_t m_rom_size;
        uint8_t m_ram_size;
        uint8_t m_destination_code;
        uint8_t m_old_licensee_code;
        uint8_t m_mask_rom_version_number;
        std::string m_header_checksum;
        uint16_t m_global_checksum;
        cartridge_header_t* m__root;
        kaitai::kstruct* m__parent;

    public:
        std::string unknown_space1() const { return m_unknown_space1; }
        std::string entry_point() const { return m_entry_point; }
        std::string nintendo_logo() const { return m_nintendo_logo; }
        std::string title() const { return m_title; }
        uint16_t new_licensee_code() const { return m_new_licensee_code; }
        uint8_t sgb_flag() const { return m_sgb_flag; }
        uint8_t cartridge_type() const { return m_cartridge_type; }
        uint8_t rom_size() const { return m_rom_size; }
        uint8_t ram_size() const { return m_ram_size; }
        uint8_t destination_code() const { return m_destination_code; }
        uint8_t old_licensee_code() const { return m_old_licensee_code; }
        uint8_t mask_rom_version_number() const { return m_mask_rom_version_number; }
        std::string header_checksum() const { return m_header_checksum; }
        uint16_t global_checksum() const { return m_global_checksum; }
        cartridge_header_t* _root() const { return m__root; }
        kaitai::kstruct* _parent() const { return m__parent; }
    };
}
