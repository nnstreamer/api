# ML (Machine Learning) C-API

nns_capi_include = []
nns_capi_include += include_directories('include')
nns_capi_include += include_directories('src')
if not get_option('enable-tizen')
  nns_capi_include += include_directories('include/platform')
endif

nns_capi_common_srcs = []
nns_capi_single_srcs = []
nns_capi_pipeline_srcs = []
nns_capi_service_srcs = []

nns_capi_common_srcs += join_paths(meson.current_source_dir(), 'src', 'ml-api-common.c')
nns_capi_common_srcs += join_paths(meson.current_source_dir(), 'src', 'ml-api-inference-internal.c')
nns_capi_single_srcs += join_paths(meson.current_source_dir(), 'src', 'ml-api-inference-single.c')
nns_capi_pipeline_srcs += join_paths(meson.current_source_dir(), 'src', 'ml-api-inference-pipeline.c')

if get_option('enable-tizen')
  if get_option('enable-tizen-feature-check')
    nns_capi_common_srcs += join_paths(meson.current_source_dir(), 'src', 'ml-api-common-tizen-feature-check.c')
  endif

  if get_option('enable-tizen-privilege-check')
    nns_capi_common_srcs += join_paths(meson.current_source_dir(), 'src', 'ml-api-inference-tizen-privilege-check.c')
  endif
endif

nns_capi_headers = []
nns_capi_headers += join_paths(meson.current_source_dir(), 'include', 'nnstreamer.h')
nns_capi_headers += join_paths(meson.current_source_dir(), 'include', 'nnstreamer-single.h')
nns_capi_headers += join_paths(meson.current_source_dir(), 'include', 'ml-api-common.h')
nns_capi_headers += join_paths(meson.current_source_dir(), 'include', 'ml-api-service.h')

if get_option('enable-tizen')
  # header for Tizen internal API
  nns_capi_headers += join_paths(meson.current_source_dir(), 'include', 'nnstreamer-tizen-internal.h')
else
  nns_capi_headers += join_paths(meson.current_source_dir(), 'include', 'platform', 'tizen_error.h')
endif


# Dependencies
nns_capi_common_deps = [glib_dep, gmodule_dep, nnstreamer_single_dep]
nns_capi_deps = [nnstreamer_dep, glib_dep, gmodule_dep, gst_dep, gst_app_dep, leveldb_dep]

if (get_option('enable-tizen'))
  message('C-API is in Tizen mode')

  tizen_deps = [
    dependency('capi-system-info'),
  ]

  if get_option('enable-tizen-privilege-check')
    tizen_deps += dependency('dpm')
    tizen_deps += dependency('capi-privacy-privilege-manager')
    tizen_deps += dependency('mm-camcorder')

    if (tizenVmajor >= 5)
      tizen_deps += dependency('mm-resource-manager')
    endif
  endif

  nns_capi_deps += tizen_deps
  nns_capi_common_deps += tizen_deps

  nns_capi_common_deps += [dependency('dlog')]
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
nns_capi_common_dep = declare_dependency(link_with: nns_capi_common_lib)
nns_capi_common_deps += nns_capi_common_dep
nns_capi_deps += nns_capi_common_dep
nns_capi_test_deps = nns_capi_deps

if (get_option('enable-tizen'))
  nns_capi_deps += [dependency('dlog')]
endif


# Single-shot API.
nns_capi_single_shared_lib = shared_library ('capi-ml-inference-single',
  nns_capi_single_srcs,
  dependencies: [nns_capi_common_deps, gobject_dep],
  include_directories: nns_capi_include,
  install: true,
  install_dir: api_install_libdir,
  version: api_version,
)

nns_capi_single_static_lib = static_library ('capi-ml-inference-single',
  nns_capi_single_srcs,
  dependencies: [nns_capi_common_deps, gobject_dep],
  include_directories: nns_capi_include,
  install: true,
  install_dir: api_install_libdir,
)

nns_capi_single_dep = declare_dependency(link_with: nns_capi_single_shared_lib)
if get_option('default_library') == 'static'
  nns_capi_single_dep = declare_dependency(link_with: nns_capi_single_static_lib)
endif


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
  include_directories: nns_capi_include,
)

# Service API
nns_capi_service_shared_lib = shared_library ('capi-ml-service',
  nns_capi_service_srcs,
  dependencies: nns_capi_deps,
  include_directories: nns_capi_include,
  install: true,
  install_dir: api_install_libdir,
  version: api_version,
)

nns_capi_service_static_lib = static_library ('capi-ml-service',
  nns_capi_service_srcs,
  dependencies: nns_capi_deps,
  include_directories: nns_capi_include,
  install: true,
  install_dir: api_install_libdir,
)

nns_capi_service_dep = declare_dependency(link_with: nns_capi_service_shared_lib)
if get_option('default_library') == 'static'
  nns_capi_service_dep = declare_dependency(link_with: nns_capi_service_static_lib)
endif

ml_inf_conf = configuration_data()
ml_inf_conf.merge_from(api_conf)
ml_inf_conf.set('ML_INFERENCE_REQUIRE', 'capi-ml-inference-single')
configure_file(input: 'capi-ml-inference.pc.in', output: 'capi-ml-inference.pc',
  install_dir: join_paths(api_install_libdir, 'pkgconfig'),
  configuration: ml_inf_conf
)

ml_inf_single_conf = configuration_data()
ml_inf_single_conf.merge_from(api_conf)
ml_inf_single_conf.set('ML_INFERENCE_SINGLE_REQUIRE', 'capi-ml-common')
configure_file(input: 'capi-ml-inference-single.pc.in', output: 'capi-ml-inference-single.pc',
  install_dir: join_paths(api_install_libdir, 'pkgconfig'),
  configuration: ml_inf_single_conf
)

ml_inf_service_conf = configuration_data()
ml_inf_service_conf.merge_from(api_conf)
ml_inf_service_conf.set('ML_INFERENCE_SERVICE_REQUIRE', 'capi-nnstreamer')
configure_file(input: 'capi-ml-service.pc.in', output: 'capi-ml-service.pc',
  install_dir: join_paths(api_install_libdir, 'pkgconfig'),
  configuration: ml_inf_service_conf
)

ml_common_api_conf = configuration_data()
ml_common_api_conf.merge_from(api_conf)
if get_option('enable-tizen')
  ml_common_api_conf.set('ML_COMMON_REQUIRE', 'capi-base-common')
else
  ml_common_api_conf.set('ML_COMMON_REQUIRE', '')
endif
configure_file(input: 'capi-ml-common.pc.in', output: 'capi-ml-common.pc',
  install_dir: join_paths(api_install_libdir, 'pkgconfig'),
  configuration: ml_common_api_conf
)

install_headers(nns_capi_headers,
  subdir: 'nnstreamer'
)
