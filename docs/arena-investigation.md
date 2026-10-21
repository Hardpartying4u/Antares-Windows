# Main menu: remove Network, replace Demo with Arena (investigation, 2026-10-21)

Investigation (read-only). **Step 1, removing Network, is done (2026-10-21)**: the button and
the Sound checkbox are removed in code and the Net Settings row is hidden; the menu row stays empty
for Arena. `fast-motion`, `mission-briefing` and `pause` now have Star Bound expected output, and
`options-controls` also snapshots the UTILITY page. Paths are relative to the repo root. `data/` is the
upstream data submodule (`git submodule status`), so every change below is made in code or in new
Star Bound files, never in `data/`.

## 1. Start Network Game

**Where it is.**
- The button is data: `data/interfaces/main.pn`, item `start-network-game` ("Start Network Game",
  key N, gamepad B, at y 284–294, second of the seven buttons).
- Its code is one binding in `src/ui/screens/main.cpp` (`kNetButton`, ~62): the action throws
  "Networked games not yet implemented." and the enabled check always returns false. So the button
  is drawn greyed out and can never be pressed. There is no network code in this port at all.

**What else mentions networking:**
- `Level::Type::NET` and `NetLevel` (`include/data/level.hpp`) and five net levels
  (`data/levels/net1.pn`–`net5.pn`). They are parsed at startup and unreachable from any menu.
  They must stay: removing the parser would make the data fail to load.
- The **Net Settings** action (number 32, F5 in every preset; `include/config/keys.hpp`,
  `src/config/bindings.cpp`). Pressing it does nothing (`player-ship.cpp` ~444 returns).
- **Options › Sound › "Speak Network Messages"** (`speech_on`, saved as `sound: {speech: ...}`).
  It speaks nothing in this port.
- A few `TODO(sfiera)` comments about networked games (`initial.cpp`, `play-again.cpp`,
  `messages.cpp`); harmless.

**Removing it fully would mean:**
1. Main menu: `remove_widget("start-network-game")` in `MainScreen` (the way the Key Controls
   screen already removes data widgets), and drop the `kNetButton` binding. Close the gap by moving
   the buttons below up one row (26 px) in code, or let Arena take the slot (§4).
2. Options › Sound: remove the "Speak Network Messages" checkbox the same way. Keep `speech_on` in
   the config so old configs still load.
3. Key Controls: hide the Net Settings row. The action number stays: replays store action numbers
   and configs store its binding, so it can't be deleted, only hidden and left doing nothing.
4. Leave `NetLevel` parsing and the net levels alone (upstream data).
5. Docs: `docs/current-state.md` (main menu, Options › Sound, Utility key table), `docs/design.md`
   (decision).
6. Tests: every offscreen test that snapshots the main menu changes, because the menu looks
   different. Those are `main-screen` (already fails on Windows), `options` (its expected output is
   already in `test-starbound/`) and `mission-briefing`, whose expected output is upstream's
   `test/`. It would need a Star Bound copy in `test-starbound/`, as `options` got. `options` and
   `options-controls` also change if the Sound checkbox and the Net Settings row go.

## 2. Demo

**How it works.**
- "Demo" (key D, gamepad Y) plays a **recorded replay**, not an AI-vs-AI battle. `MainScreen`
  picks one of `data/replays/*.NLRP` at random and pushes `ReplayGame`
  (`src/ui/flows/replay-game.cpp`), which replays recorded human input on a normal campaign level.
- There are three replays. Their headers give chapter ids 5, 7 and 27, which `Level::get(id − 1)`
  turns into **ch02**, **ch03** and **srtm (Space Race)**.
- Replays always run Classic controls (`modern_controls_for_level`).

**Idle attract mode.** Yes: after **30 s** with no key, mouse or gamepad input on the main menu
(`kMainDemoTimeOutTime`), `MainScreen::fire_timer` plays a random demo replay or the intro scroll
text. Any input resets the timer.

**Tests.** None use the demo replays. The replay tests use the upstream recordings in `test/`
(`scripts/test.py`), and the `modern-*` tests use `test/` recordings too. Removing the Demo button
changes only the main-menu snapshots listed in §1.6.

**Recommendation.**
- **Remove** the Demo *button*.
- **Keep** the idle attract mode as it is: it costs nothing, makes the title screen feel alive,
  and needs the replays (which live in `data/` anyway).
- Later the attract mode could also show an Arena battle, but only once Arena has an AI-only
  mode.

## 3. Arena: levels, scenarios and AI

**How levels are defined.**
- Each level is a Procyon file (`data/levels/*.pn`) with:
  - `type` (solo / net / demo) and `chapter`;
  - `players` (human or cpu, race, earning power);
  - `initials` (every ship, planet and asteroid: `base`, `owner`, `at`, `flagship`, `target`,
    optional `build` list);
  - `conditions` (when, then actions such as `win`/`lose`);
  - `briefings`, `par`, `song`.
