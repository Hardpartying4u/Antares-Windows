# Modern control mode: investigation (2026-10-20)

Read-only investigation, before any code. The goal is an **Options toggle** (Classic is the
default). In Modern it gives C&C-style control:

- select several ships: drag a box, shift-click to add, Ctrl+number control groups;
- right-click to move them;
- a **patrol** command;
- an **attack-move** command: move to a point but engage any enemy met on the way, unlike a plain
  move.

Mouse-wheel zoom already works in both modes (M2 stage 2, `d7ddff19`).

Code references are to `windows-working-build` at `f306a8ec`. `docs/controls-investigation.md` covers
the input path; this document covers orders, ship behaviour and what Modern needs.

## Summary

- **The engine already has two kinds of move.**
  - An order to an **object** (the Ctrl key) flies straight there. On the way the ship doesn't divert
    to fight, but it fires at anything in its arcs and fights back once it is hit.
  - An order to a **point** (Hold Position, Go To My Position) diverts to engage any enemy within
    about two subsectors. It then gives up after 15 s of fighting a target that doesn't hit back.
  - So a plain **move** reuses the object-order behaviour and needs no new AI. **Attack-move** is
    the point order with the 15 s give-up switched off: a one-flag change. **Patrol** is new: a small
    waypoint loop on top of attack-move.
- **The scenario data doesn't get in the way.** Orders live in engine state (`SpaceObject`,
  `Admiral`), not in `data/`. Level scripts only read the single **control** and **target**
  objects (tutorials) and give scripted orders (`target` actions). New orders need no data change,
  provided the admiral's single control object stays meaningful.
- **Multi-select fits as a selection list beside the existing control object.** The list is
  changed only by queued input, at a major tick. Its first ship stays the admiral's `control()`, so
  the minicomputer, labels and tutorial conditions keep working.
- **Recommended order:**
  1. the toggle;
  2. right-click orders for the current ship;
  3. attack-move;
  4. multi-select;
  5. control groups;
  6. patrol.

  Leave out formations, multi-point patrol queues, ordering the flagship by mouse, and mouse firing.

## 1. Orders and ship behaviour today

### 1.1 Who decides what a ship does

Each major tick (every 3rd tick, `GamePlay::fire_timer`, `src/game/main.cpp` ~473–488) runs, in
order:

1. `NonplayerShipThink()` (`src/game/non-player-ship.cpp` ~189). Every object with `kCanThink` or
   that is the flagship gets "keys" (thrust, turn, fire, warp) from its presence state. Ships in
   normal space use `ThinkObjectNormalPresence` (~439).
2. `AdmiralThink()` (`src/game/admiral.cpp` ~585). Bases earn and build, then each admiral thinks.
   **`Admiral::think` returns at once for anyone but a computer player** (~619). The AI that picks
   destinations therefore never touches the human player's ships: they move only when ordered, by
   the player or by a level script.
3. Queued input reaches `PlayerShip` (keys, clicks, wheel, gamepad), then `PlayerShip::update()`.
4. Collisions, then level conditions.

**The flagship** (`kIsPlayerShip`) is flown by the player's keys. On autopilot (Shift+Tab) the AI
flies it to the admiral's target.

**Every other ship** runs the same AI as the enemy's ships, steered by its destination:

- `destObject`: an object to go to; it follows the object as it moves.
- or `destinationLocation`: a point.
- or neither: the ship stays where it is.

### 1.2 Player orders that exist

