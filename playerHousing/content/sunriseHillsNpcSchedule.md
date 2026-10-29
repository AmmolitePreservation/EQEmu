# Sunrise Hills NPC schedule

What Live's Sunrise Hills NPCs do on their own, for NPC-14 (the stage show) and NPC-12 (Gebsle Nogflop's rounds), read from MQ2PeridotLive's behavior recordings in livedumps. Everything here repeats identically cycle after cycle; none of it depended on players being near.

## Sources

| livedumps capture | Instance | What it gives |
|---|---|---|
| `peridotLiveBehavior_bristle_neighborhood_1634_Pariator_20261001-011228.txt` | 1634 | 61.5 h, 46 complete cycles, Pariator 34 units from the stage: every walk, animation, appearance change and line |
| `..._4719_Telashak_20261018-224825.txt`, `..._4727_Sukarin_20261018-232757.txt` | 4719, 4727 | 55 h each, 46 cycles, walks and sitting only |
| `..._16096_Telashak_20261023-194801.txt`, `..._16098_Sukarin_20261023-195411.txt` | 16096, 16098 | 131 h each, 109 cycles, walks and sitting only |
| the other `peridotLiveBehavior_bristle_neighborhood_*` captures | 3895, 3914, 4112, 4181 | shorter runs, the same timing |

