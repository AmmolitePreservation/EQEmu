// Reads items' housing fields on EverQuest Live, for Peridot's NPC item content (playerHousing/serverPlan.md, chunk 5).
#include <mq/Plugin.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

PreSetup("MQ2PeridotLive");
PLUGIN_VERSION(0.1);

namespace {
  constexpr auto linkWait = std::chrono::seconds(10);
  constexpr const char *reportFileName = "peridotLiveItems.tsv";

  struct PendingLink {
    int itemId = 0;
    uint32_t hash = 0;
    std::chrono::steady_clock::time_point sent;
  };

  std::optional<PendingLink> pendingLink;

  std::string linkData(int itemId, uint32_t hash) {
    std::string data = fmt::format("{:05X}", itemId);
    for (int socket = 0; socket < MAX_AUG_SOCKETS; ++socket) {
      data += "0000000000";
    }
    data += "0" "0000" "00" "00000" "00000";
    data += fmt::format("{:08X}", hash);
    return data;
  }

  void report(ItemClient *item, const std::string &source) {
    const ItemDefinition *definition = item->GetItemDefinition();
    const std::string housepetFileName(definition->HousepetFileName, strnlen(definition->HousepetFileName, sizeof(definition->HousepetFileName)));
    WriteChatf("\ag[PeridotLive]\ax %d %s: housepet '\ay%s\ax', respawn %d, definition %d, upkeep %d, max %d, placeable 0x%X, placement %d, trophy %d, interactive %d",
      item->GetID(), item->GetName(), housepetFileName.c_str(), definition->NpcRespawnInterval, definition->RealEstateDefID,
      definition->RealEstateUpkeepID, definition->MaxPerRealEstate, definition->Placeable, definition->PlacementType,
      definition->TrophyBenefitID, definition->bInteractiveObject ? 1 : 0);
    const std::filesystem::path reportPath = std::filesystem::path(gPathLogs) / reportFileName;
    const bool isNew = !std::filesystem::exists(reportPath);
    std::ofstream reportFile(reportPath, std::ios::app);
    if (!reportFile) {
      WriteChatf("\ar[PeridotLive]\ax could not write %s", reportPath.string().c_str());
      return;
    }
    if (isNew) {
      reportFile << "itemId\tname\thousepetFileName\tnpcRespawnInterval\trealEstateDefinitionId\trealEstateUpkeepId\tmaxPerRealEstate\tplaceable\tplacementType\ttrophyBenefitId\tinteractiveObject\tsource\n";
    }
    reportFile << item->GetID() << '\t' << item->GetName() << '\t' << housepetFileName << '\t' << definition->NpcRespawnInterval << '\t'
      << definition->RealEstateDefID << '\t' << definition->RealEstateUpkeepID << '\t' << definition->MaxPerRealEstate << '\t' << definition->Placeable << '\t'
      << definition->PlacementType << '\t' << definition->TrophyBenefitID << '\t' << (definition->bInteractiveObject ? 1 : 0) << '\t' << source << '\n';
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
    else {
      WriteChatf("\ag[PeridotLive]\ax /peridotlive link <itemID> [hash in hex] | /peridotlive show; reports go to Logs\\%s", reportFileName);
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
