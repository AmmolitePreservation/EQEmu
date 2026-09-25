# VoteResultsWnd ("Polls")

**Status:** done (chunk 3 of the polls work; data model in `../polls.md`). Code `0x792c00`–`0x7941f0`; constructor `0x793880`; notification handler `0x793b30`; results fill `0x793300`; usage map `VoteResultsWnd.usage.md`.

## The poll list (`VRW_ClosedQuestions_List`)

**Which polls appear** (`0x7931f0`, `0x792f70`): every poll whose **state is neither Initialized (0) nor Archived (4)** and whose id is in **set A or set B** from `0x7497` (`0x5a6db0`): Upcoming, Open and Closed polls the player may see. Columns (`0x792e20`): **Status** (the state's name, `0x7f9d10`), **Poll** (the question), **Open Date**, **Close Date** (formatted dates), **Polling Location** (record `+0x38`). The row's data is the poll id. Refreshed by `0x15e2`, `0x39bb` and `0x7497`.

## Selecting a poll (`0x793300`)

**"Your Vote:"** (`VRW_SelectionValue_Label` in `VRW_Selection_Screen`), by the poll's selection type `+0x2c`: **1** (property) = your vote entry's text (`0x52b5`, e.g. the plot's name); **2** = your chosen option's label; **3** = all your chosen labels joined with ", ".

**Results** (`VRW_Results_List`, from `0x3311`, `0x7fa690`): one row per result, **Rank** (row rank), the **label** (the row's text for property polls, otherwise the key's `0x6860` label) and the **vote count**, which is **left blank unless a character check passes** (player object vtable `+0x90`; the player's GM flag (spawn `+0x25c`, Q7)). With no results: **"Results for this poll are not available."**, plus **" Check back later."** when record `+0x4c` is 0.

## Buttons

| Control | Binding | Behavior |
|---|---|---|
| `VRW_Vote_Btn` ("Open Polls") | `+0x234` | opens the Vote window in mode 2 (question polls); with none to show: `0x1de2` "There are no active polls to vote on." |
| `VRW_WinnerTeleport_Btn` ("Teleport to Selected") | `+0x238` | **disabled until a results row of a property poll (kind 1) is selected**; then asks `0x1acb` "Do you wish to be teleported to the selected real estate: %1?" (the row's text; game dialog `0x8b`, 30 s) and on yes sends **`0x499a` {0, poll id, result key, -1}** |

## Other controls

`VRW_ClosedQuestions_List` (`+0x228`), `VRW_Selection_Screen` (`+0x22c`), `VRW_Results_List` (`+0x230`); both lists sort numerically where the column is a number (notification `0x10`). Static: `VRW_Selection_Label` ("Your Vote:"), `VRW_VotingResults_Label` ("Voting Results:"), `VRW_Top_Screen`, `VRW_Bottom_Screen`, `VRW_Main_Layout`.