The later recordings were made 370 units or more from the stage and hold no NPC lines and no animation changes other than sitting and standing, so only the 1634 recording has the show's lines and gestures; Pariator's chat log (the owner's PC, `eqlog_Pariator_bristle.txt`, 2026-09-29 to 10-16, 66 fully heard cycles) agrees with it line for line.

**Conventions.** Positions are the client's x, y and z as the recorder writes them (MacroQuest's X and Y; a `/loc` prints y, x). Spawn points come from spawn records; walk points come from move records, whose z is the floor the NPC stands on, about 2.7 below the spawn record's z for the same spot. Headings are the client's 0-512. Animation ids are Live's (the spawn's Animation field); held items are model numbers (IT###) from the spawn's equipment, primary then secondary. Times are minutes:seconds from **t0**, the moment Arwik stands up as the client sees it; a line arrives about 0.2 s before the whole second it is listed at.

## The clock

- **One cycle of 72 minutes:** every cycle measured 72:00 within 6 s (109 in a row in instance 16098).
- **One clock for every instance.** All nine instances recorded between 2026-09-30 and 10-29 start their cycles at the same moment: instances 4112 and 4181 made the same move within the same second on 2026-10-17. An instance does not get its own timer.
- **A server restart moves it.** t0 fell 53.8 minutes past a 72-minute boundary counted from the Unix epoch (UTC) from 2026-09-30 to the 2026-10-21 downtime, and 18.3 minutes past one since.
- **Probably the game clock.** 72 minutes is a Norrath day and every step falls on a whole 3-minute mark (a game hour) after t0. The one `/time` reading (2026-09-30 04:41:22 UTC, '7 AM') puts t0 between 3:08 and 4:08 AM game time, which makes it 4:00 AM if the steps are game hours. Not proven: one `/time` next to a cycle start would settle it.
- Gebsle Nogflop's rounds run on the same clock (below).

## The stage show (NPC-14)

### Cast

| NPC | Race, gender, class, level | At t0 (x, y, z), heading | Held |
|---|---|---|---|
| Arwik Nimbletune | 522 Drakkin female (DKF), 1, 20 | (2089.87, -2697, 5.66), 293 | nothing |
| Lysric Loresinger | 1 Human female (HUF), 1, 20 | (2087.12, -2695.75, 5.58), 291 | nothing |
| Claire Nybright <Paintings> | 7 Half Elf female (HAF), 41, 60 | the stage spot (2056.5, -2793.9; move z 8.25), 356 | primary 500, then 11220, then 11140 (timeline); secondary 500 |
| Tanya Bekines <Guildhalls> | 1 Human female (HUF), 41, 60 | (2044.25, -2715.87, 2.95), 155 | secondary 10603 |
| Brent Harnak <Guildhall Accessories> | 1 Human male (HUM), 41, 60 | (2048, -2707.5, 2.95), 170 | secondary 10601 |

Every walk below is at speedRun 0.5 and ends by facing the heading given. Lysric, Brent and Tanya never walk home: at the times marked *home* they are back at their t0 point and heading at once.

### Timeline

| Time | Who | What |
|---|---|---|
| 0:00 | Arwik | stands; walks (2089.4, -2698.4) to (2081.0, -2721.9) by 0:02.5, faces 24 |
| 0:02 | Claire | walks from the stage spot (2056.0, -2792.5) by (2046.7, -2768.3), (2098.0, -2761.1), (2112.6, -2753.6), (2130.3, -2770.8), (2132.6, -2792.7) to her seat (2122.4, -2809.8) by 0:16.4, faces 310.75; sits at 0:16.5 |
| 0:15-2:50 | Arwik | her seven lines (below) |
| 3:00 | Arwik | walks back to (2089.9, -2697.0) by 3:02.6, faces 293, sits |
| 12:00 | Claire | primary 500 becomes 11220; stands; walks to (2123.0, -2795.4) by 12:03.5, faces 483; works there until 39:00 |
| 24:00 | Brent | walks from home to (2058.9, -2729.9) by 24:02.4, faces 52; plays until 39:00 |
| 25:00-38:00 | Brent | solo line slots |
| 39:00 | Brent | *home* |
| 39:00 | Claire | primary 11220 becomes 11140; walks from (2124.4, -2794.4) by (2128.2, -2795.7), (2132.0, -2792.1), (2131.1, -2772.7), (2113.6, -2754.3), (2095.8, -2761.5), (2048.8, -2769.0) to the stage spot (2056.5, -2793.9) by 39:15, faces 356; stands there until 0:02 |
| 39:00 | Lysric | stands; walks (2086.5, -2697.0) to (2076.0, -2720.9) by 39:02, faces 42 |
| 40:00-44:00 | Lysric | line slots |
| 45:00 | Lysric | sits; *home* |
| 45:00 | Tanya | walks from home to (2082.9, -2734.9) by 45:04, faces 11; plays until 72:00 |
| 46:00-56:00 | Tanya | solo line slots |
| 57:00 | Tanya | walks to (2068.0, -2728.0) by 57:01.5, faces 42 |
| 57:00 | Brent | walks from home to (2072.9, -2729.9) by 57:03, faces 31; plays until 72:00 |
| 58:00-71:00 | Brent and Tanya | duet line slots |
| 72:00 = 0:00 | Brent, Tanya | *home* |
| 72:00 = 0:00 | Claire | primary 11140 becomes 500 |

### Animations

The client logs a change of animation, next animation or stand state; "a/b" below is two animations taking turns, each held about the time given.

| Who | When | Animations |
|---|---|---|
| Arwik | sitting (3:03 to 0:00) | 121 held throughout |
| Arwik | 0:00-3:00 | 155, 166 and 177 (about 3 s each, five times a minute between them), each followed by 110 (0.8 s); 104 at the start and end, 121 once at the start |
| Lysric | sitting | 38/73 (12.5 s / 2.7 s); 33 as she sits and stands |
| Lysric | 39:00-45:00 | 32, 44, 57, 59 and 71 in turn (1.4 to 2.6 s each, about 25 changes a minute); 17 then 32 as she starts |
| Claire | sitting (0:16-12:00) | 38/73 (12.5 s / 2.7 s); 33 as she sits and stands |
| Claire | working (12:00-39:00) | 71/72 (about 8 s / 2.7 s), with 57 and 64 about once a minute each and 48, 77, 43, 32, 26 or 17 every few minutes |
| Claire | on the stage spot (39:15-0:02) | 71/72 (11 s / 2.7 s), with 30, 54, 64 and 65 each about once every four minutes |
| Tanya | idle (0:00-45:00) | 71/72 (12.3 s / 3.0 s) |
| Tanya | playing (45:00-72:00) | 40/71 (1.8 s / 0.6 s); 17 then 32 as she starts at 45:00 and 57:00 |
| Brent | idle (0:00-24:00, 39:00-57:00) | 71/72 (15.5 s / 4.7 s) |
| Brent | playing (24:00-39:00, 57:00-72:00) | 41/71 (3.3 s / 1.5 s); 17 then 32 as he starts |

### Lines

All of them are emotes, the NPC's name first, as written here (Live's own spellings kept). Counts are from the 46 cycles of the 1634 recording.

