# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
{
  'targets': [
    {
      'target_name': 'manager',
      'variables': {
        'depends': [
          '../../../../ui/webui/resources/cr_elements/v1_0/cr_search_field/cr_search_field.js',
          '../../../../ui/webui/resources/js/action_link.js',
          '../../../../ui/webui/resources/js/assert.js',
          '../../../../ui/webui/resources/js/compiled_resources.gyp:load_time_data',
          '../../../../ui/webui/resources/js/cr.js',
          '../../../../ui/webui/resources/js/event_tracker.js',
          '../../../../ui/webui/resources/js/cr/ui.js',
          '../../../../ui/webui/resources/js/cr/ui/command.js',
          '../../../../ui/webui/resources/js/cr/ui/focus_row.js',
          '../../../../ui/webui/resources/js/cr/ui/focus_grid.js',
          '../../../../ui/webui/resources/js/util.js',
          '../downloads/constants.js',
          '../downloads/throttled_icon_loader.js',
          'action_service.js',
          'focus_row.js',
          'item.js',
          'toolbar.js',
        ],
        'externs': [
          '<(EXTERNS_DIR)/chrome_send.js',
          '../downloads/externs.js',
        ],
      },
      'includes': ['../../../../third_party/closure_compiler/compile_js.gypi'],
    }
  ],
}
