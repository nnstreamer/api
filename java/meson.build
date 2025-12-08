# Build JNI wrapper (Same code with Android JNI)
jni_include = [join_paths(java_home, 'include'), join_paths(java_home, 'include/linux')]

nns_native_path = join_paths(meson.current_source_dir(), 'android/nnstreamer/src/main/jni')

nns_native_srcs = []
nns_native_srcs += join_paths(nns_native_path, 'nnstreamer-native-common.c')
nns_native_srcs += join_paths(nns_native_path, 'nnstreamer-native-api.c')
nns_native_srcs += join_paths(nns_native_path, 'nnstreamer-native-customfilter.c')
nns_native_srcs += join_paths(nns_native_path, 'nnstreamer-native-pipeline.c')
nns_native_srcs += join_paths(nns_native_path, 'nnstreamer-native-singleshot.c')

nns_native_deps = [nnstreamer_dep, nns_capi_dep]

if get_option('enable-ml-service')
  nns_native_srcs += join_paths(nns_native_path, 'nnstreamer-native-service.c')
  nns_native_deps += nns_capi_service_dep
endif

# Build library
shared_library('nnstreamer-native',
  nns_native_srcs,
  dependencies: nns_native_deps,
  include_directories: jni_include,
  install: true,
  install_dir: api_install_libdir
)