| Order | Input | Code | Sets |
|---|---|---|---|
| Go to target | Ctrl (Order key); gamepad | `PlayerShipGiveCommand` (`player-ship.cpp` ~1146) → `SetObjectDestination` → `OverrideObjectDestination` (`admiral.cpp` ~437) | destination **object** = the admiral's target; `timeFromOrigin` = 15 s |
| Hold position | minicomputer › Special Orders | `hold_position` (`minicomputer.cpp` ~857) → `SetObjectLocationDestination` (`admiral.cpp` ~375) | destination **point** = its own location; `timeFromOrigin` = 0 |
| Go to my position | minicomputer › Special Orders | `come_to_me` (~867) | destination **point** = the flagship's location at that moment; `timeFromOrigin` = 0 |
| Fire weapon 1 / 2 / special | minicomputer › Special Orders | `fire_weapon` (~845) | one shot with `kManualOverrideFlag` |
| Transfer control | F8; minicomputer | `transfer_control` (~815) | you fly the control ship instead |
| Build | minicomputer › Build | `build_ship` → `Admiral::build` | builds at the selected base |
| Autopilot | Shift+Tab | `TogglePlayerAutoPilot` (`player-ship.cpp` ~1129) | the flagship flies to the target |

- **Who can be ordered:** ships with `kCanAcceptDestination` and without `kStaticDestination`. The
  checks are in both destination setters.
- **What "go to target" means depends on the target:**
  - a friendly ship: **escort** (`eEscortDuty`). Near it, the ship matches its heading and warps
    when it warps.
  - a friendly base: **guard** (`eGuardDuty`).
  - anything hostile: **assault** (`eAssaultDuty`).
  - a planet, for a transport or engineer: the ship's **arrive** action (land, capture) runs on
    arrival. These actions are filtered on the object arrived at, so they do nothing at a bare point.
- **Selection is single.** Each admiral has one **control** (`_considerShip`, `Admiral::control()`)
  and one **target** (`_destinationObject`, `Admiral::target()`), set by `select_object`
  (`player-ship.cpp` ~214).
  - Click picking is `PlayerShipHandleClick` (~1052) with `GetSpritePointSelectObject`. It searches
    ±16 logical px around the click, using where each sprite was drawn.
  - The 10 hotkeys (`globals()->hotKey`) hold one object each.

### 1.3 How a ship flies and fights (`ThinkObjectNormalPresence`)

- **Whom it considers.** Each tick, collision handling (`calc_locality`, `motion.cpp` ~822) records
  for every thinking object its **nearest enemy** (`closestObject`). That is the nearest
  differently-owned object it `engages()` (per the data's `ai.combat.engages` tags) that can be
  engaged or evaded.
  - `ThinkObjectResolveTarget` (~1162) makes that its `targetObject`, unless it already has a valid
    target that isn't more than twice as far away.
- **When it engages** (the first branch, ~450): the target is engageable, within `engageRange`, and
  **`timeFromOrigin < 15 s`** (`kTimeToCheckHome`).
  - `engageRange` = the larger of 2 subsectors (`kEngageRange`) and the longest weapon range.
  - The ship turns toward the target, closes to its shortest weapon range, fires, and evades if it
    is outgunned and being looked at.
- **When it doesn't engage** (~521): it flies to its destination. Along the way:
  - it still fires at a target in range when the target is ahead, and turrets (`kAutoTarget`) fire
    regardless;
  - it still veers away from threats it can evade;
  - near the destination it brakes. Escorts match their leader's heading.
- **The 15 s clock** (`timeFromOrigin`) works as a leash:
  - it grows only while the ship is in weapon range of a hated, engageable target
    (`ThinkObjectEngageTarget` ~1244);
  - it resets to 0 when the ship is within 2 subsectors of its destination (~653), when it has no
    destination (~569), or **when it is hit** (`HitObject` ~1317).

  So a ship fights for at most about 15 s of in-range combat on the way, then carries on. A ship
  that is being shot keeps fighting back.
- **The two order kinds start the clock differently:**
  - **Object orders start it at 15 s** (`admiral.cpp` ~490). The ship goes straight to its
    destination and only fights once it is hit or arrives.
  - **Point orders end at 0.** `SetObjectLocationDestination` sets 15 s (~416) and then overwrites
    it with 0 (~428), as the original did. The ship engages enemies it meets on the way.
- **On arrival** it stays there and fights anything that comes within engage range, since the clock
  keeps resetting near the destination. When a destination object dies, the ship takes over that
  object's own destination, or stops.

### 1.4 Behaviours and order types in the engine and data

- **Duties** (`dutyType`, `space-object.hpp` ~60): none, escort, guard, assault, hostile base. The
  computer admiral uses them when choosing destinations; for player ships they are bookkeeping.
- **Computer targeting** (`Admiral::think` ~609): computer players only. It scores every possible
  destination by local friend and foe strength, base need, "blitzkrieg" phases, and each ship
  type's `orderFlags` from data (soft or hard preferences for bases, friends, foes, local or
  remote, tags).
