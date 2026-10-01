#!/usr/bin/python3
# Copyright (C) 2026 Qore Technologies, s.r.o.
# SPDX-License-Identifier: MIT
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

class ZstdDiscoveryTests(unittest.TestCase):
    def configure(self, arguments, expected, config=None):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            if config:
                (root / "zstdConfig.cmake").write_text(config)
                arguments = [*arguments, "-Dzstd_DIR=" + directory]
            (root / "CMakeLists.txt").write_text(
                'cmake_minimum_required(VERSION 3.21)\nproject(ZstdDiscovery NONE)\n'
                + f'include("{ROOT}/cmake/QoreFindZstd.cmake")\n'
                + f'if(NOT "${{zstd_FOUND}}" STREQUAL "{expected}")\n'
                + 'message(FATAL_ERROR "unexpected zstd discovery result")\nendif()\n'
                + ('if(NOT TARGET zstd::libzstd_shared)\nmessage(FATAL_ERROR "missing target")\nendif()\n'
                   if expected == "1" or expected == "TRUE" else ''))
            result = subprocess.run(["cmake", "-S", directory, "-B", str(root / "build"),
                                     *arguments], text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertEqual(result.stderr, "")

    def test_config_preferred_without_pkg_config(self):
        self.configure([], "1",
                       'set(CMAKE_DISABLE_FIND_PACKAGE_PkgConfig ON)\n'
                       'add_library(zstd::libzstd_shared INTERFACE IMPORTED)\n')

    def test_pkg_config_only_distribution(self):
        subprocess.run(["pkg-config", "--exists", "libzstd"], check=True)
        self.configure(["-DCMAKE_DISABLE_FIND_PACKAGE_zstd=ON"], "TRUE")

    def test_unavailable_optional_dependency(self):
        self.configure(["-DCMAKE_DISABLE_FIND_PACKAGE_zstd=ON",
                        "-DCMAKE_DISABLE_FIND_PACKAGE_PkgConfig=ON"], "")

if __name__ == "__main__":
    unittest.main()
