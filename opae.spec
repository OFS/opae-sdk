Summary:        Open Programmable Acceleration Engine (OPAE) SDK
Name:           opae
Version:        1.4.1
Release:        1%{?dist}
License:        BSD
ExclusiveArch:  x86_64

Group:          Development/Libraries
Vendor:         Intel Corporation
Requires:       uuid, json-c, python
URL:            https://github.com/OPAE/%{name}-sdk
Source0:        https://github.com/OPAE/opae-sdk/releases/download/%{version}-1/%{name}-%{version}-1.tar.gz

BuildRequires:  gcc, gcc-c++
BuildRequires:  cmake
BuildRequires:  git
BuildRequires:  python3-devel
BuildRequires:  json-c-devel
BuildRequires:  libuuid-devel
BuildRequires:  rpm-build
BuildRequires:  hwloc-devel
BuildRequires:  python3-sphinx
BuildRequires:  doxygen
BuildRequires:  systemd-rpm-macros
BuildRequires:  systemd
BuildRequires:  python3-pip
BuildRequires:  python3-requests
BuildRequires:  python3-docutils
BuildRequires:  python3-breathe
BuildRequires:  python3-pygments
BuildRequires:  fontawesome-fonts
BuildRequires:  fontawesome-fonts-web
BuildRequires:  python3-sphinx_rtd_theme
BuildRequires:  python3-recommonmark

%description
Open Programmable Acceleration Engine (OPAE) is a software framework
for managing and accessing programmable accelerators (FPGAs).
Its main parts are:

* OPAE Software Development Kit (OPAE SDK) (this package)
* OPAE Linux driver for Intel(R) Xeon(R) CPU with
  Integrated FPGAs and Intel(R) PAC with Arria(R) 10 GX FPGA
* Basic Building Block (BBB) library for accelerating AFU

OPAE SDK is a collection of libraries and tools to facilitate the
development of software applications and accelerators using OPAE.
It provides a library implementing the OPAE C API for presenting a
streamlined and easy-to-use interface for software applications to
discover, access, and manage FPGA devices and accelerators using
the OPAE software stack.

%package devel
Summary:    OPAE headers, sample source, and documentation
Requires:   libuuid-devel, %{name}%{?_isa} = %{version}-%{release}

%description devel
OPAE headers, tools, sample source, and documentation




%prep
%setup -q -n %{name}-%{version}-1

%build
rm -rf _build
mkdir _build
cd _build

%cmake .. -DCMAKE_INSTALL_PREFIX=/usr  -DOPAE_BUILD_SPHINX_DOC=ON -DOPAE_BUILD_TESTS=ON -DOPAE_TEST_TAG=release/1.4.1  -DOPAE_PRESERVE_REPOS=ON

%make_build  opae-c \
         bitstream \
         xfpga \
         modbmc \
         opae-cxx-core \
         hello_cxxcore \
         board_rc \
         board_vc \
         fpgaconf \
         fpgainfo \
         userclk \
         hello_fpga \
         mmlink 

make docs
make manpages

%install
mkdir -p %{buildroot}%{_datadir}/opae
cp RELEASE_NOTES.md %{buildroot}%{_datadir}/opae/RELEASE_NOTES.md
cp LICENSE %{buildroot}%{_datadir}/opae/LICENSE
cp COPYING %{buildroot}%{_datadir}/opae/COPYING

mkdir -p %{buildroot}%{_usr}/src/opae/cmake/modules

for s in FindSphinx.cmake
do
  cp "cmake/${s}" %{buildroot}%{_usr}/src/opae/cmake/
done


mkdir -p %{buildroot}%{_usr}/src/opae/opae-libs/cmake/modules
for s in FindHwloc.cmake \
         OPAE.cmake \
         FindUUID.cmake \
         Findjson-c.cmake \
         OPAECompiler.cmake \
         OPAEGit.cmake \
         OPAEPackaging.cmake 
do
  cp "opae-libs/cmake/modules/${s}" %{buildroot}%{_usr}/src/opae/opae-libs/cmake/modules
done

mkdir -p %{buildroot}%{_usr}/src/opae/samples
mkdir -p %{buildroot}%{_usr}/src/opae/samples/hello_fpga/



cp samples/hello_fpga/hello_fpga.c %{buildroot}%{_usr}/src/opae/samples/hello_fpga/

