# Machine Learning Agent
# This file should be included when ml-service feature is enabled.
ml_agent_incs = include_directories('.', 'include')
ml_agent_lib_common_srcs = files('modules.c', 'gdbus-util.c', 'ml-agent-interface.c',
  'pipeline-dbus-impl.cc', 'model-dbus-impl.cc', 'resource-dbus-impl.cc')
ml_agent_lib_service_db_srcs = files('service-db.cc')

if (get_option('enable-tizen'))
  ml_agent_lib_common_srcs += files('pkg-mgr.cc')
endif

# Generate GDbus header and code
gdbus_prog = find_program('gdbus-codegen', required: true)
gdbus_gen_pipeline_src = custom_target('gdbus-gencode',
  input: pipeline_dbus_input,
  output: ['pipeline-dbus.h', 'pipeline-dbus.c'],
  command: [gdbus_prog, '--interface-prefix', 'org.tizen',
            '--generate-c-code', 'pipeline-dbus',
            '--output-directory', meson.current_build_dir(),
            '@INPUT@'])

gdbus_gen_model_src = custom_target('gdbus-model-gencode',
  input: model_dbus_input,
  output: ['model-dbus.h', 'model-dbus.c'],
  command: [gdbus_prog, '--interface-prefix', 'org.tizen',
            '--generate-c-code', 'model-dbus',
            '--output-directory', meson.current_build_dir(),
            '@INPUT@'])

gdbus_gen_resource_src = custom_target('gdbus-resource-gencode',
  input: resource_dbus_input,
  output: ['resource-dbus.h', 'resource-dbus.c'],
  command: [gdbus_prog, '--interface-prefix', 'org.tizen',
            '--generate-c-code', 'resource-dbus',
            '--output-directory', meson.current_build_dir(),
            '@INPUT@'])

gdbus_gen_header_dep = declare_dependency(
  sources: [gdbus_gen_pipeline_src, gdbus_gen_model_src, gdbus_gen_resource_src])

ml_agent_deps = [
  gdbus_gen_header_dep,
  glib_dep,
  gio_dep,
  gst_dep,
  sqlite_dep,
  libsystemd_dep,
  json_glib_dep
]

if (get_option('enable-tizen'))
  ml_agent_deps += [
    dependency('capi-appfw-app-common'),
    dependency('capi-appfw-package-manager'),
    dependency('dlog')
  ]
endif

serviceDBPath = get_option('service-db-path')
daemon_cpp_db_path_arg = '-DDB_PATH="' + serviceDBPath + '"'

serviceDBKeyPrefix = get_option('service-db-key-prefix')
daemon_cpp_db_key_prefix_arg = '-DMESON_KEY_PREFIX="' + serviceDBKeyPrefix + '"'

ml_agent_lib_srcs = [ml_agent_lib_common_srcs, ml_agent_lib_service_db_srcs]
ml_agent_shared_lib = shared_library ('ml-agent',
  ml_agent_lib_srcs,
  dependencies: ml_agent_deps,
  include_directories: ml_agent_incs,
  install: true,
  install_dir: api_install_libdir,
  cpp_args: [daemon_cpp_db_path_arg, daemon_cpp_db_key_prefix_arg],
  version: api_version,
)
ml_agent_static_lib = static_library('ml-agent',
  ml_agent_lib_srcs,
  dependencies: ml_agent_deps,
  include_directories: ml_agent_incs,
  install: true,
  install_dir: api_install_libdir,
  cpp_args: [daemon_cpp_db_path_arg, daemon_cpp_db_key_prefix_arg],
  pic: true,
)

ml_agent_lib = ml_agent_shared_lib
if get_option('default_library') == 'static'
    ml_agent_lib = ml_agent_static_lib
endif

ml_agent_lib_common_objs = ml_agent_shared_lib.extract_objects(ml_agent_lib_common_srcs)
ml_agent_test_both_lib = both_libraries('ml-agent-test',
  ml_agent_lib_service_db_srcs,
  dependencies: ml_agent_deps,
  include_directories: ml_agent_incs,
  install: get_option('install-test'),
  install_dir: api_install_libdir,
  cpp_args: ['-DDB_PATH="."', daemon_cpp_db_key_prefix_arg],
  objects: ml_agent_lib_common_objs,
  version: api_version,
  pic: true,
)

ml_agent_test_lib = ml_agent_test_both_lib.get_shared_lib()
if get_option('default_library') == 'static'
    ml_agent_test_lib = ml_agent_test_both_lib.get_static_lib()
endif

lib_ml_agent_test_dep = declare_dependency(
  dependencies: ml_agent_deps,
  include_directories: ml_agent_incs,
  link_with: ml_agent_test_lib,
)

subdir('include')
install_headers(ml_agent_headers,
  subdir: 'ml-agent'
)

ml_agent_main_file = files('main.c')
ml_agent_executable = executable('machine-learning-agent',
  ml_agent_main_file,
  link_with: ml_agent_lib,
  dependencies: ml_agent_deps,
  include_directories: ml_agent_incs,
  install: true,
  install_dir: api_install_bindir,
  pie: true
)

# DBus Policy configuration
dbus_policy_conf = configuration_data()
configure_file(input: '../dbus/machine-learning-agent.conf.in',
  output: 'machine-learning-agent.conf',
  install_dir: dbus_policy_dir,
  configuration: dbus_policy_conf
)

# DBus System Service
dbus_system_conf = configuration_data()
configure_file(input: '../dbus/org.tizen.machinelearning.service.service.in',
  output: 'org.tizen.machinelearning.service.service',
  install_dir: dbus_system_service_dir,
  configuration: dbus_system_conf
)

# Systemd Service file
systemd_conf = configuration_data()
configure_file(input: '../dbus/machine-learning-agent.service.in',
  output: 'machine-learning-agent.service',
  install_dir: systemd_service_dir,
  configuration: systemd_conf
)

ml_agent_conf = configuration_data()
ml_agent_conf.merge_from(api_conf)
configure_file(input: 'ml-agent.pc.in', output: 'ml-agent.pc',
  install_dir: join_paths(api_install_libdir, 'pkgconfig'),
  configuration: ml_agent_conf
)
