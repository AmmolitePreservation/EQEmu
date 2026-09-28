# NPC items to author

Peridot's placeable NPC items whose NPC isn't settled by their data (`tools/exportPlaceableItems.sql`, `tools/exportNpcItemSources.sql`; `serverPlan.md`, chunk 5). The other 162, mounts and familiars, take the creature their click spell summons, which is also their look in a yard (checked on Live). Names in the item tables are the items'; the NPC shown takes the name of the NPC type chosen. **Sections 2 and 3 take Live's captured look exactly**: the livedumps master links each item to its look (`npcItems.tsv` to `npcLooks.tsv`: race, gender, texture, size and the rest of the appearance), and the content generator reads it from there, so nothing is assigned by hand. **Section 1 records each decision in its last column.** Related NPCs are the Guild Lobby's and classic Guild Hall's, matched only by name or class, for reference.

## 1. Guild services and decorations (27)

Each NPC's look is the one in The Templar's Palatial Guild Hall (the owner's, never re-skinned: Bristlebane property 65484252), which has every item here but the pool and Hyredel Swiftstride; other races seen in other halls are owners' re-skins with the Convoluted Assistant Appearance Augmentater (item 66656). Live names the dummies Training Dummy: \<expansion\>, each at its expansion's level cap.

| Item | Item name | Placeable in | Max per property | NO DROP | Respawn | Related NPCs | NPC to use |
|---|---|---|---|---|---|---|---|
| 66639 | Friendly Guild General Supply Merchant | guild hall | any | no | 750 | 345000 Yenny Werlikanin | The Templar's: race 6 (Dark Elf), female, class 41, level 70, size 5; function in S5 |
| 66640 | Friendly Guild Teleportation Assistant | guild hall | any | no | 750 | 345005 #Porter | The Templar's: race 12 (Gnome), male, class 41, level 70, size 3; function in S5 |
| 66641 | Friendly Guild Personal-Banker | guild hall | any | no | 750 | 345002 a bank broker; 344014 A Shrewd Banker; 344023 A Shrewd Banker | The Templar's: race 3 (Erudite), male, class 40, level 99, size 6; function in S5 |
| 66642 | Friendly Guild Guild-Banker | guild hall | any | no | 750 | 345003 a guild treasurer | The Templar's: race 1 (Human), female, class 66, level 70, size 6; function in S5 |
| 66643 | Friendly Guild Guild-Tribute Master | guild hall | any | no | 750 | 345001 Melody the Singer of Great Deeds | The Templar's: race 522 (Drakkin), female, class 64, level 70, size 5.7; function in S5 |
| 66644 | Friendly Guild Personal-Tribute Master | guild hall | any | no | 750 | none in the guild zones | The Templar's: race 5 (High Elf), male, class 74, level 70, size 6; function in S5 |
| 66645 | Guild Regeneration Pool | guild hall | 1 | no | 38 | 345006 #Healer | none: an interactive object (`IT23628`); its aura is spell 31874 (`serverPlan.md`) |
| 66646 | Guild Teleportation Device | guild hall | 1 | no | 750 | 345005 #Porter | The Templar's: race 706, neutral, class 1, level 70, size 6; function in S5 |
| 66647 | Friendly Guild Parcel Delivery Liaison | guild hall | any | yes | 750 |  | The Templar's: race 1 (Human), female, class 41, level 70, size 6; function in S5 |
| 66648 | Hyredel Swiftstride | guild hall, house | 1 | yes | 750 |  | Live's (not in The Templar's hall; the same in Ring of Valor's and Gathered Might Village II's): race 7 (Half Elf), male, class 41, level 70, size 5.3; function in S5 |
| 66700 | Decorative Dummy: Azia | guild hall | any | yes | 750 |  | The Templar's: race 608, neutral, class 1, level 50, size 6 (Live: Training Dummy: Pre-Planes of Power) |
| 66701 | Decorative Dummy: Beza | guild hall | any | yes | 750 |  | The Templar's: race 608, neutral, class 1, level 60, size 6 (Live: Training Dummy: The Planes of Power) |
| 66702 | Decorative Dummy: Caza | guild hall | any | yes | 750 |  | The Templar's: race 608, neutral, class 1, level 60, size 6 (Live: Training Dummy: The Legacy of Ykesha) |
| 66703 | Decorative Dummy: Dena | guild hall | any | yes | 750 |  | The Templar's: race 608, neutral, class 1, level 65, size 6 (Live: Training Dummy: Lost Dungeons of Norrath) |
| 66704 | Decorative Dummy: Ena | guild hall | any | yes | 750 |  | The Templar's: race 608, neutral, class 1, level 65, size 6 (Live: Training Dummy: Gates of Discord) |
| 66705 | Decorative Dummy: Fana | guild hall | any | yes | 750 |  | The Templar's: race 608, neutral, class 1, level 70, size 6 (Live: Training Dummy: Omens of War) |
| 66706 | Decorative Dummy: Geza | guild hall | any | yes | 750 |  | The Templar's: race 608, neutral, class 1, level 70, size 6 (Live: Training Dummy: Dragons of Norrath) |
| 66707 | Decorative Dummy: Heda | guild hall | any | yes | 750 |  | The Templar's: race 608, neutral, class 1, level 70, size 6 (Live: Training Dummy: Depths of Darkhollow) |
| 66708 | Decorative Dummy: Izah | guild hall | any | yes | 750 |  | The Templar's: race 608, neutral, class 1, level 70, size 6 (Live: Training Dummy: Prophecy of Ro) |
| 66709 | Decorative Dummy: Jaka | guild hall | any | yes | 750 |  | The Templar's: race 608, neutral, class 1, level 70, size 6 (Live: Training Dummy: The Serpent's Spine) |
| 66710 | Decorative Dummy: Kala | guild hall | any | yes | 750 |  | The Templar's: race 608, neutral, class 1, level 70, size 6 (Live: Training Dummy: The Buried Sea) |
| 66711 | Decorative Dummy: Lena | guild hall | any | yes | 750 |  | The Templar's: race 608, neutral, class 1, level 75, size 6 (Live: Training Dummy: Secrets of Faydwer) |
| 66712 | Decorative Dummy: Meda | guild hall | any | yes | 750 |  | The Templar's: race 608, neutral, class 1, level 80, size 6 (Live: Training Dummy: Seeds of Destruction) |
| 66713 | Decorative Dummy: Neza | guild hall | any | yes | 750 |  | The Templar's: race 608, neutral, class 1, level 85, size 6 (Live: Training Dummy: Underfoot) |
| 66714 | Decorative Dummy: Ozah | guild hall | any | yes | 750 |  | The Templar's: race 608, neutral, class 1, level 90, size 6 (Live: Training Dummy: House of Thule) |
| 66715 | Decorative Dummy: Paza | guild hall | any | yes | 750 |  | The Templar's: race 608, neutral, class 1, level 95, size 6 (Live: Training Dummy: Veil of Alaris [Easy]) |
| 66716 | Decorative Dummy: Qela | guild hall | any | yes | 750 |  | The Templar's: race 608, neutral, class 1, level 95, size 6 (Live: Training Dummy: Veil of Alaris [Hard]) |

