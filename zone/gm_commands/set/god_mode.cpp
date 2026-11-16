#include "../../client.h"
#include "../../../common/repositories/account_repository.h"

void SetGodMode(Client *c, const Seperator *sep)
{
	const auto arguments = sep->argnum;
	if (arguments < 2) {
		c->Message(Chat::White, "Usage: #set god_mode [on|off]");
		return;
	}

	auto t = c;
	if (c->GetTarget() && c->GetTarget()->IsClient()) {
		t = c->GetTarget()->CastToClient();
	}

	const bool   god_mode   = Strings::ToBool(sep->arg[2]);
	const uint32 account_id = c->AccountID();

	c->SetInvul(god_mode);
	c->SendAppearancePacket(AppearanceType::FlyMode, god_mode);
	c->SetHideMe(god_mode);

	c->Message(
		Chat::White,
		fmt::format(
			"Turning God Mode {} for {}, zone for GM Speed to take effect.",
			god_mode ? "on" : "off",
			c->GetTargetDescription(t)
		).c_str()
	);

	const std::string value = god_mode ? "1" : "0";
	AccountRepository::SetColumns(
		database,
		c->AccountID(),
		{{"flymode", value}, {"gmspeed", value}, {"invulnerable", value}, {"hideme", value}}
	);
}
