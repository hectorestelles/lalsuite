%define _prefix /opt/lscsoft/lalinference
%define _mandir %{_prefix}/share/man
%define _sysconfdir %{_prefix}/etc
Name: lalinference
Version: 0.0.0.1
Release: 1
Summary: LSC Algorithm Inference Library
License: GPL
Group: LAL
Source: %{name}-%{version}.tar.gz
URL: http://www.lsc-group.phys.uwm.edu/lal
Packager: Adam Mercer <adam.mercer@ligo.org>
BuildRoot: %{_tmppath}/%{name}-%{version}-root
BuildRequires: python
Requires: gsl libmetaio lal lalmetaio lalinspiral
Prefix: %{_prefix}

%description
The LSC Algorithm Inference Library for gravitational wave data analysis. This
package contains the shared-object libraries needed to run applications
that use the LAL Inference library.

%package devel
Summary: Files and documentation needed for compiling programs that use LAL Inference
Group: LAL
Requires: %{name} = %{version}
Requires: gsl-devel libmetaio-devel lal-devel lalmetaio-devel lalinspiral lalinspiral-devel
%description devel
The LSC Algorithm Inference Library for gravitational wave data analysis. This
package contains files needed build applications that use the LAL Inference
library.

%prep
%setup -q

%build
source /opt/lscsoft/lscsoft-user-env.sh
%configure --disable-gcc-flags
%{__make} V=1

%install
%makeinstall

%post
ldconfig

%postun
ldconfig

%clean
[ ${RPM_BUILD_ROOT} != "/" ] && rm -Rf ${RPM_BUILD_ROOT}
rm -Rf ${RPM_BUILD_DIR}/%{name}-%{version}

%files
%defattr(-,root,root)
%{_libdir}/*.so*
%{_sysconfdir}/*

%files devel
%defattr(-,root,root)
%{_libdir}/*a
%{_libdir}/pkgconfig/*
%{_includedir}/lal
