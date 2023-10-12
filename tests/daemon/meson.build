unittest_ml_agent = executable('unittest_ml_agent',
  'unittest_ml_agent.cc',
  dependencies: [unittest_common_dep, ml_agent_deps, gdbus_gen_test_dep],
  install: get_option('install-test'),
  install_dir: unittest_install_dir,
  include_directories: [nns_capi_include, ml_agent_incs]
)
test('unittest_ml_agent', unittest_ml_agent, env: testenv, timeout: 100)

unittest_service_db = executable('unittest_service_db',
  'unittest_service_db.cc',
  link_with: ml_agent_test_lib,
  dependencies: [unittest_common_dep, ml_agent_deps],
  install: get_option('install-test'),
  install_dir: unittest_install_dir,
  include_directories: [nns_capi_include, ml_agent_incs],
)
test('unittest_service_db', unittest_service_db, env: testenv, timeout: 100)

unittest_gdbus_util = executable('unittest_gdbus_util',
  'unittest_gdbus_util.cc',
  dependencies: [unittest_common_dep],
  link_with: ml_agent_lib,
  install: get_option('install-test'),
  install_dir: unittest_install_dir,
  include_directories: [ml_agent_incs],
)
test('unittest_gdbus_util', unittest_gdbus_util, env: testenv, timeout: 100)
