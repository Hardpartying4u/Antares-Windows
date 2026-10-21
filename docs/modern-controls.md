# Modern control mode: investigation (2026-10-20)

Read-only investigation, before any code. The goal is an **Options toggle** (Classic is the
default) that gives C&C-style control in Modern:

- select several ships: drag a box, shift-click to add, Ctrl+number control groups;
- right-click to move them;
- a **patrol** command;
- an **attack-move** command: move to a point but engage any enemy met on the way, unlike a plain
  move.

**Patrol was removed on 2026-10-21, by decision** (§5a). It didn't work in play and isn't
needed for now. The code is in git history (`807c90ba`); the sections below are the original
investigation and plan, kept as written.

Mouse-wheel zoom already works in both modes (M2 stage 2, `d7ddff19`).

Code references are to `windows-working-build` at `96488bf2`. `docs/controls-investigation.md` covers
the input path; this document covers orders, ship behaviour and what Modern needs. First version
`d27574a2`; this revision adds the detailed behaviours in §5 (groups, drag and right-click, patrol,
and each in the original design's terms).

## Summary

- **The engine already has two kinds of move.** Neither is new behaviour Modern has to invent:
  - an order to an **object** (the Ctrl key) flies straight there and fights only when hit or on
    arrival;
  - an order to a **point** (Hold Position, Go To My Position) engages enemies it meets, but gives
    up after 15 s of fighting a target that doesn't fire back.
- **What each command needs:**
  - **Move** reuses the object-order behaviour. No AI change.
  - **Attack-move** is the point order with the 15 s give-up switched off: a one-flag AI change.
  - **Patrol** is new: a waypoint loop on top of attack-move.
  - **Right-click on a friend** is the existing **escort** (on a ship) or **guard** (on a base),
    unchanged.
- **The scenario data doesn't get in the way.** Orders live in engine state (`SpaceObject`,
  `Admiral`), not in `data/`. Level scripts only read the single **control** and **target**
  objects (tutorials) and give scripted orders (`target` actions). New orders need no data change,
  provided the admiral's single control object stays meaningful.
- **Multi-select fits as a list beside the original single control object.** The list is changed
  only by queued input, at a major tick. Its first ship stays `control()`, so the minicomputer,
  labels and tutorial conditions are unchanged.
- **The camera can't jump to a group.** It is locked to your flagship, so a double-tap can only
  *point* to the group until the separate camera work (M2 stage 7) is decided.
- **Recommended order:**
  1. the toggle;
  2. right-click orders;
  3. attack-move;
  4. multi-select;
  5. control groups;
  6. two-point patrol;
  7. multi-point patrol routes, optional.

  Leave out formations, ordering the flagship by mouse, mouse firing and camera moves.

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

**In the original design each side has:**

- **one flagship**, flown by the player's own keys;
- **one control object** (`Admiral::control()`), the ship or base that orders, Build and Special
  Orders apply to;
- **one target object** (`Admiral::target()`), where orders send things and what the flagship
  aims at.

Every other ship you own is **not flown by you**. It runs the same AI as the enemy's ships, steered
only by its destination:

- `destObject`: an object to go to; it follows the object as it moves;
- or `destinationLocation`: a point;
- or neither: the ship stays where it is.

"Giving an order" means setting that destination on the one control object.

### 1.2 Player orders that exist

| Order | Input | Code | Sets |
|---|---|---|---|
| Go to target | Ctrl (Order key); gamepad | `PlayerShipGiveCommand` (`player-ship.cpp` ~1146) → `SetObjectDestination` → `OverrideObjectDestination` (`admiral.cpp` ~437) | destination **object** = the admiral's target; `timeFromOrigin` = 15 s |
| Hold position | minicomputer › Special Orders | `hold_position` (`minicomputer.cpp` ~857) → `SetObjectLocationDestination` (`admiral.cpp` ~375) | destination **point** = its own location; `timeFromOrigin` = 0 |
| Go to my position | minicomputer › Special Orders | `come_to_me` (~867) | destination **point** = the flagship's location at that moment; `timeFromOrigin` = 0 |
| Fire weapon 1 / 2 / special | minicomputer › Special Orders | `fire_weapon` (~845) | one shot with `kManualOverrideFlag` |
| Transfer control | F8; minicomputer | `transfer_control` (~815) | you fly the control ship instead; the camera follows it |
| Build | minicomputer › Build | `build_ship` → `Admiral::build` | builds at the selected base. **A new ship heads for the player's current target** (`AdmiralBuildAtObject` calls `SetObjectDestination`, `admiral.cpp` ~577) |
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
- **Selection is single.** `select_object` (`player-ship.cpp` ~214) sets control or target.
  - Click picking is `PlayerShipHandleClick` (~1052) with `GetSpritePointSelectObject`. It searches
    ±16 logical px around the click, using where each sprite was drawn.
  - The 10 hotkeys (`globals()->hotKey`, keys 1–0) each hold **one** object: tap to select (or
    target with Shift), hold 0.85 s to assign.
  - A hotkey whose object has died is cleared the next time it is pressed (`handle_hotkeys` ~749).
