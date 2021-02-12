%define         opae_release 2

Summary:        Open Programmable Acceleration Engine (OPAE) SDK
Name:           opae
Version:        2.0.0
Release:        %{opae_release}%{?dist}
License:        BSD
ExclusiveArch:  x86_64

Group:          Development/Libraries
Vendor:         Intel Corporation
Requires:       uuid, json-c, python3
URL:            https://github.com/OPAE/%{name}-sdk
Source0:        https://github.com/OPAE/opae-sdk/releases/download/%{version}-%{release}/%{name}-%{version}-%{opae_release}.tar.gz



BuildRequires:  gcc, gcc-c++
BuildRequires:  cmake
BuildRequires:  python3-devel
BuildRequires:  json-c-devel
BuildRequires:  libuuid-devel
BuildRequires:  rpm-build
BuildRequires:  hwloc-devel
BuildRequires:  python3-sphinx
BuildRequires:  doxygen
BuildRequires:  systemd
BuildRequires:  pybind11-devel
BuildRequires:  python3-setuptools
BuildRequires:  tbb-devel
BuildRequires:  git
BuildRequires:  python3-pip
BuildRequires:  python3-virtualenv
BuildRequires:  systemd-rpm-macros

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


%{?python_disable_dependency_generator}


%prep
%setup -q -n %{name}-%{version}-%{opae_release}

%build
rm -rf _build
mkdir _build
cd _build

%cmake .. -DCMAKE_INSTALL_PREFIX=/usr  -DOPAE_PRESERVE_REPOS=ON -DOPAE_BUILD_LEGACY=ON -DOPAE_BUILD_SAMPLES=ON -B $PWD

%make_build 




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
  chmod a+x %{buildroot}%{_usr}/src/opae/opae-libs/cmake/modules/$s
done

mkdir -p %{buildroot}%{_usr}/src/opae/samples
mkdir -p %{buildroot}%{_usr}/src/opae/samples/hello_fpga/
mkdir -p %{buildroot}%{_usr}/src/opae/samples/hello_events/
mkdir -p %{buildroot}%{_usr}/src/opae/samples/object_api/


cp samples/hello_fpga/hello_fpga.c %{buildroot}%{_usr}/src/opae/samples/hello_fpga/
cp samples/hello_events/hello_events.c %{buildroot}%{_usr}/src/opae/samples/hello_events/
cp samples/object_api/object_api.c %{buildroot}%{_usr}/src/opae/samples/object_api/


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
DESTDIR=%{buildroot}  cmake -DCOMPONENT=toolmmlink -P cmake_install.cmake
DESTDIR=%{buildroot}  cmake -DCOMPONENT=opaeboardlib -P cmake_install.cmake
DESTDIR=%{buildroot}  cmake -DCOMPONENT=toolfpgametrics -P cmake_install.cmake
DESTDIR=%{buildroot}  cmake -DCOMPONENT=toolbist_app -P cmake_install.cmake
DESTDIR=%{buildroot}  cmake -DCOMPONENT=toolfpga_dma_test -P cmake_install.cmake
DESTDIR=%{buildroot}  cmake -DCOMPONENT=toolfpga_dma_N3000_test -P cmake_install.cmake
DESTDIR=%{buildroot}  cmake -DCOMPONENT=toolfpgabist -P cmake_install.cmake


DESTDIR=%{buildroot}  cmake -DCOMPONENT=opaecxxutils -P cmake_install.cmake
DESTDIR=%{buildroot}  cmake -DCOMPONENT=opaecxxnlb -P cmake_install.cmake
DESTDIR=%{buildroot}  cmake -DCOMPONENT=toolfpgadiagapps -P cmake_install.cmake
DESTDIR=%{buildroot}  cmake -DCOMPONENT=toolfpgadiag -P cmake_install.cmake
DESTDIR=%{buildroot}  cmake -DCOMPONENT=toolfpgad -P cmake_install.cmake
DESTDIR=%{buildroot}  cmake -DCOMPONENT=toolfpgad_api -P cmake_install.cmake
DESTDIR=%{buildroot}  cmake -DCOMPONENT=toolfpgad_vc -P cmake_install.cmake

prev=$PWD
pushd %{_topdir}/BUILD/%{name}-%{version}-%{opae_release}/python/opae.admin/
%{__python3} setup.py install --single-version-externally-managed  --root=%{buildroot} 
popd

pushd %{_topdir}/BUILD/%{name}-%{version}-%{opae_release}/python/pacsign
%{__python3} setup.py install --single-version-externally-managed --root=%{buildroot} 
popd

pushd %{_topdir}/BUILD/%{name}-%{version}-%{opae_release}/scripts
install -m 755 eth_group_mdev.sh %{buildroot}/usr/bin/eth_group_mdev.sh
popd



for file in %{buildroot}%{python3_sitelib}/opae/admin/tools/{fpgaflash,fpgaotsu,fpgaport,fpgasupdate,ihex2ipmi,rsu,super_rsu,bitstream_info}.py; do
   chmod a+x $file
done


%files
%dir %{_datadir}/opae
%doc %{_datadir}/opae/RELEASE_NOTES.md
%license %{_datadir}/opae/LICENSE
%license %{_datadir}/opae/COPYING