mkdir -p %{buildroot}/%{_mandir}/man8/

cp _build/sphinx/man/*/userclk.*    %{buildroot}/%{_mandir}/man8/
cp _build/sphinx/man/*/fpgainfo.*    %{buildroot}/%{_mandir}/man8/
cp _build/sphinx/man/*/fpgaconf.*    %{buildroot}/%{_mandir}/man8/
cp _build/sphinx/man/*/mmlink.*    %{buildroot}/%{_mandir}/man8/

cd _build

DESTDIR=%{buildroot}  cmake -DCOMPONENT=opaeclib -P cmake_install.cmake
DESTDIR=%{buildroot}  cmake -DCOMPONENT=opaecxxcorelib -P cmake_install.cmake
DESTDIR=%{buildroot}  cmake -DCOMPONENT=samples -P cmake_install.cmake
DESTDIR=%{buildroot}  cmake -DCOMPONENT=opaetoolslibs -P cmake_install.cmake
DESTDIR=%{buildroot}  cmake -DCOMPONENT=toolfpgainfo -P cmake_install.cmake
DESTDIR=%{buildroot}  cmake -DCOMPONENT=toolfpgaconf -P cmake_install.cmake
DESTDIR=%{buildroot}  cmake -DCOMPONENT=tooluserclk -P cmake_install.cmake
DESTDIR=%{buildroot}  cmake -DCOMPONENT=toolmmlink -P cmake_install.cmake
DESTDIR=%{buildroot}  cmake -DCOMPONENT=samplebin -P cmake_install.cmake
DESTDIR=%{buildroot}  cmake -DCOMPONENT=libopaeheaders -P cmake_install.cmake
DESTDIR=%{buildroot}  cmake -DCOMPONENT=toolpackager -P cmake_install.cmake
DESTDIR=%{buildroot}  cmake -DCOMPONENT=jsonschema -P cmake_install.cmake


%files
%dir %{_datadir}/opae
%doc %{_datadir}/opae/RELEASE_NOTES.md
%license %{_datadir}/opae/LICENSE
%license %{_datadir}/opae/COPYING
%{_libdir}/libopae-c.so.%{version}
%{_libdir}/libopae-c.so.1

%{_libdir}/libbitstream.so.%{version}
%{_libdir}/libbitstream.so.1

%{_libdir}/libopae-cxx-core.so.%{version}
%{_libdir}/libopae-cxx-core.so.1


%files devel
%dir %{_includedir}/opae
%{_includedir}/opae/*
%dir %{_usr}/src/opae
%{_usr}/src/opae/samples/hello_fpga/hello_fpga.c
%{_usr}/src/opae/cmake/*
%{_usr}/src/opae/opae-libs/cmake/modules/*

%{_libdir}/libopae-c.so
%{_libdir}/libbitstream.so
%{_libdir}/libopae-cxx-core.so

%{_libdir}/opae/libxfpga.so*
%{_libdir}/opae/libmodbmc.so*
%{_libdir}/opae/libboard_rc.so*
%{_libdir}/opae/libboard_vc.so*

%{_bindir}/fpgaconf
%{_bindir}/fpgainfo
%{_bindir}/mmlink
%{_bindir}/userclk
%{_bindir}/hello_fpga
%{_bindir}/hello_cxxcore
%{_bindir}/afu_json_mgr
%{_bindir}/packager

%{_usr}/share/opae/*
%{_mandir}/man8/userclk.*
%{_mandir}/man8/fpgainfo.*
%{_mandir}/man8/mmlink.*
%{_mandir}/man8/fpgaconf.*


%changelog
* Fri Apr 17 2020 Korde Nakul <nakul.korde@intel.com> 1.4.1-1
- OPAE git repository layout changes.
- Removed Safe String module dependency.
- Various bug fixes.
- Ported python tools to python3.6.
- Various Static code scan bug fixes.
- Removed pybind11 3rd component from OPAE source repository.

* Tue Dec 17 2019 Korde Nakul <nakul.korde@intel.com> 1.4.0-1
- Added support to FPGA Linux kernel Device Feature List (DFL) driver patch set2.
- Increased test cases and test coverage
- Various bug fixes
- Various compiler warning fixes
- Various memory leak fixes
- Various Static code scan bug fixes
- Added new FPGA MMIO API to write 512 bits