## 2. House pets with no summoning spell (59)

Their look travels in the item's house-pet model string, which Peridot's data lacks.

| Item | Item name | Icon | Placeable in |
|---|---|---|---|
| 52509 | Doomsday Beetle | 927 | guild hall, house, yard, guild yard |
| 52593 | Mini Jokester | 650 | guild hall, house, yard, guild yard |
| 54769 | Airship in a Bottle | 3991 | guild hall, house, yard, guild yard |
| 54998 | Baby Red Dragon | 2603 | guild hall, house |
| 55444 | Scarecrow Pet | 1481 | guild hall, house, yard |
| 55445 | Skeleton Pet | 3184 | guild hall, house, yard, guild yard |
| 57534 | Chocolate Bunny | 3657 | guild hall, house, yard, guild yard |
| 60380 | Olive | 1489 | guild hall, house, yard, guild yard |
| 60958 | Flame Wurmling | 1906 | guild hall, house, yard, guild yard |
| 61949 | Young Tiger Raptor | 833 | guild hall, house, yard, guild yard |
| 107105 | Vampire Bat | 791 | guild hall, house, yard, guild yard |
| 107106 | Stambles the Reincarnate | 3659 | guild hall, house, yard, guild yard |
| 107109 | Polar Bear Cub | 2909 | guild hall, house, yard, guild yard |
| 107111 | Tiny Icepup | 2907 | guild hall, house, yard, guild yard |
| 107135 | Scrounging Rat | 3303 | guild hall, house, yard, guild yard |
| 107136 | Dirty Rat | 3303 | guild hall, house, yard, guild yard |
| 107137 | Fabled Icerat | 3303 | guild hall, house, yard, guild yard |
| 107138 | Herbert the Bouncing Baby Bunny | 3659 | guild hall, house, yard, guild yard |
| 107139 | Albert the Hopping Hare | 3657 | guild hall, house, yard, guild yard |
| 107140 | David the Rambunctious Rabbit | 3656 | guild hall, house, yard, guild yard |
| 107141 | Robert the Leaping Longear | 3658 | guild hall, house, yard, guild yard |
| 107142 | Shik`Nar Spawn | 1242 | guild hall, house, yard, guild yard |
| 107143 | Venomous Crawler | 1089 | guild hall, house, yard, guild yard |
| 107144 | Itty Bitty Spider | 1089 | guild hall, house, yard, guild yard |
| 107145 | Tiny Terrorantula | 1089 | guild hall, house, yard, guild yard |
| 107146 | Aviak Starling | 2908 | guild hall, house, yard, guild yard |
| 107147 | Aviak Chick | 2908 | guild hall, house, yard, guild yard |
| 107148 | Aviak Egret | 2908 | guild hall, house, yard, guild yard |
| 107149 | Aviak Quetzel | 2908 | guild hall, house, yard, guild yard |
| 107150 | Aviak Guardian | 2908 | guild hall, house, yard, guild yard |
| 107151 | Aviak Avocet | 2908 | guild hall, house, yard, guild yard |
| 107152 | Black Bear Cub | 2909 | guild hall, house, yard, guild yard |
| 107153 | Surefall Brown Bear Cub | 2909 | guild hall, house, yard, guild yard |
| 107154 | Beautiful Black Bear | 2909 | guild hall, house, yard, guild yard |
| 107155 | Jaggedpine Brown Bear | 2909 | guild hall, house, yard, guild yard |
| 107156 | Guardian Worg | 1910 | guild hall, house, yard, guild yard |
| 107157 | Guardian Worg Pup | 1910 | guild hall, house, yard, guild yard |
| 107158 | Haze Panther | 2910 | guild hall, house, yard, guild yard |
| 107159 | Haze Panther Cub | 2910 | house, yard |
| 107160 | Icefall Leopard | 2910 | guild hall, house, yard, guild yard |
| 107161 | Icefall Leopard Cub | 2910 | house, yard |
| 107162 | Timorous Tigeraptor | 1473 | guild hall, house, yard, guild yard |
| 107163 | Dolorous Jester of Guk | 1245 | guild hall, house |
| 107164 | Obstructive Werewolf | 743 | guild hall, house |
| 107165 | Friendly Canine | 2907 | guild hall, house, yard, guild yard |
| 107166 | Friendly Puppy | 2907 | house, yard |
| 107167 | Desert Coyote | 2907 | house, yard |
| 107210 | Action Goblin: Green | 2633 | house |
| 107211 | Action Goblin: Red | 2673 | house |
| 107212 | Action Goblin: Blue | 2633 | house |
| 107213 | Action Goblin: Purple | 2633 | house |
| 107214 | Action Goblin: Tan | 2673 | guild hall, house |
| 107215 | Action Goblin: Aqua | 2633 | guild hall, house |
| 107216 | Decorative Polar Bear | 2909 | guild hall, house |
| 107217 | Decorative Black Bear | 2909 | guild hall, house |
| 107218 | Decorative Brown Bear | 2909 | guild hall, house |
| 107219 | Armored Action Centaur | 1907 | guild hall, house |
| 107350 | The Punting Gnome | 994 | yard, guild yard |
| 124273 | Miniature Action Lashun | 894 | guild hall, house |

## 3. Summons whose creature is missing (27)

Their spell names a mount or pet Peridot's database doesn't have, so EQEmu can't summon them either.

| Item | Item name | Spell | Names | Missing |
|---|---|---|---|---|
| 17572 | Collar of the Cursed Lupine | 20745 Familiar: Red Werewolf Cub | `FamiliarRedWerewolfCub` | `pets` row |
| 40700 | Familiar of Severilous | 12882 Familiar: Gilded Dragon | `FamiliarGildedDragon` | `pets` row |
| 43000 | Everburning Ember | 21381 Familiar: Fire Elemental | `FamiliarFireElemental` | `pets` row |
| 43596 | Cit Chac's Collar | 21621 Familiar: Puma Cub | `FamiliarPumaCub` | `pets` row |
| 44496 | Panpipes of the Dancer | 21639 Familiar: Satyr | `FamiliarSatyrBasic` | `pets` row |
| 45193 | Fae-Touched Gnome | 21650 Familiar: Pocket Gnome | `FamiliarPocketGnome` | `pets` row |
| 47088 | Stone Companion | 21930 Familiar: Stone Companion | `FamiliarStoneCompanion` | `pets` row |
| 47090 | Ticking Companion | 21932 Familiar: Ticking Companion | `FamiliarTickingCompanion` | `pets` row |
| 47091 | Springwork Spider | 21933 Familiar: Springwork Spider | `FamiliarSpringworkSpider` | `pets` row |
| 47812 | Captured Bone of the Lost | 21957 Familiar: Zombie | `FamiliarZombie` | `pets` row |
| 48090 | Unstable Ooze | 22473 Unstable Ooze | `FamiliarUnstableOoze` | `pets` row |
| 48092 | Spitting Ooze | 22475 Spitting Ooze | `FamiliarSpittingOoze` | `pets` row |
| 48979 | Guardian Wolf Whistle | 16905 Familiar: Guardian Wolf | `FamiliarGuardianWolf` | `pets` row |
| 48991 | Erollisi's Doll | 16914 Familiar: Erollisi's Doll | `FamiliarErrolisiDoll` | `pets` row |
| 48992 | Caged Aggressive Rat | 16915 Familiar: Aggressive Rat | `FamiliarAggressiveRat` | `pets` row |
| 52198 | Walking Death | 19999 Familiar: Walking Death | `FamiliarWalkingDeath` | `pets` row |
| 52285 | Clockwork Fireworks Engineer | 24065 Familiar: Clockwork Fireworks Engineer | `FamiliarFireworksEngineer` | `pets` row |
| 53769 | Verdant Selyrah Whistle | 12327 Verdant Selyrah | `SumMSLm4Run2` | `horses` row |
| 54931 | Spore Curer Bait | 24056 Spore Curer | `FamiliarDCSporali` | `pets` row |
| 54977 | Drake of Pure Nature | 17846 Drake of Pure Nature | `FamiliarPureNature` | `pets` row |
| 56063 | Familiar of Thankful Days | 23489 Familiar: Edible Aviak | `FamiliarEdibleAviak` | `pets` row |
| 57388 | Santug Claugg Familiar | 27380 Familiar: Santug Claugg | `FamiliarSantugClaugg` | `pets` row |
| 57888 | Solusek Ro Puppet | 27789 Solusek Ro Puppet | `FamiliarSolusekRoDoll` | `pets` row |
| 60939 | Venomous Raptor Familiar | 9696 Venomous Raptor Familiar | `FamiliarVenomousRaptor` | `pets` row |
| 61951 | Maraca Familiar | 27855 Maraca Familiar | `FamiliarMaraca` | `pets` row |
| 64702 | Tumbleweed Twig | 12340 Familiar: Tumbleweed | `FamiliarTumbleweed` | `pets` row |
| 64713 | Steamwork Companion | 12342 Familiar: Steamwork Flier | `FamiliarSteamworkCompanion` | `pets` row |

## Reference: NPCs in the Guild Lobby and classic Guild Hall (46)

| Zone | NPC | NPC name | Class | Race | Gender | Texture | Merchant list |
|---|---|---|---|---|---|---|---|
| guildhall | 345006 | #Healer | 1 Warrior | 240 | 2 | 0 |  |
| guildhall | 345005 | #Porter | 1 Warrior | 240 | 2 | 0 |  |
| guildhall | 345002 | a bank broker <Banker> | 40 Banker | 1 | 1 | 10 |  |
| guildhall | 345000 | Yenny Werlikanin <General Supplies> | 41 Merchant | 12 | 1 | 15 | 345000 |
| guildhall | 345004 | Zeflmin Werlikanin <Portal Crystals> | 41 Merchant | 12 | 0 | 15 | 345004 |
| guildhall | 345001 | Melody the Singer of Great Deeds <Guild Tribute Master> | 64 Guild Tribute Master | 1 | 1 | 3 |  |
| guildhall | 345003 | a guild treasurer <Guild Banker> | 66 Guild Banker | 1 | 0 | 11 |  |
| guildlobby | 344018 | Neighborhood Guild Hall | 0 none | 127 | 0 | 0 |  |
| guildlobby | 344022 | Standard Guild Hall | 0 none | 127 | 0 | 0 |  |
| guildlobby | 344015 | A Corpse Harvester <Corpse Destroyer> | 1 Warrior | 85 | 2 | 11 |  |
| guildlobby | 344002 | A Priest of Luclin <Corpse Summoner> | 1 Warrior | 128 | 0 | 11 |  |
| guildlobby | 344003 | A Priest of Luclin <Corpse Summoner> | 1 Warrior | 6 | 0 | 11 |  |
| guildlobby | 344004 | A Priest of Luclin <Corpse Summoner> | 1 Warrior | 3 | 0 | 11 |  |
| guildlobby | 344020 | A Priest of Luclin <Corpse Summoner> | 1 Warrior | 6 | 0 | 11 |  |
| guildlobby | 344028 | A Priest of Luclin <Corpse Summoner> | 1 Warrior | 3 | 0 | 11 |  |
| guildlobby | 344032 | A Priest of Luclin <Corpse Summoner> | 1 Warrior | 128 | 0 | 11 |  |
| guildlobby | 344001 | A Priestess of Luclin <Corpse Summoner> | 1 Warrior | 6 | 1 | 11 |  |
| guildlobby | 344024 | A Priestess of Luclin <Corpse Summoner> | 1 Warrior | 6 | 1 | 11 |  |
| guildlobby | 344017 | Annabelle <Housing Information> | 1 Warrior | 1 | 1 | 15 |  |
| guildlobby | 344031 | FSP-General-NoEffect | 1 Warrior | 567 | 2 | 0 |  |
| guildlobby | 344007 | Guardian Jasmina | 1 Warrior | 1 | 1 | 3 |  |
| guildlobby | 344039 | Guardian Jasmina | 1 Warrior | 1 | 1 | 3 |  |
| guildlobby | 344012 | Guardian Krogone | 1 Warrior | 1 | 0 | 3 |  |
| guildlobby | 344033 | Guardian Krogone | 1 Warrior | 1 | 0 | 3 |  |
| guildlobby | 344008 | Guardian Mandavar | 1 Warrior | 1 | 0 | 3 |  |
| guildlobby | 344040 | Guardian Mandavar | 1 Warrior | 1 | 0 | 3 |  |
| guildlobby | 344005 | Guardian Omanaru | 1 Warrior | 130 | 0 | 3 |  |
| guildlobby | 344037 | Guardian Omanaru | 1 Warrior | 130 | 0 | 3 |  |
| guildlobby | 344006 | Guardian Rooksis | 1 Warrior | 3 | 1 | 3 |  |
| guildlobby | 344035 | Guardian Rooksis | 1 Warrior | 3 | 1 | 3 |  |
| guildlobby | 344011 | Guardian Velldro | 1 Warrior | 130 | 0 | 3 |  |
| guildlobby | 344034 | Guardian Velldro | 1 Warrior | 130 | 0 | 3 |  |
| guildlobby | 344009 | High Priest of Luclin <Soulbinder> | 1 Warrior | 1 | 0 | 11 |  |
| guildlobby | 344036 | High Priest of Luclin <Soulbinder> | 1 Warrior | 1 | 0 | 4 |  |
| guildlobby | 344010 | High Priestess of Luclin <Soulbinder> | 1 Warrior | 1 | 1 | 11 |  |
| guildlobby | 344026 | High Priestess of Luclin <Soulbinder> | 1 Warrior | 1 | 1 | 11 |  |
| guildlobby | 344041 | Sage Vonglor | 1 Warrior | 12 | 0 | 1 |  |
| guildlobby | 344038 | #Kesiri Dawnlark <Guild Halls> | 8 Bard | 4 | 1 | 3 |  |
| guildlobby | 344014 | A Shrewd Banker <Banker> | 40 Banker | 11 | 0 | 3 |  |
| guildlobby | 344023 | A Shrewd Banker <Banker> | 40 Banker | 11 | 0 | 3 |  |
| guildlobby | 344000 | A Disciple of Luclin <Summoning Reagents> | 41 Merchant | 12 | 0 | 11 | 344000 |
| guildlobby | 344030 | A Disciple of Luclin <Summoning Reagents> | 41 Merchant | 12 | 0 | 11 | 344000 |
| guildlobby | 344013 | Magus Alaria | 41 Merchant | 1 | 1 | 13 | 22114 |
| guildlobby | 344021 | Magus Alaria <Magus> | 41 Merchant | 1 | 1 | 13 | 22114 |
| guildlobby | 344029 | augment aura | 62 LDoN Treasure | 127 | 0 | 0 |  |
| guildlobby | 344025 | Tavid Dennant | 70 Alternate Currency Merchant | 1 | 0 | 14 |  |
