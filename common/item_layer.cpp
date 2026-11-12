#include "item_layer.h"
#include "eqemu_logsys.h"
#include "evolving_items.h"
#include "misc_functions.h"
#include "repositories/character_data_repository.h"
#include "repositories/inventory_snapshots_repository.h"
#include "repositories/item_instances_repository.h"
#include "repositories/trader_repository.h"
#include "shareddb.h"
#include "strings.h"
#include <algorithm>
#include <array>
#include <map>
#include <set>

namespace {

using Row = ItemInstancesRepository::ItemInstances;

Row RowFor(const EQ::ItemInstance &inst, const ItemLocation &location)
{
	Row row{};

	row.guid                = inst.GetGUID();
	row.item_id             = inst.GetID();
	row.charges             = inst.GetCharges();
	row.color               = inst.GetColor();
	row.attuned             = inst.IsAttuned();
	row.custom_data         = inst.GetCustomDataString();
	row.ornament_icon       = inst.GetOrnamentationIcon();
	row.ornament_idfile     = inst.GetOrnamentationIDFile();
	row.ornament_hero_model = inst.GetOrnamentHeroModel();
	row.location            = location;

	return row;
}

std::string Describe(const EQ::ItemInstance &inst)
{
	return fmt::format(
		"item [{}] guid [{} {} {}]",
		inst.GetID(),
		inst.GetGUID().first,
		inst.GetGUID().second,
		inst.GetGUID().third
	);
}

std::string Describe(const ItemLocation &location)
{
	if (location.parent_guid) {
		return fmt::format(
			"parent [{} {} {}] index [{}]",
			location.parent_guid->first,
			location.parent_guid->second,
			location.parent_guid->third,
			location.parent_index
		);
	}

	if (!location.holder_type) {
		return "no location";
	}

	return fmt::format(
		"holder [{}] [{}] slot [{}]",
		ItemHolderTypeName(*location.holder_type),
		location.holder_id,
		location.slot ? std::to_string(*location.slot) : "none"
	);
}

constexpr int issue_attempts = 3;

using GUIDKey = std::array<uint32, 3>;

GUIDKey KeyOf(const ItemGUID &guid)
{
	return {guid.first, guid.second, guid.third};
}

// One item fits one place: a slot of a holder, or an index under a parent.
std::string PlaceOf(const ItemLocation &location)
{
	if (location.parent_guid) {
		return fmt::format(
			"parent {} {} {} {}",
			location.parent_guid->first,
			location.parent_guid->second,
			location.parent_guid->third,
			location.parent_index
		);
	}

	return fmt::format(
		"holder {} {} {}",
		ItemHolderTypeName(*location.holder_type),
		location.holder_id,
		location.slot ? *location.slot : 0
	);
}

struct SnapshotPlacement {
	Row                         row;
	std::optional<ItemLocation> current;
	bool                        reissued;
};

bool PlaceSnapshot(Database &db, uint32 character_id, const ItemLayer::Owner &owner, const std::vector<Row> &snapshot)
{
	std::set<GUIDKey> in_snapshot;
	for (const auto &s : snapshot) {
		in_snapshot.insert(KeyOf(s.guid));
	}

	// Items gained since the snapshot leave, as EQEmu's restore deleted every inventory row first.
	std::map<GUIDKey, ItemLocation> held_at;
	std::vector<ItemGUID>           gained;
	for (const auto &h : ItemInstancesRepository::GetHeld(db, InventorySnapshotsRepository::HolderFilter(character_id))) {
		if (in_snapshot.contains(KeyOf(h.guid))) {
			held_at[KeyOf(h.guid)] = h.location;
		}
		else {
			gained.push_back(h.guid);
		}
	}

	if (!gained.empty() && ItemInstancesRepository::RetireKeys(db, gained, ItemRetirement::RolledBack) != static_cast<int>(gained.size())) {
		LogError("Restoring a snapshot of character [{}] could not retire the [{}] items gained since", character_id, gained.size());
		return false;
	}

	// Each snapshot item keeps its row if the character holds it or it was retired; one someone else holds now is made
	// again with a new GUID, as EQEmu's restore made it again.
	std::map<GUIDKey, ItemGUID> restored_guid;
	std::set<GUIDKey>           reissued;
	std::optional<uint32>       next_counter;
	for (const auto &s : snapshot) {
		const auto key = KeyOf(s.guid);
		if (held_at.contains(key)) {
			restored_guid[key] = s.guid;
			continue;
		}

		const auto current = ItemInstancesRepository::FindOne(db, s.guid);
		if (!current) {
			LogError("Snapshot of character [{}] names guid [{} {} {}], which has no row", character_id, s.guid.first, s.guid.second, s.guid.third);
			return false;
		}

		if (current->retirement_state) {
			restored_guid[key] = s.guid;
			continue;
		}

		if (!next_counter) {
			next_counter = ItemInstancesRepository::NextCounter(db, owner.character_id, owner.origin_world_id);
		}

		if (!next_counter || *next_counter == std::numeric_limits<uint32>::max()) {
			LogError("No next counter for character [{}] world [{}]", owner.character_id, owner.origin_world_id);
			return false;
		}

		const ItemGUID guid{owner.character_id, owner.origin_world_id, (*next_counter)++};
		restored_guid[key] = guid;
		reissued.insert(key);

		LogInfo(
			"Restoring a snapshot of character [{}] made item [{}] guid [{} {} {}] again as guid [{} {} {}]; the original is at {}",
			character_id,
			s.item_id,
			s.guid.first,
			s.guid.second,
			s.guid.third,
			guid.first,
			guid.second,
			guid.third,
			Describe(current->location)
		);
	}

	std::vector<SnapshotPlacement> pending;
	std::set<GUIDKey>              missing_parents;
	for (auto row : snapshot) {
		const auto key = KeyOf(row.guid);

		if (row.location.parent_guid) {
			const auto parent = restored_guid.find(KeyOf(*row.location.parent_guid));
			if (parent == restored_guid.end()) {
				LogError("Snapshot of character [{}] holds guid [{} {} {}] under a parent it does not hold", character_id, row.guid.first, row.guid.second, row.guid.third);
				return false;
			}

			row.location.parent_guid = parent->second;
		}

		std::optional<ItemLocation> current;
		if (const auto h = held_at.find(key); h != held_at.end()) {
			current = h->second;
		}

		row.guid = restored_guid.at(key);
		if (reissued.contains(key)) {
			missing_parents.insert(KeyOf(row.guid));
		}

		pending.push_back(SnapshotPlacement{row, current, reissued.contains(key)});
	}

	std::set<std::string> occupied;
	for (const auto &[key, location] : held_at) {
		occupied.insert(PlaceOf(location));
	}

	// Rows swapping places wait on each other; when nothing can move, one steps aside to the in-transit slot.
	const auto transit = ItemLocation::InTransit(character_id);
	while (!pending.empty()) {
		bool placed = false;
		for (auto p = pending.begin(); p != pending.end();) {
			const auto target         = PlaceOf(p->row.location);
			const bool at_target      = p->current && PlaceOf(*p->current) == target;
			const bool parent_missing = p->row.location.parent_guid && missing_parents.contains(KeyOf(*p->row.location.parent_guid));
			if (parent_missing || (!at_target && occupied.contains(target))) {
				++p;
				continue;
			}

			const bool written = p->reissued ?
				ItemInstancesRepository::InsertOne(db, p->row).Success() :
				ItemInstancesRepository::UpdateOne(db, p->row);
			if (!written) {
				LogError("Restoring a snapshot of character [{}] could not place guid [{} {} {}] at {}", character_id, p->row.guid.first, p->row.guid.second, p->row.guid.third, Describe(p->row.location));
				return false;
			}

			if (p->current) {
				occupied.erase(PlaceOf(*p->current));
			}

			occupied.insert(target);
			missing_parents.erase(KeyOf(p->row.guid));
			p      = pending.erase(p);
			placed = true;
		}

		if (placed) {
			continue;
		}

		auto aside = std::ranges::find_if(pending, [&](const SnapshotPlacement &p) { return p.current && PlaceOf(*p.current) != PlaceOf(transit); });
		if (aside == pending.end() || occupied.contains(PlaceOf(transit)) || !ItemInstancesRepository::UpdateLocation(db, aside->row.guid, transit)) {
			LogError("Restoring a snapshot of character [{}] could not place [{}] items", character_id, pending.size());
			return false;
		}

		occupied.erase(PlaceOf(*aside->current));
		occupied.insert(PlaceOf(transit));
		aside->current = transit;
	}

	return true;
}

}

