#include "housingUpkeep.h"

#include "../common/eqemu_logsys.h"
#include "../common/rulesys.h"
#include "../common/timer.h"

namespace housing {
  namespace {
    Timer upkeepTimer;
    bool intervalRefused = false;
    bool chargingReported = false;

    void chargeDueUpkeep() {
      if (!chargingReported) {
        LogInfo("Housing upkeep tick is running, but charging is not written yet (playerHousing/serverWork.md S8): nothing is charged");
        chargingReported = true;
      }
    }
  }

  void processUpkeep() {
    if (!RuleB(Housing, EnableHousing) || !RuleB(Housing, EnableUpkeep)) {
      return;
    }
    const int tickSeconds = RuleI(Housing, UpkeepTickSeconds);
    if (tickSeconds <= 0 || tickSeconds > 4294967) {
      if (!intervalRefused) {
        LogError("Housing:UpkeepTickSeconds is [{}]; it must be 1 to 4294967, so upkeep is not running", tickSeconds);
        intervalRefused = true;
      }
      return;
    }
    intervalRefused = false;
    const uint32 intervalMilliseconds = static_cast<uint32>(tickSeconds) * 1000;
    if (!upkeepTimer.Enabled()) {
      upkeepTimer.Start(intervalMilliseconds);
      chargeDueUpkeep();
      return;
    }
    if (upkeepTimer.GetSetAtTrigger() != intervalMilliseconds) {
      upkeepTimer.SetTimer(intervalMilliseconds);
    }
    if (upkeepTimer.Check()) {
      chargeDueUpkeep();
    }
  }
}
