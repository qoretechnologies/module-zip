# Copyright (C) 2026 Qore Technologies, s.r.o.
# SPDX-License-Identifier: MIT
# Prefer upstream's config; some distributions only ship libzstd.pc.
find_package(zstd QUIET CONFIG)
if(NOT zstd_FOUND)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(QORE_ZSTD QUIET IMPORTED_TARGET libzstd)
        if(QORE_ZSTD_FOUND)
            add_library(zstd::libzstd_shared ALIAS PkgConfig::QORE_ZSTD)
            set(zstd_FOUND TRUE)
            set(zstd_VERSION "${QORE_ZSTD_VERSION}")
        endif()
    endif()
endif()