- Ships, races, sprites and sounds are referenced by name from `data/objects`, `data/races` and
  the rest.

**Can we add our own arena levels without touching `data/`? Yes, with a small loader.**
- Resources resolve plugin → factory scenario → application folder (`src/data/resource.cpp`
  ~96–98, ~236). The level *list*, though, comes from a single place (`list_resources`): a plugin
  replaces the whole campaign, so an arena can't be a plugin next to the campaign.
- Proposed: a new Star Bound folder **`arena/`** beside `data/` and `art/` (packaged by
  `scripts/dist.py`), holding e.g. `arena/small.pn`. A small loader parses each file with the
  existing level reader (`Resource::level` / `level(pn::value)`). They are kept out of
  `plug.chapters`, so the campaign is unchanged.
- The levels name factory ships (`ish/cruiser`, `gai/gunship`, ...). Original vs Enhanced art then
  works automatically: `art/` overrides sprites at load as for any level, and `data/` is never
  touched.

**Enemy AI** (`src/game/admiral.cpp`, `non-player-ship.cpp`; more in
`docs/modern-controls.md` §1).
- Every ship not flown by a human runs the same per-ship AI. It turns toward its nearest hated
  enemy within engage range (at least 2 subsectors or its longest weapon range), closes in, fires
  and evades, and otherwise flies to its destination.
- A **cpu** admiral (`Admiral::think`) keeps choosing destinations for its ships from objects with
  `kCanBeDestination` (ships count, not just bases). Its choice is weighed by local friend and foe
  strength and the ships' `ai.target` data, so its fleet does go after the player's.
- In campaign levels the enemy is usually scripted (`target: {..., lock: true}`) or defends bases.
  The `net` levels show the cpu admiral fighting without scripts, but with bases and building.
- **Good enough for a first Arena?** Probably. Ship-to-ship fighting is the same as in the
  campaign. The unknown is how a cpu admiral with **no bases and no building** spreads its fleet:
  it may split up or dither between targets. Step 2 of §5 checks this before any UI is built.
  Fallbacks if it is weak:
  - give enemy ships an initial `target` on the player's flagship (no lock), as levels already do;
  - or give the enemy a planet to defend and the player a planet to take, as in the net levels.

## 4. Proposed Arena (first version)

- **Main menu:** "Arena" replaces "Demo". It can't use A (About Ares has it), so it keeps Demo's
  key **D** and gamepad **Y**. Same hue and style. With Network gone, Arena takes Network's slot
  under Start New Game, and the buttons below close up.
- **Arena screen:** a simple list of 3 preset battles, each with a one-line description and its
  fleet sizes. Built with the same `InterfaceScreen` widgets as Options, defined in code (no new
  data in `data/`). Proposed battles:
  1. **Skirmish** (small): Ishiman flagship plus 4 cruisers vs 6 Gaitori gunships and fighters.
  2. **Fleet action** (medium): a mixed Ishiman fleet of about 12 vs Gaitori and Cantharan, about
     15.
  3. **Armada** (large): about 20 a side, carriers launching fighters.
- **Play:** a normal level, with the `type` the solo flow expects. The player's flagship has
  `flagship: true`, and the other friendly ships are owned by the player and take orders. The
  current preset applies, with Modern mouse controls and the group bar when a Modern preset is
  chosen (Arena is not a replay, demo or tutorial, so `modern_controls_for_level` already allows
  it). No building, no money (earning 0), no briefing pages (or one short one).
- **Win:** when no enemy ships are left (a `ships` condition on the cpu player: count = 0).
  **Lose:** when your flagship is destroyed (a `destroyed` condition on the flagship). Both are
  existing condition types, so this is level data, not new code.
- **After:** the existing debriefing screen (time, kills, losses, against `par`), then back to
  the main menu. No campaign progress or save is touched.

**Later (not in the first version):**
- a fleet picker (races, sizes, points budget);
- waves (more enemies arrive over time, using the existing `create`-object actions on a timer);
- score and best times stored per battle;
- an AI-only "spectate" option that could also feed the idle attract mode;
- 2v2 sides with an AI ally.

## 5. Build order, tests and risks

1. **Remove Network** (menu button, Sound checkbox, hide the Net Settings row). Tests: new
   `test-starbound/` expected outputs for `mission-briefing`, and updated `options` and
   `options-controls`. No in-game check (key and menu only; menu drawing at 2× if wanted).
2. **Arena prototype as a test scenario** (no UI): write `arena/skirmish.pn` and run it the way
   the in-game checks already do (`level_scenario.py`-style plugin). Check the cpu fleet engages,
   the win/lose conditions fire and the debriefing appears. **Stop for review** if the AI needs
   help (§3).
3. **Arena loader**: read `arena/*.pn` with the level reader and keep them out of the campaign.
   Unit test: the files parse, have a flagship, a cpu player and win/lose conditions.
