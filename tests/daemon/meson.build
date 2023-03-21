unittest_ml_agent = executable('unittest_ml_agent',
  'unittest_ml_agent.cc',
  dependencies: [unittest_common_dep, ai_service_daemon_deps, gdbus_gen_header_test_dep],
  install: get_option('install-test'),
  install_dir: unittest_install_dir
)
test('unittest_ml_agent', unittest_ml_agent, env: testenv, timeout: 100)

unittest_dbus_model = executable('unittest_dbus_model',
  'unittest_dbus_model.cc',
  dependencies: [unittest_common_dep, ai_service_daemon_deps],
  install: get_option('install-test'),
  install_dir: unittest_install_dir
)
test('unittest_dbus_model', unittest_dbus_model, env: testenv, timeout: 100)

unittest_service_db = executable('unittest_service_db',
  'unittest_service_db.cc',
  dependencies: [unittest_common_dep, service_db_dep_for_test],
  install: get_option('install-test'),
  install_dir: unittest_install_dir
)
test('unittest_service_db', unittest_service_db, env: testenv, timeout: 100)
