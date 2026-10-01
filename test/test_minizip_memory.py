#!/usr/bin/env python3
"""Test the patched private minizip source through its public API.

Copyright (C) 2026 Qore Technologies, s.r.o.
"""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


@unittest.skipUnless(sys.platform.startswith("linux"), "GNU linker allocation injection")
class MinizipMemoryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="minizip-memory-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.root = Path(cls.temporary.name)
        source = Path(os.environ.get("QORE_MINIZIP_SOURCE_DIR", ROOT / "src/minizip-ng"))
        patched = cls.root / "source"
        shutil.copytree(source, patched, ignore=shutil.ignore_patterns(".git"))
        subprocess.run(["patch", "-p1", "--batch", "--forward", "-i",
                        str(ROOT / "patches/minizip-memory-bounds.patch")],
                       cwd=patched, check=True, capture_output=True)
        build = cls.root / "build-debug"
        # Generate minizip's platform configuration using its own probes.
        # Compression/encryption are exercised by the module integration suite;
        # these tests isolate memory ownership and bounds without those libraries.
        options = ["COMPAT", "ZLIB", "BZIP2", "LZMA", "ZSTD", "PPMD", "LIBCOMP", "PKCRYPT",
                   "WZAES", "OPENSSL", "LIBBSD", "FETCH_LIBS", "BUILD_TESTS",
                   "BUILD_UNIT_TESTS", "BUILD_FUZZ_TESTS"]
        (cls.root / "CMakeLists.txt").write_text('''cmake_minimum_required(VERSION 3.21)
project(MinizipMemoryTests LANGUAGES C)
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(source vendor)
target_compile_options(minizip-ng PRIVATE -Wall -Wextra -Werror)
add_executable(probe "${PROBE_SOURCE}")
target_compile_options(probe PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_link_options(probe PRIVATE -Wl,--wrap=malloc)
target_link_libraries(probe PRIVATE minizip-ng)
''')
        subprocess.run(["cmake", "-S", str(cls.root), "-B", str(build),
                        "-DCMAKE_BUILD_TYPE=Debug", "-DMZ_SANITIZER=OFF",
                        "-DPROBE_SOURCE=" + str(ROOT / "test/minizip-memory-probe.c"),
                        *["-DMZ_" + option + "=OFF" for option in options]],
                       check=True)
        subprocess.run(["cmake", "--build", str(build), "--parallel", "2"], check=True)
        cls.probe = build / "probe"

    def check_case(self, case):
        command = [str(self.probe), case]
        if os.environ.get("QORE_TEST_VALGRIND") == "1":
            command = ["valgrind", "--quiet", "--leak-check=full",
                       "--show-leak-kinds=all", "--errors-for-leak-kinds=all",
                       "--error-exitcode=99", *command]
        result = subprocess.run(command, capture_output=True, text=True, timeout=60)
        self.assertEqual(0, result.returncode, result.stdout + result.stderr)
        self.assertEqual("", result.stderr)
        self.assertEqual("PASS\n", result.stdout)

    def test_comment_replacement(self):
        self.check_case("comment")

    def test_reopen_with_smaller_buffer(self):
        self.check_case("reopen")

    def test_failed_growth_preserves_data(self):
        self.check_case("growth")

    def test_seek_boundaries(self):
        self.check_case("seek")

    def test_fixed_buffer_and_invalid_sizes(self):
        self.check_case("fixed-buffer")


if __name__ == "__main__":
    unittest.main()
