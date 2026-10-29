#ifndef EQEMU_CHARACTER_EVOLVING_ITEMS_REPOSITORY_H
#define EQEMU_CHARACTER_EVOLVING_ITEMS_REPOSITORY_H

#include "../database.h"
#include "../strings.h"
#include "base/base_character_evolving_items_repository.h"
#include "../item_guid.h"

#include <boost/tuple/tuple_comparison.hpp>

class CharacterEvolvingItemsRepository: public BaseCharacterEvolvingItemsRepository {
public:
	static ItemGUID GUIDOf(const CharacterEvolvingItems &e)
	{
		return ItemGUID{e.guid_first, e.guid_second, e.guid_third};
	}

	static void SetGUID(CharacterEvolvingItems &e, const ItemGUID &guid)
	{
		e.guid_first  = guid.first;
		e.guid_second = guid.second;
		e.guid_third  = guid.third;
	}

	static CharacterEvolvingItems FindByGUID(Database &db, const ItemGUID &guid)
	{
		const auto l = GetWhere(
			db,
			fmt::format("guid_first = {} AND guid_second = {} AND guid_third = {} LIMIT 1", guid.first, guid.second, guid.third)
		);

		return l.empty() ? NewEntity() : l.front();
	}

	static CharacterEvolvingItems SetCurrentAmountAndProgression(Database& db, const uint64 id, const uint64 amount, const double progression)
	{
		auto e = FindOne(db, id);
		if (e.id == 0) {
			return NewEntity();
		}

		e.current_amount = amount;
		e.progression    = progression;
		UpdateOne(db, e);

		return e;
	}

	static CharacterEvolvingItems SetEquipped(Database& db, const uint64 id, const bool equipped)
	{
		auto e = FindOne(db, id);
		if (e.id == 0) {
			return NewEntity();
		}

		e.equipped = equipped;
		UpdateOne(db, e);

		return e;
	}
};

#endif //EQEMU_CHARACTER_EVOLVING_ITEMS_REPOSITORY_H
