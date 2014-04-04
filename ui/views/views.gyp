# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
{
  'variables': {
    'chromium_code': 1,
  },
  'targets': [
    {
      'target_name': 'views',
      'type': '<(component)',
      'dependencies': [
        '../../base/base.gyp:base',
        '../../base/base.gyp:base_i18n',
        '../../base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '../../skia/skia.gyp:skia',
        '../../third_party/icu/icu.gyp:icui18n',
        '../../third_party/icu/icu.gyp:icuuc',
        '../../url/url.gyp:url_lib',
        '../accessibility/accessibility.gyp:accessibility',
        '../accessibility/accessibility.gyp:ax_gen',
        '../aura/aura.gyp:aura',
        '../base/strings/ui_strings.gyp:ui_strings',
        '../base/ui_base.gyp:ui_base',
        '../compositor/compositor.gyp:compositor',
        '../events/events.gyp:events',
        '../events/events.gyp:events_base',
        '../gfx/gfx.gyp:gfx',
        '../gfx/gfx.gyp:gfx_geometry',
        '../native_theme/native_theme.gyp:native_theme',
        '../resources/ui_resources.gyp:ui_resources',
        '../wm/wm.gyp:wm_core',
      ],
      'export_dependent_settings': [
        '../accessibility/accessibility.gyp:ax_gen',
      ],
      'defines': [
        'VIEWS_IMPLEMENTATION',
      ],
      'sources': [
        # All .cc, .h under views, except unittests
        'accessibility/native_view_accessibility.cc',
        'accessibility/native_view_accessibility.h',
        'accessibility/native_view_accessibility_win.cc',
        'accessibility/native_view_accessibility_win.h',
        'accessible_pane_view.cc',
        'accessible_pane_view.h',
        'animation/bounds_animator.cc',
        'animation/bounds_animator.h',
        'animation/scroll_animator.cc',
        'animation/scroll_animator.h',
        'background.cc',
        'background.h',
        'border.cc',
        'border.h',
        'bubble/bubble_border.cc',
        'bubble/bubble_border.h',
        'bubble/bubble_delegate.cc',
        'bubble/bubble_delegate.h',
        'bubble/bubble_frame_view.cc',
        'bubble/bubble_frame_view.h',
        'bubble/bubble_window_targeter.cc',
        'bubble/bubble_window_targeter.h',
        'bubble/tray_bubble_view.cc',
        'bubble/tray_bubble_view.h',
        'button_drag_utils.cc',
        'button_drag_utils.h',
        'color_chooser/color_chooser_listener.h',
        'color_chooser/color_chooser_view.cc',
        'color_chooser/color_chooser_view.h',
        'color_constants.cc',
        'color_constants.h',
        'context_menu_controller.h',
        'controls/button/blue_button.cc',
        'controls/button/blue_button.h',
        'controls/button/button.cc',
        'controls/button/button.h',
        'controls/button/checkbox.cc',
        'controls/button/checkbox.h',
        'controls/button/custom_button.cc',
        'controls/button/custom_button.h',
        'controls/button/image_button.cc',
        'controls/button/image_button.h',
        'controls/button/label_button.cc',
        'controls/button/label_button.h',
        'controls/button/label_button_border.cc',
        'controls/button/label_button_border.h',
        'controls/button/menu_button.cc',
        'controls/button/menu_button.h',
        'controls/button/menu_button_listener.h',
        'controls/button/radio_button.cc',
        'controls/button/radio_button.h',
        'controls/button/text_button.cc',
        'controls/button/text_button.h',
        'controls/combobox/combobox.cc',
        'controls/combobox/combobox.h',
        'controls/combobox/combobox_listener.h',
        'controls/focusable_border.cc',
        'controls/focusable_border.h',
        'controls/glow_hover_controller.cc',
        'controls/glow_hover_controller.h',
        'controls/image_view.cc',
        'controls/image_view.h',
        'controls/label.cc',
        'controls/label.h',
        'controls/link.cc',
        'controls/link.h',
        'controls/link_listener.h',
        'controls/menu/display_change_listener_aura.cc',
        'controls/menu/menu.cc',
        'controls/menu/menu.h',
        'controls/menu/menu_2.cc',
        'controls/menu/menu_2.h',
        'controls/menu/menu_config.cc',
        'controls/menu/menu_config.h',
        'controls/menu/menu_config_views.cc',
        'controls/menu/menu_config_win.cc',
        'controls/menu/menu_controller.cc',
        'controls/menu/menu_controller.h',
        'controls/menu/menu_controller_delegate.h',
        'controls/menu/menu_delegate.cc',
        'controls/menu/menu_delegate.h',
        'controls/menu/menu_message_pump_dispatcher.cc',
        'controls/menu/menu_message_pump_dispatcher.h',
        'controls/menu/menu_message_pump_dispatcher_linux.cc',
        'controls/menu/menu_message_pump_dispatcher_win.cc',
        'controls/menu/menu_host.cc',
        'controls/menu/menu_host.h',
        'controls/menu/menu_host_root_view.cc',
        'controls/menu/menu_host_root_view.h',
        'controls/menu/menu_insertion_delegate_win.h',
        'controls/menu/menu_item_view.cc',
        'controls/menu/menu_item_view.h',
        'controls/menu/menu_listener.cc',
        'controls/menu/menu_listener.h',
        'controls/menu/menu_model_adapter.cc',
        'controls/menu/menu_model_adapter.h',
        'controls/menu/menu_runner.cc',
        'controls/menu/menu_runner.h',
        'controls/menu/menu_runner_handler.h',
        'controls/menu/menu_scroll_view_container.cc',
        'controls/menu/menu_scroll_view_container.h',
        'controls/menu/menu_separator.h',
        'controls/menu/menu_separator_views.cc',
        'controls/menu/menu_separator_win.cc',
        'controls/menu/menu_wrapper.h',
        'controls/menu/native_menu_win.cc',
        'controls/menu/native_menu_win.h',
        'controls/menu/menu_image_util.cc',
        'controls/menu/menu_image_util.h',
        'controls/menu/submenu_view.cc',
        'controls/menu/submenu_view.h',
        'controls/message_box_view.cc',
        'controls/message_box_view.h',
        'controls/native/native_view_host.cc',
        'controls/native/native_view_host.h',
        'controls/native/native_view_host_aura.cc',
        'controls/native/native_view_host_aura.h',
        'controls/prefix_delegate.h',
        'controls/prefix_selector.cc',
        'controls/prefix_selector.h',
        'controls/progress_bar.cc',
        'controls/progress_bar.h',
        'controls/resize_area.cc',
        'controls/resize_area.h',
        'controls/resize_area_delegate.h',
        'controls/scroll_view.cc',
        'controls/scroll_view.h',
        'controls/scrollbar/base_scroll_bar.cc',
        'controls/scrollbar/base_scroll_bar.h',
        'controls/scrollbar/base_scroll_bar_button.cc',
        'controls/scrollbar/base_scroll_bar_button.h',
        'controls/scrollbar/base_scroll_bar_thumb.cc',
        'controls/scrollbar/base_scroll_bar_thumb.h',
        'controls/scrollbar/kennedy_scroll_bar.cc',
        'controls/scrollbar/kennedy_scroll_bar.h',
        'controls/scrollbar/native_scroll_bar_views.cc',
        'controls/scrollbar/native_scroll_bar_views.h',
        'controls/scrollbar/native_scroll_bar_wrapper.h',
        'controls/scrollbar/native_scroll_bar.cc',
        'controls/scrollbar/native_scroll_bar.h',
        'controls/scrollbar/overlay_scroll_bar.cc',
        'controls/scrollbar/overlay_scroll_bar.h',
        'controls/scrollbar/scroll_bar.cc',
        'controls/scrollbar/scroll_bar.h',
        'controls/separator.cc',
        'controls/separator.h',
        'controls/single_split_view.cc',
        'controls/single_split_view.h',
        'controls/single_split_view_listener.h',
        'controls/slide_out_view.cc',
        'controls/slide_out_view.h',
        'controls/slider.cc',
        'controls/slider.h',
        'controls/styled_label.cc',
        'controls/styled_label.h',
        'controls/styled_label_listener.h',
        'controls/tabbed_pane/tabbed_pane.cc',
        'controls/tabbed_pane/tabbed_pane.h',
        'controls/tabbed_pane/tabbed_pane_listener.h',
        'controls/table/table_header.cc',
        'controls/table/table_header.h',
        'controls/table/table_utils.cc',
        'controls/table/table_utils.h',
        'controls/table/table_view.cc',
        'controls/table/table_view.h',
        'controls/table/table_view_observer.h',
        'controls/table/table_view_row_background_painter.h',
        'controls/textfield/textfield.cc',
        'controls/textfield/textfield.h',
        'controls/textfield/textfield_controller.cc',
        'controls/textfield/textfield_controller.h',
        'controls/textfield/textfield_model.cc',
        'controls/textfield/textfield_model.h',
        'controls/throbber.cc',
        'controls/throbber.h',
        'controls/tree/tree_view.cc',
        'controls/tree/tree_view.h',
        'controls/tree/tree_view_controller.cc',
        'controls/tree/tree_view_controller.h',
        'corewm/tooltip.h',
        'corewm/tooltip_aura.cc',
        'corewm/tooltip_aura.h',
        'corewm/tooltip_controller.cc',
        'corewm/tooltip_controller.h',
        'corewm/tooltip_win.cc',
        'corewm/tooltip_win.h',
        'debug_utils.cc',
        'debug_utils.h',
        'drag_controller.h',
        'drag_utils.cc',
        'drag_utils.h',
        'event_utils.h',
        'event_utils_aura.cc',
        'event_utils_win.cc',
        'focus/external_focus_tracker.cc',
        'focus/external_focus_tracker.h',
        'focus/focus_manager.cc',
        'focus/focus_manager.h',
        'focus/focus_manager_delegate.h',
        'focus/focus_manager_factory.cc',
        'focus/focus_manager_factory.h',
        'focus/focus_search.cc',
        'focus/focus_search.h',
        'focus/view_storage.cc',
        'focus/view_storage.h',
        'focus/widget_focus_manager.cc',
        'focus/widget_focus_manager.h',
        'ime/input_method_base.cc',
        'ime/input_method_base.h',
        'ime/input_method_bridge.cc',
        'ime/input_method_bridge.h',
        'ime/input_method_delegate.h',
        'ime/input_method.h',
        'ime/mock_input_method.cc',
        'ime/mock_input_method.h',
        'layout/box_layout.cc',
        'layout/box_layout.h',
        'layout/fill_layout.cc',
        'layout/fill_layout.h',
        'layout/grid_layout.cc',
        'layout/grid_layout.h',
        'layout/layout_constants.h',
        'layout/layout_manager.cc',
        'layout/layout_manager.h',
        'linux_ui/linux_ui.h',
        'linux_ui/linux_ui.cc',
        'linux_ui/native_theme_change_observer.h',
        'linux_ui/status_icon_linux.h',
        'linux_ui/status_icon_linux.cc',
        'linux_ui/window_button_order_observer.h',
        'metrics.cc',
        'metrics.h',
        'metrics_aura.cc',
        'mouse_constants.h',
        'mouse_watcher.cc',
        'mouse_watcher.h',
        'mouse_watcher_view_host.cc',
        'mouse_watcher_view_host.h',
        'native_theme_delegate.h',
        'painter.cc',
        'painter.h',
        'rect_based_targeting_utils.cc',
        'rect_based_targeting_utils.h',
        'repeat_controller.cc',
        'repeat_controller.h',
        'round_rect_painter.cc',
        'round_rect_painter.h',
        'shadow_border.cc',
        'shadow_border.h',
        'touchui/touch_editing_menu.cc',
        'touchui/touch_editing_menu.h',
        'touchui/touch_selection_controller_impl.cc',
        'touchui/touch_selection_controller_impl.h',
        'view.cc',
        'view.h',
        'view_aura.cc',
        'view_constants.cc',
        'view_constants.h',
        'view_constants_aura.cc',
        'view_constants_aura.h',
        'view_model.cc',
        'view_model.h',
        'view_model_utils.cc',
        'view_model_utils.h',
        'view_targeter.cc',
        'view_targeter.h',
        'views_switches.cc',
        'views_switches.h',
        'views_delegate.cc',
        'views_delegate.h',
        'widget/desktop_aura/desktop_capture_client.cc',
        'widget/desktop_aura/desktop_capture_client.h',
        'widget/desktop_aura/desktop_cursor_loader_updater.h',
        'widget/desktop_aura/desktop_cursor_loader_updater_auralinux.cc',
        'widget/desktop_aura/desktop_cursor_loader_updater_auralinux.h',
        'widget/desktop_aura/desktop_cursor_loader_updater_aurawin.cc',
        'widget/desktop_aura/desktop_dispatcher_client.cc',
        'widget/desktop_aura/desktop_dispatcher_client.h',
        'widget/desktop_aura/desktop_drag_drop_client_aurax11.cc',
        'widget/desktop_aura/desktop_drag_drop_client_aurax11.h',
        'widget/desktop_aura/desktop_drag_drop_client_win.cc',
        'widget/desktop_aura/desktop_drag_drop_client_win.h',
        'widget/desktop_aura/desktop_drop_target_win.cc',
        'widget/desktop_aura/desktop_drop_target_win.h',
        'widget/desktop_aura/desktop_event_client.cc',
        'widget/desktop_aura/desktop_event_client.h',
        'widget/desktop_aura/desktop_factory_ozone.cc',
        'widget/desktop_aura/desktop_factory_ozone.h',
        'widget/desktop_aura/desktop_focus_rules.cc',
        'widget/desktop_aura/desktop_focus_rules.h',
        'widget/desktop_aura/desktop_native_cursor_manager.cc',
        'widget/desktop_aura/desktop_native_cursor_manager.h',
        'widget/desktop_aura/desktop_native_widget_aura.cc',
        'widget/desktop_aura/desktop_native_widget_aura.h',
        'widget/desktop_aura/desktop_window_tree_host.h',
        'widget/desktop_aura/desktop_window_tree_host_ozone.cc',
        'widget/desktop_aura/desktop_window_tree_host_win.cc',
        'widget/desktop_aura/desktop_window_tree_host_win.h',
        'widget/desktop_aura/desktop_window_tree_host_x11.cc',
        'widget/desktop_aura/desktop_window_tree_host_x11.h',
        'widget/desktop_aura/desktop_screen.h',
        'widget/desktop_aura/desktop_screen_ozone.cc',
        'widget/desktop_aura/desktop_screen_position_client.cc',
        'widget/desktop_aura/desktop_screen_position_client.h',
        'widget/desktop_aura/desktop_screen_win.cc',
        'widget/desktop_aura/desktop_screen_win.h',
        'widget/desktop_aura/desktop_screen_x11.cc',
        'widget/desktop_aura/desktop_screen_x11.h',
        'widget/desktop_aura/x11_desktop_handler.cc',
        'widget/desktop_aura/x11_desktop_handler.h',
        'widget/desktop_aura/x11_desktop_window_move_client.cc',
        'widget/desktop_aura/x11_desktop_window_move_client.h',
        'widget/desktop_aura/x11_scoped_capture.cc',
        'widget/desktop_aura/x11_scoped_capture.h',
        'widget/desktop_aura/x11_whole_screen_move_loop.cc',
        'widget/desktop_aura/x11_whole_screen_move_loop.h',
        'widget/desktop_aura/x11_whole_screen_move_loop_delegate.h',
        'widget/desktop_aura/x11_window_event_filter.cc',
        'widget/desktop_aura/x11_window_event_filter.h',
        'widget/drop_helper.cc',
        'widget/drop_helper.h',
        'widget/root_view.cc',
        'widget/root_view.h',
        'widget/monitor_win.cc',
        'widget/monitor_win.h',
        'widget/native_widget.h',
        'widget/native_widget_aura.cc',
        'widget/native_widget_aura.h',
        'widget/native_widget_delegate.h',
        'widget/native_widget_private.h',
        'widget/tooltip_manager_aura.cc',
        'widget/tooltip_manager_aura.h',
        'widget/tooltip_manager.cc',
        'widget/tooltip_manager.h',
        'widget/widget.cc',
        'widget/widget.h',
        'widget/widget_aura_utils.cc',
        'widget/widget_aura_utils.h',
        'widget/widget_delegate.cc',
        'widget/widget_delegate.h',
        'widget/widget_deletion_observer.cc',
        'widget/widget_deletion_observer.h',
        'widget/widget_hwnd_utils.cc',
        'widget/widget_hwnd_utils.h',
        'widget/widget_observer.h',
        'widget/window_reorderer.cc',
        'widget/window_reorderer.h',
        'win/appbar.cc',
        'win/appbar.h',
        'win/fullscreen_handler.cc',
        'win/fullscreen_handler.h',
        'win/hwnd_message_handler.cc',
        'win/hwnd_message_handler.h',
        'win/hwnd_message_handler_delegate.h',
        'win/hwnd_util.h',
        'win/hwnd_util_aurawin.cc',
        'win/scoped_fullscreen_visibility.cc',
        'win/scoped_fullscreen_visibility.h',
        'window/client_view.cc',
        'window/client_view.h',
        'window/custom_frame_view.cc',
        'window/custom_frame_view.h',
        'window/dialog_client_view.cc',
        'window/dialog_client_view.h',
        'window/dialog_delegate.cc',
        'window/dialog_delegate.h',
        'window/frame_background.cc',
        'window/frame_background.h',
        'window/frame_buttons.h',
        'window/native_frame_view.cc',
        'window/native_frame_view.h',
        'window/non_client_view.cc',
        'window/non_client_view.h',
        'window/window_resources.h',
        'window/window_shape.cc',
        'window/window_shape.h',
      ],
      'include_dirs': [
        '../../third_party/wtl/include',
      ],
      'conditions': [
        ['chromeos==1', {
          'sources/': [
            ['exclude', 'widget/desktop_aura'],
          ],
        }],
        ['use_ash==0', {
          'sources!': [
            'bubble/tray_bubble_view.cc',
            'bubble/tray_bubble_view.h',
          ],
        }],
        ['chromeos==0 and use_x11==1', {
          'dependencies': [
            '../display/display.gyp:display',
          ],
        }],
        ['OS=="linux" and chromeos==0', {
          'dependencies': [
            '../shell_dialogs/shell_dialogs.gyp:shell_dialogs',
          ],
        }, { # OS=="linux" and chromeos==0
          'sources/': [
            ['exclude', 'linux_ui'],
          ],
        }],
        ['OS=="win"', {
          'dependencies': [
            # For accessibility
            '../../third_party/iaccessible2/iaccessible2.gyp:iaccessible2',
          ],
          'include_dirs': [
            '../../third_party/wtl/include',
          ],
          'link_settings': {
            'libraries': [
              '-limm32.lib',
              '-loleacc.lib',
            ],
            'msvs_settings': {
              'VCLinkerTool': {
                'DelayLoadDLLs': [
                  'user32.dll',
                ],
              },
            },
          },
          # TODO(jschuh): crbug.com/167187 fix size_t to int truncations.
          'msvs_disabled_warnings': [ 4267, ],
        }],
        ['OS!="win"', {
          'sources!': [
            'controls/menu/menu_wrapper.h',
            'controls/menu/menu_2.cc',
            'controls/menu/menu_2.h',
            'win/fullscreen_handler.cc',
            'win/fullscreen_handler.h',
            'win/hwnd_message_handler.cc',
            'win/hwnd_message_handler.h',
            'win/hwnd_message_handler_delegate.h',
            'win/scoped_fullscreen_visibility.cc',
            'win/scoped_fullscreen_visibility.h',
            'widget/widget_hwnd_utils.cc',
            'widget/widget_hwnd_utils.h',
          ],
        }],
        ['use_ozone==1', {
          'dependencies': [
            '../ozone/ozone.gyp:ozone',
          ],
        }],
        ['use_x11==1', {
          'dependencies': [
            '../../build/linux/system.gyp:x11',
            '../../build/linux/system.gyp:xrandr',
          ],
        }],
      ],
    }, # target_name: views
    {
      'target_name': 'views_test_support',
      'type': 'static_library',
      'dependencies': [
        '../../base/base.gyp:base',
        '../../ipc/ipc.gyp:test_support_ipc',
        '../../skia/skia.gyp:skia',
        '../../testing/gtest.gyp:gtest',
        '../aura/aura.gyp:aura',
        '../aura/aura.gyp:aura_test_support',
        '../base/ui_base.gyp:ui_base',
        '../compositor/compositor.gyp:compositor',
        '../events/events.gyp:events',
        '../gfx/gfx.gyp:gfx',
        '../gfx/gfx.gyp:gfx_geometry',
        '../wm/wm.gyp:wm_core',
        'views',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        'corewm/tooltip_controller_test_helper.cc',
        'corewm/tooltip_controller_test_helper.h',
        'test/capture_tracking_view.cc',
        'test/capture_tracking_view.h',
        'test/child_modal_window.cc',
        'test/child_modal_window.h',
        'test/desktop_test_views_delegate.cc',
        'test/desktop_test_views_delegate.h',
        'test/menu_runner_test_api.cc',
        'test/menu_runner_test_api.h',
        'test/test_views.cc',
        'test/test_views.h',
        'test/test_views_delegate.cc',
        'test/test_views_delegate.h',
        'test/test_widget_observer.cc',
        'test/test_widget_observer.h',
        'test/ui_controls_factory_desktop_aurax11.cc',
        'test/ui_controls_factory_desktop_aurax11.h',
        'test/views_test_base.cc',
        'test/views_test_base.h',
        'test/widget_test.cc',
        'test/widget_test.h',
      ],
      'conditions': [
        ['chromeos==1', {
          'sources!': [
            'test/ui_controls_factory_desktop_aurax11.cc',
            'test/ui_controls_factory_desktop_aurax11.h',
          ],
        }],
      ],
    },  # target_name: views_test_support
    {
      'target_name': 'views_unittests',
      'type': 'executable',
      'dependencies': [
        '../../base/base.gyp:base',
        '../../base/base.gyp:base_i18n',
        '../../base/base.gyp:test_support_base',
        '../../skia/skia.gyp:skia',
        '../../testing/gtest.gyp:gtest',
        '../../third_party/icu/icu.gyp:icui18n',
        '../../third_party/icu/icu.gyp:icuuc',
        '../../url/url.gyp:url_lib',
        '../accessibility/accessibility.gyp:accessibility',
        '../aura/aura.gyp:aura',
        '../aura/aura.gyp:aura_test_support',
        '../base/strings/ui_strings.gyp:ui_strings',
        '../base/ui_base.gyp:ui_base',
        '../compositor/compositor.gyp:compositor',
        '../compositor/compositor.gyp:compositor_test_support',
        '../events/events.gyp:events',
        '../gfx/gfx.gyp:gfx',
        '../gfx/gfx.gyp:gfx_geometry',
        '../resources/ui_resources.gyp:ui_resources',
        '../resources/ui_resources.gyp:ui_test_pak',
        '../ui_unittests.gyp:ui_test_support',
        '../wm/wm.gyp:wm_core',
        'views',
        'views_test_support',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        'accessibility/native_view_accessibility_win_unittest.cc',
        'accessible_pane_view_unittest.cc',
        'animation/bounds_animator_unittest.cc',
        'bubble/bubble_border_unittest.cc',
        'bubble/bubble_delegate_unittest.cc',
        'bubble/bubble_frame_view_unittest.cc',
        'bubble/bubble_window_targeter_unittest.cc',
        'controls/button/custom_button_unittest.cc',
        'controls/button/image_button_unittest.cc',
        'controls/button/label_button_unittest.cc',
        'controls/combobox/combobox_unittest.cc',
        'controls/label_unittest.cc',
        'controls/menu/menu_model_adapter_unittest.cc',
        'controls/native/native_view_host_aura_unittest.cc',
        'controls/native/native_view_host_unittest.cc',
        'controls/prefix_selector_unittest.cc',
        'controls/progress_bar_unittest.cc',
        'controls/scrollbar/scrollbar_unittest.cc',
        'controls/scroll_view_unittest.cc',
        'controls/single_split_view_unittest.cc',
        'controls/slider_unittest.cc',
        'controls/styled_label_unittest.cc',
        'controls/tabbed_pane/tabbed_pane_unittest.cc',
        'controls/table/table_utils_unittest.cc',
        'controls/table/table_view_unittest.cc',
        'controls/table/test_table_model.cc',
        'controls/table/test_table_model.h',
        'controls/textfield/textfield_unittest.cc',
        'controls/textfield/textfield_model_unittest.cc',
        'controls/tree/tree_view_unittest.cc',
        'corewm/capture_controller_unittest.cc',
        'corewm/tooltip_aura_unittest.cc',
        'corewm/tooltip_controller_unittest.cc',
        'focus/focus_manager_test.h',
        'focus/focus_manager_test.cc',
        'focus/focus_manager_unittest.cc',
        'focus/focus_traversal_unittest.cc',
        'ime/input_method_bridge_unittest.cc',
        'layout/box_layout_unittest.cc',
        'layout/grid_layout_unittest.cc',
        'rect_based_targeting_utils_unittest.cc',
        'run_all_unittests.cc',
        'touchui/touch_selection_controller_impl_unittest.cc',
        'view_model_unittest.cc',
        'view_model_utils_unittest.cc',
        'view_unittest.cc',
        'widget/desktop_aura/desktop_focus_rules_unittest.cc',
        'widget/desktop_aura/desktop_native_widget_aura_unittest.cc',
        'widget/desktop_aura/desktop_window_tree_host_win_unittest.cc',
        'widget/desktop_aura/desktop_screen_x11_unittest.cc',
        'widget/desktop_aura/desktop_screen_position_client_unittest.cc',
        'widget/native_widget_aura_unittest.cc',
        'widget/native_widget_unittest.cc',
        'widget/root_view_unittest.cc',
        'widget/widget_unittest.cc',
        'widget/window_reorderer_unittest.cc',
        'window/dialog_client_view_unittest.cc',
        'window/dialog_delegate_unittest.cc',
      ],
      'conditions': [
        ['chromeos==0', {
          'sources!': [
            'touchui/touch_selection_controller_impl_unittest.cc',
          ],
        }, { # use_chromeos==1
          'sources/': [
            ['exclude', 'ime/input_method_bridge_unittest.cc'],
            ['exclude', 'widget/desktop_aura'],
          ],
        }],
        ['OS=="win"', {
          'link_settings': {
            'libraries': [
              '-limm32.lib',
              '-loleacc.lib',
              '-lcomctl32.lib',
            ]
          },
          'include_dirs': [
            '../third_party/wtl/include',
          ],
          'msvs_settings': {
            'VCManifestTool': {
              'AdditionalManifestFiles': [
                '$(ProjectDir)\\test\\views_unittest.manifest',
              ],
            },
          },
        }],
        ['OS=="win" and win_use_allocator_shim==1', {
          'dependencies': [
            '../../base/allocator/allocator.gyp:allocator',
          ],
        }],
        # TODO(dmikurube): Kill linux_use_tcmalloc. http://crbug.com/345554
        ['OS=="linux" and ((use_allocator!="none" and use_allocator!="see_use_tcmalloc") or (use_allocator=="see_use_tcmalloc" and linux_use_tcmalloc==1))', {
           # See http://crbug.com/162998#c4 for why this is needed.
          'dependencies': [
            '../../base/allocator/allocator.gyp:allocator',
          ],
        }],
        ['use_ozone==1', {
          'sources!': [
            'corewm/capture_controller_unittest.cc',
          ],
        }],
      ],
    },  # target_name: views_unittests
  ],
}
