Name:		ndctl
Version:	56
Release:	2%{?dist}
Summary:	Manage "libnvdimm" subsystem devices (Non-volatile Memory)
License:	GPLv2
Group:		System Environment/Base
Url:		https://github.com/pmem/ndctl
Source0:	https://github.com/pmem/%{name}/archive/v%{version}.tar.gz#/%{name}-%{version}.tar.gz
Patch0:		22bffbe-daxctl-add-libuuid-to-the-build.patch
Patch1:		77d84b2-ndctl-create-namespace-include-dax-info-in-operation-result.patch
Patch2:		rhel-ndctl-limit-device-dax-to-4k-by-default.patch
Patch3:		rhel-ndctl-document-4k-alignment-default.patch
Patch4:		2cf2acc-libndctl-add-support-for-the-MSFT-family-of-DSM-functions.patch

Requires:	ndctl-libs%{?_isa} = %{version}-%{release}
Requires:	daxctl-libs%{?_isa} = %{version}-%{release}
BuildRequires:	autoconf
BuildRequires:	asciidoc
BuildRequires:	xmlto
BuildRequires:	automake
BuildRequires:	libtool
BuildRequires:	pkgconfig
BuildRequires:	pkgconfig(libkmod)
BuildRequires:	pkgconfig(libudev)
BuildRequires:	pkgconfig(uuid)
BuildRequires:	pkgconfig(json-c)
BuildRequires:	pkgconfig(bash-completion)

%description
Utility library for managing the "libnvdimm" subsystem.  The "libnvdimm"
subsystem defines a kernel device model and control message interface for
platform NVDIMM resources like those defined by the ACPI 6+ NFIT (NVDIMM
Firmware Interface Table).


%package -n ndctl-devel
Summary:	Development files for libndctl
License:	LGPLv2
Group:		Development/Libraries
Requires:	ndctl-libs%{?_isa} = %{version}-%{release}

%description -n ndctl-devel
The %{name}-devel package contains libraries and header files for
developing applications that use %{name}.

%package -n daxctl
Summary:        Manage Device-DAX instances
License:        GPLv2
Group:          System Environment/Base
Requires:       daxctl-libs%{?_isa} = %{version}-%{release}

%description -n daxctl
The daxctl utility provides enumeration and provisioning commands for
the Linux kernel Device-DAX facility. This facility enables DAX mappings
of performance / feature differentiated memory without need of a
filesystem.

%package -n daxctl-devel
Summary:	Development files for libdaxctl
License:	LGPLv2
Group:		Development/Libraries
Requires:	daxctl-libs%{?_isa} = %{version}-%{release}

%description -n daxctl-devel
The %{name}-devel package contains libraries and header files for
developing applications that use %{name}, a library for enumerating
"Device DAX" devices.  Device DAX is a facility for establishing DAX
mappings of performance / feature-differentiated memory.


%package -n ndctl-libs
Summary:	Management library for "libnvdimm" subsystem devices (Non-volatile Memory)
License:	LGPLv2
Group:		System Environment/Libraries
Requires:	daxctl-libs%{?_isa} = %{version}-%{release}


%description -n ndctl-libs
Libraries for %{name}.

%package -n daxctl-libs
Summary:	Management library for "Device DAX" devices
License:	LGPLv2
Group:		System Environment/Libraries

%description -n daxctl-libs
Device DAX is a facility for establishing DAX mappings of performance /
feature-differentiated memory. daxctl-libs provides an enumeration /
control API for these devices.


%prep
%setup -q ndctl-%{version}
%patch0 -p1
%patch1 -p1
%patch2 -p1
%patch3 -p1
%patch4 -p1

%build
echo %{version} > version
./autogen.sh
%configure --disable-static --enable-local --disable-silent-rules
make %{?_smp_mflags}

%install
%make_install
find $RPM_BUILD_ROOT -name '*.la' -exec rm -f {} ';'

%check
make check

%post -n ndctl-libs -p /sbin/ldconfig

%postun -n ndctl-libs -p /sbin/ldconfig

%post -n daxctl-libs -p /sbin/ldconfig

%postun -n daxctl-libs -p /sbin/ldconfig

%define bashcompdir %(pkg-config --variable=completionsdir bash-completion)

%files
%license util/COPYING licenses/BSD-MIT licenses/CC0
%{_bindir}/ndctl
%{_mandir}/man1/ndctl*
%{bashcompdir}/

%files -n daxctl
%license util/COPYING licenses/BSD-MIT licenses/CC0
%{_bindir}/daxctl
%{_mandir}/man1/daxctl*

%files -n ndctl-libs
%doc README.md
%license util/COPYING licenses/BSD-MIT licenses/CC0
%{_libdir}/libndctl.so.*

%files -n daxctl-libs
%doc README.md
%license util/COPYING licenses/BSD-MIT licenses/CC0
%{_libdir}/libdaxctl.so.*

%files -n ndctl-devel
%license util/COPYING
%{_includedir}/ndctl/
%{_libdir}/libndctl.so
%{_libdir}/pkgconfig/libndctl.pc

%files -n daxctl-devel
%license util/COPYING
%{_includedir}/daxctl/
%{_libdir}/libdaxctl.so
%{_libdir}/pkgconfig/libdaxctl.pc


%changelog
* Tue May 30 2017 Jeff Moyer <jmoyer@redhat.com> - 56-2
- bump release
- Related: bz#1440902 bz#1446689

* Wed May 24 2017 Jeff Moyer <jmoyer@redhat.com> - 56-2
- Update documentation to reflect 4k alignment
- Add support for the MSFT family of DSM functions
- Resolves: bz#1440902 bz#1446689

* Sun Mar 26 2017 Jeff Moyer <jmoyer@redhat.com> - 56-1
- Rebase to upstream version 56
- Default to 4k alignment for device dax
- Resolves: bz#1384873 bz#1384642 bz#1349233 bz#1357451

* Mon Aug 29 2016 Dave Anderson <anderson@redhat.com> - 54.1
- Update to 54.1 to address ixpdimm_sw requirements
- Resolves bz#1271425

* Wed Jul  6 2016 Jeff Moyer <jmoyer@redhat.com> - 53.1-4
- Fix up duplicate "-v" documentation in man page
- Fix bogus test in invalidate_namespace_options
- Resolves: bz#1350404 bz#1271425

* Mon Jun 20 2016 Jeff Moyer <jmoyer@redhat.com> - 53.1-3
- make ndctl Require ndctl-libs-{version}-{release}
- Resolves bz#1271425

* Wed Jun  1 2016 Jeff Moyer <jmoyer@redhat.com> - 53.1-2
- initial import for RHEL
- Resolves bz#1271425

* Fri May 27 2016 Dan Williams <dan.j.williams@intel.com> - 53-1
- add daxctl-libs + daxctl-devel packages
- add bash completion

* Mon Apr 04 2016 Dan Williams <dan.j.williams@intel.com> - 52-1
- Initial rpm submission to Fedora