std::string ItemLayer::HolderFilter(ItemHolderType holder_type, uint32 holder_id)
{
	return fmt::format("holder_type = '{}' AND holder_id = {}", ItemHolderTypeName(holder_type), holder_id);
}

std::string ItemLayer::GUIDFilter(const ItemGUID &guid)
{
	return ItemInstancesRepository::KeyFilter(guid);
}

EQ::ItemInstance *ItemLayer::LoadOne(Database &db, SharedDatabase &content_db, const ItemGUID &guid)
{
	auto held = Load(db, content_db, GUIDFilter(guid));
	if (held.size() != 1) {
		LogError("guid [{} {} {}] loaded [{}] held items, not 1", guid.first, guid.second, guid.third, held.size());
		for (auto &h : held) {
			safe_delete(h.inst);
		}
		return nullptr;
	}

	return held.front().inst;
}

bool ItemLayer::SetCharges(Database &db, const ItemGUID &guid, int16 charges)
{
	auto results = db.QueryDatabase(
		fmt::format(
			"UPDATE {} SET charges = {} WHERE {} AND retirement_state IS NULL",
			ItemInstancesRepository::TableName(),
			charges,
			ItemInstancesRepository::KeyFilter(guid)
		)
	);
	if (!results.Success()) {
		LogError("Failed to set charges [{}] on guid [{} {} {}]", charges, guid.first, guid.second, guid.third);
		return false;
	}

	return true;
}