- **Ownership changes keep the destination.** When a ship is captured, `set_owner` (`space-object.cpp`
  ~595) leaves its destination alone. `control()` stops returning it, because control is only a
  ship the admiral owns.

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
  - Invalid entries (dead, captured, id changed) are pruned whenever the list is read.
- **Groups beside the hotkeys** in `globals()`: 10 lists of the same pairs (§5.1).
- **Per-ship order state in `SpaceObject`:**
  - an order mode (none, move, attack-move, patrol);
  - a patrol route of up to 8 points and the index of the next one.

  About 70 bytes, or 24 for two-point patrol only. The default constructor already clears new
  objects.

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
- **Modifiers:** mouse events carry none. Shift, Ctrl and Alt state, and an armed command key
  (A, P), come from the queued key events delivered earlier in the same tick; the multimap keeps
  them in arrival order.

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
- **New ships head for your current target** (§1.2). In Modern a right-click on an enemy sets the
  target, so ships you build afterwards also go there. That is today's behaviour, carried over.

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
     mode is attack-move or patrol. The ship then fights each enemy it meets until it dies or leaves
     range, and resumes the move.
   - Everything else (which enemy, approach, firing, evading) is the existing code.
3. **Patrol:** attack-move plus a route.
   - In `NonplayerShipThink`, before `ThinkObjectNormalPresence`, a ship in patrol mode that is
     within `max(arrive distance, 1/4 subsector)` of its current waypoint gets the next waypoint as
     its destination. The patrol state is kept.
4. **Cancelling:** any other destination (a new player order, a scripted `target` action, a
   creation action) clears the mode through the two setters (§1.6, rule 2).

All of this sits behind `order mode != none`. Classic never sets it, so Classic play and the replay
tests run the original code path. The patrol switch and the clock exception are each a few lines in
the hot loop, and cost nothing when the mode is none.

## 4. What the player sees, at every scale

All drawing is in logical pixels, like the rest of the game. At UI N× it is magnified with crisp
nearest-neighbour pixels, so a 1 px line is N physical pixels and sizes stay in proportion to
sprites. On the wide window the play area is wider (2400×720 logical at 5120×1440, UI 2×). A box can
cover more ships and right-click points can be further to the sides; both use the same mapping.
Mouse coordinates are already exact at every scale (M1).

| Element | Looks like | Draw notes |
|---|---|---|
| Mode toggle | Options › Key Controls: **Controls: Classic / Modern**, next to Reset to Defaults; saved in `config.pn`, takes effect from the next level | Built in code like Video options; not in data |
| Selection box | Thin yellow rubber-band rectangle while dragging | Clipped to the viewport; drawn after sprites, before the panels; a drag starting over the panels is ignored |
| Selected-ship markers | Four small yellow corner brackets around each selected ship, at least 5 px so 1:16 blips still read; the primary (the control object) is brighter and keeps the yellow CONTROL label | Sprite size × current scale, from `sprite->where`; off-screen ships aren't marked (optionally highlighted on the radar) |
| Order feedback | A short-lived ring at the clicked point: green for move, red for attack-move, blue for a patrol point. The order sound plays (`sys.sound.order()`). A right-clicked object gets the sky-blue TARGET label as in Classic | |
| Order cursor | The existing crosshair, tinted by what a right-click would do: red over an enemy (attack), green over empty space (move), cyan over a friend (escort or guard); red while attack-move is armed, blue while patrol is armed | Today's cursor is drawn as lines (`GameCursor::draw`), so no new art |
| Patrol route | For selected patrolling ships: a dashed blue line through the waypoints, a small diamond at each, the next waypoint brighter, the closing leg dimmer | World points converted with `scale_to_viewport` each frame and clipped to the viewport; dashes 4 px on, 4 px off at every UI scale |
| Group numbers | A small digit in the tactical font beside each member's marker. The CONTROL line reads the primary's name plus "+N" | |

**Things the game doesn't have yet:**

- the control-mode preference and toggle;
- the mode captured per level, with Classic forced for replays, demos and tutorials;
- a selection list in `Admiral`, and group lists;
- a screen-to-world inverse of `scale_to_viewport`;
- per-ship order state, the move/attack-move/patrol setters, the clock exception and the patrol
  step;
