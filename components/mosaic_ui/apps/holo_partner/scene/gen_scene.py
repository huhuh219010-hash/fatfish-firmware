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
    button,
    container,
    label,
    layer,
    shared_charset,
    shared_prefix,
    scene_out_path,
    write_scene,
)

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

    # A deliberately original, asset-free placeholder mark.  It keeps this
    # first milestone independent from third-party artwork and licensing.
    objs.append(container(page, 120, 90, 240, 240, bg="#0B1C26",
                          radius=120, border="#4DE8E8", border_w=3))
    objs.append(container(page, 141, 111, 198, 198, bg="#123F50",
                          radius=99, border="#1C7186", border_w=2))
    objs.append(container(page, 158, 128, 164, 164, bg="#28C7C7",
                          radius=82))

    # Minimal friendly face.
    objs.append(container(page, 193, 190, 20, 28, bg="#061116",
                          radius=10))
    objs.append(container(page, 267, 190, 20, 28, bg="#061116",
                          radius=10))
    objs.append(container(page, 211, 249, 58, 10, bg="#061116",
                          radius=5))
    objs.append(container(page, 82, 164, 16, 16, bg="#4DE8E8", radius=8))
    objs.append(container(page, 386, 132, 10, 10, bg="#7A5CFF", radius=5))
    objs.append(container(page, 376, 276, 18, 18, bg="#FF6B9D", radius=9))

    objs.append(label(page, 0, 344, CONTENT_W, 48, "HoloPartner",
                      size=36, color="#FCFCFF", align="center", font=BOLD))
    objs.append(label(page, 0, 394, CONTENT_W, 30,
                      "Hello from your device", size=18,
                      color="#9AB6C4", align="center"))
    objs.append(button(page, 18, 18, 88, 44, "Back", bg="#181819",
                       fg="#FCFCFF", radius=22, size=16,
                       name="holo_back", callback="holo_back",
                       border="#3B3C3D", border_w=1))

    write_scene(scene_out_path(HERE, "holo_partner_480.json"),
                "holo_partner", objs, font=FONT)


if __name__ == "__main__":
    main()