std::optional<ItemLayer::Owner> ItemLayer::OwnerOf(Database &db, uint32 character_id)
{
	const auto c = CharacterDataRepository::FindOne(db, character_id);
	if (!c.id) {
		LogError("Character [{}] does not exist, so nothing can be issued from its counter", character_id);
		return std::nullopt;
	}

	return Owner{c.id, static_cast<uint16>(c.origin_world_id)};
}

// After a restore the snapshot's holders hold exactly what the snapshot held, in one transaction.
bool ItemLayer::RestoreSnapshot(Database &db, uint32 character_id, uint32 time_index)
{
	const auto owner = OwnerOf(db, character_id);
	if (!owner) {
		return false;
	}

	const auto snapshot = InventorySnapshotsRepository::GetItemRows(db, character_id, time_index);
	if (snapshot.empty()) {
		LogError("Character [{}] has no snapshot at [{}]", character_id, time_index);
		return false;
	}

	DBcore::Transaction transaction(db);
	if (!PlaceSnapshot(db, character_id, *owner, snapshot)) {
		return false;
	}

	return transaction.Commit();
}

// An item with augments or contents is several rows, written as one.
bool ItemLayer::HasChildren(const EQ::ItemInstance &inst)
{
	for (const auto &[index, child] : inst.m_contents) {
		if (child) {
			return true;
		}
	}

	return false;
}

bool ItemLayer::Save(SharedDatabase &db, EQ::ItemInstance &inst, const ItemLocation &location, const std::optional<Owner> &owner)
{
	if (!inst.GetItem()) {
		LogError("Cannot save an item instance without item data at {}", Describe(location));
		return false;
	}

	std::optional<DBcore::Transaction> tree;
	if (HasChildren(inst)) {
		tree.emplace(db);
	}

	if (!inst.HasGUID()) {
		if (!owner) {
			LogError("Item [{}] has no GUID and no owner to issue one at {}", inst.GetID(), Describe(location));
			return false;
		}

		if (!Issue(db, inst, location, *owner)) {
			return false;
		}
	} else if (!ItemInstancesRepository::UpdateOne(db, RowFor(inst, location))) {
		LogError("Failed to update {} at {}", Describe(inst), Describe(location));
		return false;
	}

	if (!SaveChildren(db, inst, owner)) {
		return false;
	}

	return !tree || tree->Commit();
}
bool ItemLayer::Issue(SharedDatabase &db, EQ::ItemInstance &inst, const ItemLocation &location, const Owner &owner)
{
	for (int attempt = 1; attempt <= issue_attempts; ++attempt) {
		const auto next = ItemInstancesRepository::NextCounter(db, owner.character_id, owner.origin_world_id);
		if (!next) {
			LogError("No next counter for character [{}] world [{}]", owner.character_id, owner.origin_world_id);
			return false;
		}

		inst.m_guid = ItemGUID{owner.character_id, owner.origin_world_id, *next};

		auto results = ItemInstancesRepository::InsertOne(db, RowFor(inst, location));
		if (results.Success()) {
			return true;
		}

		const bool counter_collision = results.ErrorNumber() == 1062 && Strings::Contains(results.ErrorMessage(), "PRIMARY");
		inst.m_guid = ItemGUID{};

		if (!counter_collision) {
			LogError("Failed to issue item [{}] at {}: {}", inst.GetID(), Describe(location), results.ErrorMessage());
			return false;
		}

		LogWarning(
			"Counter collision issuing item [{}] for character [{}] world [{}] at [{}], attempt [{}] of [{}]",
			inst.GetID(),
			owner.character_id,
			owner.origin_world_id,
			*next,
			attempt,
			issue_attempts
		);
	}

	LogError("Gave up issuing item [{}] for character [{}] after [{}] counter collisions", inst.GetID(), owner.character_id, issue_attempts);
	return false;
}

