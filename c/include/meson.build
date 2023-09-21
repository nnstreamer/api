nns_capi_headers = files('nnstreamer.h', 'nnstreamer-single.h', 'ml-api-common.h')
if get_option('enable-tizen')
  # header for Tizen internal API
  nns_capi_headers += files('nnstreamer-tizen-internal.h')
else
  subdir('platform')
endif

nns_capi_service_headers = files('ml-api-service.h')
nns_capi_experimental_headers = files('ml-api-experimental.h')
