# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'media_router',
      'type': 'static_library',
      'include_dirs': [
        '<(DEPTH)',
      ],
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/url/url.gyp:url_lib',
      ],
      'sources': [
        'common.h',
        'media_route.cc',
        'media_route.h',
        'media_route_id.h',
        'media_router.h',
        'media_router_impl.cc',
        'media_router_impl.h',
        'media_router_impl_factory.cc',
        'media_router_impl_factory.h',
        'media_routes_observer.cc',
        'media_routes_observer.h',
        'media_sink.cc',
        'media_sink.h',
        'media_sinks_observer.cc',
        'media_sinks_observer.h',
        'media_source.cc',
        'media_source.h',
        'media_source_helper.cc',
        'media_source_helper.h',
        'route_id_manager.cc',
        'route_id_manager.h',
      ],
    },
  ],
}
