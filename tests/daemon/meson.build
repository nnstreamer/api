unittest_ml_agent = executable('unittest_ml_agent',
  'unittest_ml_agent.cc',
  dependencies: [unittest_common_dep, gdbus_gen_header_test_dep],
  install: get_option('install-test'),
  install_dir: unittest_install_dir
)
test('unittest_ml_agent', unittest_ml_agent, env: testenv, timeout: 100)
