Summary:        Open Programmable Acceleration Engine
Name:           opae-sdk
Version:        1.4.0
Release:        1
License:        BSD 3.0
Group:          opae
Vendor:         Intel Corporation
Prefix:         /usr
Requires:       opae-libs , opae-devel , opae-tools, opae-tools-extra
Requires:       opae-ase
URL:             https://github.com/OPAE/%{name}
Source0:         https://github.com/OPAE/%{name}/%{name}.tar.gz

ExclusiveArch: %{ix86} x86_64

BuildRequires:     gcc,gcc-c++
BuildRequires:     cmake
BuildRequires:     json-c-devel
BuildRequires:     libuuid-devel
BuildRequires:     rpm-build
BuildRequires:     hwloc-devel
BuildRequires:     gtest-devel
BuildRequires:     python-sphinx
BuildRequires:     doxygen


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
Prefix:     /etc


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
mkdir -p %{buildroot}/etc/systemd/system/
mv %{buildroot}%{_usr}/lib/systemd/system/fpgad.service  %{buildroot}/etc/systemd/system/fpgad.service

%clean

%post
mkdir -p /etc/ld.so.conf.d
echo "" > /etc/ld.so.conf.d/opae-c.conf
ldconfig

%postun

%pre

%preun
rm -f -- /etc/ld.so.conf.d/opae-c.conf 
ldconfig

%files
%defattr(-,root,root,-)

%files libs
%defattr(-,root,root,-)
%{_usr}/lib64/libopae-c.so*
%{_usr}/lib64/libopae-cxx*
%{_usr}/lib64/libxfpga.so*
%{_usr}/lib64/libbmc.so*
%{_usr}/lib64/libmodbmc.so*
%{_usr}/lib64/libbitstream.so*
%{_usr}/lib64/libboard_rc.so*
%{_usr}/lib64/libboard_vc.so*

%files devel
%defattr(-,root,root,-)
%{_usr}/bin/afu_platform_config
%{_usr}/bin/afu_platform_info
%{_usr}/bin/afu_synth_setup
%{_usr}/bin/rtl_src_config
%{_usr}/bin/hello_fpga
%dir %{_usr}/include/opae
%{_usr}/include/opae/*
%dir %{_usr}/include/safe_string
%{_usr}/include/safe_string/safe_string.h
%{_usr}/lib64/libsafestr.a
%dir %{_usr}/share/opae
%dir %{_usr}/share/opae/*
%{_usr}/share/opae/*
%dir %{_usr}/src/opae
%{_usr}/src/opae/*


%files tools
%defattr(-,root,root,-)
%{_usr}/bin/fpgaconf*
%{_usr}/bin/fpgainfo*
%{_usr}/bin/fpgaport*
%{_usr}/bin/fpgametrics*
%{_usr}/bin/fpgad*
/etc/opae/fpgad.cfg*
/etc/sysconfig/fpgad.conf*
/etc/systemd/system/fpgad.service
%{_usr}/lib64/libfpgad-api.so*
%{_usr}/lib64/libfpgad-xfpga.so*
%{_usr}/lib64/libfpgad-vc.so*



%files tools-extra
%defattr(-,root,root,-)
%{_usr}/bin/afu_json_mgr
%{_usr}/bin/bist_app
%{_usr}/bin/bist_app.py
%{_usr}/bin/bist_common.py
%{_usr}/bin/bist_dma.py
%{_usr}/bin/bist_def.py
%{_usr}/bin/bist_nlb3.py
%{_usr}/bin/bist_nlb0.py
%{_usr}/bin/coreidle
%{_usr}/bin/fpga_dma_vc_test
%{_usr}/bin/fpga_dma_test
%{_usr}/bin/bist
%{_usr}/bin/fpgabist
%{_usr}/bin/fpgadiag
%{_usr}/bin/fpgaflash
%{_usr}/bin/hssi_config
%{_usr}/bin/hssi_loopback
%{_usr}/bin/mmlink
%{_usr}/bin/nlb0
%{_usr}/bin/nlb3
%{_usr}/bin/nlb7
%{_usr}/bin/super-rsu
%{_usr}/bin/packager
%{_usr}/bin/mactest
%{_usr}/bin/fpgalpbk
%{_usr}/bin/fpgastats
%{_usr}/bin/pac_hssi_config.py
%{_usr}/bin/ras
%{_usr}/bin/userclk
%{_usr}/lib64/libhssi*
%{_usr}/lib64/libopae-c++-nlb.so*
%{_usr}/lib64/libopae-c++-utils.so*
%dir %{_usr}/share/opae
%dir %{_usr}/share/opae/python/
%dir %{_usr}/share/opae/python/*
%{_usr}/share/opae/python/*


%files ase
%defattr(-,root,root,-)
%{_usr}/bin/afu_sim_setup
%{_usr}/bin/with_ase
%{_usr}/lib64/libase*
%{_usr}/lib64/libopae-c-ase.so*
%dir %{_usr}/share/opae
%dir %{_usr}/share/opae/ase/
%dir %{_usr}/share/opae/ase/*
%{_usr}/share/opae/ase/*

%files tests
%defattr(-,root,root,-)
%{_usr}/bin/hello_fpga
%{_usr}/bin/hello_events

%changelog
* Mon Dec 17 2019 Korde Nakul <nakul.korde@intel.com> -1.4.0
- Added support to FPGA Linux kernel Device Feature List (DFL) driver patch set2.
- Increased test cases and test coverage
- Various bug fixes
- Various compiler warning fixes
- Various memory leak fixes
- Various Static code scan bug fixes
- Added new FPGA MMIO API to write 512 bits