- Modern mouse handling in `PlayerShip`:
  - drag tracking;
  - modifier tracking from key events;
  - the armed A and P command states;
  - double-tap timing for groups;
- drawing: rubber band, markers, order rings, patrol routes, group digits, cursor tint;
- minicomputer Special Orders (Hold, Come To Me, Fire) applied to every selected ship;
- Ctrl handled differently in Modern. The Order key is Ctrl and fires on key-down, which collides
  with Ctrl+number. In Modern, Order fires on Ctrl **release** only if nothing else was pressed
  meanwhile; a rebindable Modern preset is M2 stage 4.

## 5. The requested behaviours in detail

Each subsection says what the player does, what the engine does, and what it means in the original
design's terms: one flagship you fly, one control object, one target, and AI-flown ships steered
only by their destinations.

### 5.1 Control groups

**What the player does**

- **Assign:** select ships, then press **Ctrl+1…9 or 0**.
  - The group becomes exactly the current selection, replacing what it held, and the digits
    appear on the members.
  - Assigning an empty selection clears the group.
- **Select:** press the number. The group's live members become the selection; the first one is
  the primary.
  - **Shift+number** adds the group to the current selection.
- **Double-tap the number** (two presses within 0.4 s). C&C jumps the camera to the group, but
  **the camera is locked to your flagship** (`scaled_screen` is centred on `g.ship`, `motion.cpp`
  ~421), so it can't jump. Until the camera work (M2 stage 7) is decided, a double-tap:
  - shows the CONTROL label's off-screen pointer (`keepOnScreenAnyway` and its hint line, already
    used for off-screen labels) at the group's primary for 2 s;
  - brackets the group on the radar.

  Two alternatives, both not recommended: Transfer Control (F8) to the primary moves the camera
  but changes which ship you fly; a view-only "fit the group" zoom would mean a zoom level outside
  `g.zoom`, which levels test.

**When ships are destroyed or captured**

- **A member is destroyed:** it drops out of the group the next time the group is used, by the
  same handle and id check that clears dead hotkeys today. The other members stay.
- **A member is captured:** it drops out, because a group only holds ships you own (§1.2).
- **The primary dies:** the next live member becomes the primary when the group is selected.
- **The whole group is gone:**
  - the number selects nothing;
  - a short warning sound plays (`sys.sound.warning()`, as Classic does for an impossible order);
  - the digit is free to assign again;
  - the current selection is left as it was, not emptied.
- **The flagship in a group:** it can be a member, and it is selected with the group, but group
  orders skip it, because you fly it yourself.

**In the original design's terms**

- Today's hotkeys 1–0 hold one object each, which can be your own ship, a base or an enemy; tapping
  selects or targets it.
- In Modern the same 10 keys hold groups of your **own** ships. Enemies and bases can't be grouped:
  you target them by right-clicking, and you select a base for Build by clicking it.
- Classic keeps the original hotkeys unchanged.

### 5.2 Drag selection and right-click orders

**Selecting**

- **Left-click** an own ship: select it alone. It becomes the control object, as in Classic.
- **Left-drag** from empty space: a box. Releasing it selects every own ship inside that can take
  orders: `kCanAcceptDestination`, not `kStaticDestination`, not a base, not the flagship.
  - The ship nearest the box's centre becomes the primary.
- **Shift+click** adds a ship, or removes one already selected.
- **Shift+drag** adds the box's ships to the selection.
- Clicking a base or the flagship selects it alone. A base still needs to be the control object
  for Build.

**Right-click is context-sensitive**

What a right-click does depends on what's under the cursor, picked with the same ±16 px search as
today's clicks:

| Under the cursor | Order | Engine | Behaviour |
|---|---|---|---|
| Enemy ship or base | **Attack it** | Set target, then the object order (as Ctrl): `eAssaultDuty` | Flies at it and engages; fights back if hit on the way |
| Friendly ship | **Follow (escort)** | Object order on a friendly ship: `eEscortDuty` | Flies to it, then matches its heading and warps when it warps: the game's own follow |
| Friendly base | **Guard** | Object order on a friendly base: `eGuardDuty` | Flies to it and stays, fighting anything that comes within engage range |
| Planet (with a transport or engineer) | **Capture or land** | Object order; the ship's arrive action | As Classic |
| Empty space | **Move there** | New: point destination with the clock started at 15 s | Flies there; fights only if hit; stops and holds there |

With several ships selected, every orderable one gets the same order; the flagship is skipped.
With nothing but the flagship selected, a right-click only targets, as a Classic right-click does.

**Plain move against attack-move**

