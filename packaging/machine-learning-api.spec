###########################################################################
# Default features for Tizen releases
# If you want to build RPM for other Linux distro, you may need to
# touch these values for your needs.
%define		enable_tizen_privilege 1
%define		enable_tizen_feature 1

%define		tensorflow_support 0
%define		tensorflow_lite_support	1
%define		tensorflow2_lite_support 1
%define		tensorflow2_gpu_delegate_support 1
%define		nnfw_support 1
%define		armnn_support 0
%define		tizen_sensor_support 0

%define		release_test 0
%define		test_script $(pwd)/packaging/run_unittests.sh
###########################################################################
# Conditional features for Tizen releases
%if (0%{?tizen_version_major} == 6 && 0%{?tizen_version_minor} < 5) || (0%{?tizen_version_major} < 6)
%define		tensorflow2_lite_support 0
%define		tensorflow2_gpu_delegate_support 0
%endif

%ifnarch %arm aarch64 x86_64 i586 i686 %ix86
%define		nnfw_support 0
%endif

%ifnarch %arm aarch64
%define		armnn_support 0
%endif

# Disable a few features for TV releases
%if "%{?profile}" == "tv"
%define		enable_tizen_privilege 0
%endif

%if 0%{tizen_version_major} >= 6
%define		tizen_sensor_support 1
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
# Synchronize the version information for ML API.
# 1. Ubuntu  : ./debian/changelog
# 2. Tizen   : ./packaging/machine-learning-api.spec
# 3. Meson   : ./meson.build
# 4. Android : ./java/android/nnstreamer/src/main/jni/Android.mk
Version:	1.7.2
Release:	0
Group:		Machine Learning/ML Framework
Packager:	MyungJoo Ham <myungjoo.ham@samsung.com>
License:	Apache-2.0
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

%if 0%{?tensorflow_support}
BuildRequires:	tensorflow
BuildRequires:	tensorflow-devel
BuildRequires:	nnstreamer-tensorflow
%endif

%if 0%{?tensorflow_lite_support}
BuildRequires:	tensorflow-lite-devel
BuildRequires:	nnstreamer-tensorflow-lite
%endif

%if 0%{?tensorflow2_lite_support}
BuildRequires:	tensorflow2-lite-devel
BuildRequires:	nnstreamer-tensorflow2-lite
%if 0%{?tensorflow2_gpu_delegate_support}
BuildRequires:	pkgconfig(gles20)
%endif
%endif

%if 0%{?nnfw_support}
BuildRequires:	nnfw-devel
BuildRequires:	nnstreamer-nnfw
%ifarch %arm aarch64
BuildRequires:	libarmcl
BuildConflicts:	libarmcl-release
%endif
%endif

%if 0%{?armnn_support}
BuildRequires:	armnn-devel
BuildRequires:	nnstreamer-armnn
BuildRequires:	libarmcl
BuildConflicts:	libarmcl-release
%endif
%endif # unit_test

Requires:	nnstreamer-misc
%if 0%{?tizen_sensor_support}
Requires:	nnstreamer-tizen-sensor
%endif

%description
Tizen ML(Machine Learning) native API for NNStreamer.
You can construct a data stream pipeline with neural networks easily.

%package devel
Summary:	Tizen Native API Devel Kit for NNStreamer
Group:		Machine Learning/ML Framework
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
%description -n capi-machine-learning-common-devel
Common headers for Tizen Machine Learning API.

%package -n capi-machine-learning-tizen-internal-devel
Summary:	Tizen internal headers for Tizen Machine Learning API
Group:		Machine Learning/ML Framework
Requires:	capi-machine-learning-inference-devel = %{version}-%{release}
%description -n capi-machine-learning-tizen-internal-devel
Tizen internal headers for Tizen Machine Learning API.

%if 0%{?release_test}
%package -n capi-machine-learning-unittests
Summary:	Unittests for Tizen Machine Learning API
Group:		Machine Learning/ML Framework
Requires:	capi-machine-learning-inference = %{version}-%{release}
%description -n capi-machine-learning-unittests
Unittests for Tizen Machine Learning API.
%endif
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

%if 0%{?release_test}
%define install_test -Dinstall-test=true
%else
%define install_test -Dinstall-test=false
%endif

%prep
%setup -q
cp %{SOURCE1001} .

%build
# Remove compiler flags for meson to decide the cpp version
CXXFLAGS=`echo $CXXFLAGS | sed -e "s|-std=gnu++11||"`

mkdir -p build

meson --buildtype=plain --prefix=%{_prefix} --sysconfdir=%{_sysconfdir} --libdir=%{_libdir} \
	--bindir=%{_bindir} --includedir=%{_includedir} %{install_test} \
	%{enable_tizen} %{enable_tizen_privilege_check} %{enable_tizen_feature_check} \
	build

ninja -C build %{?_smp_mflags}

# Run test
%if 0%{?unit_test}
bash %{test_script} ./tests/capi/unittest_capi_inference

%if 0%{?nnfw_support}
bash %{test_script} ./tests/capi/unittest_capi_inference_nnfw_runtime
%endif
%endif # unit_test

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

%if 0%{?release_test}
%files -n capi-machine-learning-unittests
%manifest capi-machine-learning-inference.manifest
%{_bindir}/unittest-ml
%endif

%changelog
* Fri Feb 05 2021 MyungJoo Ham <myungjoo.ham@samsung.com>
- Started ML API packaging for 1.7.1
