# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },
  'targets': [{
    'target_name': 'events_ozone',
    'type': '<(component)',
    'dependencies': [
      '../../../base/base.gyp:base',
      '../../../base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
    ],
    'defines': [
      'EVENTS_OZONE_IMPLEMENTATION',
    ],
    'sources': [
      'chromeos/cursor_controller.cc',
      'chromeos/cursor_controller.h',
      'device/device_event.cc',
      'device/device_event.h',
      'device/device_event_observer.h',
      'device/device_manager.cc',
      'device/device_manager.h',
      'device/device_manager_manual.cc',
      'device/device_manager_manual.h',
      'device/udev/device_manager_udev.cc',
      'device/udev/device_manager_udev.h',
      'events_ozone_export.h',
    ],
    'conditions': [
      ['use_udev==0', {
        'sources/': [
          ['exclude', '_udev\\.(h|cc)$'],
        ],
      }],
      ['use_ozone_evdev==1 and use_udev==1', {
        'dependencies': [
          '<(DEPTH)/device/udev_linux/udev.gyp:udev_linux',
        ],
      }],
    ],
  }, {
    'target_name': 'events_ozone_evdev',
    'type': '<(component)',
    'dependencies': [
      '../../../base/base.gyp:base',
      '../../gfx/gfx.gyp:gfx',
      '../../ozone/ozone.gyp:ozone_base',
      '../devices/events_devices.gyp:events_devices',
      '../events.gyp:dom4_keycode_converter',
      '../platform/events_platform.gyp:events_platform',
      'events_ozone',
      'events_ozone_layout',
    ],
    'defines': [
      'EVENTS_OZONE_EVDEV_IMPLEMENTATION',
      'USE_EVDEV',
    ],
    'direct_dependent_settings': {
      'defines': [
        'USE_EVDEV',
      ],
    },
    'sources': [
      'evdev/libgestures_glue/event_reader_libevdev_cros.cc',
      'evdev/libgestures_glue/event_reader_libevdev_cros.h',
      'evdev/libgestures_glue/gesture_interpreter_libevdev_cros.cc',
      'evdev/libgestures_glue/gesture_interpreter_libevdev_cros.h',
      'evdev/libgestures_glue/gesture_logging.cc',
      'evdev/libgestures_glue/gesture_logging.h',
      'evdev/libgestures_glue/gesture_property_provider.cc',
      'evdev/libgestures_glue/gesture_property_provider.h',
      'evdev/libgestures_glue/gesture_timer_provider.cc',
      'evdev/libgestures_glue/gesture_timer_provider.h',
      'evdev/event_converter_evdev.cc',
      'evdev/event_converter_evdev.h',
      'evdev/event_converter_evdev_impl.cc',
      'evdev/event_converter_evdev_impl.h',
      'evdev/event_device_info.cc',
      'evdev/event_device_info.h',
      'evdev/event_dispatch_callback.cc',
      'evdev/event_dispatch_callback.h',
      'evdev/event_factory_evdev.cc',
      'evdev/event_factory_evdev.h',
      'evdev/event_modifiers_evdev.cc',
      'evdev/event_modifiers_evdev.h',
      'evdev/events_ozone_evdev_export.h',
      'evdev/input_controller_evdev.cc',
      'evdev/input_controller_evdev.h',
      'evdev/input_injector_evdev.cc',
      'evdev/input_injector_evdev.h',
      'evdev/keyboard_evdev.cc',
      'evdev/keyboard_evdev.h',
      'evdev/mouse_button_map_evdev.cc',
      'evdev/mouse_button_map_evdev.h',
      'evdev/tablet_event_converter_evdev.cc',
      'evdev/tablet_event_converter_evdev.h',
      'evdev/touch_event_converter_evdev.cc',
      'evdev/touch_event_converter_evdev.h',
    ],
    'conditions': [
      ['use_ozone_evdev==1 and use_evdev_gestures==1', {
        'dependencies': [
          '<(DEPTH)/build/linux/system.gyp:libgestures',
          '<(DEPTH)/build/linux/system.gyp:libevdev-cros',
        ],
        'defines': [
          'USE_EVDEV_GESTURES',
        ],
        'direct_dependent_settings': {
          'defines': [
            'USE_EVDEV_GESTURES',
          ],
        },
      }, {
        'sources/': [
          ['exclude', '^evdev/libgestures_glue/'],
        ],
      }],
      ['use_ozone_evdev==1', {
        'defines': ['USE_OZONE_EVDEV=1'],
      }],
    ],
  }, {
    'target_name': 'events_ozone_layout',
    'type': '<(component)',
    'dependencies': [
      '../../../base/base.gyp:base',
      '../events.gyp:dom4_keycode_converter',
      '../events.gyp:events_base',
    ],
    'defines': [
      'EVENTS_OZONE_LAYOUT_IMPLEMENTATION',
    ],
    'sources': [
      'layout/events_ozone_layout_export.h',
      'layout/keyboard_layout_engine.h',
      'layout/keyboard_layout_engine_manager.cc',
      'layout/keyboard_layout_engine_manager.h',
      'layout/layout_util.cc',
      'layout/layout_util.h',
      'layout/no/no_keyboard_layout_engine.cc',
      'layout/no/no_keyboard_layout_engine.h',
      'layout/stub/stub_keyboard_layout_engine.cc',
      'layout/stub/stub_keyboard_layout_engine.h',
    ],
    'conditions': [
      ['use_xkbcommon==1', {
        'dependencies': [
          '../../../build/linux/system.gyp:xkbcommon',
        ],
        'defines': [
          'USE_XKBCOMMON',
        ],
        'direct_dependent_settings': {
          'defines': [
            'USE_XKBCOMMON',
          ],
        },
        'sources': [
          'layout/xkb/xkb.h',
          'layout/xkb/xkb_evdev_codes.cc',
          'layout/xkb/xkb_evdev_codes.h',
          'layout/xkb/xkb_key_code_converter.h',
          'layout/xkb/xkb_keyboard_code_conversion.cc',
          'layout/xkb/xkb_keyboard_code_conversion.h',
          'layout/xkb/xkb_keyboard_layout_engine.cc',
          'layout/xkb/xkb_keyboard_layout_engine.h',
          'layout/xkb/xkb_keysym.h',
          'layout/xkb/scoped_xkb.h',
        ],
      }],
    ],
  }]
}
