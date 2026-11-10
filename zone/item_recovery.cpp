#include "item_recovery.h"

#include "../common/eqemu_logsys.h"
#include "../common/rulesys.h"
#include "../common/strings.h"
#include "zone.h"

#include <algorithm>
#include <cctype>

std::vector<ItemRetirement> ItemRecovery::s_states;

bool ItemRecovery::LoadRules(Zone &zone)
{
	std::vector<ItemRetirement> states;
	for (auto name : Strings::Split(RuleS(World, RecoverableItemStates), ',')) {
		std::erase_if(name, [](unsigned char c) { return std::isspace(c); });
		if (name.empty()) {
			continue;
		}

		const auto state = ItemRetirementFromName(name);
		if (!state) {
			LogError("Rule World:RecoverableItemStates names [{}], which is not a retirement state", name);
			return false;
		}

		if (*state == ItemRetirement::Stacked) {
			LogError("Rule World:RecoverableItemStates names stacked, whose units live on in the stack they joined and cannot be recovered");
			return false;
		}

		states.push_back(*state);
	}

	if (!states.empty() && !zone.GetCurrencyItemID(currency)) {
		LogError("Recovery needs alternate currency [{}] (Live's Token of Reclamation), which the zone did not load", currency);
		return false;
	}

	s_states = states;
	return true;
}