%{_libdir}/libopae-c.so.%{version}
%{_libdir}/libopae-c.so.2
%{_libdir}/libbitstream.so.%{version}
%{_libdir}/libbitstream.so.2
%{_libdir}/libopae-cxx-core.so.%{version}
%{_libdir}/libopae-cxx-core.so.2
%{_libdir}/libopae-c++-utils.so.%{version}
%{_libdir}/libopae-c++-utils.so.2
%{_libdir}/libopae-c++-nlb.so.%{version}
%{_libdir}/libopae-c++-nlb.so.2
%{_libdir}/libfpgad-api.so.%{version}
%{_libdir}/libfpgad-api.so.2


%post devel
%systemd_post fpgad.service

%preun devel
%systemd_preun fpgad.service


%files devel
%dir %{_includedir}/opae
%{_includedir}/opae/*
%dir %{_usr}/src/opae
%{_usr}/src/opae/samples/hello_fpga/hello_fpga.c
%{_usr}/src/opae/samples/hello_events/hello_events.c
%{_usr}/src/opae/samples/object_api/object_api.c
%{_usr}/src/opae/cmake/*
%{_usr}/src/opae/opae-libs/cmake/modules/*

%{_libdir}/opae/libboard_a10gx.so*
%{_libdir}/opae/libboard_n3000.so*
%{_libdir}/opae/libboard_d5005.so*
%{_libdir}/libopae-c++-nlb.so
%{_libdir}/libopae-cxx-core.so
%{_libdir}/libopae-c++-utils.so
%{_libdir}/libopae-c.so
%{_libdir}/libbitstream.so
%{_libdir}/libfpgad-api.so
%{_libdir}/opae/libxfpga.so*
%{_libdir}/opae/libmodbmc.so*
%{_bindir}/bist_app*
%{_bindir}/dummy_afu
%{_bindir}/bist_common.py*
%{_bindir}/bist_dma.py*
%{_bindir}/bist_def.py*
%{_bindir}/bist_nlb3.py*
%{_bindir}/bist_nlb0.py*
%{_bindir}/fpgabist*
%{_bindir}/nlb0*
%{_bindir}/nlb3*
%{_bindir}/nlb7*
%{_bindir}/fecmode*
%{_bindir}/fpgamac*
%{_bindir}/fvlbypass*
%{_bindir}/mactest*
%{_bindir}/fpgadiag*
%{_bindir}/fpgalpbk*
%{_bindir}/fpgastats*
%{_bindir}/bitstreaminfo*
%{_bindir}/fpgaflash*
%{_bindir}/fpgaotsu*
%{_bindir}/fpgaport*
%{_bindir}/fpgasupdate*
%{_bindir}/rsu*
%{_bindir}/super-rsu*
%{_bindir}/fpgaconf
%{_bindir}/fpgainfo
%{_bindir}/mmlink
%{_bindir}/userclk
%{_bindir}/hello_fpga
%{_bindir}/object_api
%{_bindir}/hello_events
%{_bindir}/hello_cxxcore
%{_bindir}/afu_json_mgr
%{_bindir}/packager
%{_bindir}/fpgametrics
%{_bindir}/fpga_dma_N3000_test
%{_bindir}/fpga_dma_test
%{_bindir}/PACSign*
%{_bindir}/fpgad
%config(noreplace) %{_sysconfdir}/opae/fpgad.cfg*
%config(noreplace) %{_sysconfdir}/sysconfig/fpgad.conf*
%{_unitdir}/fpgad.service
%{_libdir}/opae/libfpgad-vc.so*
%{_bindir}/eth_group_mdev.sh
%{_usr}/share/opae/*
%{_datadir}/doc/opae.admin/LICENSE
%{python3_sitelib}/opae*
%{python3_sitelib}/pacsign*
# part of the jsonschema testsuite, do not deliver
%exclude /usr/share/opae/python/jsonschema-2.3.0/json/bin/jsonschema_suite

%changelog
* Mon Dec 14 2020 The OPAE Dev Team <opae@lists.01.org> - 2.0.0-2
- Update OPAE spec file and tarball generation script
- Fix build errors

* Thu Sep 17 2020 Ananda Ravuri <ananda.ravuri@intel.com> 2.0.0-1
- Various Static code scan bug fixes
- Added support to FPGA Linux kernel Device Feature List (DFL) driver.
- Added support to PAC card N3000 series.
- Added PACSign, bitstream_info, fpgasudpate, rsu, fpgaotsu, fpgaport  python tools.
- Added ethernet tools for PAC card N3000.
- Various bug fixes
- Various memory leak fixes.
- Various Static code scan bug fixes
- Added python3 support.
- OPAE USMG API are deprecated.
- Updated OPAE documentation.  


* Tue Dec 17 2019 Korde Nakul <nakul.korde@intel.com> 1.4.0-1
- Added support to FPGA Linux kernel Device Feature List (DFL) driver patch set2.
- Increased test cases and test coverage
- Various bug fixes
- Various compiler warning fixes
- Various memory leak fixes
- Various Static code scan bug fixes
- Added new FPGA MMIO API to write 512 bits
