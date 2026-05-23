cmake_minimum_required(VERSION 3.5.0)

project(qore-zip-module)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)

set(PROJECT_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.12.0")
    cmake_policy(SET CMP0074 NEW)

    if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.18.0")
        cmake_policy(SET CMP0053 NEW)
    endif()
endif()

cmake_policy(SET CMP0042 NEW)

if (UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

find_package(Qore 2.0 REQUIRED)

# Find zlib (required by minizip-ng)
find_package(ZLIB REQUIRED)

# Find optional compression libraries
find_package(BZip2)
find_package(LibLZMA)
find_package(zstd)
find_package(OpenSSL)

# Check for C++11.
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
    message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

# minizip-ng configuration - build with PIC for shared library use
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(MZ_COMPAT OFF CACHE BOOL "Enable compatibility layer" FORCE)
set(MZ_ZLIB ON CACHE BOOL "Enable zlib compression" FORCE)
set(MZ_BZIP2 ${BZIP2_FOUND} CACHE BOOL "Enable BZip2 compression" FORCE)
set(MZ_LZMA ${LIBLZMA_FOUND} CACHE BOOL "Enable LZMA compression" FORCE)
set(MZ_ZSTD ${zstd_FOUND} CACHE BOOL "Enable Zstandard compression" FORCE)
set(MZ_PKCRYPT ON CACHE BOOL "Enable PKWARE traditional encryption" FORCE)
set(MZ_WZAES ${OPENSSL_FOUND} CACHE BOOL "Enable WinZip AES encryption" FORCE)
set(MZ_OPENSSL ${OPENSSL_FOUND} CACHE BOOL "Use OpenSSL for AES" FORCE)
set(MZ_SIGNING OFF CACHE BOOL "Disable signing" FORCE)
set(MZ_COMPRESS_ONLY OFF CACHE BOOL "Build both compression and decompression" FORCE)
set(MZ_DECOMPRESS_ONLY OFF CACHE BOOL "Build both compression and decompression" FORCE)
set(MZ_BUILD_TESTS OFF CACHE BOOL "Disable minizip-ng tests" FORCE)
set(MZ_BUILD_UNIT_TESTS OFF CACHE BOOL "Disable minizip-ng unit tests" FORCE)
set(MZ_BUILD_FUZZ_TESTS OFF CACHE BOOL "Disable minizip-ng fuzz tests" FORCE)
# Pin sanitizer to OFF: minizip-ng v4.2 only accepts OFF/memory/address/undefined/thread
# (the earlier "AUTO" value was removed), and we don't want submodule sanitizer state to
# leak into the module build.
set(MZ_SANITIZER OFF CACHE STRING "Disable minizip-ng sanitizers" FORCE)

# minizip-ng is consumed as a git submodule, but modern OpenSSL (3.x built without
# the legacy ENGINE API - e.g. Fedora's OpenSSL 3.5, where <openssl/engine.h> no
# longer exists) needs a small compatibility patch that upstream has not adopted
# (as of 4.2.1 and current develop, mz_crypt_openssl.c still includes engine.h
# unconditionally). Patching the submodule working tree in place leaves it dirty,
# which breaks `git pull --rebase` and `git submodule update` in persistent
# checkouts. Instead, copy the submodule into the build tree and patch the COPY,
# leaving the checked-out submodule pristine.
set(MINIZIP_SRC_DIR "${CMAKE_BINARY_DIR}/minizip-ng")
file(COPY "${CMAKE_SOURCE_DIR}/src/minizip-ng" DESTINATION "${CMAKE_BINARY_DIR}"
     PATTERN ".git" EXCLUDE)

# Apply patches to the build-tree copy of minizip-ng before building.
# OpenSSL 3.2+ removed the ENGINE API - apply compatibility patch.
if(EXISTS "${CMAKE_SOURCE_DIR}/patches/openssl-3.2-fix.patch")
    # Dry-run first: patch --dry-run returns 0 when the patch would apply, 1 when
    # already applied, >1 on error. Only invoke the real apply on a 0, otherwise
    # patch writes a .rej alongside the source on every configure. (file(COPY)
    # refreshes the copy each configure, so "already applied" is normally only
    # seen when the destination file was not re-copied.)
    execute_process(
        COMMAND patch -p1 --dry-run --forward --silent -i "${CMAKE_SOURCE_DIR}/patches/openssl-3.2-fix.patch"
        WORKING_DIRECTORY "${MINIZIP_SRC_DIR}"
        RESULT_VARIABLE PATCH_DRY_RESULT
        OUTPUT_QUIET
        ERROR_QUIET
    )
    if(PATCH_DRY_RESULT EQUAL 0)
        execute_process(
            COMMAND patch -p1 --forward -i "${CMAKE_SOURCE_DIR}/patches/openssl-3.2-fix.patch"
            WORKING_DIRECTORY "${MINIZIP_SRC_DIR}"
            RESULT_VARIABLE PATCH_RESULT
            OUTPUT_QUIET
            ERROR_QUIET
        )
        if(NOT PATCH_RESULT EQUAL 0)
            message(WARNING "Failed to apply OpenSSL 3.2 compatibility patch (patch exit code: ${PATCH_RESULT})")
        endif()
    elseif(PATCH_DRY_RESULT GREATER 1)
        message(WARNING "OpenSSL 3.2 compatibility patch dry-run failed (exit code: ${PATCH_DRY_RESULT})")
    endif()
endif()

# Add minizip-ng subdirectory (the patched build-tree copy, never the submodule)
add_subdirectory("${MINIZIP_SRC_DIR}" "${CMAKE_BINARY_DIR}/minizip-ng-build")

# Some distros (e.g. Debian/Ubuntu aarch64) ship libzstd.a without -fPIC, but
# minizip-ng links zstd::libzstd_static when built as a static archive. Linking
# that non-PIC .a into our shared .qmod fails with R_AARCH64_ADR_PREL_PG_HI21.
# Swap to the shared zstd target — every other compression dep is shared anyway.
# INTERFACE_LINK_LIBRARIES wraps each dep in $<LINK_ONLY:...>, so substring
# replace (rather than list TRANSFORM) is what reaches inside the genex.
if(zstd_FOUND AND TARGET zstd::libzstd_shared AND TARGET minizip-ng)
    foreach(_mz_prop LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
        get_target_property(_mz_val minizip-ng ${_mz_prop})
        if(_mz_val)
            string(REPLACE "zstd::libzstd_static" "zstd::libzstd_shared"
                   _mz_val "${_mz_val}")
            set_target_properties(minizip-ng PROPERTIES ${_mz_prop} "${_mz_val}")
        endif()
    endforeach()
endif()

set(QPP_SRC
    src/QC_ZipFile.qpp
    src/QC_ZipEntry.qpp
    src/QC_ZipInputStream.qpp
    src/QC_ZipOutputStream.qpp
)

set(CPP_SRC
    src/zip-module.cpp
    src/QoreZipFile.cpp
    src/ZipInputStream.cpp
    src/ZipOutputStream.cpp
)

qore_wrap_qpp_value(QPP_SOURCES METALIST QPP_META_SRC ${QPP_SRC})

foreach (it ${QPP_SOURCES})
    GET_FILENAME_COMPONENT(_outfile ${it} NAME_WE)
    set(QPP_DOX ${QPP_DOX} ${CMAKE_CURRENT_BINARY_DIR}/${_outfile}.dox.h)
endforeach()

set(module_name "zip")

set(QMOD
    qlib/ZipDataProvider
)

set(QORE_DOX_TMPL_SRC
  docs/mainpage.dox.tmpl
)

add_library(${module_name} MODULE ${CPP_SRC} ${QPP_SOURCES})

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${MINIZIP_SRC_DIR})
include_directories(${ZLIB_INCLUDE_DIRS})
target_include_directories(${module_name} PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>)

add_custom_target(QORE_INC_FILES DEPENDS ${QORE_INC_SRC})
add_dependencies(${module_name} QORE_INC_FILES)

target_link_libraries(${module_name} minizip-ng ${QORE_LIBRARY})

set(MODULE_DOX_INPUT ${CMAKE_CURRENT_BINARY_DIR}/mainpage.dox ${QPP_DOX})
string(REPLACE ";" " " MODULE_DOX_INPUT "${MODULE_DOX_INPUT}")

if (DEFINED ENV{DOXYGEN_EXECUTABLE})
    set(DOXYGEN_EXECUTABLE $ENV{DOXYGEN_EXECUTABLE})
endif()

set(DOXYGEN_IMAGE_PATH "${CMAKE_SOURCE_DIR}/docs/images" CACHE STRING "Doxygen image path" FORCE)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/images")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/doxygen/qlib/ZipDataProvider")
file(WRITE
    "${CMAKE_BINARY_DIR}/doxygen/qlib/ZipDataProvider/zip-logo.svg.dox.h"
    "/** @file zip-logo.svg.dox.h\n    @brief ZIP DataProvider logo file.\n*/\n"
)
set(MODULE_DOX_INPUT ${CMAKE_BINARY_DIR})
qore_external_binary_module(${module_name} ${PROJECT_VERSION})

qore_dist(${PROJECT_VERSION})

qore_user_modules("${QMOD}")
qore_external_user_module("qlib/ZipDataProvider" "")

# install qpp metadata files
if (COMMAND qore_install_qpp_metadata)
    qore_install_qpp_metadata(${module_name} ${QPP_META_SRC})
else()
    install(FILES ${QPP_META_SRC}
            DESTINATION ${CMAKE_INSTALL_DATADIR}/qore/metadata)
endif()

# install CLI tool
install(PROGRAMS bin/qzip DESTINATION ${CMAKE_INSTALL_BINDIR})

qore_config_info()

if (DOXYGEN_FOUND)
    qore_wrap_dox(QORE_DOX_SRC ${QORE_DOX_TMPL_SRC})
    add_custom_target(QORE_MOD_DOX_FILES DEPENDS ${QORE_DOX_SRC})
    add_dependencies(docs-module QORE_MOD_DOX_FILES)
endif()
