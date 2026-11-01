#!/usr/bin/env python3
"""Exercise Fatfish NVS migrations with injected write failures."""
import os
from pathlib import Path
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
HOST = ROOT / "third-party/esp-claw/components/claw_modules/claw_endpoint/test_host/include"

with tempfile.TemporaryDirectory(prefix="endpoint-config-test-") as tmp:
    binary = Path(tmp) / "test_config"
    cmd = [os.environ.get("CC", "cc"), "-std=gnu11", "-g", "-Wall", "-Wextra",
           "-Wno-unused-parameter", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
           "-DCONFIG_APP_WIFI_SSID=\"\"", "-DCONFIG_APP_WIFI_PASSWORD=\"\"",
           "-DCONFIG_APP_SEARCH_HTTP_ALLOWLIST=\"\"", "-DTEST_MOSAICO",
           "-include", str(HOST / "host_compat.h")]
    cmd += ["-I" + str(p) for p in [HERE / "include", HOST, ROOT / "components/app_config/include"]]
    cmd += [str(HERE / "test_config.c"), str(ROOT / "components/app_config/app_config.c"), "-o", str(binary)]
    subprocess.run(cmd, check=True)
    subprocess.run([str(binary)], check=True)
