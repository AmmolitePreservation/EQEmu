// Reads EverQuest Live's data from client memory for Peridot's housing work (playerHousing/serverPlan.md, chunk 5): items'
// housing fields, and whole-zone dumps. Nothing here asks the server for anything except the item links /peridotlive link
// clicks, as a player would.
#include <mq/Plugin.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

PreSetup("MQ2PeridotLive");
PLUGIN_VERSION(0.2);

namespace {
  constexpr auto linkWait = std::chrono::seconds(10);
  constexpr const char *reportFileName = "peridotLiveItems.tsv";

  struct PendingLink {
    int itemId = 0;
    uint32_t hash = 0;
    std::chrono::steady_clock::time_point sent;
  };

  std::optional<PendingLink> pendingLink;

  template <size_t Length>
  std::string boundedText(const char (&field)[Length]) {
    return std::string(field, strnlen(field, Length));
  }

  std::string hexBytes(const void *data, size_t size) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string hex;
    hex.reserve(size * 2);
    const auto *bytes = static_cast<const uint8_t *>(data);
    for (size_t index = 0; index < size; ++index) {
      hex += digits[bytes[index] >> 4];
      hex += digits[bytes[index] & 0x0f];
    }
    return hex;
  }

  template <typename T>
  void writeRaw(std::ofstream &out, const T &object) {
    out << "raw\t" << sizeof(T) << '\t' << hexBytes(&object, sizeof(T)) << '\n';
  }

  std::string itemLinkHash(ItemClient *item) {
    char itemLink[512] = {};
    FormatItemLink(itemLink, sizeof(itemLink), item);
    ItemLinkInfo linkInfo;
    return ParseItemLink(itemLink, linkInfo) ? fmt::format("{:08X}", linkInfo.itemHash) : "unparsed";
  }

  std::string linkData(int itemId, uint32_t hash) {
    std::string data = fmt::format("{:05X}", itemId);
    for (int socket = 0; socket < MAX_AUG_SOCKETS; ++socket) {
      data += "0000000000";
    }
    data += "0" "0000" "00" "00000" "00000";
    data += fmt::format("{:08X}", hash);
    return data;
  }

  void writeItem(std::ofstream &out, ItemClient *item) {
    const ItemDefinition *definition = item->GetItemDefinition();
    out << "item\t" << item->GetID() << '\t' << item->GetName() << '\t' << boundedText(definition->HousepetFileName) << '\t'
      << definition->NpcRespawnInterval << '\t' << definition->RealEstateDefID << '\t' << definition->RealEstateUpkeepID << '\t'
      << definition->MaxPerRealEstate << '\t' << definition->Placeable << '\t' << definition->PlacementType << '\t'
      << definition->TrophyBenefitID << '\t' << (definition->bInteractiveObject ? 1 : 0) << '\t' << itemLinkHash(item) << '\n';
    writeRaw(out, *item);
    writeRaw(out, *definition);
  }

  void report(ItemClient *item, const std::string &source) {
    const ItemDefinition *definition = item->GetItemDefinition();
    const std::string housepetFileName = boundedText(definition->HousepetFileName);
    const std::string linkHash = itemLinkHash(item);
    WriteChatf("\ag[PeridotLive]\ax %d %s: housepet '\ay%s\ax', respawn %d, definition %d, upkeep %d, max %d, placeable 0x%X, placement %d, trophy %d, interactive %d, link hash \ay%s\ax",
      item->GetID(), item->GetName(), housepetFileName.c_str(), definition->NpcRespawnInterval, definition->RealEstateDefID,
      definition->RealEstateUpkeepID, definition->MaxPerRealEstate, definition->Placeable, definition->PlacementType,
      definition->TrophyBenefitID, definition->bInteractiveObject ? 1 : 0, linkHash.c_str());
    const std::filesystem::path reportPath = std::filesystem::path(gPathLogs) / reportFileName;
    const bool isNew = !std::filesystem::exists(reportPath);
    std::ofstream reportFile(reportPath, std::ios::app);
    if (!reportFile) {
      WriteChatf("\ar[PeridotLive]\ax could not write %s", reportPath.string().c_str());
      return;
    }
    if (isNew) {
      reportFile << "itemId\tname\thousepetFileName\tnpcRespawnInterval\trealEstateDefinitionId\trealEstateUpkeepId\tmaxPerRealEstate\tplaceable\tplacementType\ttrophyBenefitId\tinteractiveObject\tsource\tlinkHash\n";
    }
    reportFile << item->GetID() << '\t' << item->GetName() << '\t' << housepetFileName << '\t' << definition->NpcRespawnInterval << '\t'
      << definition->RealEstateDefID << '\t' << definition->RealEstateUpkeepID << '\t' << definition->MaxPerRealEstate << '\t' << definition->Placeable << '\t'
      << definition->PlacementType << '\t' << definition->TrophyBenefitID << '\t' << (definition->bInteractiveObject ? 1 : 0) << '\t' << source << '\t' << linkHash << '\n';
  }

  ItemClient *displayedItem(int itemId) {
    if (!pItemDisplayManager) {
      return nullptr;
    }
    for (int index = 0; index < pItemDisplayManager->GetCount(); ++index) {
      CItemDisplayWnd *window = pItemDisplayManager->GetWindow(index);
      if (window && window->pItem && window->pItem->GetID() == itemId) {
        return window->pItem.get();
      }
    }
    return nullptr;
  }

  void sendLink(const char *itemArgument, const char *hashArgument) {
    char *end = nullptr;
    const long itemId = std::strtol(itemArgument, &end, 10);
    if (!*itemArgument || *end || itemId <= 0 || itemId > 0xFFFFF) {
      WriteChatf("\ar[PeridotLive]\ax usage: /peridotlive link <itemID> [hash in hex]");
      return;
    }
    unsigned long hash = 0;
    if (*hashArgument) {
      hash = std::strtoul(hashArgument, &end, 16);
      if (*end) {
        WriteChatf("\ar[PeridotLive]\ax the hash must be hexadecimal");
        return;
      }
    }
    const std::string link = fmt::format("{}0{}PeridotLive {}{}", ITEM_TAG_CHAR, linkData(itemId, hash), itemId, ITEM_TAG_CHAR);
    const TextTagInfo tag = ExtractLink(link);
    if (tag.tagCode != ETAG_ITEM) {
      WriteChatf("\ar[PeridotLive]\ax the built link for %ld did not parse as an item link", itemId);
      return;
    }
    if (!ExecuteTextLink(tag)) {
      WriteChatf("\ar[PeridotLive]\ax the link for %ld could not be clicked (no chat window?)", itemId);
      return;
    }
    pendingLink = PendingLink{static_cast<int>(itemId), static_cast<uint32_t>(hash), std::chrono::steady_clock::now()};
    WriteChatf("\ag[PeridotLive]\ax clicked a link for %ld with hash %08lX; waiting for the item display", itemId, hash);
  }

  void showOpenItems() {
    int shown = 0;
    if (pItemDisplayManager) {
      for (int index = 0; index < pItemDisplayManager->GetCount(); ++index) {
        CItemDisplayWnd *window = pItemDisplayManager->GetWindow(index);
        if (window && window->pItem) {
          report(window->pItem.get(), "show");
          ++shown;
        }
      }
    }
    if (!shown) {
      WriteChatf("\ay[PeridotLive]\ax no item display window is open");
    }
  }

  void dumpRealEstate(std::ofstream &out, int &definitions, int &properties, int &placedItems) {
    RealEstateManagerClient *manager = pRealEstate;
    if (!manager) {
      out << "[realEstate] no real estate manager\n";
      return;
    }
    out << "[realEstate] manager\tzoneRealEstateId\tzoneRealEstateType\tcurrentRealEstateId\tcurrentYardId\tcurrentHouseId\tcurrentMovingCrateId\tlastRefreshTime\n";
    out << "manager\t" << manager->zoneRealEstateId << '\t' << static_cast<int>(manager->zoneRealEstateType) << '\t' << manager->currentRealEstateId << '\t'
      << manager->currentYardId << '\t' << manager->currentHouseId << '\t' << manager->currentMovingCrateId << '\t' << manager->lastRefreshTime << '\n';
    writeRaw(out, *manager);

    out << "[definitions] definition\tkey\tid\tname\tdescription\taddress\tzoneInX\tzoneInY\tzoneInZ\tareaId\tcostDefinition\tupkeepDefinition\tmaxItems\tmaxNonRealEstateItems\tmaxChildren\tzoneId\tdzId\ttype\tgroupId\ticon\tmaxNpcItems\tswitchX\tswitchY\tswitchZ\tswitchHeading\tchildDefinitions\n";
    if (manager->definitions) {
      auto &table = manager->definitions->definitions;
      for (auto *entry = table.WalkFirstEntry(); entry; entry = table.WalkNextEntry(entry)) {
        const RealEstateDefinition &definition = entry->value();
        std::string children;
        for (int index = 0; index < definition.childDefinitions.GetCount(); ++index) {
          children += (index ? "," : "") + std::to_string(definition.childDefinitions[index]);
        }
        out << "definition\t" << entry->key() << '\t' << definition.id << '\t' << definition.name.c_str() << '\t' << definition.description.c_str() << '\t'
          << definition.address.c_str() << '\t' << definition.zoneInCoords.X << '\t' << definition.zoneInCoords.Y << '\t' << definition.zoneInCoords.Z << '\t'
          << definition.areaId << '\t' << definition.costDefinition << '\t' << definition.upkeepDefinition << '\t' << definition.maxItems << '\t'
          << definition.maxNonRealEstateItems << '\t' << definition.maxChildren << '\t' << definition.zoneId << '\t' << definition.dzId << '\t'
          << static_cast<int>(definition.type) << '\t' << definition.groupId << '\t' << definition.icon << '\t' << definition.maxNpcItems << '\t'
          << definition.switchCoords.X << '\t' << definition.switchCoords.Y << '\t' << definition.switchCoords.Z << '\t' << definition.switchHeading << '\t' << children << '\n';
        writeRaw(out, definition);
        ++definitions;
      }
    }

    out << "[costs] cost\tkey\tcostId\tcomponentId\tsubComponents (typeId:typeSubId:cost)\n";
    if (manager->costs) {
      auto &table = manager->costs->costs;
      for (auto *entry = table.WalkFirstEntry(); entry; entry = table.WalkNextEntry(entry)) {
        const RealEstateCostSchemeData &cost = entry->value();
        for (int index = 0; index < cost.costComponents.GetCount(); ++index) {
          const RealEstateCostComponent &component = cost.costComponents[index];
          std::string subComponents;
          for (int sub = 0; sub < component.subComponents.GetCount(); ++sub) {
            const RealEstateCostSubComponent &part = component.subComponents[sub];
            subComponents += fmt::format("{}{}:{}:{}", sub ? "," : "", part.typeId, part.typeSubId, part.cost);
          }
          out << "cost\t" << entry->key() << '\t' << cost.costId << '\t' << component.id << '\t' << subComponents << '\n';
        }
        writeRaw(out, cost);
      }
    }

    out << "[properties] property\tkey\tid\tstaticId\tparentId\tdefinitionId\tguildId\tfellowshipId\tprimaryOwner\tprimaryHandle\tdefaultPermissionLevel\tdynamicName\tdefaultUpkeepId\tupkeepExpiredTime\tvotingOptedOut\tchildIds\tescrow (currency:amount)\n";
    for (auto *entry = manager->realEstates.WalkFirstEntry(); entry; entry = manager->realEstates.WalkNextEntry(entry)) {
      const RealEstate *property = entry->value();
      if (!property) {
        continue;
      }
      std::string children;
      for (int index = 0; index < property->childIds.GetCount(); ++index) {
        children += (index ? "," : "") + std::to_string(property->childIds[index].UniqueEntityID);
      }
      std::string escrow;
      for (auto balance = property->escrowAccount.begin(); balance != property->escrowAccount.end(); ++balance) {
        escrow += fmt::format("{}{}:{}", escrow.empty() ? "" : ",", balance->first, balance->second);
      }
      out << "property\t" << entry->key() << '\t' << property->id << '\t' << property->staticId << '\t' << property->parentId.UniqueEntityID << '\t'
        << property->definitionId << '\t' << property->guildId.UniqueEntityID << '\t' << property->fellowshipId.UniqueEntityID << '\t'
        << property->primaryOwner.c_str() << '\t' << property->primaryHandle.c_str() << '\t' << property->defaultPermissionLevel << '\t'
        << property->dynamicName.c_str() << '\t' << property->defaultUpkeepId << '\t' << static_cast<long long>(property->upkeepExpiredTime) << '\t'
        << (property->votingOptedOut ? 1 : 0) << '\t' << children << '\t' << escrow << '\n';
      writeRaw(out, *property);
      ++properties;
    }

    out << "[guildPlots] guildPlot\tguildId\trealEstateId\n";
    for (RealEstateManager::RealEstateHolder *holder = manager->guildPlotsByGuildId.GetFirst(); holder; holder = manager->guildPlotsByGuildId.GetNext(holder)) {
      const EqGuid guildId = manager->guildPlotsByGuildId.GetKey(holder);
      out << "guildPlot\t" << guildId.UniqueEntityID << '\t' << (holder->realEstate ? holder->realEstate->id : 0) << '\n';
      writeRaw(out, *holder);
    }

    out << "[accessLists] accessList\tkey\n";
    for (auto *entry = manager->accessLists.WalkFirstEntry(); entry; entry = manager->accessLists.WalkNextEntry(entry)) {
      out << "accessList\t" << entry->key() << '\n';
      if (entry->value()) {
        writeRaw(out, *entry->value());
      }
    }

    out << "[itemLists] itemList\tkey\trealEstateId\tnonRealEstateItemCount\tnpcItemCount\tupkeepItemCount\n";
    out << "  placedItem\tkey\trealEstateId\trealEstateItemId\tplaced\tupkeepExpiredTime\tx\ty\tz\theading\tpitch\troll\townerName\townerHandle\townerNameHashKey\n";
    for (auto *entry = manager->itemLists.WalkFirstEntry(); entry; entry = manager->itemLists.WalkNextEntry(entry)) {
      RealEstateItems *list = entry->value();
      if (!list) {
        continue;
      }
      out << "itemList\t" << entry->key() << '\t' << list->realEstateId << '\t' << list->nonRealEstateItemCount << '\t' << list->npcItemCount << '\t' << list->upkeepItemCount << '\n';
      writeRaw(out, *list);
      for (auto *itemEntry = list->realEstateItems.WalkFirstEntry(); itemEntry; itemEntry = list->realEstateItems.WalkNextEntry(itemEntry)) {
        const RealEstateItem *placed = itemEntry->value();
        if (!placed) {
          continue;
        }
        out << "placedItem\t" << itemEntry->key() << '\t' << placed->GetRealEstateId() << '\t' << placed->GetRealEstateItemId() << '\t' << (placed->IsPlaced() ? 1 : 0) << '\t'
          << static_cast<long long>(placed->GetUpkeepExpiredTime()) << '\t' << placed->GetX() << '\t' << placed->GetY() << '\t' << placed->GetZ() << '\t'
          << placed->GetHeading() << '\t' << placed->GetPitch() << '\t' << placed->GetRoll() << '\t' << placed->GetOwnerName().c_str() << '\t'
          << placed->GetOwnerHandle().c_str() << '\t' << placed->GetOwnerNameHashKey() << '\n';
        writeRaw(out, *placed);
        if (ItemPtr item = placed->GetItem()) {
          writeItem(out, item.get());
        }
        ++placedItems;
      }
    }
    out << "[itemGroups] not dumped: the client's item group manager has no reachable instance in eqlib\n";
  }

  void dumpZone() {
    if (!pLocalPC || !pZoneInfo) {
      WriteChatf("\ar[PeridotLive]\ax not in a zone");
      return;
    }
    const std::time_t now = std::time(nullptr);
    std::tm local = {};
    localtime_s(&local, &now);
    char stamp[32] = {};
    std::strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", &local);
    const std::string zoneName = boundedText(pZoneInfo->ShortName);
    const std::filesystem::path dumpPath = std::filesystem::path(gPathLogs) / fmt::format("peridotLiveZone_{}_{}_{}.txt", zoneName, pLocalPC->instance, stamp);
    std::ofstream out(dumpPath);
    if (!out) {
      WriteChatf("\ar[PeridotLive]\ax could not write %s", dumpPath.string().c_str());
      return;
    }
    out << "# MQ2PeridotLive zone dump: everything read from client memory, nothing requested from the server.\n"
      << "# Each record's named fields are followed by a raw line: its size and every byte, in eqlib's layout for this client.\n";
    out << "time\t" << stamp << '\n';
    out << "zone\t" << zoneName << '\t' << boundedText(pZoneInfo->LongName) << '\t' << pZoneInfo->ZoneID << '\t' << pLocalPC->zoneId << '\t' << pLocalPC->instance << '\n';
    writeRaw(out, *pZoneInfo);

    int spawns = 0;
    out << "[spawns] spawn\tspawnId\tname\tdisplayedName\tlastname\ttype\trace\tclass\tgender\tlevel\tx\ty\tz\theading\tavatarHeight\theight\tguildId\trealEstateItemId\n";
    if (pSpawnManager) {
      for (PlayerClient *spawn = pSpawnManager->FirstSpawn; spawn; spawn = spawn->GetNext()) {
        out << "spawn\t" << spawn->SpawnID << '\t' << boundedText(spawn->Name) << '\t' << boundedText(spawn->DisplayedName) << '\t' << boundedText(spawn->Lastname) << '\t'
          << static_cast<int>(spawn->Type) << '\t' << spawn->GetRace() << '\t' << spawn->GetClass() << '\t' << spawn->GetGender() << '\t'
          << static_cast<int>(spawn->GetLevel()) << '\t' << spawn->X << '\t' << spawn->Y << '\t' << spawn->Z << '\t' << spawn->Heading << '\t'
          << spawn->AvatarHeight << '\t' << spawn->Height << '\t' << spawn->GuildID << '\t' << spawn->RealEstateItemId << '\n';
        writeRaw(out, *spawn);
        ++spawns;
      }
    }

    int doors = 0;
    out << "[doors] door\tid\tname\ttype\tstate\tx\ty\tz\theading\tdefaultX\tdefaultY\tdefaultZ\tdefaultHeading\tdefaultState\tscaleFactor\tkey\tspellId\tdynDoorId\tusable\tvisible\tscript\n";
    for (int index = 0; pSwitchMgr && index < pSwitchMgr->GetCount(); ++index) {
      EQSwitch *door = pSwitchMgr->GetSwitch(index);
      if (!door) {
        continue;
      }
      out << "door\t" << door->ID << '\t' << boundedText(door->Name) << '\t' << static_cast<int>(door->Type) << '\t' << static_cast<int>(door->State) << '\t'
        << door->X << '\t' << door->Y << '\t' << door->Z << '\t' << door->Heading << '\t' << door->DefaultX << '\t' << door->DefaultY << '\t'
        << door->DefaultZ << '\t' << door->DefaultHeading << '\t' << static_cast<int>(door->DefaultState) << '\t' << door->ScaleFactor << '\t'
        << door->Key << '\t' << door->SpellID << '\t' << door->DynDoorID << '\t' << (door->bUsable ? 1 : 0) << '\t' << (door->bVisible ? 1 : 0) << '\t'
        << boundedText(door->Script) << '\n';
      writeRaw(out, *door);
      ++doors;
    }

    int grounds = 0;
    out << "[ground] ground\tdropId\tzoneId\tdropSubId\tname\tx\ty\tz\theading\tpitch\troll\tscale\tweight\texpires\n";
    for (EQGroundItem *ground = EQGroundItemListManager::Instance().Top; ground; ground = ground->pNext) {
      out << "ground\t" << ground->DropID << '\t' << ground->ZoneID << '\t' << ground->DropSubID << '\t' << boundedText(ground->Name) << '\t'
        << ground->X << '\t' << ground->Y << '\t' << ground->Z << '\t' << ground->Heading << '\t' << ground->Pitch << '\t' << ground->Roll << '\t'
        << ground->Scale << '\t' << ground->Weight << '\t' << ground->Expires << '\n';
      writeRaw(out, *ground);
      if (ground->Item) {
        writeItem(out, ground->Item.get());
      }
      ++grounds;
    }

    int worldObjects = 0;
    out << "[placedObjects] placedObject\trecordNum\titemGuid\trealEstateId\trealEstateItemId\tisNpc\tplacingItemNpcId\tname\tscale\theading\tangle\troll\tx\ty\tz\n";
    for (EQPlacedItem *placed = EQPlacedItemManager::Instance().Top; placed; placed = placed->pNext) {
      out << "placedObject\t" << placed->RecordNum << '\t' << hexBytes(&placed->ItemGuid, sizeof(placed->ItemGuid)) << '\t' << placed->RealEstateID << '\t'
        << placed->RealEstateItemID << '\t' << (placed->bIsNPC ? 1 : 0) << '\t' << placed->PlacingItemNpcID << '\t' << boundedText(placed->Name) << '\t'
        << placed->Scale << '\t' << placed->Heading << '\t' << placed->Angle << '\t' << placed->Roll << '\t' << placed->X << '\t' << placed->Y << '\t' << placed->Z << '\n';
      writeRaw(out, *placed);
      ++worldObjects;
    }

    int definitions = 0;
    int properties = 0;
    int placedItems = 0;
    dumpRealEstate(out, definitions, properties, placedItems);
    WriteChatf("\ag[PeridotLive]\ax wrote %s: %d spawns, %d doors, %d ground items, %d placed objects, %d definitions, %d properties, %d property items",
      dumpPath.filename().string().c_str(), spawns, doors, grounds, worldObjects, definitions, properties, placedItems);
  }

  void peridotLiveCommand(PlayerClient *, const char *line) {
    char action[MAX_STRING] = {};
    char first[MAX_STRING] = {};
    char second[MAX_STRING] = {};
    GetArg(action, line, 1);
    GetArg(first, line, 2);
    GetArg(second, line, 3);
    if (ci_equals(action, "link")) {
      sendLink(first, second);
    }
    else if (ci_equals(action, "show")) {
      showOpenItems();
    }
    else if (ci_equals(action, "dump")) {
      dumpZone();
    }
    else {
      WriteChatf("\ag[PeridotLive]\ax /peridotlive link <itemID> [hash in hex] | show | dump; files go to Logs");
    }
  }
}

PLUGIN_API void InitializePlugin() {
  AddCommand("/peridotlive", peridotLiveCommand);
}

PLUGIN_API void ShutdownPlugin() {
  RemoveCommand("/peridotlive");
}

PLUGIN_API void OnPulse() {
  if (!pendingLink) {
    return;
  }
  if (ItemClient *item = displayedItem(pendingLink->itemId)) {
    report(item, fmt::format("link hash {:08X}", pendingLink->hash));
    pendingLink.reset();
    return;
  }
  if (std::chrono::steady_clock::now() - pendingLink->sent > linkWait) {
    WriteChatf("\ar[PeridotLive]\ax no item display opened for %d within %lld seconds: the link with hash %08X was not answered",
      pendingLink->itemId, static_cast<long long>(linkWait.count()), pendingLink->hash);
    pendingLink.reset();
  }
}
