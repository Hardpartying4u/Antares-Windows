#!/usr/bin/env python3
# Copyright (C) 2026 The Star Bound Authors
# This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
"""Compiles a Windows resource script for the build: win_rc.py RC_FILE OUT_RES.

Runs the Windows SDK's rc.exe (on PATH in a vcvars64 environment). Relative paths in the script
are found next to it."""

import os
import subprocess
import sys


def main():
    rc_file, out_res = sys.argv[1], sys.argv[2]
    include = os.path.dirname(os.path.abspath(rc_file))
    sys.exit(subprocess.call(["rc.exe", "/nologo", "/I", include, "/fo", out_res, rc_file]))


if __name__ == "__main__":
    main()
