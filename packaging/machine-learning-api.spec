###########################################################################
# Default features for Tizen releases
# If you want to build RPM for other Linux distro, you may need to
# touch these values for your needs.
%define		enable_tizen_privilege 1
%define		enable_tizen_feature 1

###########################################################################
# Conditional features for Tizen releases

# Disable a few features for TV releases
%if "%{?profile}" == "tv"
%define		enable_tizen_privilege 0
%endif

# If it is tizen, we can export Tizen API packages.
%bcond_with tizen

# Note that debug packages generate an additional build and storage cost.
# If you do not need debug packages, run '$ gbs build ... --define "_skip_debug_rpm 1"'.
%if "%{?_skip_debug_rpm}" == "1"
%global debug_package %{nil}
%global __debug_install_post %{nil}
%endif

###########################################################################
# Package / sub-package definitions
Name:		capi-machine-learning-inference
Summary:	Tizen native API for NNStreamer
# Synchronize the version information with NNStreamer.
# 1. Ubuntu : ./debian/changelog
# 2. Tizen  : ./packaging/machine-learning-api.spec
# 3. Meson  : ./meson.build
Version:	1.7.2
Release:	0
Group:		Machine Learning/ML Framework
Packager:	MyungJoo Ham <myungjoo.ham@samsung.com>
License:	Apache-2.0
Provides:	capi-nnstreamer = %{version}-%{release}
Source0:	machine-learning-api-%{version}.tar.gz
Source1001:	capi-machine-learning-inference.manifest

## Define build requirements ##
Requires:	nnstreamer
BuildRequires:	nnstreamer-devel
BuildRequires:	nnstreamer-devel-internal
BuildRequires:	glib2-devel
BuildRequires:	gstreamer-devel
BuildRequires:	gst-plugins-base-devel
BuildRequires:	meson >= 0.50.0

%if %{with tizen}
%if 0%{?enable_tizen_privilege}
BuildRequires:	pkgconfig(dpm)
BuildRequires:	pkgconfig(capi-privacy-privilege-manager)
BuildRequires:	pkgconfig(mm-camcorder)
%if 0%{tizen_version_major} >= 5
BuildRequires:	pkgconfig(mm-resource-manager)
%endif
%endif
BuildRequires:	pkgconfig(capi-system-info)
BuildRequires:	pkgconfig(capi-base-common)
BuildRequires:	pkgconfig(dlog)
%endif # tizen

# For test
%if 0%{?testcoverage}
# to be compatible with gcc-9, lcov should have a higher version than 1.14.1
BuildRequires:	lcov
%endif

%if 0%{?unit_test}
BuildRequires:	gtest-devel
%endif

%description
Tizen ML(Machine Learning) native API for NNStreamer.
You can construct a data stream pipeline with neural networks easily.

%package devel
Summary:	Tizen Native API Devel Kit for NNStreamer
Group:		Machine Learning/ML Framework
Provides:	capi-nnstreamer-devel = %{version}-%{release}
Requires:	capi-machine-learning-inference = %{version}-%{release}
Requires:	capi-machine-learning-common-devel
%description devel
Developmental kit for Tizen Native NNStreamer API.

%package devel-static
Summary:	Static library for Tizen Native API
Group:		Machine Learning/ML Framework
Requires:	capi-machine-learning-inference-devel = %{version}-%{release}
%description devel-static
Static library of capi-machine-learning-inference-devel package.

%package -n capi-machine-learning-common-devel
Summary:	Common headers for Tizen Machine Learning API
Group:		Machine Learning/ML Framework
Provides:	capi-ml-common-devel = %{version}-%{release}
%description -n capi-machine-learning-common-devel
Common headers for Tizen Machine Learning API.

%package -n capi-machine-learning-tizen-internal-devel
Summary:	Tizen internal headers for Tizen Machine Learning API
Group:		Machine Learning/ML Framework
Requires:	capi-machine-learning-inference-devel = %{version}-%{release}
%description -n capi-machine-learning-tizen-internal-devel
Tizen internal headers for Tizen Machine Learning API.

###########################################################################
# Define build options
%define enable_tizen -Denable-tizen=false
%define enable_tizen_privilege_check -Denable-tizen-privilege-check=false
%define enable_tizen_feature_check -Denable-tizen-feature-check=false

%if %{with tizen}
%define enable_tizen -Denable-tizen=true -Dtizen-version-major=0%{tizen_version_major}

%if 0%{?enable_tizen_privilege}
%define enable_tizen_privilege_check -Denable-tizen-privilege-check=true
%endif

%if 0%{?enable_tizen_feature}
%define enable_tizen_feature_check -Denable-tizen-feature-check=true
%endif
%endif # tizen

%prep
%setup -q
cp %{SOURCE1001} .

%build
# Remove compiler flags for meson to decide the cpp version
CXXFLAGS=`echo $CXXFLAGS | sed -e "s|-std=gnu++11||"`

mkdir -p build

meson --buildtype=plain --prefix=%{_prefix} --sysconfdir=%{_sysconfdir} --libdir=%{_libdir} \
	--bindir=%{nnstbindir} --includedir=%{_includedir} \
	%{enable_tizen} %{enable_tizen_privilege_check} %{enable_tizen_feature_check} \
	build

ninja -C build %{?_smp_mflags}

%install
DESTDIR=%{buildroot} ninja -C build %{?_smp_mflags} install

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%manifest capi-machine-learning-inference.manifest
%defattr(-,root,root,-)
%license LICENSE
%{_libdir}/libcapi-nnstreamer.so.*

%files devel
%{_includedir}/nnstreamer/nnstreamer.h
%{_includedir}/nnstreamer/nnstreamer-single.h
%{_libdir}/libcapi-nnstreamer.so
%{_libdir}/pkgconfig/capi-ml-inference.pc

%files devel-static
%{_libdir}/libcapi-nnstreamer.a

%files -n capi-machine-learning-common-devel
%{_includedir}/nnstreamer/ml-api-common.h
%{_libdir}/pkgconfig/capi-ml-common.pc

%files -n capi-machine-learning-tizen-internal-devel
%{_includedir}/nnstreamer/nnstreamer-tizen-internal.h

%changelog
* Fri Feb 05 2021 MyungJoo Ham <myungjoo.ham@samsung.com>
- Started ML API packaging for 1.7.1
