###########################################################################
# Default features for Tizen releases
# If you want to build RPM for other Linux distro, you may need to
# touch these values for your needs.
%define		enable_tizen_privilege 1
%define		enable_tizen_feature 1

# Below features are used for unittest.
# Do not add neural network dependency in API source.
%define		tensorflow_support 0
%define		tensorflow_lite_support	1
%define		tensorflow2_lite_support 1
%define		tensorflow2_gpu_delegate_support 1
%define		nnfw_support 1
%define		armnn_support 0

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
Version:	1.8.0
Release:	0
Group:		Machine Learning/ML Framework
Packager:	MyungJoo Ham <myungjoo.ham@samsung.com>
License:	Apache-2.0
Source0:	machine-learning-api-%{version}.tar
Source1001:	capi-machine-learning-inference.manifest

## Define build requirements ##
Requires:	capi-machine-learning-inference-single = %{version}-%{release}
Requires:	capi-machine-learning-inference-pipeline = %{version}-%{release}
%ifarch aarch64 x86_64
Provides:	libcapi-nnstreamer.so(64bit)
%else
Provides:	libcapi-nnstreamer.so
%endif
BuildRequires:	nnstreamer-devel
BuildRequires:	nnstreamer-devel-internal
BuildRequires:	glib2-devel
BuildRequires:	gstreamer-devel
BuildRequires:	gst-plugins-base-devel
BuildRequires:	meson >= 0.50.0
BuildRequires:  pkgconfig(leveldb)

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
BuildRequires:	pkgconfig(libtzplatform-config)
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

%description
Tizen ML(Machine Learning) native API for NNStreamer.
You can construct a data stream pipeline with neural networks easily.

%package devel
Summary:	Tizen Native API Devel Kit for NNStreamer
Group:		Machine Learning/ML Framework
Requires:	capi-machine-learning-inference = %{version}-%{release}
Requires:	capi-machine-learning-inference-single-devel
Requires:	capi-machine-learning-inference-pipeline-devel
%description devel
Developmental kit for Tizen Native NNStreamer API.

%package devel-static
Summary:	Static library for Tizen Native API
Group:		Machine Learning/ML Framework
Requires:	capi-machine-learning-inference-devel = %{version}-%{release}
%description devel-static
Static library of capi-machine-learning-inference-devel package.

%package -n capi-machine-learning-common
Summary:	Common utility functions for Tizen Machine Learning API
Group:		Machine Learning/ML Framework
Requires:	nnstreamer
%description -n capi-machine-learning-common
Tizen ML(Machine Learning) native API's common parts.

%package -n capi-machine-learning-common-devel
Summary:	Common headers for Tizen Machine Learning API
Group:		Machine Learning/ML Framework
Requires:	capi-machine-learning-common = %{version}-%{release}
%description -n capi-machine-learning-common-devel
Common headers for Tizen Machine Learning API.

%package -n capi-machine-learning-common-devel-static
Summary:	Static library of common utility functions for Tizen Machine Learning API
Group:		Machine Learning/ML Framework
Requires:	capi-machine-learning-common-devel = %{version}-%{release}
%description -n capi-machine-learning-common-devel-static
Static library of common headers for Tizen Machine Learning API.

%package -n capi-machine-learning-inference-single
Summary:	Tizen Machine Learning Single-shot API
Group:		Machine Learning/ML Framework
Requires:	capi-machine-learning-common = %{version}-%{release}
%description -n capi-machine-learning-inference-single
Tizen Machine Learning Single-shot API.

%package -n capi-machine-learning-inference-single-devel
Summary:	Single-shot headers for Tizen Machine Learning API
Group:		Machine Learning/ML Framework
Requires:	capi-machine-learning-inference-single = %{version}-%{release}
Requires:	capi-machine-learning-common-devel = %{version}-%{release}
%description -n capi-machine-learning-inference-single-devel
Single-shot headers for Tizen Machine Learning API.

%package -n capi-machine-learning-inference-single-devel-static
Summary:	Static library of Tizen Machine Learning Single-shot API
Group:		Machine Learning/ML Framework
Requires:	capi-machine-learning-inference-single = %{version}-%{release}
%description -n capi-machine-learning-inference-single-devel-static
Static library of Tizen Machine Learning Single-shot API.

%package -n capi-machine-learning-inference-pipeline
Summary:	Tizen Machine Learning Pipeline API
Group:		Machine Learning/ML Framework
Requires:	capi-machine-learning-common = %{version}-%{release}
%description -n capi-machine-learning-inference-pipeline
Tizen Machine Learning Pipeline API.

