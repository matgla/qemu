tests = {
  'rp2040_gpio-tests': [],
}

test_env = environment()
test_env.set('G_TEST_SRCDIR', meson.current_source_dir())
test_env.set('G_TEST_BUILDDIR', meson.current_build_dir())

test_deps = {
}

foreach test_name, extra: tests
  src = [test_name + '.c']
  deps = [moduletest]

  exe = executable(test_name, src, genh, dependencies: deps, 
    objects: libs_modules.get('arm-softmmu').get('lib').extract_all_objects(recursive: true),
    link_depends: [qemu_syms, libmoduletest] + libs_modules.get('arm-softmmu').get('link_to'))

  test(test_name, exe,
       depends: test_deps.get(test_name, []),
       env: test_env,
       args: ['--tap', '-k'],
       protocol: 'tap',
       timeout: slow_tests.get(test_name, 30),
       priority: slow_tests.get(test_name, 30),
       suite: ['module'])
endforeach
