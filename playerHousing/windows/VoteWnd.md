# VoteWnd ("Vote")

**Status:** done (chunk 2 of the polls work; data model in `../polls.md`). Code `0x7941f0`–`0x796c00`; constructor `0x795b30`; notification handler `0x794e90`; usage map `VoteWnd.usage.md`.

## Opening and modes

| Opener | Mode (`+0x228`) | Shows |
|---|---|---|
| **the Purchase window's Vote button** (`0x74d500` -> `0x795fc0`) | **1**, key `+0x22c` = **real-estate manager `+0x98`, the plot the player is on** | every **property poll** (kind 1) the player can vote on |
| **Open Polls** in the results window (`0x793b30` -> `0x796040`) | **2** | every **question poll** (kind 2) the player can vote on |
| a **chat link** "pollid=" (`0x7961c0` -> `0x7960b0`) | **3**, poll id `+0x230` | that one poll, if the player can vote on it |
| **server message `0x7a4d`** (`0x795b00`) | **3** | (one poll) |
| **auto-open** (`0x5a7360`, see `../polls.md`) | | open polls needing the player's vote, once |

"Can vote" = the poll is **Open** (state 2) and its id is in **set A** from `0x7497`. The title reads **"Open Polls (%d)"** (`0x1de3`, the count at `+0x264`); each poll's question reads **"( poll id ) question"**.

## Per poll (`0x7953c0`)

- **Kind 1 (property poll):** one **"This One"** button (a clone of `VW_Btn_Screen`, `0x7949b0`) whose option key is the window's key (**the plot**), plus **"Your Vote: …"** (`0x793f60`; "None" before voting).
- **Kind 2, single choice (`+0x2c` = 2):** a radio group "RadioGroupPoll%d" (`0x794b30`), one `VW_Radio_Screen` clone per option.
- **Kind 2, multiple choice (`+0x2c` = 3):** a checkbox group "CheckGroupPoll%d" (`0x794cc0`) limited to **record `+0x34`** ticks.
- Each option's control carries its **option key** (control `+0x34`); its label comes from the `0x6860` table (`0x7fa630`); **options without a label entry are skipped**. The player's current votes (`0x52b5`) pre-select the controls.

## Voting

**Every click sends `0x15a8` at once** (no submit button): poll id, the clicked option key, a selected byte, and the keys of the other ticked checkboxes, none for a radio or the property button (`../polls.md`). **Un-ticking your last checkbox is refused** (the client re-ticks it).

## Controls

| Control | Binding | Role |
|---|---|---|
| `VW_Main_Layout` | `+0x268` | the layout the poll sections are cloned into |
| `VW_Question_Label` | `+0x26c` | template for a poll's question |
| `VW_Btn_Screen` / `VW_Btn` / `VW_Btn_Label` | `+0x270` | the property-poll template ("This One", "Your Vote:") |
| `VW_Radio_Screen` / `VW_Radio` / `VW_Radio_Label` | `+0x274` | the single-choice option template |
| `VW_Checkbox_Screen` / `VW_Checkbox` / `VW_Checkbox_Label` | `+0x278` | the multiple-choice option template |
| `VW_Close_Button` | `+0x27c` | closes the window |

The spacers (`VW_Btn_Spacer`, `VW_Radio_Spacer`, `VW_Checkbox_Spacer`) and the two layout templates in the XML are only cloned with their screens.
