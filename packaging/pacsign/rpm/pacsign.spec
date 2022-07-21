%define         opae_release @RELEASE@

Summary:        Programmable Acceleration Card Signing Utility
Name:           pacsign
Version:        @VERSION@
Release:        %{opae_release}%{?dist}
License:        BSD
BuildArch:      noarch

Group:          Development/Libraries
Vendor:         Intel Corporation
Requires:       python3
Requires:       openssl-devel

URL:            https://github.com/OPAE/%{name}-sdk
Source0:        https://github.com/OPAE/opae-sdk/releases/download/%{version}-%{opae_release}/%{name}-%{version}-%{opae_release}.tar.gz

BuildRequires:  python3-devel

%description
PACSign is the Programmable Accelerator Card Signing utility.

%{?python_disable_dependency_generator}
# Workaround a problem with pybind11 *.so not having build-id's
%undefine _missing_build_ids_terminate_build

# We don't have any debug sources.
%global debug_package %{nil}


%prep
%setup -q -n %{name}-%{version}-%{opae_release}

%build
%{__python3} setup.py build

%install
pushd %{_topdir}/BUILD/%{name}-%{version}-%{opae_release}
%{__python3} setup.py install --single-version-externally-managed --root=%{buildroot}
popd

%clean
rm -rf %{buildroot}

%files
%{_bindir}/PACSign
%{python3_sitelib}/pacsign*

%changelog