%package -n capi-machine-learning-inference-pipeline-devel
Summary:	pipeline headers for Tizen Machine Learning API
Group:		Machine Learning/ML Framework
Requires:	capi-machine-learning-inference-pipeline = %{version}-%{release}
Requires:	capi-machine-learning-common-devel = %{version}-%{release}
%description -n capi-machine-learning-inference-pipeline-devel
pipeline headers for Tizen Machine Learning API.

%package -n capi-machine-learning-inference-pipeline-devel-static
Summary:	Static library of Tizen Machine Learning Pipeline API
Group:		Machine Learning/ML Framework
Requires:	capi-machine-learning-inference-pipeline = %{version}-%{release}
%description -n capi-machine-learning-inference-pipeline-devel-static
Static library of Tizen Machine Learning Pipeline API.

%package -n capi-machine-learning-service
Summary:	Tizen Machine Learning Service API
Group:		Machine Learning/ML Framework
Requires:	capi-machine-learning-common = %{version}-%{release}
%description -n capi-machine-learning-service
Tizen Machine Learning Service API.

%package -n capi-machine-learning-service-devel
Summary:	ML Service headers for Tizen Machine Learning API
Group:		Machine Learning/ML Framework
Requires:	capi-machine-learning-service = %{version}-%{release}
%description -n capi-machine-learning-service-devel
ML Service headers for Tizen Machine Learning API.

%package -n capi-machine-learning-service-devel-static
Summary:	Static library of Tizen Machine Learning Service API
Group:		Machine Learning/ML Framework
Requires:	capi-machine-learning-service = %{version}-%{release}
%description -n capi-machine-learning-service-devel-static
Static library of Tizen Machine Learning Service API.

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

# To generage gcov package, --define "gcov ON"
%if 0%{?gcov:1}
%package gcov
Summary:    Tizen Machine Learning API gcov objects
Group:		Machine Learning/ML Framework
%description gcov
Tizen Machine Learning API gcov objects.
%endif

%if 0%{?testcoverage}
%package -n capi-machine-learning-unittest-coverage
Summary:	Unittest coverage result for Tizen Machine Learning API
%description -n capi-machine-learning-unittest-coverage
HTML pages of lcov results of ML API generated during rpm build
%endif
###########################################################################
# Define build options
%define enable_tizen -Denable-tizen=false
%define enable_tizen_privilege_check -Denable-tizen-privilege-check=false
%define enable_tizen_feature_check -Denable-tizen-feature-check=false
%define service_db_path ""

%if %{with tizen}
%define enable_tizen -Denable-tizen=true -Dtizen-version-major=0%{?tizen_version_major} -Dtizen-version-minor=0%{?tizen_version_minor}

%if 0%{?enable_tizen_privilege}
%define enable_tizen_privilege_check -Denable-tizen-privilege-check=true
%endif

%if 0%{?enable_tizen_feature}
%define enable_tizen_feature_check -Denable-tizen-feature-check=true
%endif
%define service_db_path -Dservice-db-path=%{TZ_SYS_GLOBALUSER_DB}
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

%if 0%{?testcoverage}
# To test coverage, disable optimizations (and should unset _FORTIFY_SOURCE to use -O0)
CFLAGS=`echo $CFLAGS | sed -e "s|-O[1-9]|-O0|g"`
CFLAGS=`echo $CFLAGS | sed -e "s|-Wp,-D_FORTIFY_SOURCE=[1-9]||g"`
CXXFLAGS=`echo $CXXFLAGS | sed -e "s|-O[1-9]|-O0|g"`
CXXFLAGS=`echo $CXXFLAGS | sed -e "s|-Wp,-D_FORTIFY_SOURCE=[1-9]||g"`
# also, use the meson's base option, -Db_coverage, instead of --coverage/-fprofile-arcs and -ftest-coverage
%define enable_test_coverage -Db_coverage=true
%else
%define enable_test_coverage -Db_coverage=false
%endif

%if 0%{?gcov:1}
export CFLAGS+=" -fprofile-arcs -ftest-coverage"
export CXXFLAGS+=" -fprofile-arcs -ftest-coverage"
%endif

mkdir -p build

meson --buildtype=plain --prefix=%{_prefix} --sysconfdir=%{_sysconfdir} --libdir=%{_libdir} \
	--bindir=%{_bindir} --includedir=%{_includedir} %{install_test} %{enable_test_coverage} \
	%{enable_tizen} %{enable_tizen_privilege_check} %{enable_tizen_feature_check} \
	%{service_db_path} \
	build

ninja -C build %{?_smp_mflags}

# Run test
%if 0%{?unit_test}
bash %{test_script} ./tests/capi/unittest_capi_inference_single
bash %{test_script} ./tests/capi/unittest_capi_inference
bash %{test_script} ./tests/capi/unittest_datatype_consistency

