// Copyright (C) 2026 The Star Bound Authors
// This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
//
// Mouse-wheel zoom steps, and the polled gamepad's mapping to the game's gamepad events.

#include <gmock/gmock.h>

#include "data/resource.hpp"
#include "game/control-groups.hpp"
#include "game/modern-orders.hpp"
#include "game/player-ship.hpp"
#include "game/sys.hpp"
#include "math/rotation.hpp"
#include "ui/gamepad-poller.hpp"

namespace antares {
namespace {

using ControlsTest = ::testing::Test;

// The wheel and the zoom keys step through the same full list, in the zoom keys' order: 2:1, 1:1,
// 1:2, 1:4, 1:16, nearest hostile, nearest object, all (zoom_step(), player-ship.cpp).
const Zoom kZoomOrder[] = {Zoom::DOUBLE,    Zoom::ACTUAL, Zoom::HALF,   Zoom::QUARTER,
                           Zoom::SIXTEENTH, Zoom::FOE,    Zoom::OBJECT, Zoom::ALL};

TEST_F(ControlsTest, WheelZoomOutReachesEveryLevel) {
    Zoom z = Zoom::DOUBLE;
    for (int i = 1; i < 8; ++i) {
        z = zoom_step(z, false);
        EXPECT_EQ(z, kZoomOrder[i]) << i;
    }
    EXPECT_EQ(zoom_step(Zoom::ALL, false), Zoom::ALL);  // the widest: stays
}

TEST_F(ControlsTest, WheelZoomInReachesEveryLevel) {
    Zoom z = Zoom::ALL;
    for (int i = 6; i >= 0; --i) {
        z = zoom_step(z, true);
        EXPECT_EQ(z, kZoomOrder[i]) << i;
    }
    EXPECT_EQ(zoom_step(Zoom::DOUBLE, true), Zoom::DOUBLE);  // the closest: stays
}

// The automatic zooms are steps like the others: out from 1:16 to nearest hostile, in from all to
// nearest object (not a jump back to 1:16, as the wheel did from 2026-10-20 to 2026-10-21).
TEST_F(ControlsTest, WheelZoomThroughAutomaticZooms) {
    EXPECT_EQ(zoom_step(Zoom::SIXTEENTH, false), Zoom::FOE);
    EXPECT_EQ(zoom_step(Zoom::FOE, false), Zoom::OBJECT);
    EXPECT_EQ(zoom_step(Zoom::OBJECT, true), Zoom::FOE);
    EXPECT_EQ(zoom_step(Zoom::FOE, true), Zoom::SIXTEENTH);
}

// Records the events a poller sends.
struct Recorder : public EventReceiver {
    std::vector<std::pair<int, Gamepad::Button>> buttons;  // +1 down, -1 up
    std::vector<std::pair<double, double>>       sticks;
    void gamepad_button_down(const GamepadButtonDownEvent& e) override { buttons.push_back({+1, e.button}); }
    void gamepad_button_up(const GamepadButtonUpEvent& e) override { buttons.push_back({-1, e.button}); }
    void gamepad_stick(const GamepadStickEvent& e) override {
        if (e.stick == Gamepad::Stick::LS) {
            sticks.push_back({e.x, e.y});
        }
    }
};

void poll(GamepadPoller* p, const GamepadPoller::State& s, Recorder* r, bool connected = true) {
    for (const auto& e : p->update(s, connected, wall_time())) {
        e->send(r);
    }
}

TEST_F(ControlsTest, GamepadButtons) {
    // Standard-layout buttons in GLFW order map to the macOS driver's buttons.
    const Gamepad::Button expected[GamepadPoller::kButtons] = {
            Gamepad::Button::A,     Gamepad::Button::B,     Gamepad::Button::X,
            Gamepad::Button::Y,     Gamepad::Button::LB,    Gamepad::Button::RB,
            Gamepad::Button::BACK,  Gamepad::Button::START, Gamepad::Button::NONE,
            Gamepad::Button::LSB,   Gamepad::Button::RSB,   Gamepad::Button::UP,
            Gamepad::Button::RIGHT, Gamepad::Button::DOWN,  Gamepad::Button::LEFT,
    };
    for (int i = 0; i < GamepadPoller::kButtons; ++i) {
        GamepadPoller          p;
        Recorder               r;
        GamepadPoller::State   s;
        s.buttons[i] = true;
        poll(&p, s, &r);
        poll(&p, s, &r);  // held: nothing new
        s.buttons[i] = false;
        poll(&p, s, &r);
        if (expected[i] == Gamepad::Button::NONE) {
            EXPECT_TRUE(r.buttons.empty()) << "button " << i << " (Guide) is not used";
        } else {
            ASSERT_EQ(r.buttons.size(), 2u) << "button " << i;
            EXPECT_EQ(r.buttons[0], std::make_pair(+1, expected[i]));
            EXPECT_EQ(r.buttons[1], std::make_pair(-1, expected[i]));
        }
    }
}

TEST_F(ControlsTest, GamepadTriggersWithHysteresis) {
    GamepadPoller        p;
    Recorder             r;
    GamepadPoller::State s;
    s.axes[5] = 0.0f;  // right trigger half way (50 %): not yet pressed
    poll(&p, s, &r);
    EXPECT_TRUE(r.buttons.empty());
    s.axes[5] = 0.8f;  // 90 %: pressed
    poll(&p, s, &r);
    s.axes[5] = -0.1f;  // 45 %: still held
    poll(&p, s, &r);
    s.axes[5] = -0.5f;  // 25 %: released
    poll(&p, s, &r);
    ASSERT_EQ(r.buttons.size(), 2u);
    EXPECT_EQ(r.buttons[0], std::make_pair(+1, Gamepad::Button::RT));
    EXPECT_EQ(r.buttons[1], std::make_pair(-1, Gamepad::Button::RT));
}

TEST_F(ControlsTest, GamepadStickDeadZoneAndSteps) {
    GamepadPoller        p;
    Recorder             r;
    GamepadPoller::State s;
    s.axes[1] = -0.5f;  // inside the dead zone (y is down on the pad)
    poll(&p, s, &r);
    EXPECT_TRUE(r.sticks.empty());
    s.axes[1] = -1.0f;  // full up
    poll(&p, s, &r);
    ASSERT_EQ(r.sticks.size(), 1u);
    EXPECT_NEAR(r.sticks[0].first, 0.0, 1e-6);
    EXPECT_NEAR(r.sticks[0].second, 1.0, 1e-6);  // up is sent as (0, +1)
    s.axes[0] = 0.02f;  // a tiny wobble stays in the same step: nothing sent
    poll(&p, s, &r);
    EXPECT_EQ(r.sticks.size(), 1u);
    s.axes[0] = 0.0f;
    s.axes[1] = -0.5f;  // back to 0.5: still outside the release radius (0.45)
    poll(&p, s, &r);
    EXPECT_EQ(r.sticks.size(), 1u);
    s.axes[1] = -0.3f;  // centred
    poll(&p, s, &r);
    ASSERT_EQ(r.sticks.size(), 2u);
    EXPECT_EQ(r.sticks[1], std::make_pair(0.0, 0.0));
}

TEST_F(ControlsTest, GamepadDisconnectReleasesEverything) {
    GamepadPoller        p;
    Recorder             r;
    GamepadPoller::State s;
    s.buttons[0] = true;  // A held
    s.axes[0]    = 1.0f;  // stick right
    poll(&p, s, &r);
    poll(&p, s, &r, false);
    ASSERT_EQ(r.buttons.size(), 2u);
    EXPECT_EQ(r.buttons[1], std::make_pair(-1, Gamepad::Button::A));
    ASSERT_EQ(r.sticks.size(), 2u);
    EXPECT_EQ(r.sticks[1], std::make_pair(0.0, 0.0));
}

// The stick vector the poller sends turns the flagship the way the stick points: PlayerShip turns
// it into a heading with GetAngleFromVector (+180), and the heading's world vector (y down the
// screen) must point the same way as the stick.
TEST_F(ControlsTest, GamepadStickSteersTheRightWay) {
    sys.rot_table = Resource::rotation_table();
    struct {
        float pad_x, pad_y;  // as the pad reports it (y down)
        int   want_h, want_v;  // sign of the heading on screen
    } cases[] = {{0, -1, 0, -1}, {0, 1, 0, 1}, {1, 0, 1, 0}, {-1, 0, -1, 0}};
    for (const auto& c : cases) {
        GamepadPoller        p;
        Recorder             r;
        GamepadPoller::State s;
        s.axes[0] = c.pad_x;
        s.axes[1] = c.pad_y;
        poll(&p, s, &r);
        ASSERT_EQ(r.sticks.size(), 1u);
        int32_t direction = GetAngleFromVector(r.sticks[0].first * 32768, r.sticks[0].second * 32768);
        mAddAngle(direction, ROT_180);
        Fixed h, v;
        GetRotPoint(&h, &v, direction);
        auto sign = [](Fixed f) { return (f > Fixed::from_float(0.5)) ? 1 : (f < Fixed::from_float(-0.5)) ? -1 : 0; };
        EXPECT_EQ(sign(h), c.want_h) << "pad " << c.pad_x << "," << c.pad_y;
        EXPECT_EQ(sign(v), c.want_v) << "pad " << c.pad_x << "," << c.pad_y;
    }
}

// What a Modern right-click does, from what is under the cursor.
TEST_F(ControlsTest, ModernRightClickOrders) {
    using O = ModernOrder;
    //                        orderable, object, hostile, base,  is the selected ship
    EXPECT_EQ(classify_order(true, false, false, false, false), O::MOVE);    // empty space
    EXPECT_EQ(classify_order(true, true, true, false, false), O::ATTACK);    // enemy ship
    EXPECT_EQ(classify_order(true, true, true, true, false), O::ATTACK);     // enemy base
    EXPECT_EQ(classify_order(true, true, false, false, false), O::FOLLOW);   // friendly ship
    EXPECT_EQ(classify_order(true, true, false, true, false), O::GUARD);     // friendly base
    EXPECT_EQ(classify_order(true, true, false, false, true), O::NONE);      // itself
    // Nothing orderable selected (the flagship, a base, a locked ship): a right-click only
    // targets, as in Classic.
    for (bool object : {false, true}) {
        for (bool hostile : {false, true}) {
            EXPECT_EQ(classify_order(false, object, hostile, false, false), O::NONE);
        }
    }
}

// Screen to world is the inverse of the game's world to screen (scale_to_viewport) at every zoom
// (1:16 to 2:1, and in between as the zoom eases) and wherever the play area starts on screen.
TEST_F(ControlsTest, ScreenToWorldInvertsScaleToViewport) {
    const Rect bounds(1073741824 - 9000, 1073741824 - 5000, 1073741824 + 9000, 1073741824 + 5000);
    for (int32_t factor : {256, 512, 1024, 1500, 2048, 3000, 4096, 6000, 8192}) {
        const Scale scale{factor};
        for (Point origin : {Point(128, 0), Point(32, 0), Point(1100, 0)}) {
            for (int x = origin.h; x < origin.h + 2400; x += 37) {
                for (int y = 0; y < 1440; y += 41) {
                    const Point world = viewport_to_world(Point(x, y), bounds, scale, origin);
                    const Point back(
                            scale_by(world.h - bounds.left, scale) + origin.h,
                            scale_by(world.v - bounds.top, scale) + origin.v);
                    if (factor <= SCALE_SCALE.factor) {
                        // At 1:1 and further out each pixel covers whole world units: exact.
                        ASSERT_EQ(back, Point(x, y)) << factor;
                    } else {
                        // Zoomed in, a world unit covers several pixels: the nearest one.
                        ASSERT_LE(std::abs(back.h - x), 1) << factor;
                        ASSERT_LE(std::abs(back.v - y), 1) << factor;
                    }
                }
            }
        }
    }
}

// Which ships a drag box takes: the player's own that can take orders; never the flagship, a base,
// a ship whose destination the level locks, or someone else's.
TEST_F(ControlsTest, GroupSelectable) {
    const uint32_t ship = kCanAcceptDestination | kCanBeDestination;
    EXPECT_TRUE(group_selectable(ship, true, false));
    EXPECT_FALSE(group_selectable(ship, true, true));                          // flagship
    EXPECT_FALSE(group_selectable(ship, false, false));                        // not the player's
    EXPECT_FALSE(group_selectable(ship | kIsDestination, true, false));        // a base
    EXPECT_FALSE(group_selectable(ship | kStaticDestination, true, false));    // locked
    EXPECT_FALSE(group_selectable(kCanBeDestination, true, false));            // takes no orders
}

TEST_F(ControlsTest, DragBox) {
    const Point a(100, 50), b(200, 150);
    for (const Point& corner1 : {a, b}) {  // either corner first
        const Point& corner2 = (corner1 == a) ? b : a;
        EXPECT_TRUE(in_box(Point(150, 100), corner1, corner2));
        EXPECT_TRUE(in_box(Point(100, 50), corner1, corner2));    // edges count
        EXPECT_TRUE(in_box(Point(200, 150), corner1, corner2));
        EXPECT_FALSE(in_box(Point(99, 100), corner1, corner2));
        EXPECT_FALSE(in_box(Point(150, 151), corner1, corner2));
    }
    EXPECT_TRUE(in_box(Point(200, 50), Point(200, 150), Point(100, 50)));  // other diagonal
}

// Group moves: one ship goes to the point; a group spreads on a centred grid, each spot distinct,
// the same every time.
TEST_F(ControlsTest, FormationOffsets) {
    EXPECT_EQ(formation_offset(0, 1, 160), Point(0, 0));
    for (int count = 2; count <= 12; ++count) {
        std::vector<Point> spots;
        int64_t            sum_h = 0, sum_v = 0;
        for (int i = 0; i < count; ++i) {
            const Point p = formation_offset(i, count, 160);
            EXPECT_EQ(p, formation_offset(i, count, 160)) << "deterministic";
            for (const Point& q : spots) {
                EXPECT_NE(p, q) << count << " ships: no two on one spot";
                EXPECT_GE(std::max(std::abs(p.h - q.h), std::abs(p.v - q.v)), 160);
            }
            EXPECT_LE(std::max(std::abs(p.h), std::abs(p.v)), 2 * 160) << "small";
            spots.push_back(p);
            sum_h += p.h;
            sum_v += p.v;
        }
        if ((count == 4) || (count == 9)) {  // full grids are centred on the point
            EXPECT_EQ(sum_h, 0);
            EXPECT_EQ(sum_v, 0);
        }
    }
    EXPECT_EQ(formation_offset(0, 4, 160), Point(-80, -80));
    EXPECT_EQ(formation_offset(3, 4, 160), Point(80, 80));
}

// Control groups: save, recall (only ships still valid, the lost ones dropped for good), add.
TEST_F(ControlsTest, ControlGroupsSaveRecallAdd) {
    ControlGroups groups;
    const GroupEntry a{4, 104}, b{8, 108}, c{10, 110}, d{12, 112};
    auto all_valid = [](const GroupEntry&) { return true; };

    groups.save(0, {a, b, b, c});  // a duplicate is kept once
    EXPECT_EQ(groups.recall(0, all_valid), (std::vector<GroupEntry>{a, b, c}));
    EXPECT_TRUE(groups.recall(1, all_valid).empty());   // another group is untouched
    EXPECT_TRUE(groups.recall(-1, all_valid).empty());  // out of range: nothing
    EXPECT_TRUE(groups.recall(10, all_valid).empty());

    groups.save(0, {d});  // saving replaces the group
    EXPECT_EQ(groups.recall(0, all_valid), (std::vector<GroupEntry>{d}));
    groups.save(0, {});   // saving an empty selection clears it
    EXPECT_TRUE(groups.recall(0, all_valid).empty());

    // Shift+number: the group appended to the selection, nothing twice, in order.
    EXPECT_EQ(ControlGroups::add({b, d}, {a, b, c}), (std::vector<GroupEntry>{b, d, a, c}));
}

TEST_F(ControlsTest, ControlGroupsDropLostShips) {
    ControlGroups groups;
    const GroupEntry a{4, 104}, b{8, 108}, c{10, 110};
    groups.save(2, {a, b, c});

    // b died (or was captured or locked): peek (drawing) skips it but keeps it stored...
    auto without_b = [&](const GroupEntry& e) { return !(e == b); };
    EXPECT_EQ(groups.peek(2, without_b), (std::vector<GroupEntry>{a, c}));
    EXPECT_EQ(groups.peek(2, [](const GroupEntry&) { return true; }),
              (std::vector<GroupEntry>{a, b, c}));
    // ...recall drops it for good: it doesn't come back when its slot is valid again.
    EXPECT_EQ(groups.recall(2, without_b), (std::vector<GroupEntry>{a, c}));
    EXPECT_EQ(groups.recall(2, [](const GroupEntry&) { return true; }),
              (std::vector<GroupEntry>{a, c}));
    // A reused object slot (same number, new id) is a different ship.
    const GroupEntry reused{4, 999};
    auto ids_match = [&](const GroupEntry& e) { return !(e == a) || (reused.id == a.id); };
    EXPECT_EQ(groups.recall(2, ids_match), (std::vector<GroupEntry>{c}));
    // Every group was lost: recalling it gives nothing.
    EXPECT_TRUE(groups.recall(2, [](const GroupEntry&) { return false; }).empty());

    groups.save(5, {a});
    groups.clear();  // a new level
    EXPECT_TRUE(groups.recall(5, [](const GroupEntry&) { return true; }).empty());
}

}  // namespace
}  // namespace antares
