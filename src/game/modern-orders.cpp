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
        case ModernOrder::ATTACK_MOVE: return Hue::RED;
        default: return Hue::SKY_BLUE;
    }
}

const char* order_name(ModernOrder o) {
    switch (o) {
        case ModernOrder::ATTACK: return "attack";
        case ModernOrder::FOLLOW: return "follow";
        case ModernOrder::GUARD: return "guard";
        case ModernOrder::MOVE: return "move";
        case ModernOrder::ATTACK_MOVE: return "attack-move";
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
    auto selection = modern_selection();
    return classify_order(
            !selection.empty(), object.get() != nullptr,
            object.get() && (object->owner != g.admiral),
            object.get() && (object->attributes & kIsDestination),
            (selection.size() == 1) && (object == selection[0]));
}

bool selectable(Handle<SpaceObject> o) {
    return o.get() && (o->active == kObjectInUse) &&
           group_selectable(o->attributes, o->owner == g.admiral, o == g.admiral->flagship());
}

// The selection with the dead, captured and locked ships dropped, and matched to the control
// object: if that changed some other way, the selection is just it. Pure: drawing uses it too.
std::vector<std::pair<Handle<SpaceObject>, int32_t>> current_selection() {
    std::vector<std::pair<Handle<SpaceObject>, int32_t>> sel;
    auto control = g.admiral.get() ? g.admiral->control() : SpaceObject::none();
    if (control != g.modern_selection_control) {
        if (selectable(control)) {
            sel.emplace_back(control, control->id);
        }
        return sel;
    }
    for (const auto& s : g.modern_selection) {
        if (s.first.get() && (s.first->id == s.second) && selectable(s.first)) {
            sel.push_back(s);
        }
    }
    return sel;
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

// The command armed by the attack-move key (view only: the crosshair's colour).
ModernCommand gArmed = ModernCommand::NONE;

// The group pointer being shown (view only, wall time).
struct {
    int       group = -1;
    wall_time since;
} gPointer;

bool group_valid(const GroupEntry& e) {
    Handle<SpaceObject> o(e.number);
    return (e.number >= 0) && (e.number < kMaxSpaceObject) && (o->id == e.id) && selectable(o);
}

void log_event(pn::string_view what) {
    if (modern_order_log) {
        modern_order_log(pn::format("{0}\t{1}\n", g.time.time_since_epoch().count(), what));
    }
}

pn::string ship_names(const std::vector<Handle<SpaceObject>>& ships) {
    pn::string names;
    for (auto o : ships) {
        names += pn::format("{0}{1} #{2}", names.empty() ? "" : ";", o->long_name(), o.number());
    }
    return names;
}

// The drag box being drawn: view only, set from PlayerShip's queued mouse events.
struct {
    bool  active = false;
    Point start;
} gDrag;

}  // namespace

void modern_drag(bool active, Point start) {
    gDrag.active = active;
    gDrag.start  = start;
}

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

bool modern_orderable(Handle<Admiral> admiral) { return !modern_selection().empty(); }

bool group_selectable(uint32_t attributes, bool owned_by_player, bool is_flagship) {
    return owned_by_player && !is_flagship && (attributes & kCanAcceptDestination) &&
           !(attributes & (kStaticDestination | kIsDestination));
}

bool in_box(Point p, Point a, Point b) {
    return (std::min(a.h, b.h) <= p.h) && (p.h <= std::max(a.h, b.h)) &&
           (std::min(a.v, b.v) <= p.v) && (p.v <= std::max(a.v, b.v));
}

std::vector<Handle<SpaceObject>> modern_selection() {
    g.modern_selection         = current_selection();
    g.modern_selection_control = g.admiral.get() ? g.admiral->control() : SpaceObject::none();
    std::vector<Handle<SpaceObject>> ships;
    for (const auto& s : g.modern_selection) {
        ships.push_back(s.first);
    }
    return ships;
}

std::vector<Handle<SpaceObject>> modern_ships_in_box(Point a, Point b) {
    std::vector<Handle<SpaceObject>> ships;
    for (auto o : SpaceObject::all()) {
        if (selectable(o) && o->sprite.get() && in_box(o->sprite->where, a, b)) {
            ships.push_back(o);
        }
    }
    return ships;
}

void modern_select(const std::vector<Handle<SpaceObject>>& ships, bool add) {
    modern_selection();  // matched to the control object first
    if (!add) {
        g.modern_selection.clear();
    }
    for (auto o : ships) {
        bool present = false;
        for (const auto& s : g.modern_selection) {
            present = present || (s.first == o);
        }
        if (!present && selectable(o)) {
            g.modern_selection.emplace_back(o, o->id);
        }
    }
}

bool give_attack_move(Handle<Admiral> admiral, Point world) {
    auto ships = modern_selection();
    if (ships.empty()) {
        return false;
    }
    for (int i = 0; i < ships.size(); ++i) {
        const Point offset = formation_offset(i, ships.size(), kFormationSpacing);
        Point       p(world.h + offset.h, world.v + offset.v);
        // The Classic point order: the 15 s clock is left at 0, so the ship engages enemies it
        // meets within engage range, then carries on to the point.
        SetObjectLocationDestination(ships[i], &p);
        log_order(ships[i], ModernOrder::ATTACK_MOVE, pn::format("to {0}, {1}", p.h, p.v));
    }
    if (admiral == g.admiral) {
        sys.sound.order();
        show_marker(ModernOrder::ATTACK_MOVE, world, SpaceObject::none());
    }
    return true;
}

void modern_arm(ModernCommand command) { gArmed = command; }

ModernOrder modern_command_click(ModernCommand command, Point where) {
    if (!g.ship.get() || (g.key_mask & (kMouseMask | kOrderKey)) || !viewport().contains(where)) {
        return ModernOrder::NONE;
    }
    if (command == ModernCommand::ATTACK_MOVE) {
        auto object = object_at(where);
        if (object.get() && (object->owner != g.admiral)) {
            PlayerShipHandleClick(where, 1);  // a click on an enemy: target and attack it
            return give_object_order(g.admiral, object) ? ModernOrder::ATTACK : ModernOrder::NONE;
        }
        return give_attack_move(g.admiral, screen_to_world(where)) ? ModernOrder::ATTACK_MOVE
                                                                    : ModernOrder::NONE;
    }
    return ModernOrder::NONE;
}

void modern_group_save(int group) {
    auto                    ships = modern_selection();
    std::vector<GroupEntry> entries;
    for (auto o : ships) {
        entries.push_back(GroupEntry{o.number(), o->id});
    }
    g.modern_groups.save(group, entries);
    log_event(pn::format("group {0} saved\t{1}", (group + 1) % 10, ship_names(ships)));
}

std::vector<Handle<SpaceObject>> modern_group_ships(int group) {
    std::vector<Handle<SpaceObject>> ships;
    for (const auto& e : g.modern_groups.recall(group, group_valid)) {
        ships.push_back(Handle<SpaceObject>(e.number));
    }
    return ships;
}

void modern_group_pointer(int group) {
    gPointer.group = group;
    gPointer.since = now();
    log_event(pn::format("group {0} pointer", (group + 1) % 10));
}

void modern_selection_matches_control() {
    g.modern_selection_control = g.admiral->control();
}

Point formation_offset(int index, int count, int32_t spacing) {
    if (count <= 1) {
        return Point(0, 0);
    }
    int cols = 1;
    while (cols * cols < count) {
        ++cols;
    }
    const int rows = (count + cols - 1) / cols;
    const int col  = index % cols;
    const int row  = index / cols;
    return Point((2 * col - (cols - 1)) * spacing / 2, (2 * row - (rows - 1)) * spacing / 2);
}

bool modern_controls_for_level(bool replay, bool tutorial) {
    if (replay) {
        return modern_controls_in_replays;
    }
    return !tutorial && (sys.prefs->controls_preset() != ControlPreset::CLASSIC);
}

bool give_object_order(Handle<Admiral> admiral, Handle<SpaceObject> object) {
    if (!object.get()) {
        return false;
    }
    std::vector<Handle<SpaceObject>> ships;
    for (auto o : modern_selection()) {
        if (o != object) {  // a ship isn't ordered to itself
            ships.push_back(o);
        }
    }
    if (ships.empty()) {
        return false;
    }
    const ModernOrder order = classify_order(
            true, true, object->owner != admiral, object->attributes & kIsDestination, false);
    admiral->set_target(object);
    for (auto o : ships) {
        SetObjectDestination(o);  // the Classic order: go to the admiral's target
        log_order(o, order, pn::format("{0} #{1}", object->long_name(), object.number()));
    }
    if (admiral == g.admiral) {
        sys.sound.order();
        show_marker(order, object->location, object);
    }
    return true;
}

bool give_move_order(Handle<Admiral> admiral, Point world) {
    auto ships = modern_selection();
    if (ships.empty()) {
        return false;
    }
    for (int i = 0; i < ships.size(); ++i) {
        // A small grid around the point, so a group doesn't stack on one spot.
        const Point offset = formation_offset(i, ships.size(), kFormationSpacing);
        Point       p(world.h + offset.h, world.v + offset.v);
        SetObjectLocationDestination(ships[i], &p);
        // A move flies straight there, like an object order: the 15 s clock starts used up, so
        // the ship doesn't turn aside to fight unless it is hit (docs/modern-controls.md §3).
        ships[i]->timeFromOrigin = kTimeToCheckHome;
        log_order(ships[i], ModernOrder::MOVE, pn::format("to {0}, {1}", p.h, p.v));
    }
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
            pn::string names;
            for (const auto& sel : current_selection()) {
                names += pn::format("{0}{1} #{2}", names.empty() ? "" : ";",
                                    sel.first->long_name(), sel.first.number());
            }
            timing_mark(pn::format("modern selection\t{0}", names));
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
    // Brackets on every selected ship and on the control object (also when it can't take orders:
    // a base, the flagship); the control object brighter. Then the drag box.
    std::vector<Handle<SpaceObject>> marked;
    if (control.get()) {
        marked.push_back(control);
    }
    for (const auto& sel : current_selection()) {
        if (sel.first != control) {
            marked.push_back(sel.first);
        }
    }
    for (auto o : marked) {
        if (!o->sprite.get()) {
            continue;
        }
        const auto& frame = o->sprite->table->at(o->sprite->whichShape);
        const Rect  world = scale_sprite_rect(frame, o->location, o->naturalScale);
        const Point tl    = scale_to_viewport(Point(world.left, world.top));
        const Point br    = scale_to_viewport(Point(world.right, world.bottom));
        Rect        r(tl.h, tl.v, std::max(tl.h, br.h), std::max(tl.v, br.v));
        r.inset(-3, -3);
        if (clip.contains(r.center())) {
            draw_brackets(
                    rects, r, 6, 4,
                    GetRGBTranslateColorShade(Hue::YELLOW, (o == control) ? LIGHTEST : LIGHT));
        }
    }
    if (gDrag.active) {
        const Point m = sys.video->get_mouse();
        Rect        box(std::min(gDrag.start.h, m.h), std::min(gDrag.start.v, m.v),
                        std::max(gDrag.start.h, m.h) + 1, std::max(gDrag.start.v, m.v) + 1);
        box.left   = std::max(box.left, clip.left);
        box.top    = std::max(box.top, clip.top);
        box.right  = std::min(box.right, clip.right);
        box.bottom = std::min(box.bottom, clip.bottom);
        if ((box.right > box.left) && (box.bottom > box.top)) {
            const RgbColor c = GetRGBTranslateColorShade(Hue::YELLOW, MEDIUM);
            rects.fill(Rect(box.left, box.top, box.right, box.top + 1), c);
            rects.fill(Rect(box.left, box.bottom - 1, box.right, box.bottom), c);
            rects.fill(Rect(box.left, box.top, box.left + 1, box.bottom), c);
            rects.fill(Rect(box.right - 1, box.top, box.right, box.bottom), c);
        }
    }

    // The group pointer (a double-tapped number): for 1.5 s, brackets around the group's middle
    // if it is on screen, else a line from the middle of the play area toward it.
    if ((gPointer.group >= 0) && (now() - gPointer.since < secs(1) + std::chrono::milliseconds(500))) {
        auto ships = g.modern_groups.peek(gPointer.group, group_valid);
        if (!ships.empty()) {
            int64_t h = 0, v = 0;
            for (const auto& e : ships) {
                h += Handle<SpaceObject>(e.number)->location.h;
                v += Handle<SpaceObject>(e.number)->location.v;
            }
            const Point    c = scale_to_viewport(
                    Point(int32_t(h / int64_t(ships.size())), int32_t(v / int64_t(ships.size()))));
            const RgbColor color = GetRGBTranslateColorShade(Hue::YELLOW, LIGHTEST);
            if (clip.contains(c)) {
                draw_brackets(rects, Rect(c.h - 20, c.v - 20, c.h + 20, c.v + 20), 20, 8, color);
            } else {
                const Point  m = clip.center();
                const double dx = c.h - m.h, dy = c.v - m.v;
                const double t  = std::min(
                        (dx != 0) ? (clip.width() / 2 - 12) / std::abs(dx) : 1e9,
                        (dy != 0) ? (clip.height() / 2 - 12) / std::abs(dy) : 1e9);
                const Point  end(m.h + int32_t(dx * t), m.v + int32_t(dy * t));
                Lines().draw(m, end, color);
                draw_brackets(rects, Rect(end.h - 6, end.v - 6, end.h + 6, end.v + 6), 6, 3, color);
            }
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
    if (gArmed != ModernCommand::NONE) {
        *color = GetRGBTranslateColorShade(Hue::RED, LIGHT);
        return true;
    }
    const ModernOrder order = order_for(object_at(where));
    if (order == ModernOrder::NONE) {
        return false;
    }
    *color = GetRGBTranslateColorShade(order_hue(order), MEDIUM);
    return true;
}

}  // namespace antares
