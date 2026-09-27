#include "housing.h"

#include "housingCatalog.h"
#include "housingEvents.h"
#include "housingGeometry.h"
#include "housingSession.h"
#include "housingStore.h"

#include "../../common/seperator.h"
#include "../../common/strings.h"
#include "../client.h"
#include "../zone.h"
#include "../zonedb.h"

#include <charconv>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>
#include <vector>

extern Zone *zone;

namespace housing {
  namespace {
    constexpr float regionBelowFeet = 100.0f;
    constexpr float regionAboveFeet = 900.0f;
    const std::set<std::string> numericDefinitionFields = {"purchase_cost_id", "upkeep_cost_id", "max_items", "max_non_real_estate_items",
                                                           "max_pet_items", "max_houses", "region_id", "zone_id", "zone_version", "type", "category"};

    void say(Client *client, const std::string &text) {
      client->Message(Chat::White, "%s", text.c_str());
    }

    void refuse(Client *client, const std::string &text) {
      client->Message(Chat::Red, "%s", text.c_str());
    }

    bool parseInteger(const char *text, int64 &value) {
      const char *end = text + std::strlen(text);
      const auto result = std::from_chars(text, end, value);
      return *text && result.ec == std::errc() && result.ptr == end;
    }

    bool parseSize(const char *text, float &value) {
      char *end = nullptr;
      value = std::strtof(text, &end);
      return *text && *end == '\0' && value > 0.0f;
    }

    const char *typeName(uint32 type) {
      static const char *names[] = {"none", "regular zone", "guild hall", "player house", "plot", "neighborhood", "town", "moving crate", "guild plot"};
      return type < 9 ? names[type] : "unknown";
    }

    RegionBox boxOf(const RealEstateRegionsRepository::RealEstateRegions &region) {
      return RegionBox{region.x, region.y, region.z, region.size_x, region.size_y, region.size_z, region.heading};
    }

    int64 coinOf(uint32 costId) {
      const CostOption *coin = catalog().option(costId, 0);
      return coin ? coin->items[0].amount : 0;
    }

    bool applyEdit(Client *client, const std::string &done, const std::vector<std::string> &statements) {
      content_db.TransactionBegin();
      for (const auto &statement : statements) {
        auto results = content_db.QueryDatabase(statement);
        if (!results.Success()) {
          content_db.TransactionRollback();
          refuse(client, fmt::format("Refused: {}", results.ErrorMessage()));
          return false;
        }
      }
      if (!catalog().load()) {
        const auto problems = catalog().problems();
        content_db.TransactionRollback();
        catalog().load();
        refuse(client, "Refused: the catalog would not load after that change:");
        for (const auto &problem : problems) {
          refuse(client, "  " + problem);
        }
        return false;
      }
      if (!content_db.TransactionCommit().Success()) {
        catalog().load();
        refuse(client, "Refused: the database did not commit the change.");
        return false;
      }
      sendEvent(EventType::catalogChanged, EventTarget::allZones, 0);
      say(client, done + "; every zone reloads the catalog.");
      return true;
    }

    void listRegionsHere(Client *client) {
      int found = 0;
      for (const auto &[id, region] : catalog().allRegions()) {
        if (!regionContains(boxOf(region), client->GetX(), client->GetY(), client->GetZ())) {
          continue;
        }
        std::vector<std::string> users;
        for (const auto &[definitionId, entry] : catalog().allDefinitions()) {
          if (entry.row.region_id == id) {
            users.push_back(fmt::format("{} ({})", definitionId, entry.row.name));
          }
        }
        say(client, fmt::format("Region {} '{}': center ({:.1f}, {:.1f}, {:.1f}), across {:.1f}, along {:.1f}, height {:.1f}, heading {:.1f}; definitions: {}",
          id, region.name, region.x, region.y, region.z, region.size_x, region.size_y, region.size_z, region.heading, users.empty() ? "none" : Strings::Join(users, ", ")));
        found++;
      }
      if (!found) {
        say(client, "No region contains you.");
      }
    }

