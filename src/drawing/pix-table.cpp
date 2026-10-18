// Copyright (C) 1997, 1999-2001, 2008 Nathan Lamont
// Copyright (C) 2008-2017 The Antares Authors
//
// This file is part of Antares, a tactical space combat game.
//
// Antares is free software: you can redistribute it and/or modify it
// under the terms of the Lesser GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Antares is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with Antares.  If not, see http://www.gnu.org/licenses/

#include "drawing/pix-table.hpp"

#include <pn/array>
#include <pn/map>
#include <pn/output>
#include <sfz/sfz.hpp>

#include "data/resource.hpp"
#include "data/sprite-data.hpp"
#include "drawing/color.hpp"
#include "game/sys.hpp"
#include "video/driver.hpp"

using sfz::range;
using std::unique_ptr;
using std::vector;

namespace antares {

// Sprite sheets come at 1x and optionally at @2x and @4x. Returns the order in which to try them
// for a draw scale: first the smallest sheet at least as detailed as the screen (shrinking a
// sharper sheet looks better than enlarging a blurrier one), then larger ones, then smaller ones.
// For example 2 -> 2, 4, 1; 3 -> 4, 2, 1; 4 -> 4, 2, 1. A 1x screen always uses the 1x sheet.
static vector<int> sheet_scales_to_try(int draw_scale) {
    static const int kSheetScales[] = {1, 2, 4};
    vector<int>      order;
    for (int s : kSheetScales) {
        if (s >= draw_scale) {
            order.push_back(s);
        }
    }
    for (int i = 2; i >= 0; --i) {
        if (kSheetScales[i] < draw_scale) {
            order.push_back(kSheetScales[i]);
        }
    }
    return order;
}

NatePixTable::NatePixTable(pn::string_view name, Hue hue, int draw_scale)
        : _draw_scale(draw_scale > 0 ? draw_scale : sys.video->scale()) {
    SpriteData  data    = Resource::sprite_data(name);
    ArrayPixMap image   = Resource::sprite_image(name);
    ArrayPixMap overlay = Resource::sprite_overlay(name);

    if (image.size() != overlay.size()) {
        throw std::runtime_error("size mismatch between image and overlay");
    }

    // Use a higher-resolution sheet when the screen can show it. It must be an exact multiple of
    // the 1x sheet, because frame rectangles and pivots stay in 1x units: they define collision
    // bounds and weapon mounts, so only the texture changes.
    int scale = 1;
    for (int s : sheet_scales_to_try(_draw_scale)) {
        if (s == 1) {
            break;
        } else if (!Resource::sprite_exists(name, s)) {
            continue;
        }
        ArrayPixMap hi_image   = Resource::sprite_image(name, s);
        ArrayPixMap hi_overlay = Resource::sprite_overlay(name, s);
        const Size  want{image.size().width * s, image.size().height * s};
        if ((hi_image.size() != want) || (hi_overlay.size() != want)) {
            pn::err.format(
                    "sprites/{0}: ignoring @{1}x sheets: not exactly {1} times the 1x size\n",
                    name, s);
            continue;
        }
        image   = std::move(hi_image);
        overlay = std::move(hi_overlay);
        scale   = s;
        break;
    }

    for (SpriteData::Frame frame : data.frames) {
        const int i = _frames.size();
        Rect      sprite{frame.left, frame.top, frame.right, frame.bottom};
        Rect      bounds = sprite;
        bounds.offset(-frame.cx, -frame.cy);
        Rect sheet_rect{sprite.left * scale, sprite.top * scale, sprite.right * scale,
                        sprite.bottom * scale};
        if (hue == Hue::GRAY) {
            _frames.emplace_back(bounds, scale, image.view(sheet_rect), name, i);
        } else {
            _frames.emplace_back(
                    bounds, scale, image.view(sheet_rect), name, i, overlay.view(sheet_rect), hue);
        }
    }
}

NatePixTable::~NatePixTable() {}

const NatePixTable::Frame& NatePixTable::at(size_t index) const { return _frames[index]; }

size_t NatePixTable::size() const { return _size; }

NatePixTable::Frame::Frame(
        Rect bounds, int scale, const PixMap& image, pn::string_view name, int frame,
        const PixMap& overlay, Hue hue)
        : _bounds(bounds),
          _scale(scale),
          _pix_map(bounds.width() * scale, bounds.height() * scale) {
    load_image(image);
    load_overlay(overlay, hue);
    build(name, frame);
}

NatePixTable::Frame::Frame(
        Rect bounds, int scale, const PixMap& image, pn::string_view name, int frame)
        : _bounds(bounds),
          _scale(scale),
          _pix_map(bounds.width() * scale, bounds.height() * scale) {
    load_image(image);
    build(name, frame);
}

NatePixTable::Frame::~Frame() {}

void NatePixTable::Frame::load_image(const PixMap& pix) { _pix_map.copy(pix); }

void NatePixTable::Frame::load_overlay(const PixMap& pix, Hue hue) {
    for (auto x : range(_pix_map.size().width)) {
        for (auto y : range(_pix_map.size().height)) {
            RgbColor over  = pix.get(x, y);
            uint8_t  value = over.red;
            uint8_t  frac  = over.alpha;
            over           = RgbColor::tint(hue, value);
            RgbColor under = _pix_map.get(x, y);
            RgbColor composite;
            composite.red   = ((over.red * frac) + (under.red * (255 - frac))) / 255;
            composite.green = ((over.green * frac) + (under.green * (255 - frac))) / 255;
            composite.blue  = ((over.blue * frac) + (under.blue * (255 - frac))) / 255;
            composite.alpha = under.alpha;
            _pix_map.set(x, y, composite);
        }
    }
}

uint16_t       NatePixTable::Frame::width() const { return _bounds.width(); }
uint16_t       NatePixTable::Frame::height() const { return _bounds.height(); }
Point          NatePixTable::Frame::center() const { return {-_bounds.left, -_bounds.top}; }
const PixMap&  NatePixTable::Frame::pix_map() const { return _pix_map; }
const Texture& NatePixTable::Frame::texture() const { return _texture; }

void NatePixTable::Frame::build(pn::string_view name, int frame) {
    // The texture is drawn at the frame's 1x size; a larger pixmap just gives it more detail.
    _texture = sys.video->texture(pn::format("/sprites/{0}%{1}", name, frame), _pix_map, _scale);
}

}  // namespace antares