| | Move (right-click empty space) | Attack-move |
|---|---|---|
| Gesture | Right-click | See below |
| On the way | Doesn't turn aside; fires only at what is in its arcs; fights back if hit | Turns aside to engage every enemy within engage range, fights it until it is dead or out of range, then goes on |
| 15 s give-up | Starts used up, so the ship doesn't turn aside to fight | Never grows, so the ship doesn't break off a fight |
| Engine | Existing point order with `timeFromOrigin` set to 15 s | Existing point order, `timeFromOrigin` 0, plus an order-mode flag that stops it growing (`ThinkObjectEngageTarget` ~1244) |

**The attack-move gesture**

- **A, then left-click** (the RTS convention): A arms attack-move, the cursor turns red, the next
  left-click gives the order, and Esc or a right-click disarms it.
  - It needs A to be free. The planned Modern flight keys are WASD (`controls-investigation.md`
    §5), where A turns left.
  - So Modern flight must stay on the keypad or arrows, or attack-move moves to another letter.
- **Alt+right-click.** Alt is Fire 1 in the Classic defaults (Left Alt), so in Modern it would
  also fire weapon 1 unless Modern moves Fire 1 off Alt.
- **Ctrl+right-click.** Conflict-free in both presets once Ctrl acts as a modifier in Modern (§4).
- **Decided 2026-10-20:** **Ctrl+right-click**, plus a rebindable key (G in the Modern preset, an
  action since M2 stage 4); Modern flight stays on WASD. The original recommendation was **A, then
  left-click**, with **Ctrl+right-click** as an always-available second gesture. Decide WASD against A when the Modern preset is set (M2 stage
  4). Both reach the same order.

**What the engine must do for attack-move to engage on the way**

- A point destination, and `timeFromOrigin` left at 0.
- The order-mode flag that keeps the 15 s clock from growing, so the first-branch engage condition
  (`timeFromOrigin < 15 s`, ~454) stays true.
- Nothing else. Choosing the nearest enemy, closing in, firing, evading and resuming the
  destination are all existing code.
- "Met on the way" means within engage range: about two subsectors, or the ship's longest weapon
  range if that is longer. Only enemies its data says it engages count (`ai.combat.engages` tags).
  That is how every AI ship already picks fights.

### 5.3 Patrol

**What the player does**

- **P, then left-click a point.** Each selected ship patrols back and forth between where it is
  now and that point.
- **Shift+left-click** while patrol is still armed adds more waypoints, up to 8. The route then
  loops through them: here → 1 → 2 → … → here → 1 …
- Esc or a right-click disarms P without giving an order.
- **Shift+right-click on empty space while ships are patrolling** appends a waypoint to their
  route. Outside a patrol it queues nothing; move queues are left out (§6).
- **Patrolling ships keep going until given another order.** Any new order replaces the patrol:
  a right-click, a minicomputer order, or a scripted order from the level.
- **They engage enemies they meet, then resume.** Between waypoints they behave as on attack-move.
  When a fight ends, the ship returns to its destination, which is still the current waypoint.

**What exists and what is new**

| Part | Exists? |
|---|---|
| Flying to a point, braking, holding | Yes: point destinations |
| Engaging enemies met, then resuming | Yes: the first-branch engage, plus the attack-move flag (§3) |
| Evading a stronger enemy | Yes (faithful; a patrol ship may flee) |
| A route of points, and moving on at each | **New:** the per-ship route (§2.1) and the step in `NonplayerShipThink` (§3) |
| Cancel on any other order | **New:** cleared by the destination setters (§1.6) |

**How the route is drawn**

- When patrolling ships are selected: a dashed blue line through their waypoints, a small diamond
  at each, the next waypoint brighter, and the closing leg back to the start dimmer.
- Unselected patrolling ships show nothing, to keep the battle view clean.
- Waypoints are world points converted to the screen each frame, so they stay put while the view
  moves and zooms with the flagship. Legs that leave the viewport are clipped.
- At 1:16 a route can span the screen; at 2:1 it is usually off screen. Then only the on-screen
  part draws, and the radar shows nothing extra.

**In the original design's terms:** a patrol is a destination that changes by itself. To the rest
of the game, a patrolling ship is an AI ship with a point destination, like a Hold Position ship
whose point moves along a route.

### 5.4 What this means for ships you don't fly

- **Every order goes to AI-flown ships.** You only ever give a ship a destination and, with the
  new flag, permission to keep fighting. Its skill, weapon choice, evasion and fleeing come from
  its data, exactly as for the enemy's ships. Modern changes what you can say to your fleet, not
  how it flies.
- **The single control object stays.** It is the primary of the selection. Everything built on it
  keeps working:
  - the CONTROL line;
  - Build at a base;
  - Transfer Control (F8);
  - the tutorials' `{control: 0}` checks.
