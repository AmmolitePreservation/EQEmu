#include "item_layer.h"
#include "eqemu_logsys.h"
#include "misc_functions.h"
#include "repositories/item_instances_repository.h"
#include "shareddb.h"
#include "strings.h"
#include <array>
#include <map>

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

}

std::string ItemLayer::HolderFilter(ItemHolderType holder_type, uint32 holder_id)
{
	return fmt::format("holder_type = '{}' AND holder_id = {}", ItemHolderTypeName(holder_type), holder_id);
}

bool ItemLayer::Save(SharedDatabase &db, EQ::ItemInstance &inst, const ItemLocation &location, const std::optional<Owner> &owner)
{
	if (!inst.GetItem()) {
		LogError("Cannot save an item instance without item data at {}", Describe(location));
		return false;
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

	return SaveChildren(db, inst, owner);
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

	if (!ItemInstancesRepository::UpdateOne(db, RowFor(inst, location))) {
		LogError("Failed to update {} at {}", Describe(inst), Describe(location));
		return false;
	}

	for (const auto &[index, child] : inst.m_contents) {
		if (child && !Update(db, *child, ItemLocation::InParent(inst.GetGUID(), index))) {
			return false;
		}
	}

	return true;
}

bool ItemLayer::MoveRow(Database &db, const ItemGUID &guid, const ItemLocation &location)
{
	if (!ItemInstancesRepository::UpdateLocation(db, guid, location)) {
		LogError("Failed to move guid [{} {} {}] to {}", guid.first, guid.second, guid.third, Describe(location));
		return false;
	}

	return true;
}

bool ItemLayer::Retire(Database &db, const EQ::ItemInstance &inst, ItemRetirement reason)
{
	if (!inst.HasGUID()) {
		LogError("Cannot retire item [{}] as [{}]: it has no GUID", inst.GetID(), ItemRetirementName(reason));
		return false;
	}

	return RetireTree(db, inst, reason);
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

std::vector<ItemLayer::Held> ItemLayer::Load(Database &db, SharedDatabase &content_db, const std::string &holder_filter)
{
	std::vector<Held> held;

	std::map<std::array<uint32, 3>, EQ::ItemInstance *> by_guid;

	for (const auto &row : ItemInstancesRepository::GetHeld(db, holder_filter)) {
		const auto *item = content_db.GetItem(row.item_id);
		if (!item) {
			LogError(
				"guid [{} {} {}] names item [{}], which does not exist; skipped",
				row.guid.first,
				row.guid.second,
				row.guid.third,
				row.item_id
			);
			continue;
		}

		auto *inst = content_db.CreateBaseItem(item, row.charges);
		if (!inst) {
			continue;
		}

		inst->m_guid = row.guid;

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

		by_guid[{row.guid.first, row.guid.second, row.guid.third}] = inst;

		if (row.location.parent_guid) {
			const auto &p    = *row.location.parent_guid;
			auto        find = by_guid.find({p.first, p.second, p.third});
			if (find == by_guid.end()) {
				LogError(
					"guid [{} {} {}] names parent [{} {} {}], which was not loaded; skipped",
					row.guid.first,
					row.guid.second,
					row.guid.third,
					p.first,
					p.second,
					p.third
				);
				safe_delete(inst);
				continue;
			}

			find->second->_PutItem(static_cast<uint8>(row.location.parent_index), inst);
			continue;
		}

		held.push_back(Held{row.location, inst});
	}

	return held;
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
