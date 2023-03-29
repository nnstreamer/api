project('ml-api', 'c', 'cpp',
  version: '1.8.3',
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
nnstreamer_internal_dep = dependency('nnstreamer-internal')
nnstreamer_single_dep = dependency('nnstreamer-single')
nnstreamer_dep = dependency('nnstreamer')

if get_option('enable-ml-service')
  libsystemd_dep = dependency('libsystemd')
  sqlite_dep = dependency('sqlite3')
  json_glib_dep = dependency('json-glib-1.0')

  if get_option('enable-tizen')
    appfw_package_manager_dep = dependency('capi-appfw-package-manager')
  endif
endif

# Set version info
api_version = meson.project_version()
api_version_split = api_version.split('.')

add_project_arguments('-DVERSION="' + api_version + '"', language: ['c', 'cpp'])
add_project_arguments('-DVERSION_MAJOR="' + api_version_split[0] + '"', language: ['c', 'cpp'])
add_project_arguments('-DVERSION_MINOR="' + api_version_split[1] + '"', language: ['c', 'cpp'])
add_project_arguments('-DVERSION_MICRO="' + api_version_split[2] + '"', language: ['c', 'cpp'])

# Define warning flags for c and cpp
warning_flags = [
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
  '-Wmissing-declarations',
  '-Wmissing-include-dirs',
  '-Wmissing-prototypes',
  '-Wnested-externs',
  '-Waggregate-return',
  '-Wold-style-definition',
  '-Wdeclaration-after-statement'
]

# Setup warning flags for c and cpp
foreach extra_arg : warning_flags
  if cc.has_argument (extra_arg)
    add_project_arguments([extra_arg], language: 'c')
  endif
  if cxx.has_argument (extra_arg)
    add_project_arguments([extra_arg], language: 'cpp')
  endif
endforeach

foreach extra_arg : warning_c_flags
  if cc.has_argument (extra_arg)
    add_project_arguments([extra_arg], language: 'c')
  endif
endforeach

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

serviceDBPath = get_option('service-db-path')
add_project_arguments('-DSYS_DB_DIR="' + serviceDBPath + '"', language: ['c', 'cpp'])

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

# Set install path
api_install_prefix = get_option('prefix')
api_install_libdir = join_paths(api_install_prefix, get_option('libdir'))
api_install_bindir = join_paths(api_install_prefix, get_option('bindir'))
api_install_includedir = join_paths(api_install_prefix, get_option('includedir'))
api_install_inidir = get_option('sysconfdir')
dbus_policy_dir = join_paths(get_option('sysconfdir'), 'dbus-1', 'system.d')
dbus_system_service_dir = join_paths(api_install_prefix, 'share', 'dbus-1', 'system-services')
systemd_service_dir = join_paths(api_install_prefix, 'lib', 'systemd', 'system')

# Set default configuration
api_conf = configuration_data()
api_conf.set('VERSION', api_version)
api_conf.set('PREFIX', api_install_prefix)
api_conf.set('EXEC_PREFIX', api_install_bindir)
api_conf.set('LIB_INSTALL_DIR', api_install_libdir)
api_conf.set('INCLUDE_INSTALL_DIR', api_install_includedir)

if get_option('enable-ml-service')
  subdir('dbus')
  subdir('daemon')
endif

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
  subdir('tests')
endif
