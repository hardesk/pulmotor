project('pulmotor', 'cpp',
  version : '0.1',
  default_options : ['warning_level=2',
                     'cpp_std=c++20'])

cmake = import('cmake')

# LIBRARIES
ryml_opts = cmake.subproject_options()
ryml_opts.set_override_option('cpp_std', 'c++20')
ryml_opts.add_cmake_defines({'CMAKE_OSX_DEPLOYMENT_TARGET': '12.0'})
ryml_cmake = cmake.subproject('rapidyaml', options: ryml_opts)
ryml_dep = ryml_cmake.dependency('ryml')

doctest_proj = subproject('doctest')
doctest_dep = doctest_proj.get_variable('doctest_dep')

libbase64_opts = cmake.subproject_options()
libbase64_opts.set_override_option('cpp_std', 'c++20')
libbase64_opts.add_cmake_defines({'CMAKE_OSX_DEPLOYMENT_TARGET': '12.0'})
libbase64_cmake = cmake.subproject('libbase64', options: libbase64_opts)
libbase64_dep = libbase64_cmake.dependency('base64')

# PULMOTOR LIBRARY ITSELF
pulmotor_cpp_args = [ '-Wno-unused-parameter', '-Wno-unused-variable', '-Wno-bitwise-op-parentheses' ]
pulmotor_lib = library('pulmotor',
  [
    'src/pulmotor/archive.cpp',
    'src/pulmotor/archive.hpp',
    'src/pulmotor/binary_archive.cpp',
    'src/pulmotor/stream.cpp',
    'src/pulmotor/stream.hpp',
    'src/pulmotor/serialize.hpp',
    'src/pulmotor/util.cpp',
    'src/pulmotor/util.hpp',
    'src/pulmotor/pulmotor_types.cpp',
    'src/pulmotor/pulmotor_types.hpp',
    'src/pulmotor/yaml_util.cpp',
    'src/pulmotor/yaml_emitter.hpp',
    'src/pulmotor/yaml_archive.hpp',
    'src/pulmotor/yaml_archive.cpp',
  ],
  dependencies: [ ryml_dep, doctest_dep ],
  cpp_args: pulmotor_cpp_args,
  install : true)
pulmotor_inc = include_directories('src')
pulmotor_dep = declare_dependency(
  include_directories : pulmotor_inc,
  link_with : pulmotor_lib,
  dependencies: [ ryml_dep ])

# TESTS
# pulmotor_tests = [ 'archive.tests', 'basic.tests', 'serialize.tests', 'stream.tests', 'yaml.tests' ]
pulmotor_tests = [ 'archive.tests', 'basic.tests', 'serialize.tests', 'serialize.std.tests', 'stream.tests', 'yaml.tests' ]
foreach t : pulmotor_tests
  test_cpp_name = 'tests' / (t + '.cpp')
  test_exe = executable(t, test_cpp_name,
    dependencies: [
      pulmotor_dep,
      doctest_dep
    ],
    cpp_args : pulmotor_cpp_args )
  test(t, test_exe)
endforeach

# BENCHMARKS
nanobench_proj = subproject('nanobench')
nanobench_dep = nanobench_proj.get_variable('nanobench_dep')

# zhicheng_base64_proj = subproject('zhicheng_base64')
# zhicheng_base64_dep = zhicheng_base64_proj.get_variable('zhicheng_base64_dep')

pulmotor_benchmarks = [ 'base64.benchmark' ]
foreach b : pulmotor_benchmarks
  bench_cpp_name = 'benchmark' / (b + '.cpp')
  bench_exe = executable(b, bench_cpp_name,
    dependencies: [
      pulmotor_dep,
      nanobench_dep,
      libbase64_dep
    ],
    cpp_args : pulmotor_cpp_args )
endforeach
