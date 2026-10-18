# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Human (UNS) carrier: rounded-box shape (box_kit.py) measured from the original frames
(carve.py; camera tilt 46, distance 400 from camera_sweep.py).

A long flat deck over four hangar modules, a narrower neck with a raised fin behind the deck, two
large engine pods with a centre block between them, and two short prongs at the bow. The deck has
a bright centre spine between two dark stripes. One red light on the back of each pod. The
overlay covers the whole ship, lights included. The game draws capital ships at 2x
(tpl/capship: rotation scale 2.0).
"""

from box_kit import BoxKit, part

SHAPE = BoxKit(
    [
        part("deck", x=0.0, hw=8.0, hh=2.5, z=13.0, front=45.0, rear=-22.0, exp=6.0, chamfer=6.0),
        part("modules", region="modules", repeat=4, x=0.0, hw=9.0, hh=6.0, z=6.0, front=44.0,
             rear=-21.0, gap=1.5, exp=4.0),
        part("neck", x=0.0, hw=6.0, hh=7.0, z=9.0, front=-20.0, rear=-34.0, exp=6.0),
        part("fin", x=0.0, hw=3.0, hh=3.0, z=19.0, front=-24.0, rear=-33.0, exp=4.0, chamfer=3.0),
        part("block", region="block", x=0.0, hw=4.5, hh=5.0, z=11.0, front=-33.0, rear=-50.0, exp=5.0),
        part("pod", region="pods", mirror=True, x=10.8, hw=4.0, hh=6.0, z=11.0, front=-33.0,
             rear=-52.0, exp=5.0),
        part("prong", mirror=True, x=4.2, hw=0.8, hh=1.5, z=7.0, front=48.5, rear=42.0, exp=4.0,
             spread={"x": 1.5, "hw": 0.8, "hh": 1.5, "z": 3.0, "front": 1.5, "rear": 3.0}),
    ],
    look={
        "strip": {"x": (1.5, 4.5), "y": (-12.0, 40.0)},  # two dark deck stripes (85-90 vs 130-150)
        "strip_on": "deck",
        "lights": "per_pod",  # one red light on the back of each pod (x +-10.6, z ~11)
        "bevel": {"default": 0.45, "prong": 0.15, "fin": 0.3},
    },
)
globals().update(SHAPE.exports())
