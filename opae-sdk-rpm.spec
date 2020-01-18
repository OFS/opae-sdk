Summary:        Open Programmable Acceleration Engine
Name:           opae
Version:        1.4.0
Release:        1
License:        BSD
Group:          opae
Vendor:         Intel Corporation
Requires:       opae-libs, opae-devel, opae-tools, opae-tools-extra
URL:            https://github.com/OPAE/%{name}-sdk
Source0:        https://github.com/OPAE/%{name}/%{name}.tar.gz
ExclusiveArch:  %{ix86} x86_64

BuildRequires:  gcc, gcc-c++
BuildRequires:  cmake
BuildRequires:  python-devel
BuildRequires:  json-c-devel
BuildRequires:  libuuid-devel
BuildRequires:  rpm-build
BuildRequires:  hwloc-devel
BuildRequires:  gtest-devel
BuildRequires:  python-sphinx
BuildRequires:  doxygen

Requires:       python

%description
Open Programmable Acceleration Engine (OPAE)


%package libs
Summary:    Runtime libraries for OPAE applications
Group:      libs
Requires:   uuid, json-c

%description libs
OPAE runtime libraries


%package devel
Summary:    OPAE headers, sample source and documentation
Group:      devel
Requires:   opae-libs, libuuid-devel, json-c

%description devel
OPAE headers, sample source, and documentation


%package tools
Summary:    OPAE base tools binaries
Group:      tools

%description tools
OPAE Base Tools binaries


%package tools-extra
Summary:    OPAE extra tools binaries
Group:      tools-extra

%description tools-extra
OPAE Extra Tools binaries


%package tests
Summary:    OPAE tests package
Group:      tests

%description tests
OPAE tests


%prep
%autosetup -n %{name}

%build
rm -rf mybuild
mkdir mybuild
cd mybuild
%cmake .. -DBUILD_ASE=OFF
make -j

%install
cd mybuild
make DESTDIR=%{buildroot} install
mkdir -p %{buildroot}%{_sysconfdir}/systemd/system/
mv %{buildroot}%{_usr}/lib/systemd/system/fpgad.service %{buildroot}%{_sysconfdir}/systemd/system/fpgad.service

%clean

%post
mkdir -p %{_sysconfdir}/ld.so.conf.d
echo "" > %{_sysconfdir}/ld.so.conf.d/opae-c.conf
ldconfig

%postun

%pre

%preun
rm -f -- %{_sysconfdir}/ld.so.conf.d/opae-c.conf 
ldconfig

%files
%defattr(-,root,root,-)

%files libs
%defattr(-,root,root,-)
%{_libdir}/libopae-c.so*
%{_libdir}/libopae-cxx-core.so*
%{_libdir}/libxfpga.so*
%{_libdir}/libbmc.so*
%{_libdir}/libmodbmc.so*
%{_libdir}/libbitstream.so*
%{_libdir}/libboard_rc.so*
%{_libdir}/libboard_vc.so*

%files devel
%defattr(-,root,root,-)
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
%{_bindir}/fpgametrics*
%{_bindir}/fpgad*
%config(noreplace) %{_sysconfdir}/opae/fpgad.cfg*
%config(noreplace) %{_sysconfdir}/sysconfig/fpgad.conf*
%config(noreplace) %{_sysconfdir}/systemd/system/fpgad.service
%{_libdir}/libfpgad-api.so*
%{_libdir}/libfpgad-xfpga.so*
%{_libdir}/libfpgad-vc.so*

%files tools-extra
%defattr(-,root,root,-)
%{_bindir}/bist_app
%{_bindir}/bist_app.py
%{_bindir}/bist_common.py
%{_bindir}/bist_dma.py
%{_bindir}/bist_def.py
%{_bindir}/bist_nlb3.py
%{_bindir}/bist_nlb0.py
%{_bindir}/coreidle
%{_bindir}/fpga_dma_vc_test
%{_bindir}/bist
%{_bindir}/fpgabist
%{_bindir}/fpgadiag
%{_bindir}/hssi_config
%{_bindir}/hssi_loopback
%{_bindir}/mmlink
%{_bindir}/nlb0
%{_bindir}/nlb3
%{_bindir}/nlb7
%{_bindir}/mactest
%{_bindir}/fpgalpbk
%{_bindir}/fpgastats
%{_bindir}/ras
%{_bindir}/userclk
%{_libdir}/libhssi*
%{_libdir}/libopae-c++-nlb.so*
%{_libdir}/libopae-c++-utils.so*
%dir %{_datadir}/opae

%files tests
%defattr(-,root,root,-)
%{_bindir}/hello_fpga
%{_bindir}/hello_events

%changelog
* Tue Dec 17 2019 Korde Nakul <nakul.korde@intel.com> 1.4.0-1
- Added support to FPGA Linux kernel Device Feature List (DFL) driver patch set2.
- Increased test cases and test coverage
- Various bug fixes
- Various compiler warning fixes
- Various memory leak fixes
- Various Static code scan bug fixes
- Added new FPGA MMIO API to write 512 bits
