/*	EQEMu: Everquest Server Emulator
	
	Copyright (C) 2001-2016 EQEMu Development Team (http://eqemulator.net)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "emu_versions.h"
#include "emu_constants.h"


EQ::versions::ClientVersion EQ::versions::ValidateClientVersion(ClientVersion client_version)
{
	if (client_version <= ClientVersion::Unknown || client_version > LastClientVersion)
		return ClientVersion::Unknown;

	return client_version;
}

const char* EQ::versions::ClientVersionName(ClientVersion client_version)
{
	switch (client_version) {
	case ClientVersion::Unknown:
		return "Unknown Version";
	case ClientVersion::RoF2:
		return "RoF2";
	default:
		return "Invalid Version";
	};
}

EQ::versions::MobVersion EQ::versions::ValidateMobVersion(MobVersion mob_version)
{
	if (mob_version <= MobVersion::Unknown || mob_version > LastMobVersion)
		return MobVersion::Unknown;

	return mob_version;
}

const char* EQ::versions::MobVersionName(MobVersion mob_version)
{
	switch (mob_version) {
	case MobVersion::Unknown:
		return "Unknown Version";
	case MobVersion::RoF2:
		return "RoF2";
	case MobVersion::NPC:
		return "NPC";
	case MobVersion::NPCMerchant:
		return "NPC Merchant";
	case MobVersion::Merc:
		return "Mercenary";
	case MobVersion::Bot:
		return "Bot";
	case MobVersion::ClientPet:
		return "Client Pet";
	case MobVersion::NPCPet:
		return "NPC Pet";
	case MobVersion::MercPet:
		return "Mercenary Pet";
	case MobVersion::BotPet:
		return "Bot Pet";
	case MobVersion::OfflineRoF2:
		return "Offline RoF2";
	default:
		return "Invalid Version";
	};
}

EQ::versions::MobVersion EQ::versions::ConvertClientVersionToMobVersion(ClientVersion client_version)
{
	switch (client_version) {
	case ClientVersion::RoF2:
		return MobVersion::RoF2;
	default:
		return MobVersion::Unknown;
	}
}

const char* EQ::expansions::ExpansionName(Expansion expansion)
{
	switch (expansion) {
	case Expansion::EverQuest:
		return "EverQuest";
	case Expansion::RoK:
		return "The Ruins of Kunark";
	case Expansion::SoV:
		return "The Scars of Velious";
	case Expansion::SoL:
		return "The Shadows of Luclin";
	case Expansion::PoP:
		return "The Planes of Power";
	case Expansion::LoY:
		return "The Legacy of Ykesha";
	case Expansion::LDoN:
		return "Lost Dungeons of Norrath";
	case Expansion::GoD:
		return "Gates of Discord";
	case Expansion::OoW:
		return "Omens of War";
	case Expansion::DoN:
		return "Dragons of Norrath";
	case Expansion::DoD:
		return "Depths of Darkhollow";
	case Expansion::PoR:
		return "Prophecy of Ro";
	case Expansion::TSS:
		return "The Serpent's Spine";
	case Expansion::TBS:
		return "The Buried Sea";
	case Expansion::SoF:
		return "Secrets of Faydwer";
	case Expansion::SoD:
		return "Seeds of Destruction";
	case Expansion::UF:
		return "Underfoot";
	case Expansion::HoT:
		return "House of Thule";
	case Expansion::VoA:
		return "Veil of Alaris";
	case Expansion::RoF:
		return "Rain of Fear";
	case Expansion::CotF:
		return "Call of the Forsaken";
	default:
		return "Invalid Expansion";
	}
}

const char* EQ::expansions::ExpansionName(uint32 expansion_bit)
{
	return ExpansionName(ConvertExpansionBitToExpansion(expansion_bit));
}

uint32 EQ::expansions::ConvertExpansionToExpansionBit(Expansion expansion)
{
	switch (expansion) {
	case Expansion::RoK:
		return bitRoK;
	case Expansion::SoV:
		return bitSoV;
	case Expansion::SoL:
		return bitSoL;
	case Expansion::PoP:
		return bitPoP;
	case Expansion::LoY:
		return bitLoY;
	case Expansion::LDoN:
		return bitLDoN;
	case Expansion::GoD:
		return bitGoD;
	case Expansion::OoW:
		return bitOoW;
	case Expansion::DoN:
		return bitDoN;
	case Expansion::DoD:
		return bitDoD;
	case Expansion::PoR:
		return bitPoR;
	case Expansion::TSS:
		return bitTSS;
	case Expansion::TBS:
		return bitTBS;
	case Expansion::SoF:
		return bitSoF;
	case Expansion::SoD:
		return bitSoD;
	case Expansion::UF:
		return bitUF;
	case Expansion::HoT:
		return bitHoT;
	case Expansion::VoA:
		return bitVoA;
	case Expansion::RoF:
		return bitRoF;
	case Expansion::CotF:
		return bitCotF;
	default:
		return bitEverQuest;
	}
}

EQ::expansions::Expansion EQ::expansions::ConvertExpansionBitToExpansion(uint32 expansion_bit)
{
	switch (expansion_bit) {
	case bitRoK:
		return Expansion::RoK;
	case bitSoV:
		return Expansion::SoV;
	case bitSoL:
		return Expansion::SoL;
	case bitPoP:
		return Expansion::PoP;
	case bitLoY:
		return Expansion::LoY;
	case bitLDoN:
		return Expansion::LDoN;
	case bitGoD:
		return Expansion::GoD;
	case bitOoW:
		return Expansion::OoW;
	case bitDoN:
		return Expansion::DoN;
	case bitDoD:
		return Expansion::DoD;
	case bitPoR:
		return Expansion::PoR;
	case bitTSS:
		return Expansion::TSS;
	case bitTBS:
		return Expansion::TBS;
	case bitSoF:
		return Expansion::SoF;
	case bitSoD:
		return Expansion::SoD;
	case bitUF:
		return Expansion::UF;
	case bitHoT:
		return Expansion::HoT;
	case bitVoA:
		return Expansion::VoA;
	case bitRoF:
		return Expansion::RoF;
	case bitCotF:
		return Expansion::CotF;
	default:
		return Expansion::EverQuest;
	}
}

uint32 EQ::expansions::ConvertExpansionToExpansionsMask(Expansion expansion)
{
	switch (expansion) {
	case Expansion::RoK:
		return maskRoK;
	case Expansion::SoV:
		return maskSoV;
	case Expansion::SoL:
		return maskSoL;
	case Expansion::PoP:
		return maskPoP;
	case Expansion::LoY:
		return maskLoY;
	case Expansion::LDoN:
		return maskLDoN;
	case Expansion::GoD:
		return maskGoD;
	case Expansion::OoW:
		return maskOoW;
	case Expansion::DoN:
		return maskDoN;
	case Expansion::DoD:
		return maskDoD;
	case Expansion::PoR:
		return maskPoR;
	case Expansion::TSS:
		return maskTSS;
	case Expansion::TBS:
		return maskTBS;
	case Expansion::SoF:
		return maskSoF;
	case Expansion::SoD:
		return maskSoD;
	case Expansion::UF:
		return maskUF;
	case Expansion::HoT:
		return maskHoT;
	case Expansion::VoA:
		return maskVoA;
	case Expansion::RoF:
		return maskRoF;
	case Expansion::CotF:
		return maskCotF;
	default:
		return maskEverQuest;
	}
}
