# Copyright (C) 2026 Qore Technologies, s.r.o.
# SPDX-License-Identifier: MIT
# Use the pinned source epoch for RPM headers and installed file timestamps.
%global source_date_epoch_from_changelog 1
%global use_source_date_epoch_as_buildtime 1
%if v"%{rpmversion}" >= v"4.20"
%global build_mtime_policy clamp_to_source_date_epoch
%else
%global clamp_mtime_to_source_date_epoch 1
%endif
%bcond_without tests
%bcond_without docs
Name: qore-zip-module
Version: 1.0.0
Release: 1%{?dist}
Summary: ZIP archives, compression and encryption for Qore
License: MIT AND Zlib
URL: https://github.com/qoretechnologies/module-zip
Source0: %{name}-%{version}.tar.xz
# Pinned/repacked with the source-manifest, including the upstream Zlib license.
Source1: minizip-ng-4.2.2.tar.xz
Provides: bundled(minizip-ng) = 4.2.2
%global _find_debuginfo_dwz_opts %{nil}
BuildRequires: cmake >= 3.21
BuildRequires: make
BuildRequires: gcc-c++
BuildRequires: pkgconfig(zlib)
BuildRequires: pkgconfig(liblzma)
BuildRequires: pkgconfig(libzstd)
BuildRequires: pkgconfig(openssl)
BuildRequires: patch
%if 0%{?suse_version}
BuildRequires: libbz2-devel
%else
BuildRequires: bzip2-devel
%endif
%if %{with tests}
BuildRequires: python3 >= 3.11
BuildRequires: unzip
BuildRequires: qore-misc-tools >= 3.0.0~
%endif
BuildRequires: qore-devel >= 3.0.0~
BuildRequires: qore-rpm-macros >= 3.0.0~
%if %{with docs}
BuildRequires: doxygen
%if 0%{?suse_version}
BuildRequires: util-linux
%else
BuildRequires: util-linux-core
%endif
%endif
%{?qore_enable_aot_post}

%description
ZIP reading/writing with all compression and encryption backends, the
ZipDataProvider module and a command-line archive tool.

%if %{with docs}
%package doc
Summary: ZIP module reference documentation
BuildArch: noarch
%description doc
API reference and examples for Qore's ZIP module.
%endif

%prep
%autosetup -a 1
%build
%{?set_build_flags}
. %{_rpmconfigdir}/qore/module-env.sh
qore_set_source_prefix_maps "%{qore_debug_source_dir}"
cmake -S . -B build -G 'Unix Makefiles' \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS_RELEASE=-DNDEBUG \
  -DCMAKE_INSTALL_PREFIX=%{_prefix} -DCMAKE_INSTALL_LIBDIR=%{_lib} \
  -DCMAKE_SKIP_RPATH=ON -DCMAKE_IGNORE_PREFIX_PATH=/usr/local \
  -DQore_DIR=%{_libdir}/cmake/Qore -DQORE_EXECUTABLE=/usr/bin/qore \
  -DQORE_QPP_EXECUTABLE=/usr/bin/qpp -DQORE_QCC_EXECUTABLE=/usr/bin/qcc \
  -DQORE_MINIZIP_SOURCE_DIR:PATH=$PWD/minizip-ng-4.2.2 \
  -DMZ_FETCH_LIBS=OFF -DMZ_FORCE_FETCH_LIBS=OFF -DMZ_ZLIB_FLAVOR=zlib \
  -DFETCHCONTENT_FULLY_DISCONNECTED=ON \
  -DQORE_BUILD_AOT_MODULES=ON -DQORE_AOT_LINK_SOURCE_MODULES=OFF \
  -DQORE_QM_METADATA_ENV:STRING="QORE_MODULE_DIR=$QORE_MODULE_DIR:$PWD/qlib;QORE_MODULE_DIR_ONLY=1;QORE_INCLUDE_DIR=;LD_LIBRARY_PATH=" \
  -DCMAKE_DISABLE_FIND_PACKAGE_Doxygen=%{!?with_docs:ON}%{?with_docs:OFF}
cmake --build build -- %{?_smp_mflags}
%if %{with docs}
cmake --build build --target docs -- %{?_smp_mflags}
%endif
%install
DESTDIR=%{buildroot} cmake --install build
%qore_install_aot_sources qlib
find %{buildroot}%{_libdir}/qore-modules -type f -name '*.qmod' -exec chmod 755 {} +
sed -i '1s|.*|#!/usr/bin/qore|' %{buildroot}%{_bindir}/qzip
install -Dm644 debian/qzip.1 %{buildroot}%{_mandir}/man1/qzip.1
%if %{with docs}
install -d %{buildroot}%{_docdir}/%{name}-doc
cp -a build/docs %{buildroot}%{_docdir}/%{name}-doc/
hardlink -t -O %{buildroot}%{_docdir}/%{name}-doc
%endif
%check
%if %{with tests}
. %{_rpmconfigdir}/qore/module-env.sh
QORE_MINIZIP_SOURCE_DIR="$PWD/minizip-ng-4.2.2" \
  python3 -B -W error -m unittest discover -s test -p test_minizip_memory.py -v
for test in test/*.qtest; do
  timeout 180 /usr/bin/qore -b --enable-debug \
    -l "$PWD/build/zip-api-$(/usr/bin/qore --latest-module-api).qmod" \
    -l "$PWD/build/qlib-qmod/ZipDataProvider/ZipDataProvider.qmod" "$test" -v
done
/usr/bin/qore -b --enable-debug debian/tests/features -v
debian/tests/cli "$PWD/bin/qzip"
qore-data-provider-i18n --no-color --check-source-tree --require-standard-locales \
  --require-complete-locales --output "$PWD/qlib"
%endif
%files
%license COPYING.MIT minizip-ng-4.2.2/LICENSE
%doc README.md
%{_bindir}/qzip
%{_mandir}/man1/qzip.1*
%{_libdir}/qore-modules/zip-api-*.qmod
%{_libdir}/qore-modules/ZipDataProvider/
%{_datadir}/qore-modules/ZipDataProvider/
%dir %{_datadir}/qore/metadata/zip
%{_datadir}/qore/metadata/zip/*.meta.json
%{_datadir}/qore/i18n/
%if %{with docs}
%files doc
%license COPYING.MIT
%doc %{_docdir}/%{name}-doc/
%endif
%changelog
* Thu Oct 01 2026 David Nichols <david@qore.org> - 1.0.0-1
- Package all ZIP features with offline sources and preserve AOT metadata.
