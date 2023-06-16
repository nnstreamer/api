nns_capi_common_srcs = files('ml-api-common.c', 'ml-api-inference-internal.c')
if get_option('enable-tizen')
  if get_option('enable-tizen-feature-check')
    nns_capi_common_srcs += files('ml-api-common-tizen-feature-check.c')
  endif

  if get_option('enable-tizen-privilege-check')
    nns_capi_common_srcs += files('ml-api-inference-tizen-privilege-check.c')
  endif
endif

nns_capi_single_srcs = files('ml-api-inference-single.c')
nns_capi_pipeline_srcs = files('ml-api-inference-pipeline.c')
nns_capi_service_srcs = files('ml-api-service-common.c','ml-api-service-agent-client.c', 'ml-api-service-query-client.c')

# Build ML-API Common Lib First.
nns_capi_common_shared_lib = shared_library ('capi-ml-common',
  nns_capi_common_srcs,
  dependencies: nns_capi_common_deps,
  include_directories: nns_capi_include,
  install: true,
  install_dir: api_install_libdir,
  version: api_version,
)
nns_capi_common_static_lib = static_library ('capi-ml-common',
  nns_capi_common_srcs,
  dependencies: nns_capi_common_deps,
  include_directories: nns_capi_include,
  install: true,
  install_dir: api_install_libdir,
)
nns_capi_common_lib = nns_capi_common_shared_lib
if get_option('default_library') == 'static'
  nns_capi_common_lib = nns_capi_common_static_lib
endif

nns_capi_common_dep = declare_dependency(link_with: nns_capi_common_lib,
  dependencies: nns_capi_common_deps,
  include_directories: nns_capi_include
)


# Single-shot API.
nns_capi_single_shared_lib = shared_library ('capi-ml-inference-single',
  nns_capi_single_srcs,
  dependencies: [nns_capi_common_dep, gobject_dep],
  include_directories: nns_capi_include,
  install: true,
  install_dir: api_install_libdir,
  version: api_version,
)

nns_capi_single_static_lib = static_library ('capi-ml-inference-single',
  nns_capi_single_srcs,
  dependencies: [nns_capi_common_dep, gobject_dep],
  include_directories: nns_capi_include,
  install: true,
  install_dir: api_install_libdir,
)

nns_capi_single_lib = nns_capi_single_shared_lib
if get_option('default_library') == 'static'
  nns_capi_single_lib = nns_capi_single_static_lib
endif

nns_capi_single_dep = declare_dependency(link_with: nns_capi_single_lib,
  dependencies: [nns_capi_common_dep, gobject_dep],
  include_directories: nns_capi_include
)


# Pipeline API. (including single-shot API)
nns_capi_shared_lib = shared_library ('capi-nnstreamer',
  nns_capi_pipeline_srcs,
  dependencies: [nns_capi_deps, nns_capi_single_dep],
  include_directories: nns_capi_include,
  install: true,
  install_dir: api_install_libdir,
  version: api_version,
)

nns_capi_static_lib = static_library ('capi-nnstreamer',
  nns_capi_pipeline_srcs,
  dependencies: [nns_capi_deps, nns_capi_single_dep],
  include_directories: nns_capi_include,
  install: true,
  install_dir: api_install_libdir,
)

nns_capi_lib = nns_capi_shared_lib
if get_option('default_library') == 'static'
  nns_capi_lib = nns_capi_static_lib
endif

nns_capi_dep = declare_dependency(link_with: nns_capi_lib,
  dependencies: [nns_capi_deps, nns_capi_single_dep],
  include_directories: nns_capi_include
)


# Service API
if get_option('enable-ml-service')
  nns_capi_service_shared_lib = shared_library ('capi-ml-service',
    nns_capi_service_srcs,
    dependencies: [nns_capi_dep, ml_agentd_deps],
    include_directories: [nns_capi_include, ml_agentd_incs],
    install: true,
    install_dir: api_install_libdir,
    link_with: ml_agentd_lib,
    version: api_version,
  )

  nns_capi_service_static_lib = static_library ('capi-ml-service',
    nns_capi_service_srcs,
    dependencies: [nns_capi_dep, ml_agentd_deps],
    include_directories: [nns_capi_include, ml_agentd_incs],
    install: true,
    link_with: ml_agentd_lib,
    install_dir: api_install_libdir,
  )

  nns_capi_service_lib = nns_capi_service_shared_lib
  if get_option('default_library') == 'static'
    nns_capi_service_lib = nns_capi_service_static_lib
  endif

  nns_capi_service_dep = declare_dependency(link_with: nns_capi_service_lib,
    dependencies: [nns_capi_dep, ml_agentd_deps],
    include_directories: nns_capi_include
  )
endif