bool ItemLayer::SaveChildren(SharedDatabase &db, EQ::ItemInstance &inst, const std::optional<Owner> &owner)
{
	for (auto &[index, child] : inst.m_contents) {
		if (!child) {
			continue;
		}

		if (!Save(db, *child, ItemLocation::InParent(inst.GetGUID(), index), owner)) {
			return false;
		}
	}

	return true;
}

bool ItemLayer::Update(Database &db, const EQ::ItemInstance &inst, const ItemLocation &location)
{
	if (!inst.HasGUID()) {
		LogError("Cannot update item [{}] at {}: it has no GUID", inst.GetID(), Describe(location));
		return false;
	}

	std::optional<DBcore::Transaction> tree;
	if (HasChildren(inst)) {
		tree.emplace(db);
	}

	if (!ItemInstancesRepository::UpdateOne(db, RowFor(inst, location))) {
		LogError("Failed to update {} at {}", Describe(inst), Describe(location));
		return false;
	}

	for (const auto &[index, child] : inst.m_contents) {
		if (child && !Update(db, *child, ItemLocation::InParent(inst.GetGUID(), index))) {
			return false;
		}
	}

	return !tree || tree->Commit();
}
bool ItemLayer::MoveRow(Database &db, const ItemGUID &guid, const ItemLocation &location)
{
	if (!ItemInstancesRepository::UpdateLocation(db, guid, location)) {
		LogError("Failed to move guid [{} {} {}] to {}", guid.first, guid.second, guid.third, Describe(location));
		return false;
	}

	return true;
}

bool ItemLayer::MoveRows(Database &db, uint32 character_id, std::vector<RowMove> moves)
{
	std::set<std::string> occupied;
	for (const auto &move : moves) {
		occupied.insert(PlaceOf(move.from));
	}

	const auto transit = ItemLocation::InTransit(character_id);
	while (!moves.empty()) {
		bool placed = false;
		for (auto move = moves.begin(); move != moves.end();) {
			const auto target = PlaceOf(move->to);
			if (PlaceOf(move->from) != target && occupied.contains(target)) {
				++move;
				continue;
			}

			if (!MoveRow(db, move->guid, move->to)) {
				return false;
			}

			occupied.erase(PlaceOf(move->from));
			occupied.insert(target);
			move   = moves.erase(move);
			placed = true;
		}

		if (placed) {
			continue;
		}

		auto aside = std::ranges::find_if(moves, [&](const RowMove &move) { return PlaceOf(move.from) != PlaceOf(transit); });
		if (aside == moves.end() || occupied.contains(PlaceOf(transit)) || !MoveRow(db, aside->guid, transit)) {
			LogError("Could not move [{}] rows of character [{}] that wait on each other", moves.size(), character_id);
			return false;
		}

		occupied.erase(PlaceOf(aside->from));
		occupied.insert(PlaceOf(transit));
		aside->from = transit;
	}

	return true;
}

