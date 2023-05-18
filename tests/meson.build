# Install path for unittest
unittest_base_dir = join_paths(api_install_bindir, 'unittest-ml')
unittest_install_dir = join_paths(unittest_base_dir,'tests')

# Set dependency and test-env
testenv = environment()
testenv.set('MLAPI_SOURCE_ROOT_PATH', meson.source_root())
testenv.set('MLAPI_BUILD_ROOT_PATH', meson.build_root())

unittest_util_static = static_library('unittest_util',
  files('capi/unittest_util.c'),
  dependencies: [glib_dep],
  install: false,
)

unittest_common_dep = declare_dependency(
  link_with: [unittest_util_static],
  dependencies: [nns_capi_dep, gtest_dep],
  compile_args: ['-DFAKEDLOG=1'],
  include_directories: nns_capi_include,
)

if get_option('enable-ml-service')
  subdir('mock')
  subdir('dbus')

  ml_agentd_main_objs = ml_agent_executable.extract_objects(ml_agentd_main_file)
  executable('machine-learning-agent-test',
    test_dbus_impl_srcs,
    dependencies: [lib_ml_agentd_test_dep, gdbus_gen_test_dep],
    install: get_option('install-test'),
    install_dir: unittest_base_dir,
    link_with: lib_unittest_mock,
    objects: ml_agentd_main_objs,
  )

  subdir('services')
  subdir('daemon')
endif

subdir('capi')

if get_option('install-test')
  install_subdir('test_models', install_dir: unittest_install_dir)
endif
