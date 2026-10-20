// Copyright (C) 2026 The Star Bound Authors
// This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.

#include "game/modern-orders.hpp"

#include <algorithm>

#include "config/keys.hpp"
#include "config/preferences.hpp"
#include "drawing/sprite-handling.hpp"
#include "game/admiral.hpp"
#include "game/globals.hpp"
#include "game/motion.hpp"
#include "game/non-player-ship.hpp"
#include "game/player-ship.hpp"
#include "game/space-object.hpp"
#include "game/sys.hpp"
#include "game/time.hpp"
#include "game/timing.hpp"
#include "sound/fx.hpp"
#include "video/driver.hpp"

namespace antares {

std::function<void(pn::string_view)> modern_order_log;
bool                                 modern_controls_in_replays = false;

namespace {

const int  kClickBoundsSize = 16;  // the same reach as a Classic click (PlayerShipHandleClick)
const usecs kMarkerTime     = secs(1);

// The last order given, for its on-screen marker. View only: kept in wall time, never read by
// the game.
struct {
    bool                on = false;
    ModernOrder         order;
    Point               world;
    Handle<SpaceObject> object;
    int32_t             object_id;
    wall_time           since;
} gMarker;

Hue order_hue(ModernOrder o) {
    switch (o) {
        case ModernOrder::ATTACK: return Hue::RED;
        case ModernOrder::FOLLOW: return Hue::AQUA;
        case ModernOrder::GUARD: return Hue::GOLD;
        case ModernOrder::MOVE: return Hue::GREEN;
        default: return Hue::SKY_BLUE;
    }
}

const char* order_name(ModernOrder o) {
    switch (o) {
        case ModernOrder::ATTACK: return "attack";
        case ModernOrder::FOLLOW: return "follow";
        case ModernOrder::GUARD: return "guard";
        case ModernOrder::MOVE: return "move";
        default: return "none";
    }
}

// The object a click at `where` would pick, as a Classic right-click picks its target.
Handle<SpaceObject> object_at(Point where) {
    if (!g.ship.get()) {
        return SpaceObject::none();
    }
    Rect bounds = {
            where.h - kClickBoundsSize, where.v - kClickBoundsSize, where.h + kClickBoundsSize,
            where.v + kClickBoundsSize};
    return GetSpritePointSelectObject(
            &bounds, g.ship, kCanBeDestination | kIsDestination, g.admiral->target(),
            FRIENDLY_OR_HOSTILE);
}

ModernOrder order_for(Handle<SpaceObject> object) {
    auto control = g.admiral->control();
    return classify_order(
            modern_orderable(g.admiral), object.get() != nullptr,
            object.get() && (object->owner != g.admiral),
            object.get() && (object->attributes & kIsDestination), object == control);
}

void log_order(Handle<SpaceObject> control, ModernOrder order, pn::string_view what) {
    if (modern_order_log) {
        modern_order_log(pn::format(
                "{0}\t{1} #{2}\t{3}\t{4}\n", g.time.time_since_epoch().count(),
                control->long_name(), control.number(), order_name(order), what));
    }
}

void show_marker(ModernOrder order, Point world, Handle<SpaceObject> object) {
    gMarker.on        = true;
    gMarker.order     = order;
    gMarker.world     = world;
    gMarker.object    = object;
    gMarker.object_id = object.get() ? object->id : -1;
    gMarker.since     = now();
}

// Four corner brackets around `r`, at least `min_half` from its centre.
void draw_brackets(const Rects& rects, Rect r, int min_half, int arm, const RgbColor& color) {
    const Point c = r.center();
    r.left        = std::min(r.left, c.h - min_half);
    r.right       = std::max(r.right, c.h + min_half);
    r.top         = std::min(r.top, c.v - min_half);
    r.bottom      = std::max(r.bottom, c.v + min_half);
    for (int corner = 0; corner < 4; ++corner) {
        const int  x  = (corner & 1) ? r.right : r.left;
        const int  y  = (corner & 2) ? r.bottom : r.top;
        const int  dx = (corner & 1) ? -arm : 0;
        const int  dy = (corner & 2) ? -arm : 0;
        rects.fill(Rect(std::min(x, x + dx), y, std::max(x, x + dx) + 1, y + 1), color);
        rects.fill(Rect(x, std::min(y, y + dy), x + 1, std::max(y, y + dy) + 1), color);
    }
}

}  // namespace

ModernOrder classify_order(
        bool control_orderable, bool has_object, bool object_hostile, bool object_is_base,
        bool object_is_control) {
    if (!control_orderable) {
        return ModernOrder::NONE;
    } else if (!has_object) {
        return ModernOrder::MOVE;
    } else if (object_is_control) {
        return ModernOrder::NONE;
    } else if (object_hostile) {
        return ModernOrder::ATTACK;
    } else if (object_is_base) {
        return ModernOrder::GUARD;
    }
    return ModernOrder::FOLLOW;
}

Point viewport_to_world(Point screen, const Rect& world_bounds, Scale scale, Point viewport_origin) {
    // scale_to_viewport: screen = ((world - bounds) * factor >> SHIFT_SCALE) + viewport.
    // The inverse rounds to the world point nearest the pixel's corner, as that does.
    const int64_t factor = std::max<int64_t>(1, scale.factor);
    return Point(
            world_bounds.left +
                    static_cast<int32_t>(
                            ((static_cast<int64_t>(screen.h - viewport_origin.h) << SHIFT_SCALE) +
                             factor - 1) /
                            factor),
            world_bounds.top +
                    static_cast<int32_t>(
                            ((static_cast<int64_t>(screen.v - viewport_origin.v) << SHIFT_SCALE) +
                             factor - 1) /
                            factor));
}

Point screen_to_world(Point screen) {
    const Rect v = viewport();
    return viewport_to_world(
            screen, scaled_screen.bounds, scaled_screen.scale, Point(v.left, v.top));
}

bool modern_orderable(Handle<Admiral> admiral) {
    if (!admiral.get()) {
        return false;
    }
    auto c = admiral->control();
    return c.get() && (c != admiral->flagship()) && (c->owner == admiral) &&
           (c->attributes & kCanAcceptDestination) && !(c->attributes & kStaticDestination);
}

bool modern_controls_for_level(bool replay, bool tutorial) {
    if (replay) {
        return modern_controls_in_replays;
    }
    return !tutorial && (sys.prefs->controls_preset() != ControlPreset::CLASSIC);
}

bool give_object_order(Handle<Admiral> admiral, Handle<SpaceObject> object) {
    auto control = admiral->control();
    if (!modern_orderable(admiral) || !object.get() || (object == control)) {
        return false;
    }
    const ModernOrder order = classify_order(
            true, true, object->owner != admiral, object->attributes & kIsDestination, false);
    admiral->set_target(object);
    SetObjectDestination(control);  // the Classic order: go to the admiral's target
    log_order(control, order, pn::format("{0} #{1}", object->long_name(), object.number()));
    if (admiral == g.admiral) {
        sys.sound.order();
        show_marker(order, object->location, object);
    }
    return true;
}

bool give_move_order(Handle<Admiral> admiral, Point world) {
    auto control = admiral->control();
    if (!modern_orderable(admiral)) {
        return false;
    }
    SetObjectLocationDestination(control, &world);
    // A move flies straight there, like an object order: the 15 s clock starts used up, so the
    // ship doesn't turn aside to fight unless it is hit (docs/modern-controls.md §3).
    control->timeFromOrigin = kTimeToCheckHome;
    log_order(control, ModernOrder::MOVE, pn::format("to {0}, {1}", world.h, world.v));
    if (admiral == g.admiral) {
        sys.sound.order();
        show_marker(ModernOrder::MOVE, world, SpaceObject::none());
    }
    return true;
}

ModernOrder modern_right_click(Point where) {
    if (!g.ship.get() || (g.key_mask & kMouseMask) || !viewport().contains(where)) {
        return ModernOrder::NONE;
    }
    auto object = object_at(where);
    // First what a Classic right-click does: target the object under the cursor.
    PlayerShipHandleClick(where, 1);
    if (g.key_mask & kOrderKey) {  // a level that disables orders disables these too
        return ModernOrder::NONE;
    }
    const ModernOrder order = order_for(object);
    if ((order == ModernOrder::NONE) && modern_order_log) {
        auto control = g.admiral->control();
        modern_order_log(pn::format(
                "{0}\t{1}\ttarget only\t{2}\n", g.time.time_since_epoch().count(),
                control.get() ? pn::format("{0} #{1}", control->long_name(), control.number())
                              : pn::string{"nothing selected"},
                !modern_orderable(g.admiral) ? "the selected ship can't take orders"
                                             : "clicked the selected ship"));
    }
    if (order == ModernOrder::MOVE) {
        give_move_order(g.admiral, screen_to_world(where));
    } else if (order != ModernOrder::NONE) {
        give_object_order(g.admiral, object);
    }
    return order;
}

void draw_modern_markers() {
    if (!g.modern_controls || !g.ship.get()) {
        return;
    }
    const Rect  clip = viewport();
    const Rects rects;

    // The selected ship.
    auto control = g.admiral->control();
    if (timing_on()) {
        // With --timing, where the selected ship is on screen (logical pixels), when it changes
        // and every quarter second: lets an in-game check click on it.
        static Handle<SpaceObject> logged;
        static wall_time           next_log;
        if ((control != logged) || (now() >= next_log)) {
            logged   = control;
            next_log = now() + std::chrono::milliseconds(250);
            if (control.get() && control->sprite.get()) {
                timing_mark(pn::format(
                        "modern selected\t{0} #{1}\t{2}\t{3}", control->long_name(),
                        control.number(), control->sprite->where.h, control->sprite->where.v));
            } else {
                timing_mark("modern selected\tnone");
            }
        }
        // Four times a second, where each ship the player could order is on screen.
        static wall_time next_ships;
        if (now() >= next_ships) {
            next_ships = now() + std::chrono::milliseconds(250);
            for (auto o : SpaceObject::all()) {
                if ((o->active == kObjectInUse) && (o->owner == g.admiral) && (o != g.ship) &&
                    (o->attributes & kCanAcceptDestination) &&
                    !(o->attributes & kStaticDestination) && o->sprite.get()) {
                    timing_mark(pn::format(
                            "modern orderable\t{0} #{1}\t{2}\t{3}", o->long_name(), o.number(),
                            o->sprite->where.h, o->sprite->where.v));
                }
            }
        }
    }
    if (control.get() && control->sprite.get()) {
        const auto& frame = control->sprite->table->at(control->sprite->whichShape);
        const Rect  world = scale_sprite_rect(frame, control->location, control->naturalScale);
        const Point tl = scale_to_viewport(Point(world.left, world.top));
        const Point br = scale_to_viewport(Point(world.right, world.bottom));
        Rect        r(tl.h, tl.v, std::max(tl.h, br.h), std::max(tl.v, br.v));
        r.inset(-3, -3);
        if (clip.contains(r.center())) {
            draw_brackets(rects, r, 6, 4, GetRGBTranslateColorShade(Hue::YELLOW, LIGHTEST));
        }
    }

    // Where the last order was given: a ring that closes in over a second.
    if (gMarker.on) {
        const usecs age = now() - gMarker.since;
        if (age >= kMarkerTime) {
            gMarker.on = false;
            return;
        }
        Point world = gMarker.world;
        if (gMarker.object.get() && gMarker.object->active &&
            (gMarker.object->id == gMarker.object_id)) {
            world = gMarker.object->location;
        }
        const Point c    = scale_to_viewport(world);
        const int   half = 6 + static_cast<int>(10 * (kMarkerTime - age).count() / kMarkerTime.count());
        if (clip.contains(c)) {
            draw_brackets(
                    rects, Rect(c.h - half, c.v - half, c.h + half, c.v + half), half, half / 2,
                    GetRGBTranslateColorShade(order_hue(gMarker.order), LIGHTEST));
        }
    }
}

bool modern_cursor_color(Point where, RgbColor* color) {
    if (!g.modern_controls || !g.ship.get() || !viewport().contains(where)) {
        return false;
    }
    const ModernOrder order = order_for(object_at(where));
    if (order == ModernOrder::NONE) {
        return false;
    }
    *color = GetRGBTranslateColorShade(order_hue(order), MEDIUM);
    return true;
}

}  // namespace antares
