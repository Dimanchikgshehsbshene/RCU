#!/usr/bin/env python3
"""
Atmosphere's stratosphere loader uses system_module.mk with PORTLIBS (LIBDIRS)
listed before AMS_LIBDIRS. devkitPro's switch-portlibs ships an older
stratosphere/ldr/ldr_types.hpp, which shadows the in-tree headers from
libraries/libstratosphere and breaks loader builds (missing NsoHeader flags,
Acid flags, DecompressZstdForLoader, etc.).

This script swaps the two -I blocks so AMS_LIBDIRS wins. Safe to run
repeatedly (no-op if already fixed).
"""
from __future__ import annotations

import pathlib
import sys


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: fix_atmosphere_loader_includes.py <path/to/system_module.mk>", file=sys.stderr)
        return 2

    path = pathlib.Path(sys.argv[1])
    if not path.is_file():
        print(f"skip: not a file: {path}", file=sys.stderr)
        return 0

    text = path.read_text(encoding="utf-8", errors="replace")
    # Already AMS-first (Horizon OC / manual fix).
    needle = "$(foreach dir,$(AMS_LIBDIRS),-I$(dir)/include)"
    if text.find(needle) != -1 and text.find("$(foreach dir,$(LIBDIRS),-I$(dir)/include)") != -1:
        ams_pos = text.find(needle)
        lib_pos = text.find("$(foreach dir,$(LIBDIRS),-I$(dir)/include)")
        if ams_pos < lib_pos:
            print(f"ok: already AMS-before-LIBDIRS: {path}")
            return 0

    old = (
        "export INCLUDE\t:=\t$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \\\n"
        "\t\t\t$(foreach dir,$(LIBDIRS),-I$(dir)/include) \\\n"
        "\t\t\t$(foreach dir,$(AMS_LIBDIRS),-I$(dir)/include) \\\n"
        "\t\t\t-I$(CURDIR)/$(ATMOSPHERE_BUILD_DIR)"
    )
    new = (
        "export INCLUDE\t:=\t$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \\\n"
        "\t\t\t$(foreach dir,$(AMS_LIBDIRS),-I$(dir)/include) \\\n"
        "\t\t\t$(foreach dir,$(LIBDIRS),-I$(dir)/include) \\\n"
        "\t\t\t-I$(CURDIR)/$(ATMOSPHERE_BUILD_DIR)"
    )
    if old not in text:
        print(f"error: expected INCLUDE stanza not found in {path}", file=sys.stderr)
        return 1

    path.write_text(text.replace(old, new, 1), encoding="utf-8")
    print(f"patched: {path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
