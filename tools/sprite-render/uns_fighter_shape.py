# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Human (UNS) fighter: twin-pod shape (twin_pod.py) with stations, bounds and look measured
from the original frames (carve.py: tilt 44, distance 200).

A short forward body with a dark cockpit, two rear engine pods with one red light each on the
back, and a dark crossbar between the pods. Overlay covers the whole ship, lights included.
"""

from twin_pod import TwinPod

SHAPE = TwinPod(
    stations=[-4.0, -2.0, 0.0, 2.0, 4.0, 6.0, 8.0],
    params={
        **{f"rx{k}": (v, 0.6, 5.0) for k, v in enumerate([3.0, 2.5, 2.6, 2.6, 3.0, 3.2, 2.4])},
        **{f"rz{k}": (v, 0.6, 5.0) for k, v in enumerate([2.4, 2.4, 2.5, 2.5, 2.6, 2.5, 2.0])},
        "zc0": (-2.5, -5.0, 0.5), "zslope": (0.0, -0.2, 0.2), "hull_exp": (2.2, 1.6, 4.0),
        "hull_bot": (1.0, 0.5, 1.8), "nose_y": (10.2, 9.0, 13.5),
        "pod_x": (4.6, 3.5, 6.0), "pod_hw": (1.4, 0.8, 2.5), "pod_hh": (2.5, 0.8, 4.5),
        "pod_z": (-2.0, -5.0, 0.5), "pod_front": (-1.5, -4.0, 0.0), "pod_rear": (-10.3, -11.5, -8.5),
        "pod_exp": (4.0, 2.2, 10.0),
        "blk_hw": (3.0, 1.0, 4.5), "blk_hh": (1.5, 0.6, 3.5), "blk_z": (-2.0, -5.0, 0.5),
        "blk_front": (-1.0, -3.5, 1.0), "blk_rear": (-5.0, -7.0, -3.0), "blk_exp": (4.0, 2.2, 10.0),
    },
    look={
        "cockpit": {"hw": 1.2, "y": (5.0, 9.3)},  # dark on the centreline (33-68 vs 130-160)
        "lights": "per_pod",  # one red light on the back of each pod (x +-4.5, z -2)
        "bevel": {"hull": 0.15, "pod": 0.12, "block": 0.1},
    },
)
globals().update(SHAPE.exports())
