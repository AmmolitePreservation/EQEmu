#pragma once

#include "../common/item_location.h"
#include "../common/types.h"

#include <vector>

class Zone;

// The Recovery tab's two rules, read when the zone loads them and checked before they are used:
// World:RecoverableItemStates names the retirement states a merchant lists, World:RecoveryCurrency the
// alternate currency one recovery costs.
class ItemRecovery {
public:
	// Reads both rules into place; false, with the reason logged and nothing changed, when a state name is
	// unknown, stacked is named or the currency is not one the zone loaded.
	static bool LoadRules(Zone &zone);

	static const std::vector<ItemRetirement> &States() { return s_states; }
	static uint32 Currency() { return s_currency; }

private:
	static std::vector<ItemRetirement> s_states;
	static uint32                      s_currency;
};
