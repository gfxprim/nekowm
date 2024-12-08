#
# NEKOWM specfile
#
# (C) Cyril Hrubis metan{at}ucw.cz 2013-2024
#
#

Summary: A gfxprim proxy backend tiling window manager
Name: nekowm
Version: git
Release: 1
License: GPL-2.0-or-later
Group: System/GUI/Other
Url: https://github.com/gfxprim/nekowm
Source: nekowm-%{version}.tar.bz2
BuildRequires: libgfxprim-devel

BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot

%description
A gfxprim proxy backend tiling window manager

%prep
%setup -n nekowm-%{version}

%build
make %{?jobs:-j%jobs}

%install
DESTDIR="$RPM_BUILD_ROOT" make install

%files -n nekowm
%defattr(-,root,root)
%{_bindir}/nekowm
%{_datadir}/applications/
%{_datadir}/applications/nekowm.desktop
%{_datadir}/nekowm/
%{_datadir}/nekowm/nekowm.png
%{_unitdir}/nekowm.service

%changelog
* Fri Nov 22 2024 Cyril Hrubis <metan@ucw.cz>

  Initial version.
