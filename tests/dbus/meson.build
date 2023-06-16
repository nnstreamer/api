test_dbus_impl_srcs = files('test-dbus-impl.c')
test_dbus_input = files('test-dbus.xml')

gdbus_gen_test_src = custom_target('gdbus-gencode-test',
    input: test_dbus_input,
    output: ['test-dbus.h', 'test-dbus.c'],
    command: [gdbus_prog, '--interface-prefix', 'org.tizen',
            '--generate-c-code', 'test-dbus',
            '--output-directory', meson.current_build_dir(),
            '@INPUT@'])
gdbus_gen_test_dep = declare_dependency(sources: gdbus_gen_test_src,
    dependencies: gdbus_gen_header_dep)

