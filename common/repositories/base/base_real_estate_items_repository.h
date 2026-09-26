/**
 * DO NOT MODIFY THIS FILE
 *
 * This repository was automatically generated and is NOT to be modified directly.
 * Any repository modifications are meant to be made to the repository extending the base.
 * Any modifications to base repositories are to be made by the generator only
 *
 * @generator ./utils/scripts/generators/repository-generator.pl
 * @docs https://docs.eqemu.io/developer/repositories
 */

#ifndef EQEMU_BASE_REAL_ESTATE_ITEMS_REPOSITORY_H
#define EQEMU_BASE_REAL_ESTATE_ITEMS_REPOSITORY_H

#include "../../database.h"
#include "../../strings.h"
#include <ctime>

class BaseRealEstateItemsRepository {
public:
	struct RealEstateItems {
		uint32_t    id;
		uint32_t    property_id;
		uint32_t    owner_character_id;
		uint8_t     placed;
		float       heading;
		float       pitch;
		float       roll;
		float       scale;
		float       x;
		float       y;
		float       z;
		uint32_t    item_id;
		uint16_t    charges;
		uint32_t    color;
		uint32_t    augment_one;
		uint32_t    augment_two;
		uint32_t    augment_three;
		uint32_t    augment_four;
		uint32_t    augment_five;
		uint32_t    augment_six;
		uint8_t     instnodrop;
		std::string custom_data;
		uint32_t    ornament_icon;
		uint32_t    ornament_idfile;
		int32_t     ornament_hero_model;
		uint64_t    guid;
	};

	static std::string PrimaryKey()
	{
		return std::string("id");
	}

	static std::vector<std::string> Columns()
	{
		return {
			"id",
			"property_id",
			"owner_character_id",
			"placed",
			"heading",
			"pitch",
			"roll",
			"scale",
			"x",
			"y",
			"z",
			"item_id",
			"charges",
			"color",
			"augment_one",
			"augment_two",
			"augment_three",
			"augment_four",
			"augment_five",
			"augment_six",
			"instnodrop",
			"custom_data",
			"ornament_icon",
			"ornament_idfile",
			"ornament_hero_model",
			"guid",
		};
	}

	static std::vector<std::string> SelectColumns()
	{
		return {
			"id",
			"property_id",
			"owner_character_id",
			"placed",
			"heading",
			"pitch",
			"roll",
			"scale",
			"x",
			"y",
			"z",
			"item_id",
			"charges",
			"color",
			"augment_one",
			"augment_two",
			"augment_three",
			"augment_four",
			"augment_five",
			"augment_six",
			"instnodrop",
			"custom_data",
			"ornament_icon",
			"ornament_idfile",
			"ornament_hero_model",
			"guid",
		};
	}

	static std::string ColumnsRaw()
	{
		return std::string(Strings::Implode(", ", Columns()));
	}

	static std::string SelectColumnsRaw()
	{
		return std::string(Strings::Implode(", ", SelectColumns()));
	}

	static std::string TableName()
	{
		return std::string("real_estate_items");
	}

	static std::string BaseSelect()
	{
		return fmt::format(
			"SELECT {} FROM {}",
			SelectColumnsRaw(),
			TableName()
		);
	}

	static std::string BaseInsert()
	{
		return fmt::format(
			"INSERT INTO {} ({}) ",
			TableName(),
			ColumnsRaw()
		);
	}

	static RealEstateItems NewEntity()
	{
		RealEstateItems e{};

		e.id                  = 0;
		e.property_id         = 0;
		e.owner_character_id  = 0;
		e.placed              = 0;
		e.heading             = 0;
		e.pitch               = 0;
		e.roll                = 0;
		e.scale               = 1;
		e.x                   = 0;
		e.y                   = 0;
		e.z                   = 0;
		e.item_id             = 0;
		e.charges             = 0;
		e.color               = 0;
		e.augment_one         = 0;
		e.augment_two         = 0;
		e.augment_three       = 0;
		e.augment_four        = 0;
		e.augment_five        = 0;
		e.augment_six         = 0;
		e.instnodrop          = 0;
		e.custom_data         = "";
		e.ornament_icon       = 0;
		e.ornament_idfile     = 0;
		e.ornament_hero_model = 0;
		e.guid                = 0;

		return e;
	}

