# Install path for unittest
unittest_base_dir = join_paths(api_install_bindir, 'unittest-ml')
unittest_install_dir = join_paths(unittest_base_dir,'tests')

# Set dependency and test-env
testenv = environment()
testenv.set('MLAPI_SOURCE_ROOT_PATH', meson.source_root())
testenv.set('MLAPI_BUILD_ROOT_PATH', meson.build_root())

gtest_dep = dependency('gtest', required: false)
if gtest_dep.found()
  if get_option('enable-ml-service')
    gdbus_gen_test_src = custom_target('gdbus-gencode-test',
      input: test_dbus_input,
      output: ['test-dbus.h', 'test-dbus.c'],
      command: [gdbus_prog, '--interface-prefix', 'org.tizen',
                '--generate-c-code', 'test-dbus',
                '--output-directory', meson.current_build_dir(),
                '@INPUT@'])

    gdbus_gen_header_test_dep = declare_dependency(sources: gdbus_gen_test_src,
      dependencies: gdbus_gen_header_dep)

    executable('machine-learning-agent-test',
      [nns_ml_agent_srcs, files('daemon/test-dbus-impl.c')],
      dependencies: [ai_service_daemon_deps, gdbus_gen_header_test_dep],
      include_directories: nns_ml_agent_incs,
      cpp_args: [daemon_cpp_db_key_prefix_arg, '-DDB_PATH="."']
    )
  endif

  subdir('capi')
  if get_option('enable-ml-service')
    subdir('services')
    subdir('daemon')
  endif
else
  warning('You should install google-test on your machine.')
endif

if get_option('install-test')
  install_subdir('test_models', install_dir: unittest_install_dir)
endif
