#ifndef EQEMU_ITEM_GUID_H
#define EQEMU_ITEM_GUID_H

#include "types.h"

#include <array>
#include <optional>
#include <string>
#include <string_view>

struct ItemGUID {
	uint32 first{0};
	uint32 second{0};
	uint32 third{0};

	bool IsZero() const { return first == 0 && second == 0 && third == 0; }
	bool operator==(const ItemGUID &other) const { return first == other.first && second == other.second && third == other.third; }
	bool operator!=(const ItemGUID &other) const { return !(*this == other); }
};

// The text RoF2 and Live carry: the three words as 12 little-endian bytes, read as one big-endian 96-bit number and
// written six bits a character over this alphabet (RoF2 0xb61d20, Live's file offset 0xd99f40).
constexpr std::string_view item_guid_alphabet = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+-";
constexpr size_t           item_guid_text_length = 16;
constexpr std::string_view item_guid_zero_text = "0000000000000000";

inline std::array<uint8, 12> ItemGUIDBytes(const ItemGUID &guid)
{
	std::array<uint8, 12> bytes{};
	const uint32          words[3] = {guid.first, guid.second, guid.third};
	for (size_t word = 0; word < 3; ++word) {
		for (size_t b = 0; b < 4; ++b) {
			bytes[word * 4 + b] = static_cast<uint8>(words[word] >> (8 * b));
		}
	}

	return bytes;
}

inline std::string ItemGUIDText(const ItemGUID &guid)
{
	const auto  bytes = ItemGUIDBytes(guid);
	std::string text(item_guid_text_length, '0');
	for (size_t c = 0; c < item_guid_text_length; ++c) {
		uint32 sextet = 0;
		for (size_t bit = 0; bit < 6; ++bit) {
			const size_t position = c * 6 + bit;
			sextet = (sextet << 1) | ((bytes[position / 8] >> (7 - position % 8)) & 1);
		}
		text[c] = item_guid_alphabet[sextet];
	}

	return text;
}

// An item's GUID from client text: exactly 16 characters of the alphabet. A first word of 0 is no item (the all-zero
// text, or the client's stand-ins {0, 0, n} for an item it split itself), so it selects nothing.
inline std::optional<ItemGUID> ItemGUIDFromText(std::string_view text)
{
	if (text.size() != item_guid_text_length) {
		return std::nullopt;
	}

	std::array<uint8, 12> bytes{};
	for (size_t c = 0; c < item_guid_text_length; ++c) {
		const auto sextet = item_guid_alphabet.find(text[c]);
		if (sextet == std::string_view::npos) {
			return std::nullopt;
		}

		for (size_t bit = 0; bit < 6; ++bit) {
			const size_t position = c * 6 + bit;
			if ((sextet >> (5 - bit)) & 1) {
				bytes[position / 8] |= static_cast<uint8>(1 << (7 - position % 8));
			}
		}
	}

	ItemGUID guid;
	uint32  *words[3] = {&guid.first, &guid.second, &guid.third};
	for (size_t word = 0; word < 3; ++word) {
		for (size_t b = 0; b < 4; ++b) {
			*words[word] |= static_cast<uint32>(bytes[word * 4 + b]) << (8 * b);
		}
	}

	if (guid.first == 0) {
		return std::nullopt;
	}

	return guid;
}

#endif //EQEMU_ITEM_GUID_H
