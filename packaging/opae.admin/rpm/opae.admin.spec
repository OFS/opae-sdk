%global srcname opae.admin

Name:           python-%{srcname}
Version:        @VERSION@
Release:        @RELEASE@%{?dist}
Summary:        opae.admin is an administration package for PACs

License:        BSD
URL:            https://github.com/OPAE/opae-sdk
Source0:        %{srcname}-%{version}.tar.gz

BuildArch:      noarch

Requires:       sysvinit-tools
Requires:       pciutils

%global _description %{expand:
Enables device enumeration and management, including
firmware update and one-time secure update.}

%description %_description

%package -n python3-%{srcname}
Summary:        %{summary}
BuildRequires:  python3-devel

%description -n python3-%{srcname} %_description

%prep
%autosetup -n %{srcname}-%{version}


%build
%py3_build

%install
%py3_install

%check
#%{python3} setup.py test

# Note that there is no %%files section for the unversioned python module
%files -n python3-%{srcname}
%license %{_datadir}/doc/opae.admin/LICENSE
%doc %{_usr}/share/man/man1/bitstreaminfo.1.gz
%doc %{_usr}/share/man/man1/fpgaport.1.gz
%doc %{_usr}/share/man/man1/fpgasupdate.1.gz
%doc %{_usr}/share/man/man1/super-rsu.1.gz
%{python3_sitelib}/%{srcname}-*.egg-info/
%{python3_sitelib}/opae/
%ghost %{python3_sitelib}/opae/__init__.py
%{_bindir}/bitstreaminfo
%{_bindir}/fpgaflash
%{_bindir}/fpgaotsu
%{_bindir}/fpgaport
%{_bindir}/fpgasupdate
%{_bindir}/rsu
%{_bindir}/super-rsu

%post
if ! [ -f %{python3_sitelib}/opae/__init__.py ]; then
  cp %{python3_sitelib}/opae/admin/__init__.py %{python3_sitelib}/opae/__init__.py
fi

%changelog
