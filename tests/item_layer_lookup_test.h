#ifndef __EQEMU_TESTS_ITEM_LAYER_LOOKUP_H
#define __EQEMU_TESTS_ITEM_LAYER_LOOKUP_H

#include "cppunit/cpptest.h"
#include "../common/item_guid.h"
#include "../common/item_layer.h"
#include "../common/shareddb.h"
#include <algorithm>
#include <memory>
#include <string_view>

// SEC-1's guard. A client-sent GUID that decodes to a zero first word (the all-zero text, the client's {0, 0, n}
// placeholder for an item it split itself, or text that does not decode) selects no item, so ItemLayer::Lookup refuses
// it before any database work. Each case feeds a forged field through the same ItemGUIDFromField the RoF2 Bazaar packet
// handlers use, then calls the real Lookup and asserts a refusal. The listed-GUID accept and the out-of-scope (wrong
// trader, delisted) refuse are Lookup's database-backed arm, proven live against a running server.
class ItemLayerLookupTest : public Test::Suite {
	typedef void(ItemLayerLookupTest::*TestFunction)(void);
public:
	ItemLayerLookupTest() {
		TEST_ADD(ItemLayerLookupTest::ZeroTextIsRefused);
		TEST_ADD(ItemLayerLookupTest::PlaceholderIsRefused);
		TEST_ADD(ItemLayerLookupTest::UndecodableTextIsRefused);
		TEST_ADD(ItemLayerLookupTest::WrongLengthIsRefused);
	}

private:
	// The refusal arm returns before touching the database, so an unconnected handle is never dereferenced.
	static std::unique_ptr<EQ::ItemInstance> Lookup(const ItemGUID &guid) {
		SharedDatabase db;
		return ItemLayer::Lookup(db, db, ItemScope::ListedBy(118), guid, "forged-guid-test");
	}

	static ItemGUID FromField(const char (&field)[17]) {
		return ItemGUIDFromField(std::string_view(field, sizeof(field)));
	}

	void ZeroTextIsRefused() {
		const char field[17] = "0000000000000000";
		const auto guid = FromField(field);
		TEST_ASSERT_EQUALS(guid.first, 0u);
		TEST_ASSERT(Lookup(guid) == nullptr);
	}

	void PlaceholderIsRefused() {
		const auto text = ItemGUIDText(ItemGUID{0, 0, 2});
		char       field[17] = {};
		std::copy(text.begin(), text.end(), field);
		const auto guid = FromField(field);
		TEST_ASSERT_EQUALS(guid.first, 0u);
		TEST_ASSERT(Lookup(guid) == nullptr);
	}

	void UndecodableTextIsRefused() {
		const char field[17] = "20e308i00001000_";
		const auto guid = FromField(field);
		TEST_ASSERT_EQUALS(guid.first, 0u);
		TEST_ASSERT(Lookup(guid) == nullptr);
	}

	void WrongLengthIsRefused() {
		const char field[17] = "20e308i00001000";
		const auto guid = FromField(field);
		TEST_ASSERT_EQUALS(guid.first, 0u);
		TEST_ASSERT(Lookup(guid) == nullptr);
	}
};

#endif
