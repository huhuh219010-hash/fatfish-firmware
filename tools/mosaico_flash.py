#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 HoloPartner contributors
# SPDX-License-Identifier: Apache-2.0
"""Enter ESP-Mosaico ROM download mode and flash selected build partitions."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import subprocess
import sys
import time

import serial
from serial.tools import list_ports


ROOT = Path(__file__).resolve().parents[1]
ESPRESSIF_VID = 0x303A
USB_SERIAL_JTAG_PID = 0x1001
ROM_DOWNLOAD_PID = 0x0020
SUPPORTED_PIDS = {USB_SERIAL_JTAG_PID, ROM_DOWNLOAD_PID}
BOOT_COMMAND = b"\x1bMOSAICO_BOOT\n"
BOOT_TOUCH_BAUD = 1200
DEFAULT_PARTITIONS = ("app", "ui_apps")


def usb_ports():
    return [
        port for port in list_ports.comports()
        if port.vid == ESPRESSIF_VID and port.pid in SUPPORTED_PIDS
    ]


def is_app_port(port) -> bool:
    product = (port.product or "").lower()
    return "mosaico" in product or "mosaico" in (port.description or "").lower()


def find_port(device: str | None):
    ports = usb_ports()
    if device:
        for port in ports:
            if port.device == device:
                return port
        raise RuntimeError(f"ESP-Mosaico serial port not found: {device}")
    app_ports = [port for port in ports if is_app_port(port)]
    if len(app_ports) == 1:
        return app_ports[0]
    if len(ports) == 1:
        return ports[0]
    if not ports:
        raise RuntimeError("no Espressif USB serial port found")
    names = ", ".join(port.device for port in ports)
    raise RuntimeError(f"multiple Espressif USB serial ports found; use --port: {names}")


def request_rom_boot(port) -> None:
    print(f"Requesting ROM download mode through {port.device}...")
    # Configure the modem-control lines before opening the device.  Keep DTR
    # asserted so macOS will actually deliver CDC data, while RTS stays clear.
    # The firmware treats (DTR=1, RTS=0) as a download-mode latch only; it does
    # not reset until the explicit BOOT command is received.
    device = serial.Serial(port=None, baudrate=115200, timeout=0.2)
    device.dtr = True
    device.rts = False
    device.port = port.device
    device.open()
    try:
        device.write(BOOT_COMMAND)
        device.flush()
        # Also issue the CDC 1200-baud touch supported by current firmware.
        # Sending both requests is intentional: the text command works around
        # hosts that do not reliably deliver line-coding changes, while the
        # baud touch still works if another console reader consumed the text.
        try:
            device.baudrate = BOOT_TOUCH_BAUD
        except (OSError, serial.SerialException):
            # A successful BOOT command may disconnect USB before pyserial can
            # apply the fallback line coding.
            pass
        time.sleep(0.1)

        # Finally emulate the USB-Serial/JTAG modem-control reset sequence.
        # The firmware latches download mode at (RTS=0, DTR=1) and performs
        # the reset when the lines reach (RTS=1, DTR=0).
        try:
            device.rts = False
            device.dtr = False
            time.sleep(0.1)
            device.dtr = True
            time.sleep(0.1)
            device.rts = True
            time.sleep(0.1)
            device.dtr = False
            time.sleep(0.1)
            device.rts = False
        except (OSError, serial.SerialException):
            # One of the earlier triggers may already be re-enumerating USB.
            pass
        time.sleep(0.25)
    finally:
        try:
            device.close()
        except (OSError, serial.SerialException):
            # Disconnecting here means the device is already rebooting.
            pass


def wait_for_rom_port(app_device: str, timeout: float):
    deadline = time.monotonic() + timeout
    saw_app_disconnect = False
    while time.monotonic() < deadline:
        ports = usb_ports()
        if not any(port.device == app_device for port in ports):
            saw_app_disconnect = True
        candidates = [
            port for port in ports
            if not is_app_port(port) and
            (port.device != app_device or saw_app_disconnect)
        ]
        if len(candidates) == 1:
            print(f"ROM download port: {candidates[0].device}")
            return candidates[0]
        time.sleep(0.1)
    raise RuntimeError(
        "ROM download port did not appear after the serial BOOT command, "
        "1200-baud trigger, and DTR/RTS reset; enter BOOT manually once and "
        "rerun with its port."
    )


def load_images(build_dir: Path, names: list[str]):
    manifest_path = build_dir / "update_flasher_args.json"
    if not manifest_path.is_file():
        raise RuntimeError(f"build manifest not found: {manifest_path}")
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    images = []
    for name in names:
        entry = manifest.get(name)
        if not isinstance(entry, dict) or "offset" not in entry or "file" not in entry:
            raise RuntimeError(f"partition {name!r} is missing from {manifest_path}")
        image = build_dir / entry["file"]
        if not image.is_file():
            raise RuntimeError(f"partition image not found: {image}")
        images.extend((entry["offset"], str(image)))
    return images


def flash(port: str, baud: int, images: list[str]) -> None:
    command = [
        sys.executable, "-m", "esptool",
        "--port", port,
        "--chip", "esp32s31",
        "--baud", str(baud),
        "--before", "no-reset",
        "--after", "hard-reset",
        "--no-stub",
        "write-flash",
        "--flash-mode", "dio",
        "--flash-size", "16MB",
        "--flash-freq", "80m",
        *images,
    ]
    print("Flashing selected partitions...")
    subprocess.run(command, check=True)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", help="App or ROM serial port; auto-detected by default")
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build")
    parser.add_argument("--partitions", nargs="+", default=list(DEFAULT_PARTITIONS),
                        help="partition names from update_flasher_args.json")
    parser.add_argument("--baud", type=int, default=460800)
    parser.add_argument("--wait", type=float, default=12.0,
                        help="seconds to wait for the ROM port")
    args = parser.parse_args()

    try:
        build_dir = args.build_dir.resolve()
        images = load_images(build_dir, args.partitions)
        selected = find_port(args.port)
        if is_app_port(selected):
            request_rom_boot(selected)
            selected = wait_for_rom_port(selected.device, args.wait)
        else:
            print(f"Using existing ROM download port: {selected.device}")
        flash(selected.device, args.baud, images)
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"mosaico_flash: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