- **The flagship stays yours.** It is never moved by Modern orders. Autopilot (Shift+Tab) still
  flies it to the target.
- **Ships with a static destination can't be ordered,** in Classic as in Modern. Box select leaves
  them out.
- **New ships head for your target** (§1.2).
- **Scripted orders override Modern ones** (§1.6), so a level can still take a ship away for its
  story.

## 5a. Built so far

- **Step B, right-click orders for one selected ship: done 2026-10-20.** See
  `docs/current-state.md` §2.4a for the behaviour, how the engine stores the order and the tests.
  - The Modern mode is on with a Modern preset, outside replays, the demo and the tutorials (as
    proposed in §2.4).
  - A move is a point destination with the 15 s clock started (§3, step 1). No order-mode flag was
    needed yet; attack-move (step C) will add one.
  - A scripted order replaces a Modern one because the Modern order is only a destination.
  - The step A pieces this needed (the mode decided at level start, Classic for replays and
    tutorials) came with it; the separate toggle is the preset choice itself.

- **Step D, multi-select: done 2026-10-20** (`docs/current-state.md` §2.4b).
  - It includes a drag box, Shift+click, Shift+drag, group orders, a grid formation for group
    moves and brackets on every selected ship.
  - The selection is a list in the game state whose first ship is the control object.
  - Orders are unchanged in kind: each selected ship gets the step B order.
- **Step E, control groups: done 2026-10-20** (`docs/current-state.md` §2.4c).
  - Ctrl+number saves, the number recalls, Shift+number adds, and a double tap shows a pointer.
  - In the Modern mode the number keys are groups, not hotkeys, and the Order key fires on release
    unless a number was pressed while it was held. That resolves the Ctrl clash in Modern: WASD.
  - Group digits beside the markers (§4) are not drawn yet.
- **The group bar: done 2026-10-21** (`docs/current-state.md` §2.4c2). A box per group that has
  ships along the bottom edge of the play area ("1 · 5"), counts kept live, a blink on save and on
  Shift+number, lit while the group is the selection. A click on a box acts as the number, a
  double-click as a double tap, and clicks on the bar never reach the play area. Red messages rise
  above the bar while it shows (chosen over a bar floating above the message lane). Modern mode
  only.
- **Steps C and F, attack-move and two-point patrol: done 2026-10-21** (`docs/current-state.md`
  §2.4d), as planned in §5b.
  - Attack-move key + click, or Ctrl+right-click: point orders with the engage clock at 0.
  - Patrol key + click: point orders plus records that `modern_patrol_step()` turns at each end.
  - Not built: the optional "don't give up after 15 s" flag for attack-move, and multi-point
    patrol (step G).
