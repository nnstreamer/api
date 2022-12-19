# Machine Learing Agent
if get_option('enable-ml-service')
  nns_ml_agent_incs = include_directories('includes')
  nns_ml_agent_srcs = files('main.c', 'modules.c', 'gdbus-util.c', 'service-db.cc', 'pipeline-module.cc', 'model-dbus-impl.cc')

  # Generate GDbus header and code
  gdbus_prog = find_program('gdbus-codegen', required : true)
  gdbus_gen_pipeline_src = custom_target('gdbus-gencode',
    input : pipeline_dbus_input,
    output : ['pipeline-dbus.h', 'pipeline-dbus.c'],
    command : [gdbus_prog, '--interface-prefix', 'org.tizen',
              '--generate-c-code', 'pipeline-dbus',
              '--output-directory', meson.current_build_dir(),
              '@INPUT@'])

  gdbus_gen_model_src = custom_target('gdbus-model-gencode',
    input : model_dbus_input,
    output : ['model-dbus.h', 'model-dbus.c'],
    command : [gdbus_prog, '--interface-prefix', 'org.tizen',
              '--generate-c-code', 'model-dbus',
              '--output-directory', meson.current_build_dir(),
              '@INPUT@'])

  gdbus_gen_header_dep = declare_dependency(sources: [gdbus_gen_pipeline_src, gdbus_gen_model_src])

  # Enable ML Agent Test
  if get_option('enable-test')
    gdbus_gen_test_src = custom_target('gdbus-gencode-test',
      input : test_dbus_input,
      output : ['test-dbus.h', 'test-dbus.c'],
      command : [gdbus_prog, '--interface-prefix', 'org.tizen',
                '--generate-c-code', 'test-dbus',
                '--output-directory', meson.current_build_dir(),
                '@INPUT@'])

    gdbus_gen_header_test_dep = declare_dependency(sources: gdbus_gen_test_src,
      dependencies: gdbus_gen_header_dep)
  endif

  ai_service_daemon_deps = [
    gdbus_gen_header_dep,
    glib_dep,
    gio_dep,
    gst_dep,
    leveldb_dep,
    libsystemd_dep
  ]

  if (get_option('enable-tizen'))
    ai_service_daemon_deps += dependency('dlog')
  endif

  serviceDBPath = get_option('service-db-path')
  daemon_cpp_db_path_arg = '-DDB_PATH="' + serviceDBPath + '"'

  serviceDBKeyPrefix = get_option('service-db-key-prefix')
  daemon_cpp_db_key_prefix_arg = '-DMESON_KEY_PREFIX="' + serviceDBKeyPrefix + '"'

  executable('machine-learning-agent',
    nns_ml_agent_srcs,
    dependencies : [ai_service_daemon_deps],
    include_directories: nns_ml_agent_incs,
    install: true,
    install_dir: api_install_bindir,
    cpp_args : [daemon_cpp_db_path_arg, daemon_cpp_db_key_prefix_arg],
    pie : true
  )

  # For unit test
  if get_option('enable-test')
    executable('machine-learning-agent-test',
      [nns_ml_agent_srcs, files('test-dbus-impl.c')],
      dependencies : [ai_service_daemon_deps, gdbus_gen_header_test_dep],
      include_directories: nns_ml_agent_incs,
      cpp_args : [daemon_cpp_db_key_prefix_arg, '-DDB_PATH="."']
    )
  endif

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
