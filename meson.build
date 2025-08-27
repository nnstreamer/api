project('ml-api', 'c', 'cpp',
  version: '1.8.8',
  license: ['Apache-2.0'],
  meson_version: '>=0.50.0',
  default_options: [
    'b_asneeded=false',
    'werror=true',
    'warning_level=1',
    'c_std=gnu89',
    'cpp_std=c++14'
  ]
)

add_project_link_arguments('-Wl,--no-as-needed', language: 'c')
add_project_link_arguments('-Wl,--no-as-needed', language: 'cpp')

cc = meson.get_compiler('c')
cxx = meson.get_compiler('cpp')

# Dependencies
glib_dep = dependency('glib-2.0')
gobject_dep = dependency('gobject-2.0')
gmodule_dep = dependency('gmodule-2.0')
if host_machine.system() == 'windows'
  gio_dep = dependency('gio-2.0')
else
  gio_dep = [dependency('gio-2.0'), dependency('gio-unix-2.0')]
endif
gst_dep = dependency('gstreamer-1.0')
gst_app_dep = dependency('gstreamer-app-1.0')
nnstreamer_single_dep = dependency('nnstreamer-single')
nnstreamer_dep = dependency('nnstreamer')

support_nnstreamer_edge = false
support_ml_offloading = false
support_training_offloading = false
if get_option('enable-ml-service')
  json_glib_dep = dependency('json-glib-1.0')
  mlops_agent_dep = dependency('mlops-agent')
  nnstreamer_edge_dep = dependency('nnstreamer-edge', required: false)
  curl_dep = cc.find_library('curl', required: false)

  add_project_arguments('-DENABLE_ML_SERVICE=1', language: ['c', 'cpp'])

  if nnstreamer_edge_dep.found()
    add_project_arguments('-DENABLE_NNSTREAMER_EDGE=1', language: ['c', 'cpp'])
    support_nnstreamer_edge = true

    if curl_dep.found()
      add_project_arguments('-DENABLE_ML_OFFLOADING=1', language: ['c', 'cpp'])
      support_ml_offloading = true

      if get_option('enable-nntrainer')
        add_project_arguments('-DENABLE_TRAINING_OFFLOADING=1', language: ['c', 'cpp'])
        support_training_offloading = true
      endif
    endif
  endif
endif

# Set version info
api_version = meson.project_version()
api_version_split = api_version.split('.')

add_project_arguments('-DVERSION="' + api_version + '"', language: ['c', 'cpp'])
add_project_arguments('-DVERSION_MAJOR=' + api_version_split[0], language: ['c', 'cpp'])
add_project_arguments('-DVERSION_MINOR=' + api_version_split[1], language: ['c', 'cpp'])
add_project_arguments('-DVERSION_MICRO=' + api_version_split[2], language: ['c', 'cpp'])

# Define warning flags for c and cpp
warning_flags = [
  '-Werror=address',
  '-Werror=array-bounds',
  '-Werror=empty-body',
  '-Werror=format=2',
  '-Werror=init-self',
  '-Werror=int-to-pointer-cast',
  '-Werror=main',
  '-Werror=missing-braces',
  '-Werror=nonnull',
  '-Werror=return-type',
  '-Werror=sequence-point',
  '-Werror=trigraphs',
  '-Werror=write-strings',
  '-Wredundant-decls',
  '-Wmissing-braces',
  '-Wmaybe-uninitialized',
  '-Wwrite-strings',
  '-Wformat',
  '-Wformat-nonliteral',
  '-Wformat-security',
  '-Winit-self',
  '-Waddress',
  '-Wno-multichar',
  '-Wvla',
  '-Wpointer-arith'
]

warning_c_flags = [
  '-Werror=implicit',
  '-Werror=pointer-to-int-cast',
  '-Werror=undef',
  '-Wmissing-declarations',
  '-Wmissing-include-dirs',
  '-Wmissing-prototypes',
  '-Wnested-externs',
  '-Waggregate-return',
  '-Wold-style-definition',
  '-Wdeclaration-after-statement'
]

# Setup warning flags for c and cpp
add_project_arguments(cc.get_supported_arguments(warning_c_flags), language: 'c')
add_project_arguments(cc.get_supported_arguments(warning_flags), language: 'c')
add_project_arguments(cxx.get_supported_arguments(warning_flags), language: 'cpp')

