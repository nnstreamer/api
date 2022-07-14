# Machine Learing Agent
if get_option('enable-machine-learning-agent')
  nns_ml_agent_srcs = []
  nns_ml_agent_incs = include_directories('includes')

  # Generate GDbus header and code
  gdbus_prog = find_program('gdbus-codegen', required : true)
  gdbus_gen_src = custom_target('gdbus-gencode',
    input : '../dbus/pipeline-dbus.xml',
    output : ['pipeline-dbus.h', 'pipeline-dbus.c'],
    command : [gdbus_prog, '--interface-prefix', 'org.tizen',
              '--generate-c-code', 'pipeline-dbus',
              '--output-directory', meson.current_build_dir(),
              '@INPUT@'])

  nns_ml_agent_srcs += gdbus_gen_src[0]
  nns_ml_agent_srcs += join_paths('main.c')
  nns_ml_agent_srcs += join_paths('modules.c')
  nns_ml_agent_srcs += join_paths('gdbus-util.c')
  nns_ml_agent_srcs += join_paths('pipeline-module.c')

  gdbus_gen_header_dep = declare_dependency(sources : gdbus_gen_src)
  dlog_dep = dependency('dlog')
  libsystemd_dep = dependency('libsystemd')
  ai_service_daemon_deps = [
    gdbus_gen_header_dep,
    glib_dep,
    gio_dep,
    gio_unix_dep,
    dlog_dep,
    libsystemd_dep
  ]

  ai_service_daemon = executable('machine-learning-agent',
    nns_ml_agent_srcs,
    dependencies : [ai_service_daemon_deps],
    include_directories: nns_ml_agent_incs,
    install: true,
    install_dir: api_install_bindir,
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
