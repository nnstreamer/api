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

  test_db_config_args = declare_dependency(
    compile_args: ['-DDB_PATH="."', daemon_cpp_db_key_prefix_arg])
  ml_agentd_main_objs = ml_agent_executable.extract_objects(ml_agentd_main_file)
  ml_agentd_common_objs = ml_agentd_lib.extract_objects(ml_agentd_lib_common_srcs)
  executable('machine-learning-agent-test',
    [ml_agentd_lib_service_db_srcs, test_dbus_impl_srcs],
    dependencies: [ml_agentd_deps, gdbus_gen_test_dep, test_db_config_args],
    include_directories: ml_agentd_incs,
    link_with: lib_unittest_mock,
    objects: [ml_agentd_common_objs, ml_agentd_main_objs],
  )

  service_db_dep_for_test = declare_dependency(
    sources: ml_agentd_lib_service_db_srcs,
    dependencies: [glib_dep, sqlite_dep, test_db_config_args],
    include_directories: ml_agentd_incs
  )

  subdir('services')
  subdir('daemon')
endif

subdir('capi')

if get_option('install-test')
  install_subdir('test_models', install_dir: unittest_install_dir)
endif
