#include "../client.h"
#include "../housing/housing.h"

void command_realestate(Client *c, const Seperator *sep)
{
	housing::handleCommand(c, sep);
}
