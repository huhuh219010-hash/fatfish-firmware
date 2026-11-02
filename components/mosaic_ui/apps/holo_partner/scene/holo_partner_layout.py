# SPDX-FileCopyrightText: 2026 HoloPartner contributors
# SPDX-License-Identifier: Apache-2.0
"""Reusable Fatfish surface for the App and Hub home."""

from scene_common import CONTENT_H, CONTENT_W, button, container, label


def append_holo_partner_content(objs, parent, *, bold_font, show_back,
                                probe_callback, idle_asset, animated,
                                look_around_asset=None):
    """Append the first Fatfish state: a continuously looping idle pose."""
    # An opaque surface lets Hub retain its existing Home objects underneath
    # during the first integration milestone.  Their runtime bindings continue
    # to exist while HoloPartner becomes the visible Home content.
    # Match the RGB565/JPEG-quantized matte baked into the board animation.
    objs.append(container(parent, 0, 0, CONTENT_W, CONTENT_H,
                          bg="#001018", name="holo_home_background"))

    # The board keeps breathing and looking-around as separate players. This
    # lets native code choose a random idle interval without cutting an action
    # short. Transparent outer margins are cropped once while character pixels
    # remain at source scale.
    idle = {
        "type": "image", "parent": parent,
        "x": 112, "y": 16, "w": 256, "h": 282,
        "image": idle_asset, "fit": "contain",
        "name": "fatfish_idle_breathing",
        "hidden": animated,
    }
    if animated:
        idle.update(animation_codec="hardware_jpeg", quality=74, max_fps=3)
    objs.append(idle)

    if animated and look_around_asset:
        objs.append({
            "type": "image", "parent": parent,
            "x": 112, "y": 16, "w": 256, "h": 282,
            "image": look_around_asset, "fit": "contain",
            "name": "fatfish_idle_look_around", "hidden": True,
            "animation_codec": "hardware_jpeg", "quality": 74,
            "max_fps": 3,
        })

    # Keep the first milestone's camera bind/action ABI available while the
    # visible surface moves to the behavior state machine. The compatibility
    # controls are hidden and cannot intercept input.
    objs.append(label(
        parent, 0, 0, 1, 1,
        "Tap the face to check camera", size=18,
        color="#9AB6C4", align="center",
        name="holo_probe_status", bind="holo_probe_status",
        font_charset=(
            " .:0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
        ),
        hidden=True,
    ))
    probe = button(parent, 0, 0, 1, 1, "", bg="#000000", opacity=0,
                   name="holo_probe", callback=probe_callback)
    probe["hidden"] = True
    objs.append(probe)

    if show_back:
        objs.append(button(parent, 18, 18, 88, 44, "Back", bg="#181819",
                           fg="#FCFCFF", radius=22, size=16,
                           name="holo_back", callback="holo_back",
                           border="#3B3C3D", border_w=1))
