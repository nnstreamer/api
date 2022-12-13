# ML (Machine Learning) C-API
nns_capi_include = []
nns_capi_include += include_directories('include')
nns_capi_include += include_directories('src')
if not get_option('enable-tizen')
  nns_capi_include += include_directories('include/platform')
endif


# Dependencies
nns_capi_common_deps = [glib_dep, gmodule_dep, nnstreamer_single_dep]
nns_capi_deps = [nnstreamer_dep, glib_dep, gmodule_dep, gio_dep, gst_dep, gst_app_dep]

if (get_option('enable-tizen'))
  message('C-API is in Tizen mode')

  tizen_deps = [
    dependency('capi-system-info'),
    dependency('dlog')
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
endif

subdir('include')
subdir('src')

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

if get_option('enable-ml-service')
  ml_inf_service_conf = configuration_data()
  ml_inf_service_conf.merge_from(api_conf)
  ml_inf_service_conf.set('ML_INFERENCE_SERVICE_REQUIRE', 'capi-nnstreamer')
  configure_file(input: 'capi-ml-service.pc.in', output: 'capi-ml-service.pc',
    install_dir: join_paths(api_install_libdir, 'pkgconfig'),
    configuration: ml_inf_service_conf
  )
endif

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

if get_option('enable-ml-service')
  install_headers(nns_capi_service_headers,
    subdir: 'nnstreamer'
  )
endif
