#!/usr/bin/env bash

##
## SPDX-License-Identifier: Apache-2.0
##
# @file  test-nnstreamer-ubuntu.sh
# @brief A script to run the Android instrumented tests of NNStreamer API on Ubuntu
#
# The JNI wrapper is the same code as Android. This script builds the Java API with
# the host stubs in java/host-test, and runs the instrumented tests on a desktop JVM
# against the JNI wrapper built by meson (-Djava-home).
#
# The following comments that start with '##@@' are for the generation of usage messages.
##@@ Test script for NNStreamer API Library on Ubuntu
##@@  - Before running this script, below variables must be set.
##@@  - JAVA_HOME: Path to JDK
##@@  - ML_API_ROOT: The source root directory of ML API
##@@
##@@ usage: test-nnstreamer-ubuntu.sh [OPTIONS]
##@@
##@@ basic options:
##@@   --help
##@@       display this help and exit
##@@   --java_home=(path to JDK)
##@@       This option overrides the JAVA_HOME variable
##@@   --ml_api_dir=(the_source_root_of_ml_api)
##@@       This option overrides the ML_API_ROOT variable
##@@   --build_dir=(meson build directory)
##@@       The meson build directory configured with -Djava-home
##@@       Default path is 'ml_api_dir/build'
##@@   --junit_classpath=(class path of JUnit 4 and Hamcrest)
##@@       Default is '/usr/share/java/junit4.jar:/usr/share/java/hamcrest-core.jar'
##@@
##@@ For example, to run the tests with the default meson build directory
##@@  ./test-nnstreamer-ubuntu.sh --java_home=/usr/lib/jvm/java-17-openjdk-amd64

# Find '--help' in the given arguments
arg_help="--help"
for arg in "$@"; do
    if [[ ${arg} == "${arg_help}" ]]; then
        sed -ne 's/^##@@ \(.*\)/\1/p' "$0" && exit 1
    fi
done

junit_classpath="/usr/share/java/junit4.jar:/usr/share/java/hamcrest-core.jar"

# Parse args
for arg in "$@"; do
    case ${arg} in
        --java_home=*)
            java_home=${arg#*=}
            ;;
        --ml_api_dir=*)
            ml_api_dir=${arg#*=}
            ;;
        --build_dir=*)
            build_dir=${arg#*=}
            ;;
        --junit_classpath=*)
            junit_classpath=${arg#*=}
            ;;
    esac
done

# Java home
if [[ -z "${java_home}" ]]; then
    [ -z "${JAVA_HOME}" ] && echo "Need to set JAVA_HOME." && exit 1
    java_home=${JAVA_HOME}
fi

echo "Java home: ${java_home}"

# ML API root directory
if [[ -z "${ml_api_dir}" ]]; then
    [ -z "${ML_API_ROOT}" ] && echo "Need to set ML_API_ROOT." && exit 1
    ml_api_dir=${ML_API_ROOT}
fi

ml_api_dir=$(realpath "${ml_api_dir}")
echo "ML API root directory: ${ml_api_dir}"

# Meson build directory
if [[ -z "${build_dir}" ]]; then
    build_dir=${ml_api_dir}/build
fi

build_dir=$(realpath "${build_dir}")
native_lib_dir=${build_dir}/java

if [[ ! -f ${native_lib_dir}/libnnstreamer-native.so ]]; then
    echo "Cannot find ${native_lib_dir}/libnnstreamer-native.so, configure meson with -Djava-home and build first."
    exit 1
fi

java_src_dir=${ml_api_dir}/java/android/nnstreamer/src
host_test_dir=${ml_api_dir}/java/host-test

work_dir=$(mktemp -d)
trap 'rm -rf "${work_dir}"' EXIT

src_dir=${work_dir}/src/org/nnsuite/nnstreamer
class_dir=${work_dir}/classes
mkdir -p "${src_dir}" "${class_dir}"

# Java API, same as build-nnstreamer-ubuntu.sh
cp "${java_src_dir}"/main/java/org/nnsuite/nnstreamer/*.java "${src_dir}"
sed -i "s|android.content.Context|Object|" "${src_dir}"/*.java
sed -i "s|android.view.Surface|Object|" "${src_dir}"/*.java
sed -i "s|@BUILD_ANDROID@|//|" "${src_dir}"/*.java

# The host build does not include ml-service.
rm -f "${src_dir}/MLService.java"

# Instrumented tests and host runner
test_files=()
for file in "${java_src_dir}"/androidTest/java/org/nnsuite/nnstreamer/APITest*.java; do
    name=$(basename "${file}" .java)
    [[ ${name} == "APITestMLService" ]] && continue
    cp "${file}" "${src_dir}"
    test_files+=("org.nnsuite.nnstreamer.${name}")
done

cp "${host_test_dir}"/src/org/nnsuite/nnstreamer/*.java "${src_dir}"

echo "Compiling the Java API and the tests."
find "${host_test_dir}/stub" "${work_dir}/src" -name "*.java" > "${work_dir}/sources.txt"

if ! "${java_home}/bin/javac" -encoding UTF-8 -nowarn -d "${class_dir}" \
        -cp "${junit_classpath}" "@${work_dir}/sources.txt"; then
    echo "Failed to compile the tests."
    exit 1
fi

echo "Running the tests."
export LD_LIBRARY_PATH=${build_dir}/c/src${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}

"${java_home}/bin/java" \
    -XX:ErrorFile="${work_dir}/hs_err_pid%p.log" \
    -Djava.library.path="${native_lib_dir}" \
    -cp "${class_dir}:${junit_classpath}" \
    org.nnsuite.nnstreamer.HostTestRunner "${host_test_dir}/exclude.txt" "${test_files[@]}"
test_res=$?

# Show the native stack if the JVM has crashed.
for file in "${work_dir}"/hs_err_pid*.log; do
    [[ -f ${file} ]] && sed -n '1,/^Java frames/p' "${file}"
done

# exit with success/failure status
exit ${test_res}
