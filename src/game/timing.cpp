// Copyright (C) 2026 The Star Bound Authors
// This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.

#include "game/timing.hpp"

#include <stdio.h>
#include <chrono>
#include <pn/output>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

namespace antares {

static FILE* timing_file = nullptr;

// Milliseconds since the process was created (so time before main() counts too), and private
// memory in MB.
static double ms_since_start() {
#ifdef _WIN32
    FILETIME created, exited, kernel, user, now;
    GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user);
    GetSystemTimePreciseAsFileTime(&now);
    ULARGE_INTEGER c, n;
    c.LowPart  = created.dwLowDateTime;
    c.HighPart = created.dwHighDateTime;
    n.LowPart  = now.dwLowDateTime;
    n.HighPart = now.dwHighDateTime;
    return (n.QuadPart - c.QuadPart) / 10000.0;
#else
    static const auto start = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start)
            .count();
#endif
}

static double private_mb() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (K32GetProcessMemoryInfo(
                GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
                sizeof(pmc))) {
        return pmc.PrivateUsage / (1024.0 * 1024.0);
    }
#endif
    return 0;
}

void timing_open(pn::string_view path) {
    pn::string p = path.copy();
    timing_file  = fopen(p.c_str(), "w");
    timing_mark("start");
}

bool timing_on() { return timing_file != nullptr; }

void timing_mark(pn::string_view event) {
    if (!timing_file) {
        return;
    }
    pn::string e = event.copy();
    fprintf(timing_file, "%.1f\t%.1f\t%s\n", ms_since_start(), private_mb(), e.c_str());
    fflush(timing_file);
}

void timing_decode(pn::string_view path, int64_t bytes, int64_t pixels, double ms) {
    if (!timing_file) {
        return;
    }
    pn::string p = path.copy();
    fprintf(timing_file, "%.1f\t%.1f\tdecode\t%s\t%lld\t%lld\t%.2f\n", ms_since_start(),
            private_mb(), p.c_str(), static_cast<long long>(bytes),
            static_cast<long long>(pixels), ms);
    fflush(timing_file);
}

}  // namespace antares
