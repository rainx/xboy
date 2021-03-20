// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "cartridge-header.h"
namespace kaitai-struct-gen {

    cartridge_header_t::cartridge_header_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent, cartridge_header_t* p__root) : kaitai::kstruct(p__io) {
        m__parent = p__parent;
        m__root = this;
        _read();
    }

    void cartridge_header_t::_read() {
        m_unknown_space1 = m__io->read_bytes(256);
        m_entry_point = m__io->read_bytes(4);
        m_nintendo_logo = m__io->read_bytes(48);
        m_title = kaitai::kstream::bytes_to_str(m__io->read_bytes(16), std::string("UTF-8"));
        m_new_licensee_code = m__io->read_u2le();
        m_sgb_flag = m__io->read_u1();
        m_cartridge_type = m__io->read_u1();
        m_rom_size = m__io->read_u1();
        m_ram_size = m__io->read_u1();
        m_destination_code = m__io->read_u1();
        m_old_licensee_code = m__io->read_u1();
        m_mask_rom_version_number = m__io->read_u1();
        m_header_checksum = m__io->read_bytes(1);
        m_global_checksum = m__io->read_u2le();
    }

    cartridge_header_t::~cartridge_header_t() {
        _clean_up();
    }

    void cartridge_header_t::_clean_up() {
    }
}