    void info(Client *client) {
      say(client, fmt::format("Housing in this zone is {}; the catalog is {} ({} definitions).",
        housingActive() ? "active" : "inactive", catalog().valid() ? "valid" : "invalid", catalog().allDefinitions().size()));
      const Session *state = session(client);
      if (!state) {
        say(client, "You have no housing session here (housing is inactive, or not a RoF2 client).");
        listRegionsHere(client);
        return;
      }
      std::vector<const Property *> here;
      for (const auto &[id, property] : state->properties) {
        if (property.host_zone_id != zone->GetZoneID() || property.host_instance_id != zone->GetInstanceID()) {
          continue;
        }
        const Definition *definition = catalog().definition(property.definition_id);
        const auto *region = definition && definition->row.region_id ? catalog().region(definition->row.region_id) : nullptr;
        if (id == state->currentPropertyId || (region && regionContains(boxOf(*region), client->GetX(), client->GetY(), client->GetZ()))) {
          here.push_back(&property);
        }
      }
      std::set<uint32> owners;
      std::set<uint32> ids;
      for (const Property *property : here) {
        owners.insert(property->owner_character_id);
        ids.insert(property->id);
      }
      const auto names = characterNames(owners);
      const auto money = balances(ids);
      for (const Property *property : here) {
        size_t children = 0;
        for (const auto &[id, other] : state->properties) {
          children += other.parent_id == property->id ? 1 : 0;
        }
        std::string escrow;
        auto held = money.find(property->id);
        if (held != money.end()) {
          for (const auto &balance : held->second) {
            escrow += fmt::format(" {}:{}", balance.currency, balance.amount);
          }
        }
        const Definition *definition = catalog().definition(property->definition_id);
        auto owner = names.find(property->owner_character_id);
        say(client, fmt::format("{} property {} '{}': definition {} ({}), owner {}, guild {}, upkeep {} in currency {}, {} children, escrow{}",
          property->id == state->currentPropertyId ? "Current" : "Standing in", property->id, property->name, property->definition_id,
          definition ? typeName(definition->row.type) : "unknown", owner == names.end() ? "none" : owner->second, property->guild_id,
          property->upkeep_current ? "current" : "expired", property->upkeep_currency, children, escrow.empty() ? " empty" : escrow));
      }
      listRegionsHere(client);
    }

    void regionCommand(Client *client, const Seperator *sep) {
      const std::string usage = "Usage: #realestate region create <across> <along> [name] | move <id> | size <id> <across> <along> | show";
      const std::string action = Strings::ToLower(sep->arg[2]);
      if (action == "show") {
        listRegionsHere(client);
        return;
      }
      const float centerZ = client->GetZ() + (regionAboveFeet - regionBelowFeet) / 2.0f;
      if (action == "create") {
        float across = 0.0f;
        float along = 0.0f;
        if (!parseSize(sep->arg[3], across) || !parseSize(sep->arg[4], along)) {
          refuse(client, usage);
          return;
        }
        auto rows = content_db.QueryDatabase("SELECT COALESCE(MAX(id), 0) + 1 FROM real_estate_regions");
        if (!rows.Success() || rows.RowCount() != 1) {
          refuse(client, "Refused: the region ids could not be read.");
          return;
        }
        auto row = rows.begin();
        const uint32 id = Strings::ToUnsignedInt(row[0]);
        const std::string name = sep->arg[5][0] ? sep->argplus[5] : fmt::format("region {}", id);
        applyEdit(client, fmt::format("Region {} created around you", id), {fmt::format(
          "INSERT INTO real_estate_regions (id, name, x, y, z, size_x, size_y, size_z, heading) VALUES ({}, '{}', {}, {}, {}, {}, {}, {}, {})",
          id, Strings::Escape(name), client->GetX(), client->GetY(), centerZ, across, along, regionBelowFeet + regionAboveFeet, client->GetHeading())});
        return;
      }
      int64 id = 0;
      if (!parseInteger(sep->arg[3], id) || id <= 0 || !catalog().region(static_cast<uint32>(id))) {
        refuse(client, usage + " (the id must name a region)");
        return;
      }
      if (action == "move") {
        applyEdit(client, fmt::format("Region {} moved to you", id), {fmt::format(
          "UPDATE real_estate_regions SET x = {}, y = {}, z = {}, heading = {} WHERE id = {}", client->GetX(), client->GetY(), centerZ, client->GetHeading(), id)});
        return;
      }
      float across = 0.0f;
      float along = 0.0f;
      if (action == "size" && parseSize(sep->arg[4], across) && parseSize(sep->arg[5], along)) {
        applyEdit(client, fmt::format("Region {} resized", id), {fmt::format(
          "UPDATE real_estate_regions SET size_x = {}, size_y = {} WHERE id = {}", across, along, id)});
        return;
      }
      refuse(client, usage);
    }

