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

%cmake .. -DCMAKE_INSTALL_PREFIX=/usr

make -j  opae-c \
         bitstream \
         xfpga \
         safestr \
         modbmc \
         opae-cxx-core \
         hello_cxxcore \
         argsfilter \
         board_rc \
         board_vc \
         fpgaconf \
         fpgainfo \
         userclk \
         hello_fpga \
         hello_events \
         object_api \
         mmlink

%install
mkdir -p %{buildroot}%{_datadir}/opae
cp RELEASE_NOTES.md %{buildroot}%{_datadir}/opae/RELEASE_NOTES.md

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
cp samples/hello_fpga.c %{buildroot}%{_usr}/src/opae/samples/
cp samples/hello_events.c %{buildroot}%{_usr}/src/opae/samples/
cp samples/object_api.c %{buildroot}%{_usr}/src/opae/samples/

cd mybuild

DESTDIR=%{buildroot}  cmake -DCOMPONENT=safestrlib -P cmake_install.cmake
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
DESTDIR=%{buildroot}  cmake -DCOMPONENT=safestrheaders -P cmake_install.cmake


%clean

%post
ldconfig

%postun
ldconfig

%pre

%preun

%files
%defattr(-,root,root,-)
%dir %{_datadir}/opae
%doc %{_datadir}/opae/RELEASE_NOTES.md
%{_libdir}/libopae-c.so*
%{_libdir}/libopae-cxx-core.so*
%{_libdir}/opae/libxfpga.so*
%{_libdir}/libbitstream.so*
%{_libdir}/opae/libmodbmc.so*
%{_libdir}/opae/libboard_rc.so*
%{_libdir}/opae/libboard_vc.so*
%{_libdir}/libsafestr.a*


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
%{_usr}/src/opae/cmake/*
%{_usr}/src/opae/opae-libs/cmake/modules/*


%files tools
%defattr(-,root,root,-)
%{_libdir}/libargsfilter.a*
%{_bindir}/fpgaconf*
%{_bindir}/fpgainfo*


%files tools-extra
%defattr(-,root,root,-)
%{_bindir}/mmlink
%{_bindir}/userclk


%files samples
%defattr(-,root,root,-)
%{_bindir}/hello_fpga
%{_bindir}/hello_events*
%{_bindir}/object_api
%{_bindir}/hello_cxxcore


%changelog
* Tue Dec 17 2019 Korde Nakul <nakul.korde@intel.com> 1.4.0-1
- Added support to FPGA Linux kernel Device Feature List (DFL) driver patch set2.
- Increased test cases and test coverage
- Various bug fixes
- Various compiler warning fixes
- Various memory leak fixes
- Various Static code scan bug fixes
- Added new FPGA MMIO API to write 512 bits
