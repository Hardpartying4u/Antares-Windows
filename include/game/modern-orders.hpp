// Copyright (C) 2026 The Star Bound Authors
// This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.

#ifndef ANTARES_GAME_MODERN_ORDERS_HPP_
#define ANTARES_GAME_MODERN_ORDERS_HPP_

#include <functional>
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

enum class ModernOrder { NONE, ATTACK, FOLLOW, GUARD, MOVE };

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

// Drawing (view only): brackets on the selected ship, a short-lived marker where an order was
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
