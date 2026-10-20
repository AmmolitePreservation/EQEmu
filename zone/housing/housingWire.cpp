#include "housingWire.h"

#include "../client.h"

#include <algorithm>
#include <cstring>

namespace housing {
  template <typename T> WireWriter &WireWriter::put(T value) {
    buffer.append(reinterpret_cast<const char *>(&value), sizeof(T));
    return *this;
  }

  WireWriter &WireWriter::u8(uint8 value) { return put(value); }
  WireWriter &WireWriter::u16(uint16 value) { return put(value); }
  WireWriter &WireWriter::u32(uint32 value) { return put(value); }
  WireWriter &WireWriter::i32(int32 value) { return put(value); }
  WireWriter &WireWriter::u64(uint64 value) { return put(value); }
  WireWriter &WireWriter::i64(int64 value) { return put(value); }
  WireWriter &WireWriter::f32(float value) { return put(value); }
  WireWriter &WireWriter::f64(double value) { return put(value); }

  WireWriter &WireWriter::lengthString(const std::string &value) {
    u32(static_cast<uint32>(value.size()));
    buffer.append(value);
    return *this;
  }

  WireWriter &WireWriter::cString(const std::string &value) {
    buffer.append(value.c_str(), strlen(value.c_str()) + 1);
    return *this;
  }

  WireWriter &WireWriter::fixedString(const std::string &value, size_t length) {
    std::string field(length, '\0');
    memcpy(field.data(), value.data(), std::min(value.size(), length - 1));
    buffer.append(field);
    return *this;
  }

  WireWriter &WireWriter::bytes(const void *data, size_t length) {
    buffer.append(static_cast<const char *>(data), length);
    return *this;
  }

  template <typename T> T WireReader::take() {
    T value{};
    if (overrun || remaining() < sizeof(T)) {
      overrun = true;
      return value;
    }
    memcpy(&value, data + position, sizeof(T));
    position += sizeof(T);
    return value;
  }

  uint8 WireReader::u8() { return take<uint8>(); }
  uint16 WireReader::u16() { return take<uint16>(); }
  uint32 WireReader::u32() { return take<uint32>(); }
  int32 WireReader::i32() { return take<int32>(); }
  uint64 WireReader::u64() { return take<uint64>(); }
  int64 WireReader::i64() { return take<int64>(); }
  float WireReader::f32() { return take<float>(); }
  double WireReader::f64() { return take<double>(); }

  std::string WireReader::lengthString() {
    const int32 size = i32();
    if (overrun || size <= 0) {
      return {};
    }
    if (remaining() < static_cast<size_t>(size)) {
      overrun = true;
      return {};
    }
    std::string value(reinterpret_cast<const char *>(data + position), size);
    position += size;
    return value;
  }

  std::string WireReader::cString() {
    if (overrun) {
      return {};
    }
    const void *end = memchr(data + position, 0, remaining());
    if (!end) {
      overrun = true;
      return {};
    }
    const size_t size = static_cast<const uint8 *>(end) - (data + position);
    std::string value(reinterpret_cast<const char *>(data + position), size);
    position += size + 1;
    return value;
  }

  std::string WireReader::fixedString(size_t length) {
    if (overrun || remaining() < length) {
      overrun = true;
      return {};
    }
    const char *start = reinterpret_cast<const char *>(data + position);
    position += length;
    return std::string(start, strnlen(start, length));
  }

  void WireReader::skip(size_t length) {
    if (overrun || remaining() < length) {
      overrun = true;
      return;
    }
    position += length;
  }

  void sendMessage(Client *client, EmuOpcode opcode, const WireWriter &message) {
    const std::string &content = message.content();
    EQApplicationPacket packet(opcode, static_cast<uint32>(content.size()));
    if (!content.empty()) {
      memcpy(packet.pBuffer, content.data(), content.size());
    }
    client->QueuePacket(&packet);
  }
}