**Arwik, every cycle, at fixed times:**

| Time | Line |
|---|---|
| 0:15 | Arwik Nimbletune fumbles nervously as she takes the stage. |
| 0:35 | Arwik Nimbletune begins in a quiet, nervous voice, 'Oh graceful flower, so sweet and so fresh...' |
| 1:05 | Arwik Nimbletune continues, 'Oh beautiful sapphire flower, you are the evening's breath...' |
| 1:35 | Arwik Nimbletune stumbles, 'Oh perfect radiant flower...' |
| 2:05 | Arwik Nimbletune continues, '...your colors are unmatched...' |
| 2:35 | Arwik Nimbletune smiles, 'Oh my beautiful, radiant flower...' |
| 2:50 | Arwik Nimbletune trails off quietly, 'Your love I shall soon catch...' |

**The other segments: a slot at each whole minute listed.** About two slots in three carry a line (1,370 of 2,024 slots), drawn evenly from the segment's lines; a slot never holds two; in about one slot in twenty the line comes 5 s early (at 0:55 of the minute before).

| Segment (slots) | Line | Seen |
|---|---|---|
| Brent's solo, 25:00-38:00 (14) | Brent Harnak beings a quiet melody reminiscent of the sounds of Freeport. | 91 |
| | Brent Harnak strikes a fiercely dissonant note in the middle of his performance. | 76 |
| | Brent Harnak transitions his tune to something akin to a march, thoughts of a militia force come to mind. | 88 |
| | Brent Harnak plays with remarkable precision as he performs the 'Songweaver's Anthem'. | 90 |
| | Brent Harnak lowers the volume of his melody to a whisper. | 87 |
| Lysric, 40:00-44:00 (5) | Lysric Loresinger begins reciting a short poem about the beauty of Ayonae Ro. | 33 |
| | Lysric Loresinger looks up to the sky before slowly retelling the ancient story of Sionachie Heartsinger's travels through Kunark. | 36 |
| | Lysric Loresinger delivers a concise lyrical limerick about the difficulty of sharing a name with her siblings. | 33 |
| | Lysric Loresinger recites a short sonnet about the harrowing adventures of Antonican gypsies. | 31 |
| | Lysric Loresinger smiles as she addresses the audience, her fans captivated by her stunning voice. | 27 |
| Tanya's solo, 46:00-56:00 (11) | Tanya Bekines recites a solemn tale of memories of her sister as her lute-playing becomes quieter. | 84 |
| | Tanya Bekines slows the pace of her song as she begins retelling the story of a young bard leaving her family in Qeynos. | 56 |
| | Tanya Bekines begins strumming an invigorating tune while belting out the lyrics of a long-forgotten battle-hymn. | 71 |
| | Tanya Bekines strikes at her instrument furiously and stamps her feet as she howls out a song detailing the intricate details of Orc execution. | 68 |
| | Tanya Bekines smiles as she surveys the audience and plucks a festive melody out on her lute. | 61 |
| The duet, 58:00-71:00 (14) | Tanya Bekines nods to Brent as she ups the tempo of their song, an instrumental highlight to the epic tale of the Queen of Thorns' journey to Taelosia. | 85 |
| | Tanya Bekines sings a soulful ballad about the life and adventures of Cros Treewind. | 68 |
| | Tanya Bekines strikes each string of her lute with a look of vicious contempt as the performers play a song reminiscent of the chaotic din of Deathknell. | 71 |
| | Brent Harnak strikes a particularly peaceful harmony with Tanya's voice as she sings a song detailing the tranquility of the Qeynos docks. | 66 |
| | Brent Harnak echoes Tanya's voice as she sings over the somewhat chaotic beauty of their instruments. | 69 |
| | Brent Harnak plays a piercing note overtop Tanya's playing. She laughs and looks back to him before continuing the song. | 79 |