4. **Arena flow and screen**: the battle list, `ArenaGame` (like `SoloGame`, without campaign
   progress), debriefing, back to the menu. Replace the Demo button (`remove_widget` plus a new
   button). Keep the idle attract mode. Offscreen test: a Star Bound script that opens Arena,
   picks a battle and snapshots the first frames (text driver).
5. **Replay test for one arena battle** (Modern mode, injected selection and orders), so fights
   stay deterministic. Then the other two battles, an in-game check at 2×, docs and packaging.

**Risks.**
- **AI without bases** may fight poorly or spread out (§3); found at step 2.
- **Performance** of large battles. The engine caps objects (`kMaxSpaceObject`); "Armada" must stay
  well under it, fighters included.
- **Main-menu snapshots** in upstream `test/` can't be edited: each affected script needs a Star
  Bound copy of its expected output (done before for `options`).
- **Keys:** "A" is About Ares; Arena needs a free key and gamepad button.
- **Replays/recording:** Arena games aren't recorded. If replay recording is ever added, the
  arena files must ship with the game so recordings can find their level.
- **Original graphics:** untouched as long as arena levels only reference existing objects.

## 6. Step 2 results: the Skirmish prototype (2026-10-21)

**The level:** `arena/skirmish.pn` (a new Star Bound folder; `data/` untouched). It has an
Ishiman flagship (`ish/hvd`) and 4 cruisers escorting it (`target: {initial: 0}`), against 4
Gaitori gunships and 2 fighters about 4,000 units away. The Gaitori have no target and no script,
so whatever they do is the cpu admiral's own choice. There is no money (earning 0), no building
and no briefing. Par is 3 min, 6 kills, 0 losses.

**How it was run (no code changes).** The `replay` tool reads `./data` relative to where it
runs. A scratch script (`arena_run.py`, kept in the session scratchpad):
1. copies `data/` into a scratch folder;
2. adds the arena level there as chapter 90;
3. writes a replay of that chapter with **no input** (5 minutes, a chosen seed);
4. runs `replay.exe --text` from that folder.

The debriefing text says who won; the text screenshots show which ships are on screen.

**Results:**
- **The AI fights.** Within about 10 s the Gaitori gunships come to the Ishiman fleet without
  any script. The escorting cruisers engage them, and with no player input all four are lost by
  about 50 s. The gunships then kill the idle flagship.
- **Lose works:** "Your flagship was destroyed." Seed 12345 ended at about 90 s, seed 777 at
  about 40 s and seed 4242 at about 70 s.
- **Win works:** a scratch copy with only the 2 fighters ended with "The Gaitori raiders are
  destroyed." at 0:24, with 2 kills and 0 losses. The debriefing table (time, losses, kills,
  score against par) appears on a win; on a loss only the text shows.
- **Deterministic:** two runs with the same seed gave identical output.
- **Finding:** a "ships = 0" win must start **disabled**. Enabled from the start, it fired at once
  ("win" at 0:03 with no kills), because the ship counts are 0 until the first ship-think pass
  has counted them. The campaign does the same (ch16, ch20 and tut3 start it disabled). The
  prototype turns it on with a `time ≥ 5s` condition.
- **Balance:** 4 gunships and 2 fighters beat 4 cruisers whose flagship is idle. With a player
  flying and ordering the fleet, it should be a fair small fight, but that needs a playtest.

**Not checked yet:** how a human plays it (Modern orders, groups), the medium and large battles,
and the cpu admiral against a fleet that is spread out.

## 7. Step 3: the arena loader (2026-10-21)

- **Where:** `default_arena_path()` (`src/config/dirs.cpp`) is `arena/` next to the application
  data folder, as `art/` is. In the package that is `Antares\arena\`, beside `data\` and `art\`;
  for tests and tools run from the repo it is `./arena`. `scripts/dist.py` packages it.
- **Loading:** `load_arena_levels()` (`src/data/arena.cpp`) reads every `*.pn` there with the
  campaign's level reader. `PluginInit` stores the battles in `plug.arena`, by file name
  (`"skirmish"`), and never in `plug.levels` or `plug.chapters`, so the campaign, demo replays and
  replay tests are unchanged. A file that doesn't parse, or isn't a playable arena battle, is
  skipped with a line on stderr (`arena/NAME.pn: why: skipped`), so one bad file never stops the
  game.
- **What a battle must have** (`arena_level_problems()`):
  - a solo level with **no chapter**;
  - one human player and at least one cpu player;
  - a flagship for the human player;
  - a win for the human and a win for a cpu player (the loss);
  - every `ships` condition starting **disabled** (§6).
- **A finding:** a `win` action needs `text`; without it the file doesn't parse.
- **Tests** (`arena-test`, a new unit test; 35 tests now):
  - every shipped battle loads, passes the checks, and names only ships that exist in `data/objects`;
  - a good file loads, while a broken file, one with a chapter, one whose `ships` win starts
    enabled and one with no flagship are each skipped with their reason;
  - a missing folder gives nothing.
- Nothing uses `plug.arena` yet; the Arena screen and game flow are step 4.
