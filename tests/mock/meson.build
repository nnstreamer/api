lib_unittest_mock = shared_library('unittest_mock',
    files('mock_package_manager.c'),
    dependencies: glib_dep,
    install: get_option('install-test'),
    install_dir: api_install_libdir,
    include_directories: nns_capi_include
)