	static RealEstateItems GetRealEstateItems(
		const std::vector<RealEstateItems> &real_estate_itemss,
		int real_estate_items_id
	)
	{
		for (auto &real_estate_items : real_estate_itemss) {
			if (real_estate_items.id == real_estate_items_id) {
				return real_estate_items;
			}
		}

		return NewEntity();
	}

	static RealEstateItems FindOne(
		Database& db,
		int real_estate_items_id
	)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {} = {} LIMIT 1",
				BaseSelect(),
				PrimaryKey(),
				real_estate_items_id
			)
		);

		auto row = results.begin();
		if (results.RowCount() == 1) {
			RealEstateItems e{};

			e.id                  = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.property_id         = row[1] ? static_cast<uint32_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.owner_character_id  = row[2] ? static_cast<uint32_t>(strtoul(row[2], nullptr, 10)) : 0;
			e.placed              = row[3] ? static_cast<uint8_t>(strtoul(row[3], nullptr, 10)) : 0;
			e.heading             = row[4] ? strtof(row[4], nullptr) : 0;
			e.pitch               = row[5] ? strtof(row[5], nullptr) : 0;
			e.roll                = row[6] ? strtof(row[6], nullptr) : 0;
			e.scale               = row[7] ? strtof(row[7], nullptr) : 1;
			e.x                   = row[8] ? strtof(row[8], nullptr) : 0;
			e.y                   = row[9] ? strtof(row[9], nullptr) : 0;
			e.z                   = row[10] ? strtof(row[10], nullptr) : 0;
			e.item_id             = row[11] ? static_cast<uint32_t>(strtoul(row[11], nullptr, 10)) : 0;
			e.charges             = row[12] ? static_cast<uint16_t>(strtoul(row[12], nullptr, 10)) : 0;
			e.color               = row[13] ? static_cast<uint32_t>(strtoul(row[13], nullptr, 10)) : 0;
			e.augment_one         = row[14] ? static_cast<uint32_t>(strtoul(row[14], nullptr, 10)) : 0;
			e.augment_two         = row[15] ? static_cast<uint32_t>(strtoul(row[15], nullptr, 10)) : 0;
			e.augment_three       = row[16] ? static_cast<uint32_t>(strtoul(row[16], nullptr, 10)) : 0;
			e.augment_four        = row[17] ? static_cast<uint32_t>(strtoul(row[17], nullptr, 10)) : 0;
			e.augment_five        = row[18] ? static_cast<uint32_t>(strtoul(row[18], nullptr, 10)) : 0;
			e.augment_six         = row[19] ? static_cast<uint32_t>(strtoul(row[19], nullptr, 10)) : 0;
			e.instnodrop          = row[20] ? static_cast<uint8_t>(strtoul(row[20], nullptr, 10)) : 0;
			e.custom_data         = row[21] ? row[21] : "";
			e.ornament_icon       = row[22] ? static_cast<uint32_t>(strtoul(row[22], nullptr, 10)) : 0;
			e.ornament_idfile     = row[23] ? static_cast<uint32_t>(strtoul(row[23], nullptr, 10)) : 0;
			e.ornament_hero_model = row[24] ? static_cast<int32_t>(atoi(row[24])) : 0;
			e.guid                = row[25] ? strtoull(row[25], nullptr, 10) : 0;

			return e;
		}

		return NewEntity();
	}

	static int DeleteOne(
		Database& db,
		int real_estate_items_id
	)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"DELETE FROM {} WHERE {} = {}",
				TableName(),
				PrimaryKey(),
				real_estate_items_id
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static int UpdateOne(
		Database& db,
		const RealEstateItems &e
	)
	{
		std::vector<std::string> v;

		auto columns = Columns();

		v.push_back(columns[1] + " = " + std::to_string(e.property_id));
		v.push_back(columns[2] + " = " + std::to_string(e.owner_character_id));
		v.push_back(columns[3] + " = " + std::to_string(e.placed));
		v.push_back(columns[4] + " = " + std::to_string(e.heading));
		v.push_back(columns[5] + " = " + std::to_string(e.pitch));
		v.push_back(columns[6] + " = " + std::to_string(e.roll));
		v.push_back(columns[7] + " = " + std::to_string(e.scale));
		v.push_back(columns[8] + " = " + std::to_string(e.x));
		v.push_back(columns[9] + " = " + std::to_string(e.y));
		v.push_back(columns[10] + " = " + std::to_string(e.z));
		v.push_back(columns[11] + " = " + std::to_string(e.item_id));
		v.push_back(columns[12] + " = " + std::to_string(e.charges));
		v.push_back(columns[13] + " = " + std::to_string(e.color));
		v.push_back(columns[14] + " = " + std::to_string(e.augment_one));
		v.push_back(columns[15] + " = " + std::to_string(e.augment_two));
		v.push_back(columns[16] + " = " + std::to_string(e.augment_three));
		v.push_back(columns[17] + " = " + std::to_string(e.augment_four));
		v.push_back(columns[18] + " = " + std::to_string(e.augment_five));
		v.push_back(columns[19] + " = " + std::to_string(e.augment_six));
		v.push_back(columns[20] + " = " + std::to_string(e.instnodrop));
		v.push_back(columns[21] + " = '" + Strings::Escape(e.custom_data) + "'");
		v.push_back(columns[22] + " = " + std::to_string(e.ornament_icon));
		v.push_back(columns[23] + " = " + std::to_string(e.ornament_idfile));
		v.push_back(columns[24] + " = " + std::to_string(e.ornament_hero_model));
		v.push_back(columns[25] + " = " + std::to_string(e.guid));

		auto results = db.QueryDatabase(
			fmt::format(
				"UPDATE {} SET {} WHERE {} = {}",
				TableName(),
				Strings::Implode(", ", v),
				PrimaryKey(),
				e.id
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static RealEstateItems InsertOne(
		Database& db,
		RealEstateItems e
	)
	{
		std::vector<std::string> v;

		v.push_back(std::to_string(e.id));
		v.push_back(std::to_string(e.property_id));
		v.push_back(std::to_string(e.owner_character_id));
		v.push_back(std::to_string(e.placed));
		v.push_back(std::to_string(e.heading));
		v.push_back(std::to_string(e.pitch));
		v.push_back(std::to_string(e.roll));
		v.push_back(std::to_string(e.scale));
		v.push_back(std::to_string(e.x));
		v.push_back(std::to_string(e.y));
		v.push_back(std::to_string(e.z));
		v.push_back(std::to_string(e.item_id));
		v.push_back(std::to_string(e.charges));
		v.push_back(std::to_string(e.color));
		v.push_back(std::to_string(e.augment_one));
		v.push_back(std::to_string(e.augment_two));
		v.push_back(std::to_string(e.augment_three));
		v.push_back(std::to_string(e.augment_four));
		v.push_back(std::to_string(e.augment_five));
		v.push_back(std::to_string(e.augment_six));
		v.push_back(std::to_string(e.instnodrop));
		v.push_back("'" + Strings::Escape(e.custom_data) + "'");
		v.push_back(std::to_string(e.ornament_icon));
		v.push_back(std::to_string(e.ornament_idfile));
		v.push_back(std::to_string(e.ornament_hero_model));
		v.push_back(std::to_string(e.guid));

		auto results = db.QueryDatabase(
			fmt::format(
				"{} VALUES ({})",
				BaseInsert(),
				Strings::Implode(",", v)
			)
		);

		if (results.Success()) {
			e.id = results.LastInsertedID();
			return e;
		}

		e = NewEntity();

		return e;
	}

	static int InsertMany(
		Database& db,
		const std::vector<RealEstateItems> &entries
	)
	{
		std::vector<std::string> insert_chunks;

		for (auto &e: entries) {
			std::vector<std::string> v;

			v.push_back(std::to_string(e.id));
			v.push_back(std::to_string(e.property_id));
			v.push_back(std::to_string(e.owner_character_id));
			v.push_back(std::to_string(e.placed));
			v.push_back(std::to_string(e.heading));
			v.push_back(std::to_string(e.pitch));
			v.push_back(std::to_string(e.roll));
			v.push_back(std::to_string(e.scale));
			v.push_back(std::to_string(e.x));
			v.push_back(std::to_string(e.y));
			v.push_back(std::to_string(e.z));
			v.push_back(std::to_string(e.item_id));
			v.push_back(std::to_string(e.charges));
			v.push_back(std::to_string(e.color));
			v.push_back(std::to_string(e.augment_one));
			v.push_back(std::to_string(e.augment_two));
			v.push_back(std::to_string(e.augment_three));
			v.push_back(std::to_string(e.augment_four));
			v.push_back(std::to_string(e.augment_five));
			v.push_back(std::to_string(e.augment_six));
			v.push_back(std::to_string(e.instnodrop));
			v.push_back("'" + Strings::Escape(e.custom_data) + "'");
			v.push_back(std::to_string(e.ornament_icon));
			v.push_back(std::to_string(e.ornament_idfile));
			v.push_back(std::to_string(e.ornament_hero_model));
			v.push_back(std::to_string(e.guid));

			insert_chunks.push_back("(" + Strings::Implode(",", v) + ")");
		}

		std::vector<std::string> v;

		auto results = db.QueryDatabase(
			fmt::format(
				"{} VALUES {}",
				BaseInsert(),
				Strings::Implode(",", insert_chunks)
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static std::vector<RealEstateItems> All(Database& db)
	{
		std::vector<RealEstateItems> all_entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{}",
				BaseSelect()
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			RealEstateItems e{};

			e.id                  = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.property_id         = row[1] ? static_cast<uint32_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.owner_character_id  = row[2] ? static_cast<uint32_t>(strtoul(row[2], nullptr, 10)) : 0;
			e.placed              = row[3] ? static_cast<uint8_t>(strtoul(row[3], nullptr, 10)) : 0;
			e.heading             = row[4] ? strtof(row[4], nullptr) : 0;
			e.pitch               = row[5] ? strtof(row[5], nullptr) : 0;
			e.roll                = row[6] ? strtof(row[6], nullptr) : 0;
			e.scale               = row[7] ? strtof(row[7], nullptr) : 1;
			e.x                   = row[8] ? strtof(row[8], nullptr) : 0;
			e.y                   = row[9] ? strtof(row[9], nullptr) : 0;
			e.z                   = row[10] ? strtof(row[10], nullptr) : 0;
			e.item_id             = row[11] ? static_cast<uint32_t>(strtoul(row[11], nullptr, 10)) : 0;
			e.charges             = row[12] ? static_cast<uint16_t>(strtoul(row[12], nullptr, 10)) : 0;
			e.color               = row[13] ? static_cast<uint32_t>(strtoul(row[13], nullptr, 10)) : 0;
			e.augment_one         = row[14] ? static_cast<uint32_t>(strtoul(row[14], nullptr, 10)) : 0;
			e.augment_two         = row[15] ? static_cast<uint32_t>(strtoul(row[15], nullptr, 10)) : 0;
			e.augment_three       = row[16] ? static_cast<uint32_t>(strtoul(row[16], nullptr, 10)) : 0;
			e.augment_four        = row[17] ? static_cast<uint32_t>(strtoul(row[17], nullptr, 10)) : 0;
			e.augment_five        = row[18] ? static_cast<uint32_t>(strtoul(row[18], nullptr, 10)) : 0;
			e.augment_six         = row[19] ? static_cast<uint32_t>(strtoul(row[19], nullptr, 10)) : 0;
			e.instnodrop          = row[20] ? static_cast<uint8_t>(strtoul(row[20], nullptr, 10)) : 0;
			e.custom_data         = row[21] ? row[21] : "";
			e.ornament_icon       = row[22] ? static_cast<uint32_t>(strtoul(row[22], nullptr, 10)) : 0;
			e.ornament_idfile     = row[23] ? static_cast<uint32_t>(strtoul(row[23], nullptr, 10)) : 0;
			e.ornament_hero_model = row[24] ? static_cast<int32_t>(atoi(row[24])) : 0;
			e.guid                = row[25] ? strtoull(row[25], nullptr, 10) : 0;

			all_entries.push_back(e);
		}

		return all_entries;
	}

	static std::vector<RealEstateItems> GetWhere(Database& db, const std::string &where_filter)
	{
		std::vector<RealEstateItems> all_entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {}",
				BaseSelect(),
				where_filter
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			RealEstateItems e{};

			e.id                  = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.property_id         = row[1] ? static_cast<uint32_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.owner_character_id  = row[2] ? static_cast<uint32_t>(strtoul(row[2], nullptr, 10)) : 0;
			e.placed              = row[3] ? static_cast<uint8_t>(strtoul(row[3], nullptr, 10)) : 0;
			e.heading             = row[4] ? strtof(row[4], nullptr) : 0;
			e.pitch               = row[5] ? strtof(row[5], nullptr) : 0;
			e.roll                = row[6] ? strtof(row[6], nullptr) : 0;
			e.scale               = row[7] ? strtof(row[7], nullptr) : 1;
			e.x                   = row[8] ? strtof(row[8], nullptr) : 0;
			e.y                   = row[9] ? strtof(row[9], nullptr) : 0;
			e.z                   = row[10] ? strtof(row[10], nullptr) : 0;
			e.item_id             = row[11] ? static_cast<uint32_t>(strtoul(row[11], nullptr, 10)) : 0;
			e.charges             = row[12] ? static_cast<uint16_t>(strtoul(row[12], nullptr, 10)) : 0;
			e.color               = row[13] ? static_cast<uint32_t>(strtoul(row[13], nullptr, 10)) : 0;
			e.augment_one         = row[14] ? static_cast<uint32_t>(strtoul(row[14], nullptr, 10)) : 0;
			e.augment_two         = row[15] ? static_cast<uint32_t>(strtoul(row[15], nullptr, 10)) : 0;
			e.augment_three       = row[16] ? static_cast<uint32_t>(strtoul(row[16], nullptr, 10)) : 0;
			e.augment_four        = row[17] ? static_cast<uint32_t>(strtoul(row[17], nullptr, 10)) : 0;
			e.augment_five        = row[18] ? static_cast<uint32_t>(strtoul(row[18], nullptr, 10)) : 0;
			e.augment_six         = row[19] ? static_cast<uint32_t>(strtoul(row[19], nullptr, 10)) : 0;
			e.instnodrop          = row[20] ? static_cast<uint8_t>(strtoul(row[20], nullptr, 10)) : 0;
			e.custom_data         = row[21] ? row[21] : "";
			e.ornament_icon       = row[22] ? static_cast<uint32_t>(strtoul(row[22], nullptr, 10)) : 0;
			e.ornament_idfile     = row[23] ? static_cast<uint32_t>(strtoul(row[23], nullptr, 10)) : 0;
			e.ornament_hero_model = row[24] ? static_cast<int32_t>(atoi(row[24])) : 0;
			e.guid                = row[25] ? strtoull(row[25], nullptr, 10) : 0;

			all_entries.push_back(e);
		}

		return all_entries;
	}

	static int DeleteWhere(Database& db, const std::string &where_filter)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"DELETE FROM {} WHERE {}",
				TableName(),
				where_filter
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static int Truncate(Database& db)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"TRUNCATE TABLE {}",
				TableName()
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static int64 GetMaxId(Database& db)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"SELECT COALESCE(MAX({}), 0) FROM {}",
				PrimaryKey(),
				TableName()
			)
		);

		return (results.Success() && results.begin()[0] ? strtoll(results.begin()[0], nullptr, 10) : 0);
	}

	static int64 Count(Database& db, const std::string &where_filter = "")
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"SELECT COUNT(*) FROM {} {}",
				TableName(),
				(where_filter.empty() ? "" : "WHERE " + where_filter)
			)
		);

		return (results.Success() && results.begin()[0] ? strtoll(results.begin()[0], nullptr, 10) : 0);
	}

	static std::string BaseReplace()
	{
		return fmt::format(
			"REPLACE INTO {} ({}) ",
			TableName(),
			ColumnsRaw()
		);
	}

	static int ReplaceOne(
		Database& db,
		const RealEstateItems &e
	)
	{
		std::vector<std::string> v;

		v.push_back(std::to_string(e.id));
		v.push_back(std::to_string(e.property_id));
		v.push_back(std::to_string(e.owner_character_id));
		v.push_back(std::to_string(e.placed));
		v.push_back(std::to_string(e.heading));
		v.push_back(std::to_string(e.pitch));
		v.push_back(std::to_string(e.roll));
		v.push_back(std::to_string(e.scale));
		v.push_back(std::to_string(e.x));
		v.push_back(std::to_string(e.y));
		v.push_back(std::to_string(e.z));
		v.push_back(std::to_string(e.item_id));
		v.push_back(std::to_string(e.charges));
		v.push_back(std::to_string(e.color));
		v.push_back(std::to_string(e.augment_one));
		v.push_back(std::to_string(e.augment_two));
		v.push_back(std::to_string(e.augment_three));
		v.push_back(std::to_string(e.augment_four));
		v.push_back(std::to_string(e.augment_five));
		v.push_back(std::to_string(e.augment_six));
		v.push_back(std::to_string(e.instnodrop));
		v.push_back("'" + Strings::Escape(e.custom_data) + "'");
		v.push_back(std::to_string(e.ornament_icon));
		v.push_back(std::to_string(e.ornament_idfile));
		v.push_back(std::to_string(e.ornament_hero_model));
		v.push_back(std::to_string(e.guid));

		auto results = db.QueryDatabase(
			fmt::format(
				"{} VALUES ({})",
				BaseReplace(),
				Strings::Implode(",", v)
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static int ReplaceMany(
		Database& db,
		const std::vector<RealEstateItems> &entries
	)
	{
		std::vector<std::string> insert_chunks;

		for (auto &e: entries) {
			std::vector<std::string> v;

			v.push_back(std::to_string(e.id));
			v.push_back(std::to_string(e.property_id));
			v.push_back(std::to_string(e.owner_character_id));
			v.push_back(std::to_string(e.placed));
			v.push_back(std::to_string(e.heading));
			v.push_back(std::to_string(e.pitch));
			v.push_back(std::to_string(e.roll));
			v.push_back(std::to_string(e.scale));
			v.push_back(std::to_string(e.x));
			v.push_back(std::to_string(e.y));
			v.push_back(std::to_string(e.z));
			v.push_back(std::to_string(e.item_id));
			v.push_back(std::to_string(e.charges));
			v.push_back(std::to_string(e.color));
			v.push_back(std::to_string(e.augment_one));
			v.push_back(std::to_string(e.augment_two));
			v.push_back(std::to_string(e.augment_three));
			v.push_back(std::to_string(e.augment_four));
			v.push_back(std::to_string(e.augment_five));
			v.push_back(std::to_string(e.augment_six));
			v.push_back(std::to_string(e.instnodrop));
			v.push_back("'" + Strings::Escape(e.custom_data) + "'");
			v.push_back(std::to_string(e.ornament_icon));
			v.push_back(std::to_string(e.ornament_idfile));
			v.push_back(std::to_string(e.ornament_hero_model));
			v.push_back(std::to_string(e.guid));

			insert_chunks.push_back("(" + Strings::Implode(",", v) + ")");
		}

		std::vector<std::string> v;

		auto results = db.QueryDatabase(
			fmt::format(
				"{} VALUES {}",
				BaseReplace(),
				Strings::Implode(",", insert_chunks)
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}
};

#endif //EQEMU_BASE_REAL_ESTATE_ITEMS_REPOSITORY_H
