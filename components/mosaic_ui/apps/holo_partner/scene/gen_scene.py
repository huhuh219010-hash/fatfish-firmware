#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 HoloPartner contributors
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent.parent.parent / "common"))

from font_paths import DEJAVU_SANS, DEJAVU_SANS_BOLD  # noqa: E402
from scene_common import (  # noqa: E402
    CONTENT_H,
    CONTENT_W,
    auto_charset,
    layer,
    shared_charset,
    shared_prefix,
    scene_out_path,
    write_scene,
)
from holo_partner_layout import append_holo_partner_content  # noqa: E402

FONT = DEJAVU_SANS
BOLD = DEJAVU_SANS_BOLD


def main() -> None:
    font_policies = {
        16: shared_charset(),
        18: shared_charset(),
        22: auto_charset(),
        36: auto_charset(),
    }
    objs, content = shared_prefix([], font_policies, BOLD)
    page = len(objs)
    objs.append(layer(content, 0, 0, CONTENT_W, CONTENT_H,
                      name="holo_partner_root"))
    append_holo_partner_content(
        objs, page, bold_font=BOLD, show_back=True,
        probe_callback="holo_probe")

    write_scene(scene_out_path(HERE, "holo_partner_480.json"),
                "holo_partner", objs, font=FONT)


if __name__ == "__main__":
    main()
