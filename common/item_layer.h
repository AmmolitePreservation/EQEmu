#ifndef EQEMU_ITEM_LAYER_H
#define EQEMU_ITEM_LAYER_H

#include "item_instance.h"
#include "item_location.h"
#include "loot.h"
#include <optional>
#include <string>
#include <vector>

class Database;
class SharedDatabase;

// The one path that issues, moves, updates and retires item instances in item_instances. Every holder loads and
// saves its items through it; nothing else writes the table.
class ItemLayer {
public:
	// The character an item lands with; its counter issues the item's GUID.
	struct Owner {
		uint32 character_id;
		uint16 origin_world_id;
	};

	struct Held {
		ItemLocation      location;
		EQ::ItemInstance *inst;
	};

	// Saves an item where it now is, with its augments and bag contents under it: an item without a GUID is issued
	// from the owner's counter, one with a GUID has its row's location and fields updated.
	static bool Save(SharedDatabase &db, EQ::ItemInstance &inst, const ItemLocation &location, const std::optional<Owner> &owner);

	// Updates the rows of an item and everything inside it where they now are; every one must already have a GUID.
	static bool Update(Database &db, const EQ::ItemInstance &inst, const ItemLocation &location);

	// Moves a row whose instance is not in memory.
	static bool MoveRow(Database &db, const ItemGUID &guid, const ItemLocation &location);

	// Retires an item and everything inside it; the row keeps its last location.
	static bool Retire(Database &db, const EQ::ItemInstance &inst, ItemRetirement reason);
	static bool RetireRow(Database &db, const ItemGUID &guid, ItemRetirement reason);

	// Retires every row in play that a holder filter matches, and everything inside those rows. Returns the count.
	static int RetireWhere(Database &db, const std::string &holder_filter, ItemRetirement reason);

	// Moves every row in play that a holder filter matches to another holder type (a pet's items when it is suspended).
	static int MoveHolder(Database &db, const std::string &holder_filter, ItemHolderType holder_type);

	// Copies every row in play that a holder filter matches, with everything inside, as new items issued from the
	// issuer's counter in a fixed order, held by new_holder_id (a copied character, a cloned bot). Runs inside the
	// caller's transaction. Returns the count, or -1.
	static int CopyRows(Database &db, const std::string &holder_filter, const Owner &issuer, uint32 new_holder_id);

	// The items a holder filter matches, each with its augments and bag contents attached and its GUID set; the rows
	// come from db, the item data from content_db (the same database in a zone, the content database in world).
	static std::vector<Held> Load(Database &db, SharedDatabase &content_db, const std::string &holder_filter);

	static std::string HolderFilter(ItemHolderType holder_type, uint32 holder_id);
	static std::string GUIDFilter(const ItemGUID &guid);
	static EQ::ItemInstance *LoadOne(Database &db, SharedDatabase &content_db, const ItemGUID &guid);
	static bool SetCharges(Database &db, const ItemGUID &guid, int16 charges);
	static std::optional<Owner> OwnerOf(Database &db, uint32 character_id);
	static bool RestoreSnapshot(Database &db, uint32 character_id, uint32 time_index);

	// The same item in a new instance: an evolving item's next level, a bag replaced by its disenchanted kind.
	static void CarryGUID(const EQ::ItemInstance &from, EQ::ItemInstance &to);

	// An instance rebuilt from a corpse's loot entry takes the entry's GUIDs, its augments included.
	static void AssignLootGUIDs(EQ::ItemInstance &inst, const LootItem &loot_item);

	// A copy that becomes a new item, such as a split piece, is issued afresh when it lands.
	static void ClearGUID(EQ::ItemInstance &inst);

	// An instance rebuilt from a side table takes the GUID of the row held at its location.
	static void AssignGUID(EQ::ItemInstance &inst, const ItemGUID &guid);
	static std::optional<ItemGUID> FindGUID(Database &db, const ItemLocation &location);

private:
	static bool Issue(SharedDatabase &db, EQ::ItemInstance &inst, const ItemLocation &location, const Owner &owner);
	static bool SaveChildren(SharedDatabase &db, EQ::ItemInstance &inst, const std::optional<Owner> &owner);
	static bool RetireTree(Database &db, const EQ::ItemInstance &inst, ItemRetirement reason);
};

#endif //EQEMU_ITEM_LAYER_H
