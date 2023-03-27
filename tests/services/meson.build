builddir_cdata = configuration_data()
builddir_cdata.set('build_dir', join_paths(meson.build_root(), 'tests'))

test_dbus_service_file = 'org.tizen.machinelearning.service.service'
configure_file(
  input: test_dbus_service_file + '.in',
  output: test_dbus_service_file,
  configuration: builddir_cdata
)
