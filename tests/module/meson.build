libqmodule_test_srcs = files(
    'module_test_main.c',
    'qmodule_test.c'
)


libqmodule_test_deps = [qemuutil] + libs_modules.get('arm-softmmu').get('deps')

libqmodule_test = static_library('qmodule_test', libqmodule_test_srcs,
    build_by_default: false,
    include_directories: include_directories('.'),
    dependencies: libqmodule_test_deps
)

qmodule_test = declare_dependency(link_with: libqmodule_test,
    dependencies: libqmodule_test_deps,
    include_directories: include_directories('.'))

subdir('hw')

exe = executable('dummy_test', 'dummy_test.c')

test_env = environment()
test_env.set('G_TEST_SRCDIR', meson.current_source_dir())
test_env.set('G_TEST_BUILDDIR', meson.current_build_dir())

test('dummy_test', exe,
    env: test_env,
    args: ['--tap', '-k'],
    protocol: 'tap',
    timeout: slow_tests.get('dummy_test', 30),
    priority: slow_tests.get('dummy_test', 30),
    suite: ['module'])