bool ItemLayer::Retire(Database &db, const EQ::ItemInstance &inst, ItemRetirement reason)
{
	if (!inst.HasGUID()) {
		LogError("Cannot retire item [{}] as [{}]: it has no GUID", inst.GetID(), ItemRetirementName(reason));
		return false;
	}

	std::optional<DBcore::Transaction> tree;
	if (HasChildren(inst)) {
		tree.emplace(db);
	}

	if (!RetireTree(db, inst, reason)) {
		return false;
	}

	return !tree || tree->Commit();
}
bool ItemLayer::RetireTree(Database &db, const EQ::ItemInstance &inst, ItemRetirement reason)
{
	if (!RetireRow(db, inst.GetGUID(), reason)) {
		return false;
	}

	for (const auto &[index, child] : inst.m_contents) {
		if (child && !RetireTree(db, *child, reason)) {
			return false;
		}
	}

	return true;
}

bool ItemLayer::RetireRow(Database &db, const ItemGUID &guid, ItemRetirement reason)
{
	const int retired = ItemInstancesRepository::Retire(db, guid, reason);
	if (retired != 1) {
		LogError(
			"Retiring guid [{} {} {}] as [{}] changed [{}] rows",
			guid.first,
			guid.second,
			guid.third,
			ItemRetirementName(reason),
			retired
		);
		return false;
	}

	return true;
}

int ItemLayer::RetireWhere(Database &db, const std::string &holder_filter, ItemRetirement reason)
{
	std::vector<ItemGUID> guids;
	for (const auto &row : ItemInstancesRepository::GetHeld(db, holder_filter)) {
		guids.push_back(row.guid);
	}

	if (guids.empty()) {
		return 0;
	}

	const int retired = ItemInstancesRepository::RetireKeys(db, guids, reason);
	if (retired != static_cast<int>(guids.size())) {
		LogError("Retiring [{}] rows as [{}] for [{}] changed [{}]", guids.size(), ItemRetirementName(reason), holder_filter, retired);
	}

	return retired;
}

int ItemLayer::MoveHolder(Database &db, const std::string &holder_filter, ItemHolderType holder_type)
{
	const int moved = ItemInstancesRepository::UpdateHolderType(db, holder_filter, holder_type);
	if (moved < 0) {
		LogError("Failed to move rows for [{}] to [{}]", holder_filter, ItemHolderTypeName(holder_type));
	}

	return moved;
}

int ItemLayer::CopyRows(Database &db, const std::string &holder_filter, const Owner &issuer, uint32 new_holder_id)
{
	const auto base = ItemInstancesRepository::NextCounter(db, issuer.character_id, issuer.origin_world_id);
	if (!base) {
		return -1;
	}

	const auto table = ItemInstancesRepository::TableName();

	// The rows and everything inside them, numbered parents first so the foreign key is met in insertion order.
	std::vector<std::string> statements = {
		"DROP TEMPORARY TABLE IF EXISTS item_copy_map",
		"CREATE TEMPORARY TABLE item_copy_map (f INT UNSIGNED, s INT UNSIGNED, t INT UNSIGNED, n INT UNSIGNED, PRIMARY KEY (f, s, t))",
		fmt::format(
			"INSERT INTO item_copy_map WITH RECURSIVE held AS ("
			"SELECT guid_first, guid_second, guid_third, 0 AS depth FROM {table} WHERE retirement_state IS NULL AND ({filter}) "
			"UNION ALL SELECT c.guid_first, c.guid_second, c.guid_third, held.depth + 1 FROM {table} c JOIN held "
			"ON c.parent_guid_first = held.guid_first AND c.parent_guid_second = held.guid_second AND c.parent_guid_third = held.guid_third "
			"WHERE c.retirement_state IS NULL) "
			"SELECT guid_first, guid_second, guid_third, ROW_NUMBER() OVER (ORDER BY depth, guid_first, guid_second, guid_third) - 1 + {base} FROM held",
			fmt::arg("table", table),
			fmt::arg("filter", holder_filter),
			fmt::arg("base", *base)
		),
		fmt::format(
			"INSERT INTO {table} (guid_first, guid_second, guid_third, item_id, charges, color, attuned, custom_data, ornament_icon, "
			"ornament_idfile, ornament_hero_model, holder_type, holder_id, slot, parent_guid_first, parent_guid_second, parent_guid_third, parent_index) "
			"SELECT {first}, {second}, m.n, i.item_id, i.charges, i.color, i.attuned, i.custom_data, i.ornament_icon, i.ornament_idfile, "
			"i.ornament_hero_model, i.holder_type, IF(i.holder_type IS NULL, NULL, {holder}), i.slot, "
			"IF(i.parent_guid_first IS NULL, NULL, {first}), IF(i.parent_guid_first IS NULL, NULL, {second}), pm.n, i.parent_index "
			"FROM {table} i JOIN item_copy_map m ON i.guid_first = m.f AND i.guid_second = m.s AND i.guid_third = m.t "
			"LEFT JOIN item_copy_map pm ON i.parent_guid_first = pm.f AND i.parent_guid_second = pm.s AND i.parent_guid_third = pm.t "
			"ORDER BY m.n",
			fmt::arg("table", table),
			fmt::arg("first", issuer.character_id),
			fmt::arg("second", issuer.origin_world_id),
			fmt::arg("holder", new_holder_id)
		),
	};

	int copied = 0;
	for (size_t i = 0; i < statements.size(); ++i) {
		auto results = db.QueryDatabase(statements[i]);
		if (!results.Success()) {
			LogError("Copying rows for [{}] to [{}] failed: {}", holder_filter, new_holder_id, results.ErrorMessage());
			db.QueryDatabase("DROP TEMPORARY TABLE IF EXISTS item_copy_map");
			return -1;
		}

		if (i == statements.size() - 1) {
			copied = static_cast<int>(results.RowsAffected());
		}
	}

	db.QueryDatabase("DROP TEMPORARY TABLE IF EXISTS item_copy_map");

	return copied;
}

