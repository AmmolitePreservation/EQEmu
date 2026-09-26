#include "housingMessages.h"

#include "housingGeometry.h"

namespace housing {
  WireWriter propertyRecord(const Property &property, const std::vector<uint32> &childIds, const std::string &ownerName, const std::vector<Balance> &propertyBalances) {
    WireWriter message;
    message.u32(property.id)
      .u32(0)
      .u32(property.parent_id)
      .u32(property.definition_id)
      .u32(property.guild_id ? property.guild_id : 0xFFFFFFFF)
      .u32(0)
      .lengthString("")
      .lengthString(ownerName)
      .u32(0)
      .u32(static_cast<uint32>(childIds.size()))
      .lengthString(property.name);
    for (uint32 child : childIds) {
      message.u32(child);
    }
    message.u8(static_cast<uint8>(propertyBalances.size()));
    for (const auto &balance : propertyBalances) {
      message.u32(balance.currency).f64(balance.amount);
    }
    message.u32(property.upkeep_option_key)
      .u32(property.upkeep_current)
      .u8(property.voting_opt_out);
    return message;
  }

  WireWriter propertyIdList(const std::vector<uint32> &ids) {
    WireWriter message;
    message.u32(static_cast<uint32>(ids.size()));
    for (uint32 id : ids) {
      message.u32(id);
    }
    return message;
  }

  WireWriter regionList(const std::vector<RealEstateRegionsRepository::RealEstateRegions> &regions) {
    WireWriter message;
    message.u32(static_cast<uint32>(regions.size()));
    for (const auto &region : regions) {
      message.u32(region.id);
      for (float value : regionMatrix(RegionBox{region.x, region.y, region.z, region.size_x, region.size_y, region.size_z, region.heading})) {
        message.f32(value);
      }
      for (int unused = 0; unused < 5; unused++) {
        message.u32(0);
      }
    }
    return message;
  }
}
