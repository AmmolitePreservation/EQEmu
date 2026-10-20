#include "housingCopies.h"

#include "database.h"
#include "rulesys.h"
#include "strings.h"

#include <ctime>
#include <stdexcept>

namespace housing {
  namespace {
    constexpr int maximumDepth = 10;
    constexpr const char *copyNote = "housing";
  }

  ZoneTemplatePoint zoneTemplateOf(Database &content, uint32 definitionId) {
    auto rows = query(content, fmt::format(
      "SELECT t.zone_id, t.zone_version, t.zone_in_x, t.zone_in_y, t.zone_in_z, t.zone_in_heading FROM real_estate_definitions d "
      "JOIN real_estate_zone_templates t ON t.dz_id = d.dz_id WHERE d.id = {}", definitionId
    ));
    if (rows.RowCount() != 1) {
      throw std::runtime_error(fmt::format("definition [{}] names no zone template", definitionId));
    }
    auto row = rows.begin();
    return ZoneTemplatePoint{
      Strings::ToUnsignedInt(row[0]), static_cast<uint8>(Strings::ToUnsignedInt(row[1])),
      Strings::ToFloat(row[2]), Strings::ToFloat(row[3]), Strings::ToFloat(row[4]), Strings::ToFloat(row[5])
    };
  }

  PropertyRow rootOf(Database &database, const PropertyRow &property) {
    PropertyRow current = property;
    for (int depth = 0; current.parent_id; depth++) {
      if (depth >= maximumDepth) {
        throw std::runtime_error(fmt::format("property [{}] has an ancestor chain deeper than [{}]", property.id, maximumDepth));
      }
      auto parents = RealEstatePropertiesRepository::GetWhere(database, fmt::format("id = {}", current.parent_id));
      if (parents.size() != 1) {
        throw std::runtime_error(fmt::format("property [{}] has ancestor [{}], which does not exist", property.id, current.parent_id));
      }
      current = parents.front();
    }
    return current;
  }

  uint32 hostZoneOf(Database &database, Database &content, const PropertyRow &property) {
    return zoneTemplateOf(content, rootOf(database, property).definition_id).zoneId;
  }

  uint16 existingCopy(Database &database, uint32 propertyId) {
    auto rows = query(database, fmt::format(
      "SELECT c.instance_id FROM real_estate_instances c JOIN instance_list l ON l.id = c.instance_id WHERE c.property_id = {}", propertyId
    ));
    return rows.RowCount() ? static_cast<uint16>(Strings::ToUnsignedInt(rows.begin()[0])) : 0;
  }

  uint32 propertyOfCopy(Database &database, uint16 instanceId) {
    auto rows = query(database, fmt::format("SELECT property_id FROM real_estate_instances WHERE instance_id = {}", instanceId));
    return rows.RowCount() ? Strings::ToUnsignedInt(rows.begin()[0]) : 0;
  }

  // One copy per property: the property's row lock orders entries racing from several zones, the counter's row all allocations.
  uint16 runningCopy(Database &database, Database &content, uint32 propertyId) {
    Transaction transaction(database);
    const PropertyRow property = lockedProperty(database, propertyId);
    if (const uint16 running = existingCopy(database, propertyId)) {
      transaction.commit();
      return running;
    }
    query(database, fmt::format("DELETE FROM real_estate_instances WHERE property_id = {}", propertyId));
    const ZoneTemplatePoint zoneTemplate = zoneTemplateOf(content, property.definition_id);
    auto counter = query(database, "SELECT next_instance_id FROM real_estate_instance_counter WHERE id = 1 FOR UPDATE");
    if (counter.RowCount() != 1) {
      throw std::runtime_error("real_estate_instance_counter has no row; world starts it at boot");
    }
    std::set<uint32> used;
    for (auto row : query(database, "SELECT id FROM instance_list")) {
      used.insert(Strings::ToUnsignedInt(row[0]));
    }
    const auto instanceId = nextInstanceId(Strings::ToUnsignedInt(counter.begin()[0]), instanceRange(RuleI(Instances, ReservedInstances)), used);
    if (!instanceId) {
      throw std::runtime_error(fmt::format("every instance id up to [{}] is in use; property [{}] gets no copy", lastInstanceId, propertyId));
    }
    // never_expires with is_global 0: both set would route the zone's instance 0 to this copy (INT-13).
    query(database, fmt::format(
      "INSERT INTO instance_list (id, zone, version, is_global, start_time, duration, expire_at, never_expires, notes) VALUES ({}, {}, {}, 0, {}, 0, 0, 1, '{}')",
      *instanceId, zoneTemplate.zoneId, zoneTemplate.zoneVersion, std::time(nullptr), copyNote
    ));
    query(database, fmt::format("INSERT INTO real_estate_instances (instance_id, property_id) VALUES ({}, {})", *instanceId, propertyId));
    query(database, fmt::format("UPDATE real_estate_instance_counter SET next_instance_id = {} WHERE id = 1", *instanceId + 1));
    transaction.commit();
    return static_cast<uint16>(*instanceId);
  }

  void discardCopy(Database &database, uint16 instanceId) {
    {
      Transaction transaction(database);
      const uint32 propertyId = propertyOfCopy(database, instanceId);
      if (!propertyId) {
        throw std::runtime_error(fmt::format("instance [{}] is no housing copy", instanceId));
      }
      query(database, fmt::format(
        "INSERT INTO character_real_estate_interiors (character_id, property_id) "
        "SELECT c.id, {} FROM character_data c JOIN instance_list l ON l.id = c.zone_instance AND l.zone = c.zone_id WHERE c.zone_instance = {} "
        "ON DUPLICATE KEY UPDATE property_id = VALUES(property_id)", propertyId, instanceId
      ));
      query(database, fmt::format("DELETE FROM real_estate_instances WHERE instance_id = {}", instanceId));
      query(database, fmt::format("DELETE FROM instance_list WHERE id = {}", instanceId));
      transaction.commit();
    }
    database.DeleteInstance(instanceId);
  }

  void restartInstanceCounter(Database &database) {
    const InstanceRange range = instanceRange(RuleI(Instances, ReservedInstances));
    query(database, fmt::format(
      "INSERT INTO real_estate_instance_counter (id, next_instance_id) VALUES (1, {0}) ON DUPLICATE KEY UPDATE next_instance_id = {0}", range.first
    ));
  }
}