// The filter's rows are the tops of what is loaded, with their contents and augments under them; a top may itself sit
// in a parent the filter did not take, as a listed item sits in its trader's satchel.
namespace {

struct ContentItem {
	bool   answered = false;
	bool   exists   = false;
	uint32 item_class = 0;
	uint32 bag_slots  = 0;
	uint32 item_type  = 0;
};

// The item store can be stale (a zone up during a #hotfix keeps the item data it booted with) or not loaded at all, so a
// row is retired only when the items table itself agrees; a server whose content lives in another database cannot
// answer here, and its rows are skipped as before.
ContentItem ReadContentItem(Database &db, uint32 item_id)
{
	ContentItem c;
	auto        results = db.QueryDatabase(fmt::format("SELECT itemclass, bagslots, itemtype FROM items WHERE id = {}", item_id));
	if (!results.Success()) {
		return c;
	}

	c.answered = true;
	for (auto row : results) {
		c.exists     = true;
		c.item_class = Strings::ToUnsignedInt(row[0]);
		c.bag_slots  = Strings::ToUnsignedInt(row[1]);
		c.item_type  = Strings::ToUnsignedInt(row[2]);
	}

	return c;
}

bool Holds(uint32 parent_class, uint32 parent_bag_slots, uint32 child_type, uint32 index)
{
	if (parent_class == EQ::item::ItemClassBag) {
		return index < parent_bag_slots;
	}

	return parent_class == EQ::item::ItemClassCommon && child_type == EQ::item::ItemTypeAugmentation &&
		index >= EQ::invaug::SOCKET_BEGIN && index <= EQ::invaug::SOCKET_END;
}

}

// A row whose item number content does not have retires missingItemRow, a row its parent cannot hold (past a bag's last
// slot, or under a non-bag without being an augment in a socket) retires unplaceable, and so do the rows under either.
static EQ::ItemInstance *Instantiate(Database &db, SharedDatabase &content_db, const EQ::ItemData *item, const ItemInstancesRepository::ItemInstances &row)
{
	auto *inst = content_db.CreateBaseItem(item, row.charges);
	if (!inst) {
		return nullptr;
	}

	ItemLayer::AssignGUID(*inst, row.guid);
	if (item->EvolvingItem) {
		EvolvingItemsManager::Instance()->FindRecord(db, *inst);
	}
	if (!row.custom_data.empty()) {
		inst->SetCustomDataString(row.custom_data);
	}
	inst->SetOrnamentIcon(row.ornament_icon);
	inst->SetOrnamentationIDFile(row.ornament_idfile);
	inst->SetOrnamentHeroModel(row.ornament_hero_model);
	inst->SetAttuned(row.attuned);
	if (row.color > 0) {
		inst->SetColor(row.color);
	}
	// A stackable item needs a charge to stay movable.
	if (row.charges == 0 && inst->IsStackable()) {
		inst->SetCharges(1);
	}
	return inst;
}

