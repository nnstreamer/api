unittest_capi_inference_single = executable('unittest_capi_inference_single',
  'unittest_capi_inference_single.cc',
  dependencies: [nns_capi_single_dep, gtest_dep],
  install: get_option('install-test'),
  install_dir: unittest_install_dir
)
test('unittest_capi_inference_single', unittest_capi_inference_single, env: testenv, timeout: 100)

unittest_capi_inference = executable('unittest_capi_inference',
  'unittest_capi_inference.cc',
  dependencies: [unittest_common_dep],
  install: get_option('install-test'),
  install_dir: unittest_install_dir
)
test('unittest_capi_inference', unittest_capi_inference, env: testenv, timeout: 100)

unittest_capi_inference_latency = executable('unittest_capi_inference_latency',
  'unittest_capi_inference_latency.cc',
  dependencies: [unittest_common_dep],
  install: get_option('install-test'),
  install_dir: unittest_install_dir
)
test('unittest_capi_inference_latency', unittest_capi_inference_latency, env: testenv, timeout: 100)

unittest_capi_datatype_consistency = executable('unittest_capi_datatype_consistency',
  'unittest_capi_datatype_consistency.cc',
  dependencies: [unittest_common_dep],
  install: get_option('install-test'),
  install_dir: unittest_install_dir
)
test('unittest_capi_datatype_consistency', unittest_capi_datatype_consistency, env: testenv, timeout: 100)

if get_option('enable-ml-service')
  unittest_capi_service_extension = executable('unittest_capi_service_extension',
    'unittest_capi_service_extension.cc',
    dependencies: [unittest_common_dep, nns_capi_service_dep],
    install: get_option('install-test'),
    install_dir: unittest_install_dir
  )
  test('unittest_capi_extension', unittest_capi_service_extension, env: testenv, timeout: 100)

  unittest_capi_service_agent_client = executable('unittest_capi_service_agent_client',
    'unittest_capi_service_agent_client.cc',
    dependencies: [unittest_common_dep, nns_capi_service_dep],
    install: get_option('install-test'),
    install_dir: unittest_install_dir
  )
  test('unittest_capi_service_agent_client', unittest_capi_service_agent_client, env: testenv, timeout: 100)

  if support_service_offloading
    unittest_capi_service_offloading = executable('unittest_capi_service_offloading',
      'unittest_capi_service_offloading.cc',
      dependencies: [unittest_common_dep, nns_capi_service_dep],
      install: get_option('install-test'),
      install_dir: unittest_install_dir
    )
    test('unittest_capi_service_offloading', unittest_capi_service_offloading, env: testenv, timeout: 100)

    unittest_capi_service_training_offloading = executable('unittest_capi_service_training_offloading',
      'unittest_capi_service_training_offloading.cc',
      dependencies: [unittest_common_dep, nns_capi_service_dep],
      install: get_option('install-test'),
      install_dir: unittest_install_dir
    )
    test('unittest_capi_service_training_offloading', unittest_capi_service_training_offloading, env: testenv, timeout: 100)
  endif
endif

if nnfw_dep.found()
  unittest_capi_inference_nnfw_runtime = executable('unittest_capi_inference_nnfw_runtime',
    'unittest_capi_inference_nnfw_runtime.cc',
    dependencies: [unittest_common_dep],
    install: get_option('install-test'),
    install_dir: unittest_install_dir
  )
  test('unittest_capi_inference_nnfw_runtime', unittest_capi_inference_nnfw_runtime, env: testenv, timeout: 100)
endif