- **Patrol (steps F and G): removed 2026-10-21, by decision.** It didn't work in play, and it
  isn't needed for now. Everything was removed:
  - the action (45; `ACTION_COUNT` is 45 again) and its keys (W in Modern: Ares, T in Modern:
    WASD);
  - its Key Controls row;
  - the armed sky-blue crosshair and the dashed route line;
  - the patrol records and per-tick step;
  - its unit test and replay steps.

  A saved config's `patrol` binding is ignored. Attack-move, drag-select, groups and right-click
  are unchanged. The code is in git history (`807c90ba`, "Modern attack-move and two-point
  patrol"), if it is ever wanted back.

## 5b. Attack-move and patrol: how they can be built (investigation, 2026-10-21)

Read-only. Code references as in §1 (`non-player-ship.cpp`, `admiral.cpp`); keys from §8.

**What the engine already does.**
- **Point orders already engage on the way.** A point destination
  (`SetObjectLocationDestination`, `admiral.cpp` ~375) ends with `timeFromOrigin = 0`. Classic's
  Hold Position and Go To My Position use this.
- **The engage rule** (`ThinkObjectNormalPresence` ~450): a ship turns aside to fight its nearest
  enemy within engage range while `timeFromOrigin < 15 s`. It then resumes its destination.
- **The 15 s clock only grows** while the ship is in weapon range of a target. It resets when the
  ship is hit, or when it is within 1024 units of its destination.
- **The result:** an existing point order already behaves as an attack-move, with one limit. After
  about 15 s of fighting a target that doesn't fire back (a base, a transport), the ship gives up
  and carries on.
- **Modern's plain move (step B)** is the same order with the clock started at 15 s, so it doesn't
  turn aside.
- **Nothing already patrols.** Classic's keys don't: the only point orders are Hold Position and
  Go To My Position. The computer admirals choose destinations (`Admiral::think`) but never alternate
  between points. The level data has no waypoint routes. Escort (follow) and guard are the nearest
  existing behaviours.

**Attack-move: built from existing orders.** No new ship behaviour.
- A then click (Modern: Ares; G in Modern: WASD), or Ctrl+right-click: each selected ship gets
  `SetObjectLocationDestination` to its formation point (the same grid as a group move). The clock
  is left at 0.
- That is exactly the Classic point order, so it inherits everything:
  - it engages enemies met within engage range, fights them, then continues;
  - it holds at the point and fights anything that comes near;
  - it goes through the per-tick queue;
  - a level's `target` action replaces it, like any destination.
- **The one difference from a C&C attack-move** is the 15 s give-up against targets that don't fire
  back.
- **If that matters in play,** the smallest fix is a per-ship flag that stops `ThinkObjectEngageTarget`
  (~1244) advancing the clock (§3, step 2). It is a few lines, in effect only on attack-move ships,
  and is cleared by any other destination. Proposal: build without it first and add it only if
  playtesting shows ships breaking off fights.

**Patrol: existing orders, plus a small per-tick step** (new state, no new ship AI).
- **The order:** W then click (Modern: Ares; T in Modern: WASD). Each selected ship gets a point
  order (clock 0, so it engages on the way) to the clicked point, plus a patrol record:
  `{ship, id, point A (where it was), point B (the click), heading for B}`. Records live in the game
  state, beside the selection.
- **The step** (once per major tick, after `PlayerShip::update()`, in record order): if a ship is
  within its arrival distance of the point it is heading for, it gets the other point as a new point
  order. The arrival distance is the larger of its data's arrive distance and a fixed minimum of
  about 200 units, because ships brake and may never reach the exact point. The switch resets the
  clock, so each leg engages enemies afresh.
- **Fighting on a leg:** the ship fights the enemies it meets, then resumes the leg, because its
  destination is unchanged.
- **Determinism:** the step reads and writes only game state, runs at a fixed point in the major
  tick in a fixed order, and uses no wall time. Replays run Classic, so the list stays empty there.
  The Modern replay tests stay repeatable.
- **Scripts and other orders win:** the step drops a record when the ship's destination is no longer
  the patrol point it set. That covers a level's `target` action, any player order, and a Classic
  key order. It also drops records for ships that die, are captured or become locked (as the
  selection does), and all records clear at level start.
- **Drawing (view only):** a dashed line between A and B for selected patrolling ships (§4).

**Recommended order:**
1. attack-move, with no engine change;
2. two-point patrol, with the per-tick step;
3. only if needed, the attack-move flag;
4. multi-point routes (Shift+click after W) later, as step G.

**Tests:**
- unit tests for the patrol step's switch and cancel rules (pure functions on positions and
  destinations);
- the modern-multi replay test extended with an attack-move and a patrol leg switch, two runs
  identical;
- in game at UI 2× and 4× from the timing log.

## 6. Effort, risk, order and "done when"

Effort: **S** about a session, **M** two or three, **L** more. Each step is its own commit with
tests.