std::vector<ItemLayer::Held> ItemLayer::Load(Database &db, SharedDatabase &content_db, const std::string &holder_filter)
{
	std::vector<Held> held;

	std::map<std::array<uint32, 3>, EQ::ItemInstance *> by_guid;
	std::set<std::array<uint32, 3>>                     skipped;
	std::set<std::array<uint32, 3>>                     retired;

	auto set_aside = [&](const ItemInstancesRepository::ItemInstances &row, bool confirmed, ItemRetirement reason, const std::string &why) {
		skipped.insert({row.guid.first, row.guid.second, row.guid.third});
		if (!confirmed) {
			LogError("guid [{} {} {}] item [{}] {}; content does not confirm it, skipped", row.guid.first, row.guid.second, row.guid.third, row.item_id, why);
			return;
		}

		if (ItemInstancesRepository::Retire(db, row.guid, reason) != 1) {
			LogError("guid [{} {} {}] item [{}] {}; retiring it failed, skipped", row.guid.first, row.guid.second, row.guid.third, row.item_id, why);
			return;
		}

		retired.insert({row.guid.first, row.guid.second, row.guid.third});
		LogError(
			"guid [{} {} {}] item [{}] {}; retired [{}]",
			row.guid.first,
			row.guid.second,
			row.guid.third,
			row.item_id,
			why,
			ItemRetirementName(reason)
		);
	};

	for (const auto &row : ItemInstancesRepository::GetHeld(db, holder_filter)) {
		if (row.location.parent_guid) {
			const auto &p = *row.location.parent_guid;
			if (skipped.contains({p.first, p.second, p.third})) {
				set_aside(
					row,
					retired.contains({p.first, p.second, p.third}),
					ItemRetirement::Unplaceable,
					fmt::format("sits under [{} {} {}], which was set aside", p.first, p.second, p.third)
				);
				continue;
			}
		}

		const auto *item = content_db.GetItem(row.item_id);
		if (!item) {
			const auto c = ReadContentItem(content_db, row.item_id);
			set_aside(row, c.answered && !c.exists, ItemRetirement::MissingItemRow, "names an item content does not have");
			continue;
		}

		if (row.location.parent_guid) {
			const auto &p      = *row.location.parent_guid;
			auto        parent = by_guid.find({p.first, p.second, p.third});
			const auto  index  = static_cast<uint32>(row.location.parent_index);
			if (parent != by_guid.end()) {
				const auto *parent_item = parent->second->GetItem();
				if (!Holds(parent_item->ItemClass, parent_item->BagSlots, item->ItemType, index)) {
					const auto pc = ReadContentItem(content_db, parent_item->ID);
					const auto cc = ReadContentItem(content_db, row.item_id);
					set_aside(
						row,
						pc.answered && cc.answered && pc.exists && cc.exists && !Holds(pc.item_class, pc.bag_slots, cc.item_type, index),
						ItemRetirement::Unplaceable,
						fmt::format("cannot be held at index [{}] of item [{}]", index, parent_item->ID)
					);
					continue;
				}
			}
		}

		auto *inst = Instantiate(db, content_db, item, row);
		if (!inst) {
			continue;
		}
		by_guid[{row.guid.first, row.guid.second, row.guid.third}] = inst;
		if (row.location.parent_guid) {
			const auto &p    = *row.location.parent_guid;
			auto        find = by_guid.find({p.first, p.second, p.third});
			if (find != by_guid.end()) {
				find->second->_PutItem(static_cast<uint8>(row.location.parent_index), inst);
				continue;
			}
		}

		held.push_back(Held{row.location, inst});
	}

	return held;
}

