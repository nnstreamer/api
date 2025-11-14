nns_capi_common_srcs = files('ml-api-common.c', 'ml-api-inference-internal.c')
nns_capi_single_srcs = files('ml-api-inference-single.c')
nns_capi_pipeline_srcs = files('ml-api-inference-pipeline.c')
nns_capi_service_srcs = files('ml-api-service.c', 'ml-api-service-extension.c', 'ml-api-service-agent-client.c', 'ml-lxm-service.c')

if support_nnstreamer_edge
  nns_capi_service_srcs += files('ml-api-service-query.c')

  if support_ml_offloading
    nns_capi_service_srcs += files('ml-api-service-offloading.c')

    if support_training_offloading
      nns_capi_service_srcs += files('ml-api-service-training-offloading.c')
    endif
  endif
endif

if get_option('enable-tizen')
  if get_option('enable-tizen-feature-check')
    nns_capi_common_srcs += files('ml-api-common-tizen-feature-check.c')
  endif

  if get_option('enable-tizen-privilege-check')
    nns_capi_pipeline_srcs += files('ml-api-inference-tizen-privilege-check.c')
  endif
endif

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
  ml_service_deps = [nns_capi_dep, gio_dep, mlops_agent_dep, json_glib_dep]
  if support_nnstreamer_edge
    ml_service_deps += nnstreamer_edge_dep

    if support_ml_offloading
      ml_service_deps += curl_dep
    endif
  endif

  if get_option('enable-tizen')
    ml_service_deps += dependency('capi-appfw-app-common')
  endif

  nns_capi_service_shared_lib = shared_library ('capi-ml-service',
    nns_capi_service_srcs,
    dependencies: ml_service_deps,
    include_directories: nns_capi_include,
    install: true,
    install_dir: api_install_libdir,
    version: api_version
  )

  nns_capi_service_static_lib = static_library ('capi-ml-service',
    nns_capi_service_srcs,
    dependencies: ml_service_deps,
    include_directories: nns_capi_include,
    install: true,
    install_dir: api_install_libdir
  )

  nns_capi_service_lib = nns_capi_service_shared_lib
  if get_option('default_library') == 'static'
    nns_capi_service_lib = nns_capi_service_static_lib
  endif

  nns_capi_service_dep = declare_dependency(link_with: nns_capi_service_lib,
    dependencies: ml_service_deps,
    include_directories: nns_capi_include
  )
endif
