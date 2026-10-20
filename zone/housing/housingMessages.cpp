#include "housingMessages.h"

#include "housingGeometry.h"

namespace housing {
  WireWriter propertyRecord(const Property &property, const RecordFields &fields) {
    WireWriter message;
    message.u32(property.id)
      .i32(fields.vacantPlayerPlot ? 0 : -1)
      .u32(property.parent_id)
      .u32(property.definition_id)
      .u32(fields.guildId ? fields.guildId : 0xFFFFFFFF)
      .u32(0)
      .lengthString(fields.owner ? std::to_string(fields.owner->accountId) : "")
      .lengthString(fields.owner ? fields.owner->name : "")
      .u32(0)
      .u32(static_cast<uint32>(fields.childIds.size()))
      .lengthString(fields.name);
    for (uint32 child : fields.childIds) {
      message.u32(child);
    }
    message.u8(static_cast<uint8>(fields.balances.size()));
    for (const auto &balance : fields.balances) {
      message.u32(balance.currency).f64(balance.amount);
    }
    message.u32(fields.upkeepOptionKey)
      .u32(property.upkeep_expired_time)
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