- **Per-ship data that shapes behaviour** (`data/objects/…`, `BaseObject`, `base-object.hpp`):
  - attributes such as `can_accept_destination`, `static_destination`, `can_engage`, `can_evade`,
    `hated`, `can_be_engaged`;
  - `ai.combat` (skill, which tags it engages);
  - `ai.escort` (power, need, class);
  - `ai.target` (prefer and force tags), which feeds `orderFlags`;
  - the `arrive` action and distance;
  - weapon `usage` (transportation, attacking, defense).
- **Level scripts** (`data/levels/*.pn`):
  - `target` actions give scripted orders. They call `OverrideObjectDestination` and may order
    player ships too (ch01, ch13, ch16).
  - `key` actions disable inputs (tutorials only).
  - `identity` conditions compare an object with `{control: 0}` or `{target: 0}` (tut2, tut3,
    tutz).
  - `computer`, `autopilot` and `zoom` conditions test the minicomputer screen, autopilot and zoom.
- **There is no patrol, attack-move or waypoint list anywhere in the engine or the data.**

### 1.5 How the requested commands map

| Command | Existing behaviour | New work |
|---|---|---|
| **Move** to a point | Point destinations exist (`SetObjectLocationDestination`) but nothing sends a ship to an arbitrary point. C&C's "move" behaves like the **object** order: go there, fight back only when hit. | A screen-to-world conversion for the click; a point order that starts the clock at 15 s like an object order. No AI change. |
| Right-click an enemy, friend, base or planet | Exactly the Ctrl order (assault, escort, guard, capture) | None: set the target, then order |
| **Attack-move** | Close to today's point order (engages on the way), but the 15 s leash makes ships drop a fight with a target that doesn't shoot back, such as a base, a transport or a mine | A per-ship flag that stops the clock growing (one condition in `ThinkObjectEngageTarget`) |
| **Patrol** | Nothing | Per-ship waypoints and a step that switches to the next one on arrival; attack-move behaviour between them |

### 1.6 Does `data/` constrain this?

**No, as long as three rules hold:**

1. **The admiral keeps one control and one target.** Tutorials test `{control: 0}` and
   `{target: 0}`; the minicomputer's CONTROL and TARGET lines and Build use them. A multi-selection
   must keep a *primary* ship as `control()`.
2. **Level orders win.** A scripted `target` action, or a destination set by a creation action,
   must cancel any Modern patrol or attack-move state on that ship. Otherwise a patrol would
   overrule the story. The simplest way: the two destination setters clear the Modern order state,
   and the Modern code sets it again after calling them.
3. **New state lives in the engine.**
   - Order mode and waypoints go in `SpaceObject`.
   - The selection goes in `Admiral`.
   - Groups go beside the hotkeys.
   - The toggle goes in `config.pn`.

   Nothing new is read from `data/`, and new on-screen text is built in code (as Video options
   is), not added to `data/strings`.

Two smaller data facts:

- `key` actions can only name the 32 existing mask bits (`KeyAction::Key`), so new Modern inputs
  must reuse existing masks (§2.4).
- Arrive actions are filtered on the object arrived at, so point moves and patrol can't trigger
  landing or capture by accident.

## 2. Multi-ship selection

### 2.1 Where the state lives

- **Selection list in `Admiral`:** `std::vector<{Handle<SpaceObject>, id}>`, capped (say 32).
  - The handle and id pair is the same check hotkeys use, so a destroyed and reused object slot
    drops out.
  - Its first entry is always `control()`. Single-selection code then needs no change.
  - Invalid entries (dead, recaptured by the enemy, id changed) are pruned whenever the list is
    read.
