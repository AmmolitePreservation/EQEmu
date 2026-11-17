#ifndef __EQEMU_TESTS_ITEM_CUSTOM_DATA_H
#define __EQEMU_TESTS_ITEM_CUSTOM_DATA_H

#include "cppunit/cpptest.h"
#include "../common/item_instance.h"

#include <stdexcept>

class ItemCustomDataTest : public Test::Suite {
	typedef void(ItemCustomDataTest::*TestFunction)(void);
public:
	ItemCustomDataTest() {
		TEST_ADD(ItemCustomDataTest::TwoKeysReadBack);
		TEST_ADD(ItemCustomDataTest::ThreeKeysReadBack);
		TEST_ADD(ItemCustomDataTest::EmptyValueReadsBack);
		TEST_ADD(ItemCustomDataTest::NumbersReadBack);
		TEST_ADD(ItemCustomDataTest::NoDataIsAnEmptyString);
		TEST_ADD(ItemCustomDataTest::OddCountIsRefused);
		TEST_ADD(ItemCustomDataTest::OldSingleKeyFormIsRefused);
		TEST_ADD(ItemCustomDataTest::OldTwoKeyFormIsRefused);
		TEST_ADD(ItemCustomDataTest::SeparatorInValueIsRefused);
		TEST_ADD(ItemCustomDataTest::SeparatorInIdentifierIsRefused);
		TEST_ADD(ItemCustomDataTest::EmptyIdentifierIsRefused);
	}

	~ItemCustomDataTest() {
	}

private:
	static std::string reloaded(const EQ::ItemInstance &saved, const std::string &identifier) {
		EQ::ItemInstance loaded;
		loaded.SetCustomDataString(saved.GetCustomDataString());
		return loaded.GetCustomData(identifier);
	}

	template<typename Action>
	static bool refuses(Action action) {
		try {
			action();
		}
		catch (const std::invalid_argument &) {
			return true;
		}
		return false;
	}

	void TwoKeysReadBack() {
		EQ::ItemInstance item;
		item.SetCustomData("a", std::string("1"));
		item.SetCustomData("b", std::string("2"));
		TEST_ASSERT_EQUALS(item.GetCustomDataString(), std::string("a^1^b^2"));
		TEST_ASSERT_EQUALS(reloaded(item, "a"), std::string("1"));
		TEST_ASSERT_EQUALS(reloaded(item, "b"), std::string("2"));
		TEST_ASSERT_EQUALS(EQ::ItemInstance::ParseCustomDataString(item.GetCustomDataString()).size(), static_cast<size_t>(2));
	}

	void ThreeKeysReadBack() {
		EQ::ItemInstance item;
		item.SetCustomData("color", std::string("red"));
		item.SetCustomData("level", std::string("7"));
		item.SetCustomData("owner", std::string("Claudehousing"));
		const auto parsed = EQ::ItemInstance::ParseCustomDataString(item.GetCustomDataString());
		TEST_ASSERT_EQUALS(parsed.size(), static_cast<size_t>(3));
		TEST_ASSERT_EQUALS(parsed.at("color"), std::string("red"));
		TEST_ASSERT_EQUALS(parsed.at("level"), std::string("7"));
		TEST_ASSERT_EQUALS(parsed.at("owner"), std::string("Claudehousing"));
	}

	void EmptyValueReadsBack() {
		EQ::ItemInstance item;
		item.SetCustomData("a", std::string(""));
		item.SetCustomData("b", std::string("2"));
		const auto parsed = EQ::ItemInstance::ParseCustomDataString(item.GetCustomDataString());
		TEST_ASSERT_EQUALS(parsed.size(), static_cast<size_t>(2));
		TEST_ASSERT_EQUALS(parsed.at("a"), std::string(""));
		TEST_ASSERT_EQUALS(parsed.at("b"), std::string("2"));
	}

	void NumbersReadBack() {
		EQ::ItemInstance item;
		item.SetCustomData("count", 42);
		item.SetCustomData("flag", true);
		TEST_ASSERT_EQUALS(reloaded(item, "count"), std::string("42"));
		TEST_ASSERT_EQUALS(reloaded(item, "flag"), std::string("1"));
	}

	void NoDataIsAnEmptyString() {
		EQ::ItemInstance item;
		TEST_ASSERT_EQUALS(item.GetCustomDataString(), std::string(""));
		TEST_ASSERT(EQ::ItemInstance::ParseCustomDataString("").empty());
	}

	void OddCountIsRefused() {
		TEST_ASSERT(refuses([] { EQ::ItemInstance::ParseCustomDataString("a^1^b"); }));
	}

	// The writer before ITM-10 ended every pair with '^'.
	void OldSingleKeyFormIsRefused() {
		TEST_ASSERT(refuses([] { EQ::ItemInstance::ParseCustomDataString("a^1^"); }));
	}

	void OldTwoKeyFormIsRefused() {
		TEST_ASSERT(refuses([] { EQ::ItemInstance::ParseCustomDataString("a^1^^b^2^"); }));
	}

	void SeparatorInValueIsRefused() {
		EQ::ItemInstance item;
		TEST_ASSERT(refuses([&item] { item.SetCustomData("a", std::string("x^y")); }));
		TEST_ASSERT_EQUALS(item.GetCustomDataString(), std::string(""));
	}

	void SeparatorInIdentifierIsRefused() {
		EQ::ItemInstance item;
		TEST_ASSERT(refuses([&item] { item.SetCustomData("a^b", std::string("1")); }));
		TEST_ASSERT(refuses([&item] { item.SetCustomData("a^b", 1); }));
	}

	void EmptyIdentifierIsRefused() {
		EQ::ItemInstance item;
		TEST_ASSERT(refuses([&item] { item.SetCustomData("", std::string("1")); }));
		TEST_ASSERT(refuses([] { EQ::ItemInstance::ParseCustomDataString("^1"); }));
	}
};

#endif
