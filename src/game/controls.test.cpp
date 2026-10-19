// Copyright (C) 2026 The Star Bound Authors
// This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
//
// Mouse-wheel zoom steps.

#include <gmock/gmock.h>

#include "game/player-ship.hpp"

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

}  // namespace
}  // namespace antares