| # | Feature | Effort | Risk | Done when |
|---|---|---|---|---|
| A | **Toggle and plumbing:** preference, Options control, mode captured at level start, Classic forced for replays, demo and tutorials | S | Low | Toggle saves and survives a restart; with Modern on, all 26 tests pass and the 16 replays are identical; a tutorial started in Modern runs Classic |
| B | **Right-click orders for the current control ship:** move, attack, escort, guard, capture; order rings; cursor tint | S–M | Low–medium | Unit test: screen→world→screen round trip within 1 px at UI 1×–4× and on the 5120-wide window at every zoom. In game: a ship flies to the clicked point and holds there; right-clicking a friend makes it follow, an enemy makes it attack, a planet with a transport captures it; Classic right-click still only targets; masks honoured |
| C | **Attack-move:** A then click, and Ctrl+right-click | S | Medium (touches the AI loop, guarded by the mode) | Test scenario (scratchpad override folder): a ship attack-moved past a non-firing enemy station destroys it before going on; plain-moved, it passes; hit while plain-moving, it fights back; replays identical |
| D | **Multi-select:** click, shift-click, box, markers, orders and Special Orders to every selected ship | M–L | Medium | Box select at UI 1×–4× and wide picks exactly the own orderable ships inside (unit test on the selection filter, in-game check); a right-click moves all of them; dead and captured ships drop out; tut2 and tutz `{control: 0}` checks still pass (in Classic) |
| E | **Control groups:** Ctrl+n, n, Shift+n, double-tap pointer, digits | M | Low–medium | n reselects after other selections; a destroyed member drops out and the rest stay; a fully destroyed group gives the warning sound and leaves the selection alone; Ctrl alone still orders (on release); Classic hotkeys unchanged |
| F | **Patrol, two points:** P then click; route drawing | M | Medium | Test scenario: a ship patrols A↔B for 3 loops, engages an enemy entering the route and resumes; a level `target` action cancels the patrol; a new order cancels it |
| G | **Patrol routes, up to 8 points:** Shift+click after P; Shift+right-click to append | S | Low (F's machinery) | A 4-point route loops in order; the route draws correctly at UI 1×–4× and on the wide window |

**Order:** A → B → C → D → E → F → G.

- B and C give the biggest change in feel for little risk and work before multi-select.
- D is the largest piece and needs B's orders.
- E needs D's lists.
- F needs C's engage rule.
- G only extends F.

**Advise leaving out:**

- **Formations or spreading:** ships ordered to one point bunch up. The original never formed up,
  and spacing changes collisions and arrival logic. Revisit only if bunching looks bad in play.
- **Queued move orders (shift-queued moves outside patrol):** more state and UI for little gain in
  Ares-sized battles. Patrol routes cover the useful part.
- **Ordering the flagship by mouse (autopilot to a point):** autopilot works with target objects
  only, and you fly the flagship yourself.
- **Mouse firing and aiming in Modern:** the mouse buttons select and order, so firing stays on
  keys.
- **Camera jumps and panning:** the view stays centred on the flagship (M2 stage 7). The
  double-tap pointer (§5.1) stands in until that is decided.
- **Select all of a type (double-click):** cheap after D, but not asked for. Optional later.

## 7. Relation to the M2 plan

This replaces M2 stages 6 (Modern mouse actions) and 8 (multi-select) of
`docs/controls-investigation.md` §6 with steps A–G above. Stage 4 (the binding model) is still
needed to make the Modern gestures rebindable. A–G hard-wire them (A, P, Ctrl+right-click,
Ctrl+number) until then.

## 8. The Modern presets (approved 2026-10-20)

There are three presets: Classic (unchanged, except that its fire keys are X and Z since 2026-10-20);
**Modern: Ares**, the default Modern layout, which has Classic's bindings since 2026-10-21; and
**Modern: WASD**.

- Inputs no preset can bind: Esc, Return and Caps Lock; the left and right mouse buttons; the
  gamepad's LB, RB and Start.
- Both Modern presets keep Classic's gamepad mapping.
- "L" is the left hand, "R" the right.

### Modern: Ares (Classic's keys, since 2026-10-21)

**Decision (2026-10-21):** Modern: Ares has exactly Classic's bindings (keys, wheel and gamepad),
with all the Modern mouse and group features: left-click and drag selection, right-click orders,
control groups, attack-move, wheel zoom and Order firing on release. Classic and Modern: WASD are
unchanged. Players who know the Ares keys get the Modern features without relearning them.

**What changed** (primary keys; every other binding was already Classic's):

| # | Action | Before | Now |
|---|---|---|---|
| 0–3 | Accelerate / decelerate / turn ccw / turn cw | ↑ ↓ ← → | Keypad 8 5 4 6 |
| 4 / 5 / 6 | Fire 1 / fire 2 / special | Space / F / D | X / Z / Space |
| 8 / 9 / 10 | Select friendly / hostile / base | Q / E / R | Keypad . / * / ÷ |
| 12 | Order | S | Ctrl |
| 13 / 14 | Zoom in / out (the wheel too, unchanged) | X / Z | Keypad + / − |
| 15–18 | Minicomputer up / down / accept / cancel | T / G / B / V | ↑ ↓ → ← |
| 19 | Transfer control | C | F8 |
| 27 | Next message | ` | Backspace |
| 44 | Attack move | A, then click | A, then click (Classic has A too, since 2026-10-21); Ctrl+right-click |

**Clash check (done first, none found):**
- **Ctrl** is Classic's Order key and also the raw group and attack-move modifier. The Modern mode
  already handles this (Modern: WASD has Ctrl as Order too): Order fires on release, not after
  Ctrl+number (§5.1) or Ctrl+right-click. The Modern replay tests run with Classic's bindings.
- **Shift** (target) and **1–0** (hotkeys, groups in the Modern mode) are the same in all presets.
- **Attack move** had no key in Classic. On 2026-10-21 Classic got A for it (A is free in Classic,
  and the action does nothing outside the Modern mode), so Modern: Ares keeps A, then click.
  Older Classic and Modern: Ares configs get A if attack move has no key and A is free (config
  version 5).
- No Classic key is used as a raw key by a Modern feature (only Ctrl and Return are).

**Saved configs:** a config (version 1 to 3) on Modern: Ares whose bindings are still the old
layout loads with the new one; one the player customised is kept (shown as changed; Reset to
Defaults gives the new layout). Saves are version 4.

The old layout, for the record:

#### Modern: Ares before 2026-10-21 (Ares-style)

The right hand steers on the arrow keys, the PC equivalent of Ares' keypad 8-4-5-6. Everything else
is under the left hand: fingers on A S D F, thumb on Space.

| # | Action | Keys | Hand | Clash / note |
|---|---|---|---|---|
| 0–3 | Accelerate / decelerate / turn counter-clockwise / turn clockwise | ↑ / ↓ / ← / → | R | Classic's minicomputer arrows move to T G B V |
| 4 | Fire weapon 1 | Space | L thumb | |
| 5 | Fire weapon 2 | F | L index | |
| 6 | Fire / special | D | L middle | |
| 7 | Warp | Tab | L pinky | Shift+Tab = autopilot. Alt is unbound, so firing while warping can't make Alt+Tab |
| 8 | Select friendly | Q | L | Shift+Q targets a friendly |
| 9 | Select hostile | E | L | |
| 10 | Select base | R | L | Shift+R targets any base |
| 11 | Target (modifier) | Left Shift | L pinky | Modern also uses Shift for: Shift+click (add to the selection), Shift+number (add a group) and patrol waypoints. Held alone for 0.75 s then released, it targets your own ship; clicks cancel that, and box drags must too |
| 12 | Order (go to target) | S | L ring | Ctrl isn't Order here, so it is only a modifier |
| 13 | Zoom in | X, wheel up | L / mouse | |
| 14 | Zoom out | Z, wheel down | L / mouse | |
| 15–18 | Minicomputer up / down / accept / cancel | T / G / B / V | L | Mirrors ↑ ↓ → ← |
| 19 | Transfer control | C | L | |
| 20–26 | Zoom shortcuts | F9–F12, End, Home, Page Up | R | Exception: rarely used view shortcuts; End, Home and Page Up sit just above the arrows |
| 27 | Next message | ` | L | |
| 28–33 | Help, volume down, volume up, music, net settings, fast motion | F1–F6 | L (stretch) | As Classic |
| 34–38 | Groups / hotkeys 1–5 | 1–5 | L | Ctrl+number assigns, the number selects, Shift+number adds |
| 39–43 | Groups / hotkeys 6–0 | 6–0 | R | Tradeoff: the steering hand leaves the arrows |
| 44 | Attack move | A, then click; also Ctrl+right-click | L pinky | Caps Lock (pause) is next to A |
| 45 | ~~Patrol~~ | Removed 2026-10-21 (§5a) | | |

Modifiers:
- **Ctrl:** only Ctrl+number (assign a group) and Ctrl+right-click (attack-move).
- **Shift:** the target modifier, as in the table.
- **Alt and the Windows keys:** unbound.

**Tradeoff:** the right hand can't steer and use the mouse at once. Ships coast; autopilot
(Shift+Tab) flies the flagship; Modern orders go to the other ships. Nothing more is added for now,
and WASD is the choice for steering while using the mouse.

### Modern: WASD

| # | Action | Keys | Hand | Clash / note |
|---|---|---|---|---|
| 0–3 | Accelerate / decelerate / turn counter-clockwise / turn clockwise | W / S / A / D | L | A steers, so attack-move can't be A-then-click |
| 4 | Fire weapon 1 | Space | L | |
| 5 | Fire weapon 2 | F | L | |
| 6 | Fire / special | R | L | |
| 7 | Warp | Tab | L | Shift+Tab = autopilot |
| 8 | Select friendly | Q | L | |
| 9 | Select hostile | E | L | |
| 10 | Select base | B | L | |
| 11 | Target (modifier) | Left Shift | L | As Ares-style |
| 12 | Order (go to target) | Ctrl | L | Clash: Ctrl is also the group modifier and part of Ctrl+right-click. Needs the planned rule: Order fires on Ctrl's release only if nothing else was pressed |
| 13 | Zoom in | =, wheel up | R / mouse | |
| 14 | Zoom out | −, wheel down | R / mouse | |
| 15–18 | Minicomputer up / down / accept / cancel | Arrows | R | The minicomputer can also be clicked |
| 19 | Transfer control | F8 | R | |
| 20–26 | Zoom shortcuts | F9–F12, End, Home, Page Up | R | |
| 27 | Next message | Backspace | R | |
| 28–33 | Help, volume down, volume up, music, net settings, fast motion | F1–F6 | L | |
| 34–38 | Groups / hotkeys 1–5 | 1–5 | L | |
| 39–43 | Groups / hotkeys 6–0 | 6–0 | R | The hand leaves the mouse |
| 44 | Attack move | G; also Ctrl+right-click | L | |
| 45 | ~~Patrol~~ | Removed 2026-10-21 (§5a) | | |
