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
  # Declare path to bin directory to run mlops-agent for unittest.
  mlops_agent_test_exec_prefix = mlops_agent_dep.get_pkgconfig_variable('exec_prefix', default: api_install_bindir)

  # Set dependency for ml-service unittests.
  service_unittest_deps = [
    unittest_common_dep,
    nns_capi_service_dep,
    declare_dependency(compile_args : '-DEXEC_PREFIX="' + mlops_agent_test_exec_prefix + '"')
  ]

  unittest_capi_service_extension = executable('unittest_capi_service_extension',
    'unittest_capi_service_extension.cc',
    dependencies: service_unittest_deps,
    install: get_option('install-test'),
    install_dir: unittest_install_dir
  )
  test('unittest_capi_extension', unittest_capi_service_extension, env: testenv, timeout: 100)

  unittest_capi_service_agent_client = executable('unittest_capi_service_agent_client',
    'unittest_capi_service_agent_client.cc',
    dependencies: service_unittest_deps,
    install: get_option('install-test'),
    install_dir: unittest_install_dir
  )
  test('unittest_capi_service_agent_client', unittest_capi_service_agent_client, env: testenv, timeout: 100)

  if support_ml_offloading
    unittest_capi_service_offloading = executable('unittest_capi_service_offloading',
      'unittest_capi_service_offloading.cc',
      dependencies: service_unittest_deps,
      install: get_option('install-test'),
      install_dir: unittest_install_dir
    )
    test('unittest_capi_service_offloading', unittest_capi_service_offloading, env: testenv, timeout: 100)

    if support_training_offloading
      unittest_capi_service_training_offloading = executable('unittest_capi_service_training_offloading',
        'unittest_capi_service_training_offloading.cc',
        dependencies: service_unittest_deps,
        install: get_option('install-test'),
        install_dir: unittest_install_dir
      )
      test('unittest_capi_service_training_offloading', unittest_capi_service_training_offloading, env: testenv, timeout: 100)
    endif
  endif

  # LXM Service Tests
  # These tests require both ml-service and llamacpp to be enabled.
  llamacpp_dep = dependency('llama', required: false)
  if llamacpp_dep.found()
    # Note: The source file itself is also conditionally compiled with ENABLE_LLAMACPP.
    unittest_capi_lxm_service = executable('unittest_capi_lxm_service',
      'unittest_capi_lxm_service.cc',
      dependencies: service_unittest_deps,
      install: get_option('install-test'),
      install_dir: unittest_install_dir
    )
    test('unittest_capi_lxm_service', unittest_capi_lxm_service, env: testenv, timeout: 120) # Increased timeout for LLM response
  else
    message('LXM Service tests will be skipped because llama dependency was not found.')
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
