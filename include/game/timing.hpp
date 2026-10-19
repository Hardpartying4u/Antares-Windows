// Copyright (C) 2026 The Star Bound Authors
// This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.

#ifndef ANTARES_GAME_TIMING_HPP_
#define ANTARES_GAME_TIMING_HPP_

#include <pn/string>

namespace antares {

// Diagnostics for start-up and load times (`--timing FILE`). Each line is tab-separated:
// milliseconds since the process started, private memory in MB, then the event. Off unless opened;
// every call is then a no-op.
void timing_open(pn::string_view path);
bool timing_on();
void timing_mark(pn::string_view event);

// Logs one image decode: resource path, file bytes, pixels and milliseconds.
void timing_decode(pn::string_view path, int64_t bytes, int64_t pixels, double ms);

}  // namespace antares

#endif  // ANTARES_GAME_TIMING_HPP_