%if 0%{?nnfw_support}
bash %{test_script} ./tests/capi/unittest_capi_inference_nnfw_runtime
%endif
%endif # unit_test

%if 0%{?gcov:1}
mkdir -p gcov-obj
find . -name '*.gcno' -exec cp '{}' gcov-obj ';'
%endif

%install
DESTDIR=%{buildroot} ninja -C build %{?_smp_mflags} install

%if 0%{?gcov:1}
mkdir -p %{buildroot}%{_datadir}/gcov/obj/%{name}
install -m 0644 gcov-obj/* %{buildroot}%{_datadir}/gcov/obj/%{name}
%endif

%if 0%{?testcoverage}
# 'lcov' generates the date format with UTC time zone by default. Let's replace UTC with KST.
# If you can get a root privilege, run ln -sf /usr/share/zoneinfo/Asia/Seoul /etc/localtime
TZ='Asia/Seoul'; export TZ

# Get commit info
VCS=`cat ${RPM_SOURCE_DIR}/machine-learning-api.spec | grep "^VCS:" | sed "s|VCS:\\W*\\(.*\\)|\\1|"`

# Create human readable coverage report web page.
# Create null gcda files if gcov didn't create it because there is completely no unit test for them.
find . -name "*.gcno" -exec sh -c 'touch -a "${1%.gcno}.gcda"' _ {} \;
# Remove gcda for meaningless file (CMake's autogenerated)
find . -name "CMakeCCompilerId*.gcda" -delete
find . -name "CMakeCXXCompilerId*.gcda" -delete
# Generate report
lcov -t 'ML API unittest coverage' -o unittest.info -c -d . -b $(pwd) --no-external
# Exclude generated files (e.g., Orc, Protobuf) and device-dependent files.
lcov -r unittest.info "*/tests/*" "*/meson*/*" "*/*@sha/*" "*/*.so.p/*" "*/*tizen*" -o unittest-filtered.info
# Visualize the report
genhtml -o result unittest-filtered.info -t "ML API %{version}-%{release} ${VCS}" --ignore-errors source -p ${RPM_BUILD_DIR}

mkdir -p %{buildroot}%{_datadir}/ml-api/unittest/
cp -r result %{buildroot}%{_datadir}/ml-api/unittest/
%endif # test coverage

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%manifest capi-machine-learning-inference.manifest
%defattr(-,root,root,-)
%license LICENSE
%{_libdir}/libcapi-nnstreamer.so*

%files devel
%{_libdir}/pkgconfig/capi-ml-inference.pc

%files devel-static
%{_libdir}/libcapi-nnstreamer.a

%files single
%{_libdir}/libcapi-ml-inference-single.so*

%files single-devel
%{_includedir}/nnstreamer/nnstreamer-single.h
%{_libdir}/pkgconfig/capi-ml-inference-single.pc

%files single-devel-static
%{_libdir}/libcapi-ml-inference-single.a

%files pipeline
%{_libdir}/libcapi-ml-inference-pipeline.so*

%files pipeline-devel
%{_includedir}/nnstreamer/nnstreamer.h
%{_libdir}/pkgconfig/capi-ml-inference-pipeline.pc

%files pipeline-devel-static
%{_libdir}/libcapi-ml-inference-pipeline.a

%files -n capi-machine-learning-common
%{_libdir}/libcapi-ml-common.so*

%files -n capi-machine-learning-common-devel
%{_includedir}/nnstreamer/ml-api-common.h
%{_libdir}/pkgconfig/capi-ml-common.pc

%files -n capi-machine-learning-common-devel-static
%{_libdir}/libcapi-ml-common.a

%files -n capi-machine-learning-tizen-internal-devel
%{_includedir}/nnstreamer/nnstreamer-tizen-internal.h

%files -n capi-machine-learning-service
%{_libdir}/libcapi-ml-service.so*

%files -n capi-machine-learning-service-devel
%{_includedir}/nnstreamer/ml-api-service.h
%{_libdir}/pkgconfig/capi-ml-service.pc

%files -n capi-machine-learning-service-devel-static
%{_libdir}/libcapi-ml-service.a

%if 0%{?release_test}
%files -n capi-machine-learning-unittests
%manifest capi-machine-learning-inference.manifest
%{_bindir}/unittest-ml
%endif

%if 0%{?gcov:1}
%files gcov
%{_datadir}/gcov/obj/*
%endif

%if 0%{?testcoverage}
%files -n capi-machine-learning-unittest-coverage
%{_datadir}/ml-api/unittest/*
%endif

%changelog
* Fri Sep 24 2021 MyungJoo Ham <myungjoo.ham@samsung.com>
- Release of 1.8.0

* Fri Feb 05 2021 MyungJoo Ham <myungjoo.ham@samsung.com>
- Started ML API packaging for 1.7.1
