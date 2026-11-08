#include "item_recovery.h"

#include "../common/eqemu_logsys.h"
#include "../common/rulesys.h"
#include "../common/strings.h"
#include "zone.h"

#include <algorithm>
#include <cctype>

std::vector<ItemRetirement> ItemRecovery::s_states;
uint32                      ItemRecovery::s_currency = 0;

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

	const auto currency = static_cast<uint32>(RuleI(World, RecoveryCurrency));
	if (currency && !zone.GetCurrencyItemID(currency)) {
		LogError("Rule World:RecoveryCurrency names alternate currency [{}], which the zone did not load", currency);
		return false;
	}

	s_states   = states;
	s_currency = currency;
	return true;
}
