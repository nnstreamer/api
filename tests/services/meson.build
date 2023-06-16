builddir_cdata = configuration_data()
builddir_cdata.set('build_dir', join_paths(meson.build_root(), 'tests'))

test_dbus_service_file = 'org.tizen.machinelearning.service.service'
configure_file(
  input: test_dbus_service_file + '.in',
  output: test_dbus_service_file,
  configuration: builddir_cdata
)

if get_option('install-test')
  installed_builddir_cdata = configuration_data()
  installed_builddir_cdata.set('build_dir', unittest_base_dir)
  test_service_file = configure_file(
    input: test_dbus_service_file + '.in',
    output: test_dbus_service_file + '.install',
    configuration: installed_builddir_cdata
  )
  install_data(
    test_service_file,
    install_dir: join_paths(unittest_install_dir, 'services'),
    install_mode: 'rw-r--r--',
    rename: test_dbus_service_file,
  )
endif
