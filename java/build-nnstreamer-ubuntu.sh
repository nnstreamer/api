#!/usr/bin/env bash

##
## SPDX-License-Identifier: Apache-2.0
##
# @file  build-nnstreamer-ubuntu.sh
# @brief A script to build NNStreamer API library for Ubuntu
#
# The following comments that start with '##@@' are for the generation of usage messages.
##@@ Build script for NNStreamer API Library
##@@  - Before running this script, below variables must be set.
##@@  - JAVA_HOME: Path to JRE
##@@  - ML_API_ROOT: The source root directory of ML API
##@@ 
##@@ usage: build-nnstreamer-ubuntu.sh [OPTIONS]
##@@ 
##@@ basic options:
##@@   --help
##@@       display this help and exit
##@@   --java_home=(path to JRE)
##@@       This option overrides the JAVA_HOME variable
##@@   --ml_api_dir=(the_source_root_of_ml_api)
##@@       This option overrides the ML_API_ROOT variable
##@@   --result_dir=(path_to_build_result)
##@@       Default path is 'ml_api_dir/ubuntu_lib'
##@@ 
##@@ For example, to build and copy library into specific directory
##@@  ./build-nnstreamer-ubuntu.sh --result_dir=<path to result>

# Find '--help' in the given arguments
arg_help="--help"
for arg in "$@"; do
    if [[ ${arg} == ${arg_help} ]]; then
        sed -ne 's/^##@@ \(.*\)/\1/p' $0 && exit 1
    fi
done

# Parse args
for arg in "$@"; do
    case ${arg} in
        --java_home=*)
            java_home=${arg#*=}
            ;;
        --ml_api_dir=*)
            ml_api_dir=${arg#*=}
            ;;
        --result_dir=*)
            result_dir=${arg#*=}
            ;;
    esac
done

# Java home
if [[ -z "${java_home}" ]]; then
    [ -z "${JAVA_HOME}" ] && echo "Need to set JAVA_HOME." && exit 1
    java_home=${JAVA_HOME}
fi

echo "Java home: ${java_home}"

buildtool_javac=${java_home}/bin/javac
buildtool_java=${java_home}/bin/java
buildtool_jar=${java_home}/bin/jar

# ML API root directory
if [[ -z "${ml_api_dir}" ]]; then
    [ -z "${ML_API_ROOT}" ] && echo "Need to set ML_API_ROOT." && exit 1
    ml_api_dir=${ML_API_ROOT}
fi

echo "ML API root directory: ${ml_api_dir}"

# Build result directory
if [[ -z "${result_dir}" ]]; then
    result_dir=${ml_api_dir}/ubuntu_lib
fi

mkdir -p ${result_dir}

# Set library name
today=$(date "+%Y-%m-%d")
nnstreamer_lib_name="nnstreamer-${today}.jar"

echo "NNStreamer library name: ${nnstreamer_lib_name}"

echo "Start to build NNStreamer library for Ubuntu"
pushd ${ml_api_dir}

# Make directory to build NNStreamer library
build_dir=build_ubuntu_jar
mkdir -p ${build_dir}

# Copy java files from Android directory
cp -r ./java/android/nnstreamer/src/main/java/org/nnsuite/nnstreamer/* ./${build_dir}

sed -i "s|android.content.Context|Object|" ${build_dir}/*.java
sed -i "s|android.view.Surface|Object|" ${build_dir}/*.java
sed -i "s|@BUILD_ANDROID@|//|" ${build_dir}/*.java

pushd ${build_dir}

${buildtool_javac} -d . *.java
lib_build_res=$?

if [[ ${lib_build_res} -ne 0 ]]; then
    echo "Failed to build NNStreamer library"
else
    ${buildtool_jar} cvfM ${nnstreamer_lib_name} org/nnsuite/nnstreamer/*.class
    cp ${nnstreamer_lib_name} ${result_dir}
fi

popd

# Remove build directory
rm -rf ${build_dir}

popd

# exit with success/failure status
exit ${lib_build_res}
