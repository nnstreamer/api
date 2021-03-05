unittest_capi_inference = executable('unittest_capi_inference',
  'unittest_capi_inference.cc',
  dependencies: [gtest_dep, nns_capi_dep],
  install: get_option('install-test'),
  install_dir: unittest_install_dir
)
test('unittest_capi_inference', unittest_capi_inference, env: testenv, timeout: 100)

unittest_capi_inference_latency = executable('unittest_capi_inference_latency',
  'unittest_capi_inference_latency.cc',
  dependencies: [gtest_dep, nns_capi_dep],
  install: get_option('install-test'),
  install_dir: unittest_install_dir
)
test('unittest_capi_inference_latency', unittest_capi_inference_latency, env: testenv, timeout: 100)

if nnfw_dep.found()
  unittest_capi_inference_nnfw_runtime = executable('unittest_capi_inference_nnfw_runtime',
    'unittest_capi_inference_nnfw_runtime.cc',
    dependencies: [gtest_dep, nns_capi_dep],
    install: get_option('install-test'),
    install_dir: unittest_install_dir
  )
  test('unittest_capi_inference_nnfw_runtime', unittest_capi_inference_nnfw_runtime, env: testenv, timeout: 100)
endif
