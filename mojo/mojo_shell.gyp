# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [{
    'target_name': 'mojo_shell_lib',
    'type': 'static_library',
    'sources': [
      'shell/application_instance.cc',
      'shell/application_instance.h',
      'shell/application_loader.h',
      'shell/application_manager.cc',
      'shell/application_manager.h',
      'shell/capability_filter.cc',
      'shell/capability_filter.h',
      'shell/connect_to_application_params.cc',
      'shell/connect_to_application_params.h',
      'shell/connect_util.cc',
      'shell/connect_util.h',
      'shell/content_handler_connection.cc',
      'shell/content_handler_connection.h',
      'shell/data_pipe_peek.cc',
      'shell/data_pipe_peek.h',
      'shell/fetcher.cc',
      'shell/fetcher.h',
      'shell/identity.cc',
      'shell/identity.h',
      'shell/native_runner.h',
      'shell/package_manager.h',
      'shell/query_util.cc',
      'shell/query_util.h',
      'shell/static_application_loader.cc',
      'shell/static_application_loader.h',
      'shell/switches.cc',
      'shell/switches.cc',
      'util/filename_util.cc',
      'util/filename_util.h',
    ],
    'dependencies': [
      '<(DEPTH)/base/base.gyp:base',
      '<(DEPTH)/base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
      '<(DEPTH)/mojo/mojo_base.gyp:mojo_application_base',
      '<(DEPTH)/mojo/mojo_base.gyp:mojo_application_bindings',
      '<(DEPTH)/mojo/mojo_base.gyp:mojo_common_lib',
      '<(DEPTH)/mojo/mojo_base.gyp:mojo_environment_chromium',
      '<(DEPTH)/mojo/mojo_base.gyp:mojo_url_type_converters',
      '<(DEPTH)/url/url.gyp:url_lib',
    ],
  }, {
    'target_name': 'mojo_fetcher_lib',
    'type': 'static_library',
    'sources': [
      'fetcher/about_fetcher.cc',
      'fetcher/about_fetcher.h',
      'fetcher/data_fetcher.cc',
      'fetcher/data_fetcher.h',
      'fetcher/local_fetcher.cc',
      'fetcher/local_fetcher.h',
      'fetcher/network_fetcher.cc',
      'fetcher/network_fetcher.h',
      'fetcher/switches.cc',
      'fetcher/switches.h',
      'fetcher/update_fetcher.cc',
      'fetcher/update_fetcher.h',
      'fetcher/url_resolver.cc',
      'fetcher/url_resolver.h',
      'package_manager/package_manager_impl.cc',
      'package_manager/package_manager_impl.h',
    ],
    'dependencies': [
      '<(DEPTH)/base/base.gyp:base',
      '<(DEPTH)/base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
      '<(DEPTH)/crypto/crypto.gyp:crypto',
      '<(DEPTH)/mojo/mojo_base.gyp:mojo_application_base',
      '<(DEPTH)/mojo/mojo_base.gyp:mojo_application_bindings',
      '<(DEPTH)/mojo/mojo_base.gyp:mojo_common_lib',
      '<(DEPTH)/mojo/mojo_base.gyp:mojo_environment_chromium',
      '<(DEPTH)/mojo/mojo_base.gyp:mojo_url_type_converters',
      '<(DEPTH)/mojo/mojo_services.gyp:network_service_bindings_lib',
      '<(DEPTH)/mojo/mojo_services.gyp:updater_bindings_lib',
      '<(DEPTH)/mojo/mojo_shell.gyp:mojo_shell_lib',
      '<(DEPTH)/net/net.gyp:net',
      '<(DEPTH)/url/url.gyp:url_lib',
    ]
  }, {
    'target_name': 'mojo_shell_unittests',
    'type': 'executable',
    'sources': [
      'shell/application_manager_unittest.cc',
      'shell/capability_filter_unittest.cc',
      'shell/query_util_unittest.cc',
      'shell/test_package_manager.cc',
      'shell/test_package_manager.h',
    ],
    'dependencies': [
      '<(DEPTH)/mojo/mojo_shell.gyp:mojo_shell_lib',
      '<(DEPTH)/mojo/mojo_shell.gyp:mojo_shell_test_bindings',
      '<(DEPTH)/base/base.gyp:base',
      '<(DEPTH)/mojo/mojo_base.gyp:mojo_application_base',
      '<(DEPTH)/mojo/mojo_base.gyp:mojo_common_lib',
      '<(DEPTH)/mojo/mojo_base.gyp:mojo_environment_chromium',
      '<(DEPTH)/mojo/mojo_base.gyp:mojo_url_type_converters',
      '<(DEPTH)/third_party/mojo/mojo_edk.gyp:mojo_run_all_unittests',
      '<(DEPTH)/third_party/mojo/mojo_public.gyp:mojo_cpp_bindings',
      '<(DEPTH)/testing/gtest.gyp:gtest',
      '<(DEPTH)/url/url.gyp:url_lib',
    ]
  }, {
    'target_name': 'mojo_shell_test_bindings',
    'type': 'static_library',
    'variables': {
      'mojom_files': [
        'shell/capability_filter_unittest.mojom',
        'shell/test.mojom',
      ],
    },
    'includes': [
      '../third_party/mojo/mojom_bindings_generator_explicit.gypi',
    ],
  }],
}
