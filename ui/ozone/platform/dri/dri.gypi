# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'internal_ozone_platform_deps': [
      'ozone_platform_dri',
    ],
    'internal_ozone_platform_unittest_deps': [
      'ozone_platform_dri_unittests',
    ],
    'internal_ozone_platforms': [
      'dri',
    ],
  },
  'targets': [
    {
      'target_name': 'ozone_platform_dri',
      'type': 'static_library',
      'dependencies': [
        '../../base/base.gyp:base',
        '../../build/linux/system.gyp:libdrm',
        '../../skia/skia.gyp:skia',
        '../base/ui_base.gyp:ui_base',
        '../display/display.gyp:display_types',
        '../display/display.gyp:display_util',
        '../events/devices/events_devices.gyp:events_devices',
        '../events/events.gyp:events',
        '../events/ozone/events_ozone.gyp:events_ozone_evdev',
        '../events/ozone/events_ozone.gyp:events_ozone_layout',
        '../events/platform/events_platform.gyp:events_platform',
        '../gfx/gfx.gyp:gfx',
        '../gfx/gfx.gyp:gfx_geometry',
      ],
      'defines': [
        'OZONE_IMPLEMENTATION',
      ],
      'sources': [
        'channel_observer.h',
        'crtc_controller.cc',
        'crtc_controller.h',
        'display_change_observer.h',
        'display_manager.cc',
        'display_manager.h',
        'display_mode_dri.cc',
        'display_mode_dri.h',
        'display_snapshot_dri.cc',
        'display_snapshot_dri.h',
        'dri_buffer.cc',
        'dri_buffer.h',
        'dri_console_buffer.cc',
        'dri_console_buffer.h',
        'dri_cursor.cc',
        'dri_cursor.h',
        'dri_gpu_platform_support.cc',
        'dri_gpu_platform_support.h',
        'dri_gpu_platform_support_host.cc',
        'dri_gpu_platform_support_host.h',
        'dri_surface.cc',
        'dri_surface.h',
        'dri_surface_factory.cc',
        'dri_surface_factory.h',
        'dri_util.cc',
        'dri_util.h',
        'dri_vsync_provider.cc',
        'dri_vsync_provider.h',
        'dri_window.cc',
        'dri_window.h',
        'dri_window_delegate.h',
        'dri_window_delegate_impl.cc',
        'dri_window_delegate_impl.h',
        'dri_window_delegate_manager.cc',
        'dri_window_delegate_manager.h',
        'dri_window_manager.cc',
        'dri_window_manager.h',
        'dri_wrapper.cc',
        'dri_wrapper.h',
        'drm_device_generator.cc',
        'drm_device_generator.h',
        'drm_device_manager.cc',
        'drm_device_manager.h',
        'gpu_lock.cc',
        'gpu_lock.h',
        'hardware_display_controller.cc',
        'hardware_display_controller.h',
        'hardware_display_plane.cc',
        'hardware_display_plane.h',
        'hardware_display_plane_manager.cc',
        'hardware_display_plane_manager.h',
        'hardware_display_plane_manager_legacy.cc',
        'hardware_display_plane_manager_legacy.h',
        'native_display_delegate_dri.cc',
        'native_display_delegate_dri.h',
        'native_display_delegate_proxy.cc',
        'native_display_delegate_proxy.h',
        'overlay_plane.cc',
        'overlay_plane.h',
        'ozone_platform_dri.cc',
        'ozone_platform_dri.h',
        'scanout_buffer.h',
        'scoped_drm_types.cc',
        'scoped_drm_types.h',
        'screen_manager.cc',
        'screen_manager.h',
      ],
    },
    {
      'target_name': 'ozone_platform_dri_unittests',
      'type': 'none',
      'dependencies': [
        '../../build/linux/system.gyp:libdrm',
        '../../skia/skia.gyp:skia',
        '../gfx/gfx.gyp:gfx_geometry',
        'ozone.gyp:ozone',
      ],
      'export_dependent_settings': [
        '../../build/linux/system.gyp:libdrm',
        '../../skia/skia.gyp:skia',
        '../gfx/gfx.gyp:gfx_geometry',
      ],
      'direct_dependent_settings': {
        'sources': [
          'dri_surface_unittest.cc',
          'dri_window_delegate_impl_unittest.cc',
          'hardware_display_controller_unittest.cc',
          'hardware_display_plane_manager_unittest.cc',
          'screen_manager_unittest.cc',
          'test/mock_dri_wrapper.cc',
          'test/mock_dri_wrapper.h',
        ],
      },
    },
  ],
}
