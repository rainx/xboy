#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

namespace state {

// Binary serialization helpers for save states.
// Data is written/read in little-endian order.

inline void write_u8(std::vector<uint8_t> &buf, uint8_t v) {
  buf.push_back(v);
}

inline void write_u16(std::vector<uint8_t> &buf, uint16_t v) {
  buf.push_back(static_cast<uint8_t>(v & 0xFF));
  buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
}

inline void write_u32(std::vector<uint8_t> &buf, uint32_t v) {
  for (int i = 0; i < 4; i++) {
    buf.push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xFF));
  }
}

inline void write_u64(std::vector<uint8_t> &buf, uint64_t v) {
  for (int i = 0; i < 8; i++) {
    buf.push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xFF));
  }
}

inline void write_i64(std::vector<uint8_t> &buf, int64_t v) {
  uint64_t u;
  std::memcpy(&u, &v, sizeof(u));
  write_u64(buf, u);
}

inline void write_bool(std::vector<uint8_t> &buf, bool v) {
  buf.push_back(v ? 1 : 0);
}

inline void write_bytes(std::vector<uint8_t> &buf, const uint8_t *data,
                         size_t len) {
  buf.insert(buf.end(), data, data + len);
}

// Read helpers — advance pos past the read data

inline uint8_t read_u8(const uint8_t *data, size_t &pos) {
  return data[pos++];
}

inline uint16_t read_u16(const uint8_t *data, size_t &pos) {
  uint16_t v = static_cast<uint16_t>(data[pos]) |
               (static_cast<uint16_t>(data[pos + 1]) << 8);
  pos += 2;
  return v;
}

inline uint32_t read_u32(const uint8_t *data, size_t &pos) {
  uint32_t v = 0;
  for (int i = 0; i < 4; i++) {
    v |= static_cast<uint32_t>(data[pos + i]) << (i * 8);
  }
  pos += 4;
  return v;
}

inline uint64_t read_u64(const uint8_t *data, size_t &pos) {
  uint64_t v = 0;
  for (int i = 0; i < 8; i++) {
    v |= static_cast<uint64_t>(data[pos + i]) << (i * 8);
  }
  pos += 8;
  return v;
}

inline int64_t read_i64(const uint8_t *data, size_t &pos) {
  uint64_t u = read_u64(data, pos);
  int64_t v;
  std::memcpy(&v, &u, sizeof(v));
  return v;
}

inline bool read_bool(const uint8_t *data, size_t &pos) {
  return data[pos++] != 0;
}

inline void read_bytes(const uint8_t *data, size_t &pos, uint8_t *out,
                        size_t len) {
  std::memcpy(out, data + pos, len);
  pos += len;
}

} // namespace state
