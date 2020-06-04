#!/usr/bin/env python
from __future__ import print_function
import os
import sys

if not os.getenv("CI_COMMIT_SHORT_SHA"):
    sys.exit(0)
with open(
    os.path.join(
        os.path.dirname(os.path.dirname(os.path.realpath(__file__))), "ELECTRON_VERSION"
    ),
    "r+",
) as f:
    vers = f.read()
    if "-" in vers:
        new_vers = vers.split("-")[0] + "-" + os.getenv("CI_COMMIT_SHORT_SHA")
        f.seek(0)
        f.write(new_vers)
        f.flush()
        f.truncate()
