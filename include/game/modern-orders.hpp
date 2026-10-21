// Copyright (C) 2026 The Star Bound Authors
// This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.

#ifndef ANTARES_GAME_MODERN_ORDERS_HPP_
#define ANTARES_GAME_MODERN_ORDERS_HPP_

#include <functional>
#include <vector>
#include <pn/string>

#include "data/handle.hpp"
#include "drawing/color.hpp"
#include "math/geometry.hpp"
#include "math/scale.hpp"

namespace antares {

class Admiral;
class SpaceObject;
class PlayerShip;

// Modern right-click orders for the selected ship (docs/modern-controls.md, step B).
//
// The Modern mode is on in a level when a Modern preset is chosen, except in replays, the demo
// and the tutorials (their text and checks teach the Classic keys). It is decided when the level
// starts (g.modern_controls) and Classic play is unchanged when it is off.
//
// In the Modern mode a right-click on the play area orders the selected ship (the admiral's
// control object), through the same per-tick input queue as every click:
//   * on an enemy: attack it (the Classic order to go to an enemy: assault duty);
//   * on a friendly ship: follow it (escort duty);
//   * on a friendly base: guard it (guard duty);
//   * on empty space: move there.
// It also makes what was clicked the target, as a Classic right-click does. The flagship is never
// ordered: with the flagship (or a base, or a ship that can't take orders) selected, a right-click
// only targets.
//
// How the engine holds the order: an object order is the existing destination object (destObject,
// with the escort, guard or assault duty OverrideObjectDestination gives it). A move is a
// destination point (destinationLocation, no destObject) with timeFromOrigin started at 15 s, as
// an object order starts it, so the ship flies there without turning aside to fight, fights back
// if hit, and holds there on arrival. Nothing else is stored, so a level's scripted orders replace
// a Modern order exactly as they replace a Classic one.

enum class ModernOrder { NONE, ATTACK, FOLLOW, GUARD, MOVE, ATTACK_MOVE };

// Multi-select (step D). The selection is a list of the player's ships (g.modern_selection) whose
// first ship is the admiral's control object, so everything built on the single control object
// keeps working. Only ships that can take orders are in it: the player's own, not the flagship, not
// a base, not one whose destination the level locks. Ships drop out when they die, are captured or
// become locked; when the control object changes some other way (a click on one ship, Select
// friendly, a hotkey, a transfer), the selection becomes just that ship (or empty).
bool group_selectable(uint32_t attributes, bool owned_by_player, bool is_flagship);
bool in_box(Point p, Point a, Point b);  // inclusive, either corner order
std::vector<Handle<SpaceObject>> modern_selection();  // pruned and matched to the control object
std::vector<Handle<SpaceObject>> modern_ships_in_box(Point a, Point b);  // selectable, by number
void modern_select(const std::vector<Handle<SpaceObject>>& ships, bool add);
void modern_selection_matches_control();  // after the caller set the control object itself

// Where ship `index` of `count` goes for a group move: a centred square grid, `spacing` apart.
// The same order every time, so group moves stay deterministic.
Point formation_offset(int index, int count, int32_t spacing);
const int32_t kFormationSpacing = 160;

// What a right-click would do, from what is under the cursor. Pure, for the cursor and the tests.
ModernOrder classify_order(
        bool control_orderable, bool has_object, bool object_hostile, bool object_is_base,
        bool object_is_control);

// Screen (logical pixel) to world coordinates: the inverse of scale_to_viewport().
Point viewport_to_world(Point screen, const Rect& world_bounds, Scale scale, Point viewport_origin);
Point screen_to_world(Point screen);

// Whether `admiral`'s control object can be given a Modern order: a ship it owns that accepts
// destinations, isn't static and isn't the flagship.
bool modern_orderable(Handle<Admiral> admiral);

// Handles a right-click at `where` (logical pixels) in the Modern mode, at a major tick. Returns
// the order given (NONE if it only targeted or did nothing).
ModernOrder modern_right_click(Point where);

// The order functions the click uses (also used by the replay test).
bool give_object_order(Handle<Admiral> admiral, Handle<SpaceObject> object);
bool give_move_order(Handle<Admiral> admiral, Point world);

// Control groups (step E). Ctrl+number saves the selection as group `group` (0-9: keys 1-0); the
// number selects the group's ships that can still take orders; Shift+number adds them. Pressing
// the number twice quickly shows a pointer to the group: the view stays on the flagship until the
// camera work (docs/design.md).
void                             modern_group_save(int group);
std::vector<Handle<SpaceObject>> modern_group_ships(int group);
void                             modern_group_pointer(int group);

// The group bar (2026-10-21): along the bottom of the play area, a box per group that has ships,
// in number order, showing its number and how many ("1 . 5", with a middle dot). A box flashes
// when its group is saved or Shift+number is pressed, and stays lit while the group is the
// selection. View only, except that a click on a box acts as the group's number
// (PlayerShip::mouse_down). While it shows, red messages rise above it
// (Messages::draw_message_screen). Only in the Modern mode.
void    draw_modern_group_bar();
int     modern_group_bar_hit(Point where);  // the group whose box is at `where`, or -1
int32_t modern_group_bar_height();          // 0 when no box shows
void    modern_group_flash(int group);

// Attack-move (step C, docs/modern-controls.md §5b), from an existing order: the attack-move key
// then a click, or Ctrl+right-click. Each selected ship gets a point order to its formation point
// with the 15 s clock left at 0, the Classic point order (Hold Position's), so it turns aside to
// fight enemies it meets, then carries on. A click on an enemy attacks it. (Patrol was removed on
// 2026-10-21; see §5a.)
enum class ModernCommand { NONE, ATTACK_MOVE };
void        modern_arm(ModernCommand command);  // the crosshair's colour while armed (view only)
ModernOrder modern_command_click(ModernCommand command, Point where);
bool        give_attack_move(Handle<Admiral> admiral, Point world);

// The drag box, drawn from where the left button went down to the mouse while it is held.
void modern_drag(bool active, Point start);

// Drawing (view only): brackets on the selected ships, a short-lived marker where an order was
// given, and the colour the crosshair takes from what a right-click would do.
void draw_modern_markers();
bool modern_cursor_color(Point where, RgbColor* color);

// Optional sink for a line per order given (the replay tool's orders.log).
extern std::function<void(pn::string_view)> modern_order_log;

// The replay test tool turns the Modern mode on in replays (they are otherwise always Classic).
extern bool modern_controls_in_replays;

// Whether the Modern mode applies to a level starting now (sets g.modern_controls).
bool modern_controls_for_level(bool replay, bool tutorial);

}  // namespace antares

#endif  // ANTARES_GAME_MODERN_ORDERS_HPP_