std::vector<ItemLayer::Recoverable> ItemLayer::LoadRecoverable(Database &db, SharedDatabase &content_db, uint32 character_id, const std::vector<ItemRetirement> &states, const std::optional<ItemGUID> &only)
{
	std::vector<Recoverable> listed;
	if (states.empty()) {
		return listed;
	}

	std::string state_list;
	for (const auto state : states) {
		state_list += fmt::format("{}'{}'", state_list.empty() ? "" : ", ", ItemRetirementName(state));
	}

	std::map<std::array<uint32, 3>, EQ::ItemInstance *> by_guid;
	std::set<std::array<uint32, 3>>                     left_out;

	const auto row_filter = only ? ItemInstancesRepository::KeyFilter(*only) : std::string("1");
	for (const auto &row : ItemInstancesRepository::GetRecoverable(db, character_id, state_list, row_filter)) {
		const std::array<uint32, 3> key{row.guid.first, row.guid.second, row.guid.third};
		if (row.location.parent_guid) {
			const auto &p = *row.location.parent_guid;
			if (left_out.contains({p.first, p.second, p.third})) {
				left_out.insert(key);
				continue;
			}
		}

		const auto *item = content_db.GetItem(row.item_id);
		if (!item) {
			LogError("guid [{} {} {}] item [{}] names an item content does not have; left off the recovery list", row.guid.first, row.guid.second, row.guid.third, row.item_id);
			left_out.insert(key);
			continue;
		}

		auto *inst = Instantiate(db, content_db, item, row);
		if (!inst) {
			left_out.insert(key);
			continue;
		}
		by_guid[key] = inst;

		if (row.location.parent_guid) {
			const auto &p = *row.location.parent_guid;
			by_guid.at({p.first, p.second, p.third})->_PutItem(static_cast<uint8>(row.location.parent_index), inst);
			continue;
		}

		listed.push_back(Recoverable{row.guid, *row.retirement_state, *row.retired_at, inst});
	}

	return listed;
}
void ItemLayer::CarryGUID(const EQ::ItemInstance &from, EQ::ItemInstance &to)
{
	to.m_guid = from.m_guid;
}

void ItemLayer::AssignLootGUIDs(EQ::ItemInstance &inst, const LootItem &loot_item)
{
	inst.m_guid = loot_item.guid;

	for (uint8 socket = 0; socket < EQ::invaug::SOCKET_COUNT; ++socket) {
		auto *augment = inst.GetAugment(socket);
		if (augment) {
			augment->m_guid = loot_item.augment_guids[socket];
		}
	}
}

void ItemLayer::ClearGUID(EQ::ItemInstance &inst)
{
	inst.m_guid = ItemGUID{};
	for (auto &[index, child] : inst.m_contents) {
		if (child) {
			ClearGUID(*child);
		}
	}
}

void ItemLayer::AssignGUID(EQ::ItemInstance &inst, const ItemGUID &guid)
{
	inst.m_guid = guid;
}

std::optional<ItemGUID> ItemLayer::FindGUID(Database &db, const ItemLocation &location)
{
	if (!location.holder_type || !location.slot) {
		return std::nullopt;
	}

	const auto rows = ItemInstancesRepository::GetWhere(
		db,
		fmt::format("{} AND slot = {} AND retirement_state IS NULL", HolderFilter(*location.holder_type, location.holder_id), *location.slot)
	);

	if (rows.size() != 1) {
		LogError("[{}] rows at {}", rows.size(), Describe(location));
		return std::nullopt;
	}

	return rows.front().guid;
}

std::string ItemScope::Text() const
{
	switch (kind) {
		case Kind::ListedBy:
			return fmt::format("the listings of trader [{}]", holder_id);
	}

	return {};
}

void ItemLayer::LogRefusal(std::string_view requester, const ItemScope &scope, const ItemGUID &guid)
{
	LogWarning("Refused item [{}] for [{}]: not among {}", ItemGUIDText(guid), requester, scope.Text());
}

std::unique_ptr<EQ::ItemInstance> ItemLayer::Lookup(Database &db, SharedDatabase &content_db, const ItemScope &scope, const ItemGUID &guid, std::string_view requester)
{
	if (guid.first == 0) {
		LogRefusal(requester, scope, guid);
		return nullptr;
	}

	switch (scope.kind) {
		case ItemScope::Kind::ListedBy: {
			const auto listing = TraderRepository::GetListing(db, scope.holder_id, guid);
			if (!listing.id) {
				LogRefusal(requester, scope, guid);
				return nullptr;
			}

			std::unique_ptr<EQ::ItemInstance> inst(LoadOne(db, content_db, guid));
			if (inst) {
				inst->SetMerchantSlot(static_cast<uint32>(listing.id));
				inst->SetPrice(listing.item_cost);
				if (inst->IsStackable()) {
					inst->SetMerchantCount(inst->GetCharges());
				}
			}

			return inst;
		}
	}

	return nullptr;
}