- **Groups beside the hotkeys** in `globals()`: 10 lists of the same pairs.
- **Per-ship order state in `SpaceObject`:**
  - an order mode (none, move, attack-move, patrol);
  - up to 2 patrol points and the index of the next one.

  About 24 bytes; the default constructor already clears new objects.

### 2.2 Staying deterministic

Mouse events already go through the per-tick queue (`RealInputSource::mouse_down/up`,
`input-source.cpp` ~77). They reach `PlayerShip` at a major tick, where a click is resolved against
that tick's sprite positions. Modern does the same:

- **Box select:**
  - the press point comes from the queued `MouseDownEvent`, the release from the queued
    `MouseUpEvent`;
  - at the release's tick, `PlayerShip` takes every own ship whose `sprite->where` lies in the box;
  - the rubber band on screen is drawn from the live mouse position. It is view only and changes
    nothing.
- **Right-click on empty space:** the click is converted to world coordinates at its tick with the
  inverse of `scale_to_viewport` (`motion.cpp` ~139), from `scaled_screen.bounds` and `.scale`.
- **Modifiers:** mouse events carry none. Shift and Ctrl state comes from the queued key events
  delivered earlier in the same tick; the multimap keeps them in arrival order.

This keeps a local game deterministic. Like today's clicks, though, the result depends on the
window size and zoom. If replay recording or network play ever return, the queue should carry
**resolved commands** instead of mouse events: "select these ids", "move these ids to this point".
Designing the Modern handler as "resolve, then apply a command" keeps that change small.

### 2.3 Replays

Replays store keyboard action numbers 0–43 only, map them through the current bindings, and never
contain mouse input. Recording is switched off.

**Risk:** Modern changes what some keys mean (number keys become groups, Ctrl becomes a group
modifier). A recorded Classic replay played while Modern is on would then diverge.

**Rule:** the control mode is read once when a level starts, into game state. **Replays and the
demo always run Classic.** The 16 replay tests then prove Classic is unchanged.

### 2.4 Tutorials and `key_mask`

**Finding:** Antares enforces only some of the masks tutorials set. Masked today:

- thrust, turn, fire and warp, through the key bits in `PlayerShip::update`;
- zoom in and out, zoom shortcuts, and the mouse (`kMouseMask`);
- the minicomputer's Build, Special and Message menus.

**Not enforced:** Select friendly, foe or base; Target (Shift); Order (Ctrl); minicomputer
up/down/accept/back. The tutorials disable these (tut1–3 use `select_friend`, `order` and so on),
but the keys still work. This is an existing gap; Modern doesn't need to fix it in Classic.

**Proposal:**

- **Tutorials run Classic.** Their text and conditions teach the Classic keys ("press Ctrl"). A
  tutorial started while Modern is on uses Classic for that level and says so in the opening
  message.
- **Modern inputs still honour the masks** that match them, in case a chapter ever uses masks.
  Today only tutorials do.

  | Modern input | Masks it honours |
  |---|---|
  | click and box select | `mouse` and `select_friend` |
  | right-click orders | `mouse` and `order` |
  | attack-move and patrol | additionally `comp_special` (the Special Orders menu) |
  | groups | `select_friend` |

### 2.5 The AI

- **Computer admirals:** no change. They never read selections.
- **The player's own ships:**
  - they still use the shared ship AI;
  - an attack-move or patrol ship still evades when outgunned (faithful);
  - with no order, ships keep today's behaviour: stay put and fight what comes within range.
- **What can be selected.** Box select takes only own ships that can accept orders: not the
  flagship, not bases, not `kStaticDestination` ships.
  - Clicking a base still selects it alone, since Build needs a base as `control()`.
  - Clicking the flagship selects it alone. Orders to it do nothing; autopilot stays on Shift+Tab.

## 3. Engaging on the move (attack-move and patrol)

**How a moving ship decides to engage** (§1.3): it engages its nearest enemy when that enemy is
within engage range and its 15 s clock is under the limit. A plain object order starts the clock
at the limit, so the ship only fights after being hit or on arrival.

**Least invasive build:**

