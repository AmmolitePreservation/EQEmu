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

  std::vector<Property> propertiesOwnedBy(uint32 characterId) {
    return RealEstatePropertiesRepository::GetWhere(database, fmt::format("owner_character_id = {}", characterId));
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

  std::map<uint32, CharacterIdentity> characterIdentities(const std::set<uint32> &characterIds) {
    std::map<uint32, CharacterIdentity> result;
    std::set<uint32> wanted;
    for (uint32 id : characterIds) {
      if (id) {
        wanted.insert(id);
      }
    }
    if (wanted.empty()) {
      return result;
    }
    auto rows = database.QueryDatabase(fmt::format("SELECT id, name, account_id FROM character_data WHERE id IN ({})", idList(wanted)));
    for (auto row : rows) {
      result[Strings::ToUnsignedInt(row[0])] = CharacterIdentity{row[1], Strings::ToUnsignedInt(row[2])};
    }
    return result;
  }
}
