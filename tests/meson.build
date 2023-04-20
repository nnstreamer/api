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

  nns_ml_agent_common_objs = nns_ml_agent_executable.extract_objects(nns_ml_agent_common_srcs)
  executable('machine-learning-agent-test',
    [nns_ml_agent_service_db_srcs, test_dbus_impl_srcs],
    dependencies: [ai_service_daemon_deps, gdbus_test_gen_dep],
    include_directories: nns_ml_agent_incs,
    link_with: lib_unittest_mock,
    objects: [nns_ml_agent_common_objs],
    cpp_args: [daemon_cpp_db_key_prefix_arg, '-DDB_PATH="."']
  )

  test_db_config_args = declare_dependency(
    compile_args: ['-DDB_PATH="."', daemon_cpp_db_key_prefix_arg])

  service_db_dep_for_test = declare_dependency(
    sources: files('../daemon/service-db.cc'),
    dependencies: [glib_dep, sqlite_dep, test_db_config_args],
    include_directories: nns_ml_agent_incs
  )

  subdir('services')
  subdir('daemon')
endif

subdir('capi')

if get_option('install-test')
  install_subdir('test_models', install_dir: unittest_install_dir)
endif