## Gebsle Nogflop's rounds (NPC-12)

Gebsle Nogflop <House Pets>: race 12 Gnome male (GNM), class 41, level 85, height 3; primary 126, secondary 11037 (11032 in instance 4727, so Live varies it).

**Two rounds per show cycle**, 880 recorded in nine instances; 846 of them follow the route below, and the rest were cut off by a recording's start or end or lack one of its short pauses. Each round starts with him appearing at the portal, (1960.0, -2911.0, 2.00) heading 73.25, at **3:13** (round A) or **45:13** (round B) after t0, and ends with him running into the portal and vanishing: round A at about 45:07, round B at about 3:07 of the next cycle, about 6 s before the next round's appearance. The rounds walk the same route and differ only in the last rest. He walks at speedRun 0.3 and runs the last leg at 2. Between walks he idles on 71/72; 17 and 32 come about five times a round, 26 twice, 18 once. His one line, heard once in Pariator's chat log, 'Gebsle Nogflop  steps through the shimmering blue portal only to reappear moments later!' (Live's double space), is absent from every recording.

Times from his appearance; walk points from the 1634 recording (instances 4727 and 16098 agree within a few units):

| Time | Walk | Then |
|---|---|---|
| 0:00-0:07 | to (1997.2, -2881.2, 2.62) | 0.5 s pause |
| 0:07-1:01 | (1962.4, -2806.4), (1950.6, -2769.0), (1929.0, -2736.6), (1900.3, -2710.3), (1808.2, -2668.1), (1785.8, -2650.9), (1780.9, -2646.5), (1771.4, -2611.5, 4.00) | 0.5 s pause |
| 1:01-2:58 | (1779.6, -2576.6), (1799.2, -2547.8), (1818.5, -2527.0), (1909.5, -2433.2), (2113.4, -2253.5), (2121.4, -2242.4), (2127.4, -2228.6), (2127.0, -2209.0), (2099.4, -2162.4), (2051.5, -2095.2), (2048.6, -2048.8), (2044.8, -2020.9), (2040.0, -2010.0, -73.38) | faces 260; rests until 11:24 |
| 11:24-13:18 | (2067.1, -2045.6), (2089.0, -2084.5), (2115.4, -2149.4), (2141.4, -2187.4), (2168.8, -2216.6), (2181.1, -2224.6), (2194.8, -2228.8), (2278.6, -2227.1), (2320.1, -2231.1), (2399.4, -2264.8), (2439.9, -2277.9), (2445.9, -2288.1), (2442.8, -2314.8), (2443.1, -2357.9), (2437.8, -2396.4), (2390.2, -2460.1), (2357.1, -2521.1, 1.12) | 0.6 s pause |
| 13:18-13:45 | (2344.0, -2611.0), (2360.2, -2702.4, 1.50) | 0.4 s pause |
| 13:45-14:07 | (2323.6, -2727.5), (2301.8, -2732.0), (2295.0, -2816.9, 1.50) | faces 48; rests until 22:11 |
| 22:11-22:53 | (2299.1, -2733.8), (2163.6, -2740.9), (2138.2, -2744.6), (2095.0, -2760.0, 2.00) | 0.5 s pause |
| 22:53-23:06 | (2047.6, -2769.5), (2027.1, -2781.7), (2027.9, -2772.0) | 0.4 s pause |
| 23:06-23:13 | (2021.1, -2796.5), (2009.5, -2818.2, 2.00) | faces 69; rests until 41:45 (round A) or 29:45 (round B) |
| 5 s from 41:45 or 29:45 | runs (2022.6, -2793.8), (2010.8, -2792.5), (1980.0, -2797.4), (2009.7, -2856.2), (2014.1, -2868.1), (2011.4, -2879.9), (1954.0, -2904.9, 2.25) | faces 317 and vanishes about 4.5 s later; the next round appears about 4.4 s after that |
