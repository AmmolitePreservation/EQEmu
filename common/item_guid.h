#ifndef EQEMU_ITEM_GUID_H
#define EQEMU_ITEM_GUID_H

#include "types.h"

struct ItemGUID {
	uint32 first{0};
	uint32 second{0};
	uint32 third{0};

	bool IsZero() const { return first == 0 && second == 0 && third == 0; }
	bool operator==(const ItemGUID &other) const { return first == other.first && second == other.second && third == other.third; }
	bool operator!=(const ItemGUID &other) const { return !(*this == other); }
};

#endif //EQEMU_ITEM_GUID_H
