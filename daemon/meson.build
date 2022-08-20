# Machine Learing Agent
if get_option('enable-machine-learning-agent')
  nns_ml_agent_srcs = []
  nns_ml_agent_gen_srcs = []
  nns_ml_agent_incs = include_directories('includes')

  dlog_dep = dependency('dlog')
  libsystemd_dep = dependency('libsystemd')

  nns_ml_agent_srcs += join_paths('main.c')
  nns_ml_agent_srcs += join_paths('modules.c')
  nns_ml_agent_srcs += join_paths('gdbus-util.c')
  nns_ml_agent_srcs += join_paths('service-db.cc')
  nns_ml_agent_srcs += join_paths('pipeline-module.cc')
  nns_ml_agent_srcs += join_paths('model-dbus-impl.cc')

  # Generate GDbus header and code
  gdbus_prog = find_program('gdbus-codegen', required : true)
  gdbus_gen_src = custom_target('gdbus-gencode',
    input : '../dbus/pipeline-dbus.xml',
    output : ['pipeline-dbus.h', 'pipeline-dbus.c'],
    command : [gdbus_prog, '--interface-prefix', 'org.tizen',
              '--generate-c-code', 'pipeline-dbus',
              '--output-directory', meson.current_build_dir(),
              '@INPUT@'])
  nns_ml_agent_gen_srcs += gdbus_gen_src

  gdbus_gen_model_src = custom_target('gdbus-model-gencode',
    input : '../dbus/model-dbus.xml',
    output : ['model-dbus.h', 'model-dbus.c'],
    command : [gdbus_prog, '--interface-prefix', 'org.tizen',
              '--generate-c-code', 'model-dbus',
              '--output-directory', meson.current_build_dir(),
              '@INPUT@'])
  nns_ml_agent_gen_srcs += gdbus_gen_model_src

  # Enable ML Agent Test
  gdbus_gen_test_src = custom_target('gdbus-gencode-test',
    input : '../dbus/test-dbus.xml',
    output : ['test-dbus.h', 'test-dbus.c'],
    command : [gdbus_prog, '--interface-prefix', 'org.tizen',
              '--generate-c-code', 'test-dbus',
              '--output-directory', meson.current_build_dir(),
              '@INPUT@'])

  gdbus_gen_header_dep = declare_dependency(sources : nns_ml_agent_gen_srcs)
  gdbus_gen_header_test_dep = declare_dependency(sources : [nns_ml_agent_gen_srcs, gdbus_gen_test_src])

  ai_service_daemon_deps = [
    gdbus_gen_header_dep,
    glib_dep,
    gio_dep,
    gio_unix_dep,
    gst_dep,
    dlog_dep,
    leveldb_dep,
    libsystemd_dep
  ]

  serviceDBPath = get_option('service-db-path')
  ai_service_daemon = executable('machine-learning-agent',
    nns_ml_agent_srcs,
    dependencies : [ai_service_daemon_deps],
    include_directories: nns_ml_agent_incs,
    install: true,
    install_dir: api_install_bindir,
    cpp_args : '-DDB_PATH="' + serviceDBPath + '"',
    pie : true
  )

  # For unit test
  nns_ml_agent_srcs += join_paths('test-dbus-impl.c')
  ai_service_daemon = executable('machine-learning-agent-test',
    nns_ml_agent_srcs,
    dependencies : [ai_service_daemon_deps, gdbus_gen_header_test_dep],
    include_directories: nns_ml_agent_incs,
    cpp_args : '-DDB_PATH="."'
  )

  # DBus Policy configuration
  dbus_policy_conf = configuration_data()
  configure_file(input: '../dbus/machine-learning-agent.conf.in', output: 'machine-learning-agent.conf',
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
endif