1. **Move to point:** a new function beside `SetObjectLocationDestination` that, after calling it,
   sets `timeFromOrigin = kTimeToCheckHome` and order mode = move. This copies the object order's
   behaviour, with no AI edit.
2. **Attack-move:** call `SetObjectLocationDestination` (which leaves the clock at 0) and set order
   mode = attack-move.
   - One AI edit: in `ThinkObjectEngageTarget` (~1244), don't advance `timeFromOrigin` while the
     mode is attack-move. The ship then fights each enemy it meets until it dies or leaves range,
     and resumes the move.
   - Everything else (which enemy, approach, firing, evading) is the existing code.
3. **Patrol:** attack-move plus two points.
   - In `NonplayerShipThink`, before `ThinkObjectNormalPresence`, a ship in patrol mode that is
     within `max(arrive distance, 1/4 subsector)` of its current point gets the other point as its
     destination. The patrol state is kept.
   - "Point A" is the ship's position when the order is given.
4. **Cancelling:** any other destination (a new player order, a scripted `target` action, a
   creation action) clears the mode through the two setters (§1.6, rule 2).

All of this sits behind `order mode != none`. Classic never sets it, so Classic play and the replay
tests run the original code path. The patrol switch and the clock exception are each a few lines in
the hot loop, and cost nothing when the mode is none.

## 4. What the player sees and does

All drawing is in logical pixels, like the rest of the game. At UI N× it is magnified with crisp
nearest-neighbour pixels, so a 1 px line is N physical pixels and sizes stay in proportion to
sprites. On the wide window the play area is wider (2400×720 logical at 5120×1440, UI 2×). A box can
cover more ships and right-click points can be further to the sides; both use the same mapping.
Mouse coordinates are already exact at every scale (M1).

| Feature | Player does | Player sees | Draw notes |
|---|---|---|---|
| Mode toggle | Options › Key Controls: **Controls: Classic / Modern** (built in code next to Reset to Defaults), saved in `config.pn` | Takes effect from the next level; a tutorial notes it runs Classic | Built in code like Video options; not in data |
| Click select | Left-click an own ship | Yellow CONTROL label (as now) plus a selection marker | |
| Shift-click | Adds or removes a ship | Marker appears or disappears | Shift is Classic's target modifier, so Modern's Shift means "add" with the mouse only |
| Box select | Left-drag from empty space in the play area; a drag starting over the panels is ignored | Thin yellow rubber-band rectangle while dragging; markers on release | Clipped to the viewport; drawn after sprites, before the panels |
| Selection markers | n/a | Four small yellow corner brackets around each selected ship, at least 5 px so 1:16 blips still read; the primary is brighter | Sprite size × current scale, from `sprite->where`; off-screen ships aren't marked (radar highlighting is optional) |
| Right-click | On empty space: move. On an enemy: attack. On a friend or base: escort or guard. On a planet: capture (transports) | A short-lived marker at the point (a green ring for move); the target gets the sky-blue TARGET label, as a Classic right-click does | Order sound `sys.sound.order()` |
| Attack-move | **Ctrl + right-click** on empty space or an enemy | Red ring at the point | Ctrl is free with the mouse in Modern (§5, groups) |
| Patrol | **P**, then right-click the far point (left-click cancels) | While armed, the crosshair is tinted; then a blue dashed line between the two points for selected patrolling ships, with a small diamond at each end | World points converted with `scale_to_viewport` each frame |
| Order cursor | Hover in the play area | The existing crosshair, tinted by what a right-click would do (red over an enemy, green over empty space, blue while patrol is armed) | Today's cursor is drawn as lines (`GameCursor::draw`), so no new art |
| Control groups | **Ctrl+1…0** assigns the selection; **1…0** selects it; Shift+number adds to the selection | A small digit beside each member's marker; the CONTROL line reads the primary's name plus "+N" | Digits in the tactical font |
| Wheel zoom | Wheel | As now (both modes) | Done |

**Things the game doesn't have yet:**

