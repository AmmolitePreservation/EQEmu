#ifndef EQEMU_ZONE_HOUSING_HOUSING_WIRE_H
#define EQEMU_ZONE_HOUSING_HOUSING_WIRE_H

#include "../../common/types.h"
#include "../../common/emu_opcodes.h"

#include <string>

class Client;

namespace housing {
  class WireWriter {
  public:
    WireWriter &u8(uint8 value);
    WireWriter &u16(uint16 value);
    WireWriter &u32(uint32 value);
    WireWriter &i32(int32 value);
    WireWriter &u64(uint64 value);
    WireWriter &i64(int64 value);
    WireWriter &f32(float value);
    WireWriter &f64(double value);
    WireWriter &lengthString(const std::string &value);
    WireWriter &cString(const std::string &value);
    WireWriter &fixedString(const std::string &value, size_t length);
    WireWriter &bytes(const void *data, size_t length);
    const std::string &content() const { return buffer; }

  private:
    template <typename T> WireWriter &put(T value);
    std::string buffer;
  };

  class WireReader {
  public:
    WireReader(const uint8 *data, size_t length) : data(data), length(length) {}
    uint8 u8();
    uint16 u16();
    uint32 u32();
    int32 i32();
    uint64 u64();
    int64 i64();
    float f32();
    double f64();
    std::string lengthString();
    std::string cString();
    std::string fixedString(size_t length);
    void skip(size_t length);
    bool failed() const { return overrun; }
    size_t remaining() const { return length - position; }

  private:
    template <typename T> T take();
    const uint8 *data;
    size_t length;
    size_t position = 0;
    bool overrun = false;
  };

  void sendMessage(Client *client, EmuOpcode opcode, const WireWriter &message);
}

#endif
