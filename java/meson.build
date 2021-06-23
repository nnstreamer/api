# Build JNI wrapper (Same code with Android JNI)
jni_include = [join_paths(java_home, 'include'), join_paths(java_home, 'include/linux')]

nns_native_path = join_paths(meson.current_source_dir(), 'android/nnstreamer/src/main/jni')

nns_native_srcs = []
nns_native_srcs += join_paths(nns_native_path, 'nnstreamer-native-api.c')
nns_native_srcs += join_paths(nns_native_path, 'nnstreamer-native-customfilter.c')
nns_native_srcs += join_paths(nns_native_path, 'nnstreamer-native-pipeline.c')
nns_native_srcs += join_paths(nns_native_path, 'nnstreamer-native-singleshot.c')

# Build library
shared_library('nnstreamer-native',
  nns_native_srcs,
  dependencies: [nnstreamer_dep, nns_capi_dep],
  include_directories: jni_include,
  install: true,
  install_dir: api_install_libdir
)