# Set project args
if get_option('enable-tizen')
  # Pass __TIZEN__ to the compiler
  add_project_arguments('-D__TIZEN__=1', language: ['c', 'cpp'])

  tizenVmajor = get_option('tizen-version-major')
  tizenVminor = get_option('tizen-version-minor')
  add_project_arguments('-DTIZENVERSION='+tizenVmajor.to_string(), language: ['c', 'cpp'])
  add_project_arguments('-DTIZENVERSIONMINOR='+tizenVminor.to_string(), language: ['c', 'cpp'])

  if get_option('enable-tizen-feature-check')
    add_project_arguments('-D__FEATURE_CHECK_SUPPORT__', language: ['c', 'cpp'])
  endif

  if get_option('enable-tizen-privilege-check')
    add_project_arguments('-D__PRIVILEGE_CHECK_SUPPORT__', language: ['c', 'cpp'])
  endif
endif

if get_option('enable-gcov')
  add_project_arguments('-DENABLE_GCOV=1', language: ['c', 'cpp'])
endif

# Check neural network framework
# TODO: add frameworks required to build and run test
# tendorflow
tf_dep = dependency('tensorflow', required: false)
if tf_dep.found()
  add_project_arguments('-DENABLE_TENSORFLOW=1', language: ['c', 'cpp'])
endif

# tendorflow-lite
tf1lite_dep = dependency('tensorflow-lite', required: false)
if tf1lite_dep.found()
  add_project_arguments('-DENABLE_TENSORFLOW_LITE=1', language: ['c', 'cpp'])
endif

tf2lite_dep = dependency('tensorflow2-lite', required: false)
if tf2lite_dep.found()
  add_project_arguments('-DENABLE_TENSORFLOW2_LITE=1', language: ['c', 'cpp'])
endif

# NNFW (ONE)
nnfw_dep = dependency('nnfw', required: false)
if not nnfw_dep.found()
  nnfw_dep = cc.find_library('nnfw-dev', required: false)
endif
if nnfw_dep.found()
  add_project_arguments('-DENABLE_NNFW_RUNTIME=1', language: ['c', 'cpp'])
endif

# ArmNN
armnn_dep = dependency('armnn', required: false)
if armnn_dep.found()
  add_project_arguments('-DENABLE_ARMNN=1', language: ['c', 'cpp'])
endif

# ONNX Runtime
onnxruntime_dep = dependency('libonnxruntime', required: false)
if onnxruntime_dep.found()
  add_project_arguments('-DENABLE_ONNXRUNTIME=1', language: ['c', 'cpp'])
endif

# ncnn
ncnn_dep = dependency('ncnn', required: false)
if ncnn_dep.found()
  add_project_arguments('-DENABLE_NCNN=1', language: ['c', 'cpp'])
endif

# llama.cpp
llamacpp_dep = dependency('llama', required: false)
if llamacpp_dep.found()
  add_project_arguments('-DENABLE_LLAMACPP=1', language: ['c', 'cpp'])
endif

# Set install path
api_install_prefix = get_option('prefix')
api_install_libdir = join_paths(api_install_prefix, get_option('libdir'))
api_install_bindir = join_paths(api_install_prefix, get_option('bindir'))
api_install_includedir = join_paths(api_install_prefix, get_option('includedir'))
api_install_inidir = get_option('sysconfdir')

# Set default configuration
api_conf = configuration_data()
api_conf.set('VERSION', api_version)
api_conf.set('PREFIX', api_install_prefix)
api_conf.set('EXEC_PREFIX', api_install_bindir)
api_conf.set('LIB_INSTALL_DIR', api_install_libdir)
api_conf.set('INCLUDE_INSTALL_DIR', api_install_includedir)

# Build C-API
subdir('c')

# Build JNI wrapper when developer sets java-home
# (e.g., -Djava-home=$JAVA_HOME from environment variables)
java_home = get_option('java-home').strip()
if java_home != ''
  message('Java home ' + java_home)
  subdir('java')
endif

# Run unittest
if get_option('enable-test')
  gtest_dep = dependency('gtest', required: false)
  if gtest_dep.found()
    subdir('tests')
  else
    warning('The enable-test option requires google-test.')
  endif
endif