    void showDefinition(Client *client, const Definition &entry) {
      const auto &row = entry.row;
      std::vector<std::string> children;
      for (uint32 child : entry.children) {
        children.push_back(std::to_string(child));
      }
      say(client, fmt::format("Definition {} '{}': type {} ({}), category {}, region {}, zone {} version {}",
        row.id, row.name, row.type, typeName(row.type), row.category, row.region_id, row.zone_id, row.zone_version));
      say(client, fmt::format("  items {}, vault {}, pets {}, houses {} (effective {})",
        row.max_items, row.max_non_real_estate_items, row.max_pet_items, row.max_houses, entry.maxHouses));
      say(client, fmt::format("  purchase cost {} ({} copper), upkeep cost {} ({} copper a day)",
        row.purchase_cost_id, coinOf(row.purchase_cost_id), row.upkeep_cost_id, coinOf(row.upkeep_cost_id)));
      say(client, fmt::format("  layout origin ({:.1f}, {:.1f}, {:.1f}) heading {:.1f}; children: {}",
        row.layout_origin_x, row.layout_origin_y, row.layout_origin_z, row.layout_heading, children.empty() ? "none" : Strings::Join(children, " ")));
    }

    void definitionCommand(Client *client, const Seperator *sep) {
      const std::string usage = "Usage: #realestate definition show <id> | set <id> <field> <value> | origin <id> | create <id> <type 1-8> <name> | child add|remove <parent> <child>";
      const std::string action = Strings::ToLower(sep->arg[2]);
      if (action == "child") {
        const std::string direction = Strings::ToLower(sep->arg[3]);
        int64 parent = 0;
        int64 child = 0;
        if ((direction != "add" && direction != "remove") || !parseInteger(sep->arg[4], parent) || !parseInteger(sep->arg[5], child) || parent <= 0 || child <= 0) {
          refuse(client, usage);
          return;
        }
        applyEdit(client, fmt::format("Definition {} child {} {}", parent, child, direction == "add" ? "added" : "removed"), {direction == "add"
          ? fmt::format("INSERT INTO real_estate_definition_children (parent_id, child_id) VALUES ({}, {})", parent, child)
          : fmt::format("DELETE FROM real_estate_definition_children WHERE parent_id = {} AND child_id = {}", parent, child)});
        return;
      }
      int64 id = 0;
      if (!parseInteger(sep->arg[3], id) || id <= 0) {
        refuse(client, usage);
        return;
      }
      const Definition *definition = catalog().definition(static_cast<uint32>(id));
      if (action == "create") {
        int64 type = 0;
        if (definition || !parseInteger(sep->arg[4], type) || type < 1 || type > 8 || !sep->arg[5][0]) {
          refuse(client, "Usage: #realestate definition create <unused id> <type 1-8> <name>");
          return;
        }
        applyEdit(client, fmt::format("Definition {} created", id), {fmt::format(
          "INSERT INTO real_estate_definitions (id, name, type) VALUES ({}, '{}', {})", id, Strings::Escape(sep->argplus[5]), type)});
        return;
      }
      if (!definition) {
        refuse(client, fmt::format("No definition {}.", id));
        return;
      }
      if (action == "show") {
        showDefinition(client, *definition);
        return;
      }
      if (action == "origin") {
        applyEdit(client, fmt::format("Definition {} layout origin set to you", id), {fmt::format(
          "UPDATE real_estate_definitions SET layout_origin_x = {}, layout_origin_y = {}, layout_origin_z = {}, layout_heading = {} WHERE id = {}",
          client->GetX(), client->GetY(), client->GetZ(), client->GetHeading(), id)});
        return;
      }
      if (action == "set") {
        const std::string field = Strings::ToLower(sep->arg[4]);
        if (field == "name" && sep->arg[5][0]) {
          applyEdit(client, fmt::format("Definition {} renamed", id), {fmt::format(
            "UPDATE real_estate_definitions SET name = '{}' WHERE id = {}", Strings::Escape(sep->argplus[5]), id)});
          return;
        }
        int64 value = 0;
        if (!numericDefinitionFields.count(field) || !parseInteger(sep->arg[5], value) || value < 0) {
          refuse(client, "Usage: #realestate definition set <id> <field> <value>; fields: name, " + Strings::Join(std::vector<std::string>(numericDefinitionFields.begin(), numericDefinitionFields.end()), ", "));
          return;
        }
        applyEdit(client, fmt::format("Definition {} {} set to {}", id, field, value), {fmt::format(
          "UPDATE real_estate_definitions SET `{}` = {} WHERE id = {}", field, value, id)});
        return;
      }
      refuse(client, usage);
    }

