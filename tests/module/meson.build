libmoduletest_srcs = files(
    'module_test_init.c'
)

libmoduletest_deps = [qemuutil] + libs_modules.get('arm-softmmu').get('deps')

libmoduletest = static_library('moduletest', libmoduletest_srcs,
    build_by_default: false,
    include_directories: include_directories('.'),
    dependencies: libmoduletest_deps
)

moduletest = declare_dependency(link_with: libmoduletest,
    dependencies: libmoduletest_deps,
    include_directories: include_directories('.'))

subdir('hw')