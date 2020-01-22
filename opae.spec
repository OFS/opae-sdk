Summary:        Open Programmable Acceleration Engine (OPAE)
Name:           opae
Version:        1.4.0
Release:        1
License:        BSD
Group:          Development/Libraries
Vendor:         Intel Corporation
Requires:       uuid, json-c, python
URL:            https://github.com/OPAE/%{name}-sdk
Source0:        https://github.com/OPAE/%{name}/%{name}.tar.gz

BuildRequires:  gcc, gcc-c++
BuildRequires:  cmake
BuildRequires:  python-devel
BuildRequires:  json-c-devel
BuildRequires:  libuuid-devel
BuildRequires:  rpm-build
BuildRequires:  hwloc-devel
BuildRequires:  python-sphinx
BuildRequires:  doxygen

%description
Runtime libraries for OPAE applications


%package devel
Summary:    OPAE headers, sample source, and documentation
Group:      Development/Libraries
Requires:   libuuid-devel

%description devel
OPAE headers, sample source, and documentation


%package tools
Summary:    OPAE base tools binaries
Group:      Development/Libraries

%description tools
OPAE Base Tools binaries

%post tools
ldconfig

%postun tools
ldconfig


%package tools-extra
Summary:    OPAE extra tools binaries
Group:      Development/Libraries

%description tools-extra
OPAE Extra Tools binaries

%post tools-extra
ldconfig

%postun tools-extra
ldconfig


%package samples
Summary:    OPAE samples apps
Group:      Development/Libraries

%description samples
OPAE samples


%prep
%autosetup -n %{name}

%build
rm -rf mybuild
mkdir mybuild
cd mybuild
%cmake .. -DBUILD_ASE=OFF -DOPAE_INSTALL_RPATH=OFF
make -j

%install
mkdir -p %{buildroot}%{_datadir}/opae
cp RELEASE_NOTES.md %{buildroot}%{_datadir}/opae/RELEASE_NOTES.md

mkdir -p %{buildroot}%{_usr}/src/opae/cmake/modules
for s in FindDBus.cmake \
         FindGLIB.cmake \
         FindOPAE.cmake \
         FindQuartus.cmake \
         FindQuesta.cmake \
         FindRT.cmake \
         FindUUID.cmake \
         FindVerilator.cmake \
         Findjson-c.cmake \
         cmake_useful.cmake \
         compiler_config.cmake \
         libraries_config.cmake
do
  cp "cmake/modules/${s}" %{buildroot}%{_usr}/src/opae/cmake/modules
done

mkdir -p %{buildroot}%{_usr}/src/opae/samples
cp samples/hello_fpga.c %{buildroot}%{_usr}/src/opae/samples/
cp samples/hello_events.c %{buildroot}%{_usr}/src/opae/samples/
cp samples/object_api.c %{buildroot}%{_usr}/src/opae/samples/

cd mybuild
make DESTDIR=%{buildroot} install
mkdir -p %{buildroot}%{_sysconfdir}/systemd/system/
mv %{buildroot}%{_usr}/lib/systemd/system/fpgad.service %{buildroot}%{_sysconfdir}/systemd/system/fpgad.service

mkdir -p %{buildroot}%{_libdir}/opae/
for l in libxfpga.so \
         libmodbmc.so \
         libfpgad-xfpga.so \
         libfpgad-vc.so
do
  mv "%{buildroot}%{_libdir}/${l}" %{buildroot}%{_libdir}/opae/
done

%clean

%post
mkdir -p %{_sysconfdir}/ld.so.conf.d
echo "" > %{_sysconfdir}/ld.so.conf.d/opae-c.conf
ldconfig

%postun
ldconfig

%pre

%preun
rm -f -- %{_sysconfdir}/ld.so.conf.d/opae-c.conf 

%files
%defattr(-,root,root,-)
%dir %{_datadir}/opae
%doc %{_datadir}/opae/RELEASE_NOTES.md
%{_libdir}/libopae-c.so*
%{_libdir}/libopae-cxx-core.so*
%dir %{_libdir}/opae
%{_libdir}/opae/libxfpga.so*
%{_libdir}/libbmc.so*
%{_libdir}/opae/libmodbmc.so*
%{_libdir}/libbitstream.so*
%{_libdir}/libboard_rc.so*
%{_libdir}/libboard_vc.so*

%files devel
%defattr(-,root,root,-)
%dir %{_includedir}/opae
%{_includedir}/opae/*
%dir %{_includedir}/safe_string
%{_includedir}/safe_string/safe_string.h
%{_libdir}/libsafestr.a
%dir %{_usr}/src/opae
%{_usr}/src/opae/samples/hello_fpga.c
%{_usr}/src/opae/samples/hello_events.c
%{_usr}/src/opae/samples/object_api.c
%{_usr}/src/opae/cmake/modules/*

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
%dir %{_libdir}/opae
%{_libdir}/opae/libfpgad-xfpga.so*
%{_libdir}/opae/libfpgad-vc.so*

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

%files samples
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
