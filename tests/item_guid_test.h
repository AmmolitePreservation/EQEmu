#ifndef __EQEMU_TESTS_ITEM_GUID_H
#define __EQEMU_TESTS_ITEM_GUID_H

#include "cppunit/cpptest.h"
#include "../common/item_guid.h"

class ItemGUIDTest : public Test::Suite {
	typedef void(ItemGUIDTest::*TestFunction)(void);
public:
	ItemGUIDTest() {
		TEST_ADD(ItemGUIDTest::LiveTextEncodes);
		TEST_ADD(ItemGUIDTest::LiveTextDecodes);
		TEST_ADD(ItemGUIDTest::ServerStampRoundTrips);
		TEST_ADD(ItemGUIDTest::ZeroTextIsNoItem);
		TEST_ADD(ItemGUIDTest::PlaceholderIsNoItem);
		TEST_ADD(ItemGUIDTest::WrongLengthIsRefused);
		TEST_ADD(ItemGUIDTest::ForeignCharacterIsRefused);
		TEST_ADD(ItemGUIDTest::FieldReadsToTerminator);
		TEST_ADD(ItemGUIDTest::FieldWithoutItemIsZero);
	}

	~ItemGUIDTest() {
	}

private:
	// Captured on Live from a Bristlebane (world 139) character's inventory.
	void LiveTextEncodes() {
		TEST_ASSERT_EQUALS(ItemGUIDText(ItemGUID{199176, 139, 1}), std::string("20e308i000010000"));
		TEST_ASSERT_EQUALS(ItemGUIDText(ItemGUID{199176, 139, 2126}), std::string("20e308i0001E2000"));
		TEST_ASSERT_EQUALS(ItemGUIDText(ItemGUID{199176, 139, 14704}), std::string("20e308i0001mEG00"));
	}

	void LiveTextDecodes() {
		const auto guid = ItemGUIDFromText("20e308i0002uB000");
		TEST_ASSERT(guid.has_value());
		TEST_ASSERT_EQUALS(guid->first, 199176u);
		TEST_ASSERT_EQUALS(guid->second, 139u);
		TEST_ASSERT_EQUALS(guid->third, 11448u);
	}

	void ServerStampRoundTrips() {
		const ItemGUID stamp{1561414312, 1793570735, 1793570581};
		const auto     guid = ItemGUIDFromText(ItemGUIDText(stamp));
		TEST_ASSERT(guid.has_value());
		TEST_ASSERT(*guid == stamp);
	}

	void ZeroTextIsNoItem() {
		TEST_ASSERT_EQUALS(ItemGUIDText(ItemGUID{}), std::string(item_guid_zero_text));
		TEST_ASSERT(!ItemGUIDFromText(item_guid_zero_text).has_value());
	}

	void PlaceholderIsNoItem() {
		TEST_ASSERT(!ItemGUIDFromText(ItemGUIDText(ItemGUID{0, 0, 2})).has_value());
	}

	void WrongLengthIsRefused() {
		TEST_ASSERT(!ItemGUIDFromText("20e308i00001000").has_value());
		TEST_ASSERT(!ItemGUIDFromText("20e308i0000100000").has_value());
	}

	void ForeignCharacterIsRefused() {
		TEST_ASSERT(!ItemGUIDFromText("20e308i00001000_").has_value());
	}

	// The client's 17-byte fields carry the text and a terminator.
	void FieldReadsToTerminator() {
		const char     field[17] = "20e308i0002uB000";
		const ItemGUID expected{199176, 139, 11448};
		TEST_ASSERT(ItemGUIDFromField(std::string_view(field, sizeof(field))) == expected);
	}

	void FieldWithoutItemIsZero() {
		const char empty[17] = "";
		const char zeros[17] = "0000000000000000";
		const char unterminated[17] = {'2', '0', 'e', '3', '0', '8', 'i', '0', '0', '0', '2', 'u', 'B', '0', '0', '0', '1'};
		TEST_ASSERT(ItemGUIDFromField(std::string_view(empty, sizeof(empty))).IsZero());
		TEST_ASSERT(ItemGUIDFromField(std::string_view(zeros, sizeof(zeros))).IsZero());
		TEST_ASSERT(ItemGUIDFromField(std::string_view(unterminated, sizeof(unterminated))).IsZero());
	}
};

#endif
