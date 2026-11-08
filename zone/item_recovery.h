#pragma once

#include "../common/item_location.h"
#include "../common/types.h"

#include <vector>

class Zone;

// The Recovery tab's rule, World:RecoverableItemStates, read when the zone loads its rules and checked before it is
// used, and the currency a recovery costs. The RoF2 client checks the character's Tokens of Reclamation (alternate
// currency 20) before it sends a request and names them in its reply message (eqgame.exe FUN_006f6090,
// FUN_006eea30), so the server charges that currency and no rule can choose another.
class ItemRecovery {
public:
	static constexpr uint32 currency = 20;

	// Reads the rule into place; false, with the reason logged and nothing changed, when a state name is unknown,
	// stacked is named or the zone did not load the currency.
	static bool LoadRules(Zone &zone);

	static const std::vector<ItemRetirement> &States() { return s_states; }

private:
	static std::vector<ItemRetirement> s_states;
};
