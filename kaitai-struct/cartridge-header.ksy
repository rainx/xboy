# Ref doc: https://gbdev.gg8.se/wiki/articles/The_Cartridge_Header
meta:
    id: cartridge_header
    endian: le
seq:
    - id: unknown_space1
      size: 0x100
    - id: entry_point
      size: 0x04
    - id: nintendo_logo
      size: 0x30
    - id: title
      type: str
      encoding: UTF-8
      size: 0x10
    - id: new_licensee_code
      type: u2
    - id: sgb_flag
      type: u1
    - id: cartridge_type
      type: u1
    - id: rom_size
      type: u1
    - id: ram_size
      type: u1
    - id: destination_code
      type: u1
    - id: old_licensee_code
      type: u1
    - id: mask_rom_version_number
      type: u1
    - id: header_checksum
      size: 0x01
    - id: global_checksum
      type: u2
