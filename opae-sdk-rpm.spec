Summary:        Open Programmable Acceleration Engine
Name:           opae
Version:        1.4.0
Release:        1
License:        BSD 3.0
Group:          opae
Vendor:         Intel Corporation
Requires:       opae-libs , opae-devel , opae-tools, opae-tools-extra
Requires:       opae-ase
URL:             https://github.com/OPAE/%{name}-sdk
Source0:         https://github.com/OPAE/%{name}/%{name}.tar.gz

ExclusiveArch: %{ix86} x86_64

BuildRequires:     gcc,gcc-c++
BuildRequires:     cmake
BuildRequires:     python-devel
BuildRequires:     json-c-devel
BuildRequires:     libuuid-devel
BuildRequires:     rpm-build
BuildRequires:     hwloc-devel
BuildRequires:     gtest-devel
BuildRequires:     python-sphinx
BuildRequires:     doxygen


Requires:         python

%description
This package contains the Open Programmable Acceleration Engine (OPAE) components

%package libs
Summary:    Runtime libraries for OPAE applications
Group:      libs
Requires:   uuid , json-c

%description libs
This package contains runtime libraries that support OPAE applications

%package devel
Summary:    OPAE headers, sample source and documentation
Group:      devel
Requires:   opae-libs , libuuid-devel , json-c

%description devel
This package contains OPAE headers, sample source and documentation


%package tools
Summary:    OPAE base tools binaries
Group:      tools
Requires:   opae-libs , opae-devel

%description tools
This package contains OPAE base tools binaries

%package tools-extra
Summary:    OPAE extra tools binaries
Group:      tools-extra
Requires:   opae-libs , opae-devel



%description tools-extra
This package contains OPAE extra tools binaries

%package ase
Summary:    OPAE AFU Simulation Environment
Group:      ase
Requires:   opae-libs , opae-devel

%description ase
This package contains OPAE AFU Simulation Environment


%package tests
Summary:    OPAE tests package
Group:      tests
Requires:   opae-libs

%description tests
This package contains OPAE tests


%prep
%autosetup -n %{name}

%build
rm -rf mybuild
mkdir mybuild
cd  mybuild
%cmake .. -DBUILD_ASE=ON
make -j


%install
cd  mybuild
make DESTDIR=%{buildroot} install
mkdir -p %{buildroot}%{_sysconfdir}/systemd/system/
mv %{buildroot}%{_usr}/lib/systemd/system/fpgad.service  %{buildroot}%{_sysconfdir}/systemd/system/fpgad.service


%clean

%post
mkdir -p  %{_sysconfdir}/ld.so.conf.d
echo "" >  %{_sysconfdir}/ld.so.conf.d/opae-c.conf
ldconfig

%postun

%pre

%preun
rm -f --  %{_sysconfdir}/ld.so.conf.d/opae-c.conf 
ldconfig

%files
%defattr(-,root,root,-)

%files libs
%defattr(-,root,root,-)
%{_libdir}/libopae-c.so*
%{_libdir}/libopae-cxx*
%{_libdir}/libxfpga.so*
%{_libdir}/libbmc.so*
%{_libdir}/libmodbmc.so*
%{_libdir}/libbitstream.so*
%{_libdir}/libboard_rc.so*
%{_libdir}/libboard_vc.so*

%files devel
%defattr(-,root,root,-)
%{_bindir}/afu_platform_config
%{_bindir}/afu_platform_info
%{_bindir}/afu_synth_setup
%{_bindir}/rtl_src_config
%{_bindir}/hello_fpga
%dir %{_includedir}/opae
%{_includedir}/opae/*
%dir %{_includedir}/safe_string
%{_includedir}/safe_string/safe_string.h
%{_libdir}/libsafestr.a
%dir %{_datadir}/opae
%dir %{_datadir}/opae/*
%{_datadir}/opae/*
%dir %{_usr}/src/opae
%{_usr}/src/opae/*


%files tools
%defattr(-,root,root,-)
%{_bindir}/fpgaconf*
%{_bindir}/fpgainfo*
%{_bindir}/fpgaport*
%{_bindir}/fpgametrics*
%{_bindir}/fpgad*
%{_sysconfdir}/opae/fpgad.cfg*
%{_sysconfdir}/sysconfig/fpgad.conf*
%{_sysconfdir}/systemd/system/fpgad.service
%{_libdir}/libfpgad-api.so*
%{_libdir}/libfpgad-xfpga.so*
%{_libdir}/libfpgad-vc.so*



%files tools-extra
%defattr(-,root,root,-)
%{_bindir}/afu_json_mgr
%{_bindir}/bist_app
%{_bindir}/bist_app.py
%{_bindir}/bist_common.py
%{_bindir}/bist_dma.py
%{_bindir}/bist_def.py
%{_bindir}/bist_nlb3.py
%{_bindir}/bist_nlb0.py
%{_bindir}/coreidle
%{_bindir}/fpga_dma_vc_test
%{_bindir}/fpga_dma_test
%{_bindir}/bist
%{_bindir}/fpgabist
%{_bindir}/fpgadiag
%{_bindir}/fpgaflash
%{_bindir}/hssi_config
%{_bindir}/hssi_loopback
%{_bindir}/mmlink
%{_bindir}/nlb0
%{_bindir}/nlb3
%{_bindir}/nlb7
%{_bindir}/super-rsu
%{_bindir}/packager
%{_bindir}/mactest
%{_bindir}/fpgalpbk
%{_bindir}/fpgastats
%{_bindir}/pac_hssi_config.py
%{_bindir}/ras
%{_bindir}/userclk
%{_libdir}/libhssi*
%{_libdir}/libopae-c++-nlb.so*
%{_libdir}/libopae-c++-utils.so*
%dir %{_datadir}/opae
%dir %{_datadir}/opae/python/
%dir %{_datadir}/opae/python/*
%{_datadir}/opae/python/*


%files ase
%defattr(-,root,root,-)
%{_bindir}/afu_sim_setup
%{_bindir}/with_ase
%{_libdir}/libase*
%{_libdir}/libopae-c-ase.so*
%dir %{_datadir}/opae
%dir %{_datadir}/opae/ase/
%dir %{_datadir}/opae/ase/*
%{_datadir}/opae/ase/*

%files tests
%defattr(-,root,root,-)
%{_bindir}/hello_fpga
%{_bindir}/hello_events

%changelog
* Mon Dec 17 2019 Korde Nakul <nakul.korde@intel.com> -1.4.0
- Added support to FPGA Linux kernel Device Feature List (DFL) driver patch set2.
- Increased test cases and test coverage
- Various bug fixes
- Various compiler warning fixes
- Various memory leak fixes
- Various Static code scan bug fixes
- Added new FPGA MMIO API to write 512 bits
