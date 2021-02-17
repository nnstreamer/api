# Install path for unittest
unittest_base_dir = join_paths(api_install_bindir, 'unittest-ml')
unittest_install_dir = join_paths(unittest_base_dir,'tests')

# Set dependency and test-env
testenv = environment()
testenv.set('NNSTREAMER_SOURCE_ROOT_PATH', meson.source_root())
testenv.set('NNSTREAMER_BUILD_ROOT_PATH', meson.build_root())

gtest_dep = dependency('gtest', required: false)
if gtest_dep.found()
  subdir('capi')
else
  warning('You should install google-test on your machine.')
endif

if get_option('install-test')
  install_subdir('test_models', install_dir: unittest_install_dir)
endif
