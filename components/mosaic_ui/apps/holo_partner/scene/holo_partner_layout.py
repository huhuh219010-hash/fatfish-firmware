# SPDX-FileCopyrightText: 2026 HoloPartner contributors
# SPDX-License-Identifier: Apache-2.0
"""Reusable HoloPartner placeholder layout for the App and Hub home."""

from scene_common import CONTENT_H, CONTENT_W, button, container, label


def append_holo_partner_content(objs, parent, *, bold_font, show_back,
                                probe_callback):
    """Append the asset-free HoloPartner surface to ``parent``."""
    # An opaque surface lets Hub retain its existing Home objects underneath
    # during the first integration milestone.  Their runtime bindings continue
    # to exist while HoloPartner becomes the visible Home content.
    objs.append(container(parent, 0, 0, CONTENT_W, CONTENT_H,
                          bg="#000000", name="holo_home_background"))

    # Deliberately original geometry: no third-party artwork is used here.
    objs.append(container(parent, 120, 90, 240, 240, bg="#0B1C26",
                          radius=120, border="#4DE8E8", border_w=3))
    objs.append(container(parent, 141, 111, 198, 198, bg="#123F50",
                          radius=99, border="#1C7186", border_w=2))
    objs.append(container(parent, 158, 128, 164, 164, bg="#28C7C7",
                          radius=82))

    # Minimal friendly face.
    objs.append(container(parent, 193, 190, 20, 28, bg="#061116",
                          radius=10))
    objs.append(container(parent, 267, 190, 20, 28, bg="#061116",
                          radius=10))
    objs.append(container(parent, 211, 249, 58, 10, bg="#061116",
                          radius=5))
    objs.append(container(parent, 82, 164, 16, 16, bg="#4DE8E8", radius=8))
    objs.append(container(parent, 386, 132, 10, 10, bg="#7A5CFF", radius=5))
    objs.append(container(parent, 376, 276, 18, 18, bg="#FF6B9D", radius=9))

    objs.append(label(parent, 0, 344, CONTENT_W, 48, "HoloPartner",
                      size=36, color="#FCFCFF", align="center",
                      font=bold_font))
    objs.append(label(
        parent, 0, 394, CONTENT_W, 30,
        "Tap the face to check camera", size=18,
        color="#9AB6C4", align="center",
        name="holo_probe_status", bind="holo_probe_status",
        font_charset=(
            " .:0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
        ),
    ))

    # Transparent hit target sits above the face artwork without changing it.
    objs.append(button(parent, 120, 90, 240, 240, "",
                       bg="#000000", radius=120, opacity=0,
                       name="holo_probe", callback=probe_callback))

    if show_back:
        objs.append(button(parent, 18, 18, 88, 44, "Back", bg="#181819",
                           fg="#FCFCFF", radius=22, size=16,
                           name="holo_back", callback="holo_back",
                           border="#3B3C3D", border_w=1))