- a control-mode preference and the toggle;
- the mode captured per level, with Classic forced for replays, demos and tutorials;
- a selection list in `Admiral`, and group lists;
- a screen-to-world inverse of `scale_to_viewport`;
- per-ship order state, the move/attack-move/patrol setters, the clock exception and the patrol
  step;
- Modern mouse handling in `PlayerShip` (drag tracking, modifier tracking from key events, the P
  arming state);
- drawing: rubber band, markers, order rings, patrol lines, group digits, cursor tint;
- minicomputer Special Orders (Hold, Come To Me, Fire) applied to every selected ship;
- Ctrl handled differently in Modern. The Order key is Ctrl and fires on key-down, which would
  collide with Ctrl+number and Ctrl+right-click. In Modern, Order fires on Ctrl **release** only if
  nothing else was pressed meanwhile; a rebindable Modern preset is M2 stage 4.

## 5. Effort, risk, order and "done when"

Effort: **S** about a session, **M** two or three, **L** more. Each step is its own commit with
tests.

| # | Feature | Effort | Risk | Done when |
|---|---|---|---|---|
| A | **Toggle and plumbing:** preference, Options control, mode captured at level start, Classic forced for replays, demo and tutorials | S | Low | Toggle saves and survives a restart; with Modern on, all 26 tests pass and the 16 replays are identical; a tutorial started in Modern runs Classic |
| B | **Right-click orders for the current control ship:** move to point, plus attack, escort, guard and capture via the existing order; order marker; cursor tint | S–M | Low–medium | Unit test: screen→world→screen round trip within 1 px at UI 1×–4× and on the 5120-wide window at every zoom. In game: a selected ship flies to the clicked point and stops there; right-clicking a planet with a transport captures it; Classic right-click still only targets; masks honoured |
| C | **Attack-move** | S | Medium (touches the AI loop, guarded by the mode) | Test scenario (scratchpad override folder): a ship attack-moved past a non-firing enemy station destroys it before going on; the same ship plain-moved passes it; hit while plain-moving, it fights back; replays identical |
| D | **Multi-select:** click, shift-click, box, markers, orders and minicomputer special orders to all selected | M–L | Medium | Box select at UI 1×–4× and wide picks exactly the own orderable ships inside (unit test on the selection filter, in-game check); a right-click moves all of them; dead ships drop out; tut2 and tutz `{control: 0}` checks still pass (in Classic) |
| E | **Control groups** | M | Low–medium | Ctrl+n assigns, n reselects after other selections, dead members drop out, digits drawn; Ctrl alone still orders (on release); Classic hotkeys unchanged |
| F | **Patrol** (two points) | M | Medium | Test scenario: a ship patrols A↔B for 3 loops, engages an enemy entering the route and resumes; a level `target` action on it cancels the patrol; a new order cancels it |

**Order:** A → B → C → D → E → F.

- B and C give the biggest change in feel for little risk and work before multi-select.
- D is the largest piece and needs B's orders.
- E needs D's lists.
- F needs C's engage rule.

**Advise leaving out:**

- **Formations or spreading:** ships ordered to one point bunch up. The original never formed up,
  and spacing changes collisions and arrival logic. Revisit only if bunching looks bad in play.
- **Patrol queues of more than two points, and shift-queued waypoints:** more UI and state for
  little gain in Ares-sized battles.
- **Ordering the flagship by mouse (autopilot to a point):** autopilot works with target objects
  only; you fly the flagship yourself.
- **Mouse firing and aiming in Modern:** the mouse buttons select and order, so firing stays on
  keys. This answers the open question in `design.md` §10.
- **Camera panning or jump-to-group:** the view stays centred on the flagship. That is the separate
  camera question (M2 stage 7).
- **Select all of a type (double-click):** cheap after D, but not asked for. Optional later.

## 6. Relation to the M2 plan

This replaces M2 stages 6 (Modern mouse actions) and 8 (multi-select) of
`docs/controls-investigation.md` §6 with steps A–F above. Stage 4 (the binding model) is still
needed to make the Modern gestures rebindable. A–F hard-wire them (P, Ctrl+right-click,
Ctrl+number) until then.
