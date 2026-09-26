#include "housingStore.h"

#include "../../common/strings.h"
#include "../zonedb.h"

#include <cstdlib>

namespace housing {
  std::string idList(const std::set<uint32> &ids) {
    std::vector<std::string> parts;
    for (uint32 id : ids) {
      parts.push_back(std::to_string(id));
    }
    return Strings::Join(parts, ",");
  }

  std::vector<Property> propertiesHostedAt(uint32 zoneId, uint32 instanceId) {
    return RealEstatePropertiesRepository::GetWhere(database, fmt::format("host_zone_id = {} AND host_instance_id = {} AND archived = 0", zoneId, instanceId));
  }

  std::vector<Property> propertiesWithInterior(uint32 instanceId) {
    if (instanceId == 0) {
      return {};
    }
    return RealEstatePropertiesRepository::GetWhere(database, fmt::format("interior_instance_id = {}", instanceId));
  }

  std::vector<Property> propertiesOwnedBy(uint32 characterId) {
    return RealEstatePropertiesRepository::GetWhere(database, fmt::format("owner_character_id = {}", characterId));
  }

  std::vector<Property> propertiesCarriedBy(uint32 characterId) {
    return RealEstatePropertiesRepository::GetWhere(
      database,
      fmt::format("contained_in_item_guid <> 0 AND contained_in_item_guid IN (SELECT guid FROM inventory WHERE character_id = {} AND guid <> 0)", characterId)
    );
  }

  std::vector<Property> propertiesById(const std::set<uint32> &ids) {
    if (ids.empty()) {
      return {};
    }
    return RealEstatePropertiesRepository::GetWhere(database, fmt::format("id IN ({})", idList(ids)));
  }

  std::vector<Property> propertiesChildOf(const std::set<uint32> &parentIds) {
    if (parentIds.empty()) {
      return {};
    }
    return RealEstatePropertiesRepository::GetWhere(database, fmt::format("parent_id IN ({})", idList(parentIds)));
  }

  std::map<uint32, std::vector<Balance>> balances(const std::set<uint32> &propertyIds) {
    std::map<uint32, std::vector<Balance>> result;
    if (propertyIds.empty()) {
      return result;
    }
    auto rows = database.QueryDatabase(fmt::format(
      "SELECT property_id, currency, amount FROM real_estate_property_balances WHERE property_id IN ({}) ORDER BY property_id, currency",
      idList(propertyIds)
    ));
    for (auto row : rows) {
      result[Strings::ToUnsignedInt(row[0])].push_back(Balance{Strings::ToUnsignedInt(row[1]), std::strtod(row[2], nullptr)});
    }
    return result;
  }

  std::map<uint32, std::string> characterNames(const std::set<uint32> &characterIds) {
    std::map<uint32, std::string> result;
    std::set<uint32> wanted;
    for (uint32 id : characterIds) {
      if (id) {
        wanted.insert(id);
      }
    }
    if (wanted.empty()) {
      return result;
    }
    auto rows = database.QueryDatabase(fmt::format("SELECT id, name FROM character_data WHERE id IN ({})", idList(wanted)));
    for (auto row : rows) {
      result[Strings::ToUnsignedInt(row[0])] = row[1] ? row[1] : "";
    }
    return result;
  }
}
