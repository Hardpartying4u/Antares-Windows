// Copyright (C) 2026 The Star Bound Authors
// This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
//
// Mouse-wheel zoom steps, and the polled gamepad's mapping to the game's gamepad events.

#include <gmock/gmock.h>

#include "data/resource.hpp"
#include "game/modern-orders.hpp"
#include "game/player-ship.hpp"
#include "game/sys.hpp"
#include "math/rotation.hpp"
#include "ui/gamepad-poller.hpp"

namespace antares {
namespace {

using ControlsTest = ::testing::Test;

TEST_F(ControlsTest, WheelZoomInSteps) {
    EXPECT_EQ(wheel_zoom(Zoom::SIXTEENTH, true), Zoom::QUARTER);
    EXPECT_EQ(wheel_zoom(Zoom::QUARTER, true), Zoom::HALF);
    EXPECT_EQ(wheel_zoom(Zoom::HALF, true), Zoom::ACTUAL);
    EXPECT_EQ(wheel_zoom(Zoom::ACTUAL, true), Zoom::DOUBLE);
    EXPECT_EQ(wheel_zoom(Zoom::DOUBLE, true), Zoom::DOUBLE);  // closest zoom: stays
}

TEST_F(ControlsTest, WheelZoomOutStopsAtSixteenth) {
    EXPECT_EQ(wheel_zoom(Zoom::DOUBLE, false), Zoom::ACTUAL);
    EXPECT_EQ(wheel_zoom(Zoom::ACTUAL, false), Zoom::HALF);
    EXPECT_EQ(wheel_zoom(Zoom::HALF, false), Zoom::QUARTER);
    EXPECT_EQ(wheel_zoom(Zoom::QUARTER, false), Zoom::SIXTEENTH);
    EXPECT_EQ(wheel_zoom(Zoom::SIXTEENTH, false), Zoom::SIXTEENTH);  // never into the auto zooms
}

TEST_F(ControlsTest, WheelFromAutomaticZoom) {
    for (Zoom z : {Zoom::FOE, Zoom::OBJECT, Zoom::ALL}) {
        EXPECT_EQ(wheel_zoom(z, true), Zoom::SIXTEENTH);  // in: to the widest fixed zoom
        EXPECT_EQ(wheel_zoom(z, false), z);               // out: stays
    }
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

}  // namespace
}  // namespace antares
