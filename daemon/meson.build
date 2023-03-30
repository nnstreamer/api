# Machine Learning Agent
# This file should be included when ml-service feature is enabled.
nns_ml_agent_incs = include_directories('includes')
nns_ml_agent_common_srcs = files('main.c', 'modules.c', 'gdbus-util.c',
  'pipeline-dbus-impl.cc', 'model-dbus-impl.cc')
nns_ml_agent_service_db_srcs = files('service-db.cc')

if (get_option('enable-tizen'))
  nns_ml_agent_common_srcs += files('pkg-mgr.c')
endif

nns_ml_agent_srcs = [nns_ml_agent_common_srcs, nns_ml_agent_service_db_srcs]

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

gdbus_gen_header_dep = declare_dependency(sources: [gdbus_gen_pipeline_src, gdbus_gen_model_src])

ai_service_daemon_deps = [
  gdbus_gen_header_dep,
  glib_dep,
  gio_dep,
  gst_dep,
  sqlite_dep,
  libsystemd_dep
]

if (get_option('enable-tizen'))
  ai_service_daemon_deps += [appfw_package_manager_dep, dependency('dlog')]
endif

serviceDBPath = get_option('service-db-path')
daemon_cpp_db_path_arg = '-DDB_PATH="' + serviceDBPath + '"'

serviceDBKeyPrefix = get_option('service-db-key-prefix')
daemon_cpp_db_key_prefix_arg = '-DMESON_KEY_PREFIX="' + serviceDBKeyPrefix + '"'

nns_ml_agent_executable = executable('machine-learning-agent',
  nns_ml_agent_srcs,
  dependencies: [ai_service_daemon_deps],
  include_directories: nns_ml_agent_incs,
  install: true,
  install_dir: api_install_bindir,
  cpp_args: [daemon_cpp_db_path_arg, daemon_cpp_db_key_prefix_arg],
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
