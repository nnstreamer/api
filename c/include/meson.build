nns_capi_headers = files('nnstreamer.h', 'nnstreamer-single.h', 'ml-api-common.h')
if get_option('enable-tizen')
  # header for Tizen internal API
  nns_capi_headers += files('nnstreamer-tizen-internal.h')
else
  subdir('platform')
endif

nns_capi_service_headers = files('ml-api-service.h')
# Use staging header for Tizen 7.0 to 8.0. It will be replaced in Tizen 9.0.
if get_option('enable-tizen')
  if ((7 <= tizenVmajor) and (tizenVmajor < 9))
    nns_capi_service_headers += files('ml-api-staging.h')
  endif
endif