    void costCommand(Client *client, const Seperator *sep) {
      int64 id = 0;
      int64 amount = 0;
      if (Strings::ToLower(sep->arg[2]) != "set" || !parseInteger(sep->arg[3], id) || id <= 0 || !parseInteger(sep->arg[4], amount) || amount == 0) {
        refuse(client, "Usage: #realestate cost set <cost id> <copper, not 0; negative is a reduction>");
        return;
      }
      auto rows = content_db.QueryDatabase(fmt::format("SELECT COUNT(*) FROM real_estate_cost_items WHERE cost_id = {} AND currency = 0", id));
      if (!rows.Success() || rows.RowCount() != 1 || Strings::ToInt(rows.begin()[0]) != 1) {
        refuse(client, fmt::format("Cost {} has no coin option.", id));
        return;
      }
      applyEdit(client, fmt::format("Cost {} set to {} copper", id, amount), {fmt::format(
        "UPDATE real_estate_cost_items SET amount = {} WHERE cost_id = {} AND currency = 0", amount, id)});
    }

    void help(Client *client) {
      say(client, "#realestate info: the properties and regions where you stand");
      say(client, "#realestate region create <across> <along> [name] | move <id> | size <id> <across> <along> | show");
      say(client, "#realestate definition show <id> | set <id> <field> <value> | origin <id> | create <id> <type 1-8> <name> | child add|remove <parent> <child>");
      say(client, "#realestate cost set <id> <copper>: a stored (item upkeep) cost");
      say(client, "#realestate reload: every zone reloads the catalog");
      say(client, "Edits are validated before they're kept and reach every zone; one that would break the catalog is refused.");
    }
  }

  void handleCommand(Client *client, const Seperator *sep) {
    const std::string subcommand = Strings::ToLower(sep->arg[1]);
    if (subcommand == "info") {
      info(client);
    }
    else if (subcommand == "region") {
      regionCommand(client, sep);
    }
    else if (subcommand == "definition") {
      definitionCommand(client, sep);
    }
    else if (subcommand == "cost") {
      costCommand(client, sep);
    }
    else if (subcommand == "reload") {
      sendEvent(EventType::catalogChanged, EventTarget::allZones, 0);
      say(client, "Every zone reloads the catalog.");
    }
    else {
      help(client);
    }
  }
}
