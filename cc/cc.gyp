# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },
  'targets': [
    {
      # GN version: //cc
      'target_name': 'cc',
      'type': '<(component)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '<(DEPTH)/gpu/gpu.gyp:gpu',
        '<(DEPTH)/media/media.gyp:media',
        '<(DEPTH)/skia/skia.gyp:skia',
        '<(DEPTH)/ui/events/events.gyp:events_base',
        '<(DEPTH)/ui/gfx/gfx.gyp:gfx',
        '<(DEPTH)/ui/gfx/gfx.gyp:gfx_geometry',
        '<(DEPTH)/ui/gl/gl.gyp:gl',
      ],
      'variables': {
        'optimize': 'max',
      },
      'export_dependent_settings': [
        '<(DEPTH)/skia/skia.gyp:skia',
      ],
      'defines': [
        'CC_IMPLEMENTATION=1',
      ],
      'sources': [
        # Note: file list duplicated in GN build.
        'animation/animation.cc',
        'animation/animation.h',
        'animation/animation_curve.cc',
        'animation/animation_curve.h',
        'animation/animation_delegate.h',
        'animation/animation_events.cc',
        'animation/animation_events.h',
        'animation/animation_host.cc',
        'animation/animation_host.h',
        'animation/animation_id_provider.cc',
        'animation/animation_id_provider.h',
        'animation/animation_player.cc',
        'animation/animation_player.h',
        'animation/animation_registrar.cc',
        'animation/animation_registrar.h',
        'animation/animation_timeline.cc',
        'animation/animation_timeline.h',
        'animation/element_animations.cc',
        'animation/element_animations.h',
        'animation/keyframed_animation_curve.cc',
        'animation/keyframed_animation_curve.h',
        'animation/layer_animation_controller.cc',
        'animation/layer_animation_controller.h',
        'animation/layer_animation_event_observer.h',
        'animation/layer_animation_value_observer.h',
        'animation/layer_animation_value_provider.h',
        'animation/scroll_offset_animation_curve.cc',
        'animation/scroll_offset_animation_curve.h',
        'animation/scrollbar_animation_controller.cc',
        'animation/scrollbar_animation_controller.h',
        'animation/scrollbar_animation_controller_linear_fade.cc',
        'animation/scrollbar_animation_controller_linear_fade.h',
        'animation/scrollbar_animation_controller_thinning.cc',
        'animation/scrollbar_animation_controller_thinning.h',
        'animation/timing_function.cc',
        'animation/timing_function.h',
        'animation/transform_operation.cc',
        'animation/transform_operation.h',
        'animation/transform_operations.cc',
        'animation/transform_operations.h',
        'base/completion_event.h',
        'base/delayed_unique_notifier.cc',
        'base/delayed_unique_notifier.h',
        'base/histograms.cc',
        'base/histograms.h',
        'base/invalidation_region.cc',
        'base/invalidation_region.h',
        'base/list_container.cc',
        'base/list_container.h',
        'base/math_util.cc',
        'base/math_util.h',
        'base/region.cc',
        'base/region.h',
        'base/resource_id.h',
        'base/rolling_time_delta_history.cc',
        'base/rolling_time_delta_history.h',
        'base/scoped_ptr_algorithm.h',
        'base/scoped_ptr_deque.h',
        'base/scoped_ptr_vector.h',
        'base/sidecar_list_container.h',
        'base/simple_enclosed_region.cc',
        'base/simple_enclosed_region.h',
        'base/switches.cc',
        'base/switches.h',
        'base/synced_property.h',
        'base/tiling_data.cc',
        'base/tiling_data.h',
        'base/time_util.h',
        'base/unique_notifier.cc',
        'base/unique_notifier.h',
        'debug/benchmark_instrumentation.cc',
        'debug/benchmark_instrumentation.h',
        'debug/debug_colors.cc',
        'debug/debug_colors.h',
        'debug/debug_rect_history.cc',
        'debug/debug_rect_history.h',
        'debug/devtools_instrumentation.h',
        'debug/frame_rate_counter.cc',
        'debug/frame_rate_counter.h',
        'debug/frame_timing_request.cc',
        'debug/frame_timing_request.h',
        'debug/frame_timing_tracker.cc',
        'debug/frame_timing_tracker.h',
        'debug/frame_viewer_instrumentation.cc',
        'debug/frame_viewer_instrumentation.h',
        'debug/invalidation_benchmark.cc',
        'debug/invalidation_benchmark.h',
        'debug/lap_timer.cc',
        'debug/lap_timer.h',
        'debug/layer_tree_debug_state.cc',
        'debug/layer_tree_debug_state.h',
        'debug/micro_benchmark.cc',
        'debug/micro_benchmark.h',
        'debug/micro_benchmark_controller.cc',
        'debug/micro_benchmark_controller.h',
        'debug/micro_benchmark_controller_impl.cc',
        'debug/micro_benchmark_controller_impl.h',
        'debug/micro_benchmark_impl.cc',
        'debug/micro_benchmark_impl.h',
        'debug/paint_time_counter.cc',
        'debug/paint_time_counter.h',
        'debug/picture_debug_util.cc',
        'debug/picture_debug_util.h',
        'debug/picture_record_benchmark.cc',
        'debug/picture_record_benchmark.h',
        'debug/rasterize_and_record_benchmark.cc',
        'debug/rasterize_and_record_benchmark.h',
        'debug/rasterize_and_record_benchmark_impl.cc',
        'debug/rasterize_and_record_benchmark_impl.h',
        'debug/rendering_stats.cc',
        'debug/rendering_stats.h',
        'debug/rendering_stats_instrumentation.cc',
        'debug/rendering_stats_instrumentation.h',
        'debug/ring_buffer.h',
        'debug/traced_display_item_list.cc',
        'debug/traced_display_item_list.h',
        'debug/traced_picture.cc',
        'debug/traced_picture.h',
        'debug/traced_value.cc',
        'debug/traced_value.h',
        'debug/unittest_only_benchmark.cc',
        'debug/unittest_only_benchmark.h',
        'debug/unittest_only_benchmark_impl.cc',
        'debug/unittest_only_benchmark_impl.h',
        'input/input_handler.cc',
        'input/input_handler.h',
        'input/layer_selection_bound.cc',
        'input/layer_selection_bound.h',
        'input/page_scale_animation.cc',
        'input/page_scale_animation.h',
        'input/scroll_elasticity_helper.cc',
        'input/scroll_elasticity_helper.h',
        'input/scroll_state.cc',
        'input/scroll_state.h',
        'input/selection_bound_type.h',
        'input/selection.h',
        'input/top_controls_manager.cc',
        'input/top_controls_manager.h',
        'input/top_controls_manager_client.h',
        'layers/append_quads_data.h',
        'layers/content_layer_client.h',
        'layers/delegated_frame_provider.cc',
        'layers/delegated_frame_provider.h',
        'layers/delegated_frame_resource_collection.cc',
        'layers/delegated_frame_resource_collection.h',
        'layers/delegated_renderer_layer.cc',
        'layers/delegated_renderer_layer.h',
        'layers/delegated_renderer_layer_impl.cc',
        'layers/delegated_renderer_layer_impl.h',
        'layers/draw_properties.h',
        'layers/heads_up_display_layer.cc',
        'layers/heads_up_display_layer.h',
        'layers/heads_up_display_layer_impl.cc',
        'layers/heads_up_display_layer_impl.h',
        'layers/io_surface_layer.cc',
        'layers/io_surface_layer.h',
        'layers/io_surface_layer_impl.cc',
        'layers/io_surface_layer_impl.h',
        'layers/layer.cc',
        'layers/layer.h',
        'layers/layer_client.h',
        'layers/layer_impl.cc',
        'layers/layer_impl.h',
        'layers/layer_iterator.h',
        'layers/layer_lists.h',
        'layers/layer_position_constraint.cc',
        'layers/layer_position_constraint.h',
        'layers/layer_utils.cc',
        'layers/layer_utils.h',
        'layers/nine_patch_layer.cc',
        'layers/nine_patch_layer.h',
        'layers/nine_patch_layer_impl.cc',
        'layers/nine_patch_layer_impl.h',
        'layers/paint_properties.h',
        'layers/painted_scrollbar_layer.cc',
        'layers/painted_scrollbar_layer.h',
        'layers/painted_scrollbar_layer_impl.cc',
        'layers/painted_scrollbar_layer_impl.h',
        'layers/picture_image_layer.cc',
        'layers/picture_image_layer.h',
        'layers/picture_image_layer_impl.cc',
        'layers/picture_image_layer_impl.h',
        'layers/picture_layer.cc',
        'layers/picture_layer.h',
        'layers/picture_layer_impl.cc',
        'layers/picture_layer_impl.h',
        'layers/render_pass_sink.h',
        'layers/render_surface_impl.cc',
        'layers/render_surface_impl.h',
        'layers/scrollbar_layer_impl_base.cc',
        'layers/scrollbar_layer_impl_base.h',
        'layers/scrollbar_layer_interface.h',
        'layers/solid_color_layer.cc',
        'layers/solid_color_layer.h',
        'layers/solid_color_layer_impl.cc',
        'layers/solid_color_layer_impl.h',
        'layers/solid_color_scrollbar_layer.cc',
        'layers/solid_color_scrollbar_layer.h',
        'layers/solid_color_scrollbar_layer_impl.cc',
        'layers/solid_color_scrollbar_layer_impl.h',
        'layers/surface_layer.cc',
        'layers/surface_layer.h',
        'layers/surface_layer_impl.cc',
        'layers/surface_layer_impl.h',
        'layers/texture_layer.cc',
        'layers/texture_layer.h',
        'layers/texture_layer_client.h',
        'layers/texture_layer_impl.cc',
        'layers/texture_layer_impl.h',
        'layers/ui_resource_layer.cc',
        'layers/ui_resource_layer.h',
        'layers/ui_resource_layer_impl.cc',
        'layers/ui_resource_layer_impl.h',
        'layers/video_frame_provider.h',
        'layers/video_frame_provider_client_impl.cc',
        'layers/video_frame_provider_client_impl.h',
        'layers/video_layer.cc',
        'layers/video_layer.h',
        'layers/video_layer_impl.cc',
        'layers/video_layer_impl.h',
        'layers/viewport.cc',
        'layers/viewport.h',
        'output/begin_frame_args.cc',
        'output/begin_frame_args.h',
        'output/bsp_tree.cc',
        'output/bsp_tree.h',
        'output/bsp_walk_action.cc',
        'output/bsp_walk_action.h',
        'output/compositor_frame.cc',
        'output/compositor_frame.h',
        'output/compositor_frame_ack.cc',
        'output/compositor_frame_ack.h',
        'output/compositor_frame_metadata.cc',
        'output/compositor_frame_metadata.h',
        'output/context_provider.cc',
        'output/context_provider.h',
        'output/copy_output_request.cc',
        'output/copy_output_request.h',
        'output/copy_output_result.cc',
        'output/copy_output_result.h',
        'output/delegated_frame_data.cc',
        'output/delegated_frame_data.h',
        'output/delegating_renderer.cc',
        'output/delegating_renderer.h',
        'output/direct_renderer.cc',
        'output/direct_renderer.h',
        'output/dynamic_geometry_binding.cc',
        'output/dynamic_geometry_binding.h',
        'output/filter_operation.cc',
        'output/filter_operation.h',
        'output/filter_operations.cc',
        'output/filter_operations.h',
        'output/geometry_binding.cc',
        'output/geometry_binding.h',
        'output/gl_frame_data.cc',
        'output/gl_frame_data.h',
        'output/gl_renderer.cc',
        'output/gl_renderer.h',
        'output/gl_renderer_draw_cache.cc',
        'output/gl_renderer_draw_cache.h',
        'output/latency_info_swap_promise.cc',
        'output/latency_info_swap_promise.h',
        'output/layer_quad.cc',
        'output/layer_quad.h',
        'output/managed_memory_policy.cc',
        'output/managed_memory_policy.h',
        'output/output_surface.cc',
        'output/output_surface.h',
        'output/output_surface_client.h',
        'output/overlay_candidate.cc',
        'output/overlay_candidate.h',
        'output/overlay_candidate_validator.h',
        'output/overlay_processor.cc',
        'output/overlay_processor.h',
        'output/overlay_strategy_common.cc',
        'output/overlay_strategy_common.h',
        'output/overlay_strategy_single_on_top.cc',
        'output/overlay_strategy_single_on_top.h',
        'output/overlay_strategy_underlay.cc',
        'output/overlay_strategy_underlay.h',
        'output/program_binding.cc',
        'output/program_binding.h',
        'output/render_surface_filters.cc',
        'output/render_surface_filters.h',
        'output/renderer.cc',
        'output/renderer.h',
        'output/renderer_capabilities.cc',
        'output/renderer_capabilities.h',
        'output/renderer_settings.cc',
        'output/renderer_settings.h',
        'output/shader.cc',
        'output/shader.h',
        'output/software_output_device.cc',
        'output/software_output_device.h',
        'output/software_renderer.cc',
        'output/software_renderer.h',
        'output/static_geometry_binding.cc',
        'output/static_geometry_binding.h',
        'output/swap_promise.h',
        'output/texture_mailbox_deleter.cc',
        'output/texture_mailbox_deleter.h',
        'output/viewport_selection_bound.cc',
        'output/viewport_selection_bound.h',
        'playback/clip_display_item.cc',
        'playback/clip_display_item.h',
        'playback/clip_path_display_item.cc',
        'playback/clip_path_display_item.h',
        'playback/compositing_display_item.cc',
        'playback/compositing_display_item.h',
        'playback/display_item.cc',
        'playback/display_item.h',
        'playback/display_item_list.cc',
        'playback/display_item_list.h',
        'playback/display_item_list_settings.cc',
        'playback/display_item_list_settings.h',
        'playback/display_list_raster_source.cc',
        'playback/display_list_raster_source.h',
        'playback/display_list_recording_source.cc',
        'playback/display_list_recording_source.h',
        'playback/drawing_display_item.cc',
        'playback/drawing_display_item.h',
        'playback/filter_display_item.cc',
        'playback/filter_display_item.h',
        'playback/float_clip_display_item.cc',
        'playback/float_clip_display_item.h',
        'playback/largest_display_item.cc',
        'playback/largest_display_item.h',
        'playback/picture.cc',
        'playback/picture.h',
        'playback/picture_pile.cc',
        'playback/picture_pile.h',
        'playback/picture_pile_impl.cc',
        'playback/picture_pile_impl.h',
        'playback/pixel_ref_map.cc',
        'playback/pixel_ref_map.h',
        'playback/raster_source.h',
        'playback/raster_source_helper.cc',
        'playback/raster_source_helper.h',
        'playback/recording_source.h',
        'playback/transform_display_item.cc',
        'playback/transform_display_item.h',
        'quads/content_draw_quad_base.cc',
        'quads/content_draw_quad_base.h',
        'quads/debug_border_draw_quad.cc',
        'quads/debug_border_draw_quad.h',
        'quads/draw_polygon.cc',
        'quads/draw_polygon.h',
        'quads/draw_quad.cc',
        'quads/draw_quad.h',
        'quads/io_surface_draw_quad.cc',
        'quads/io_surface_draw_quad.h',
        'quads/largest_draw_quad.cc',
        'quads/largest_draw_quad.h',
        'quads/picture_draw_quad.cc',
        'quads/picture_draw_quad.h',
        'quads/render_pass.cc',
        'quads/render_pass.h',
        'quads/render_pass_draw_quad.cc',
        'quads/render_pass_draw_quad.h',
        'quads/render_pass_id.cc',
        'quads/render_pass_id.h',
        'quads/shared_quad_state.cc',
        'quads/shared_quad_state.h',
        'quads/solid_color_draw_quad.cc',
        'quads/solid_color_draw_quad.h',
        'quads/stream_video_draw_quad.cc',
        'quads/stream_video_draw_quad.h',
        'quads/surface_draw_quad.cc',
        'quads/surface_draw_quad.h',
        'quads/texture_draw_quad.cc',
        'quads/texture_draw_quad.h',
        'quads/tile_draw_quad.cc',
        'quads/tile_draw_quad.h',
        'quads/yuv_video_draw_quad.cc',
        'quads/yuv_video_draw_quad.h',
        'raster/bitmap_tile_task_worker_pool.cc',
        'raster/bitmap_tile_task_worker_pool.h',
        'raster/gpu_rasterizer.cc',
        'raster/gpu_rasterizer.h',
        'raster/gpu_tile_task_worker_pool.cc',
        'raster/gpu_tile_task_worker_pool.h',
        'raster/one_copy_tile_task_worker_pool.cc',
        'raster/one_copy_tile_task_worker_pool.h',
        'raster/pixel_buffer_tile_task_worker_pool.cc',
        'raster/pixel_buffer_tile_task_worker_pool.h',
        'raster/raster_buffer.cc',
        'raster/raster_buffer.h',
        'raster/scoped_gpu_raster.cc',
        'raster/scoped_gpu_raster.h',
        'raster/task_graph_runner.cc',
        'raster/task_graph_runner.h',
        'raster/texture_compressor.cc',
        'raster/texture_compressor.h',
        'raster/texture_compressor_etc1.cc',
        'raster/texture_compressor_etc1.h',
        'raster/tile_task_runner.cc',
        'raster/tile_task_runner.h',
        'raster/tile_task_worker_pool.cc',
        'raster/tile_task_worker_pool.h',
        'raster/zero_copy_tile_task_worker_pool.cc',
        'raster/zero_copy_tile_task_worker_pool.h',
        'resources/memory_history.cc',
        'resources/memory_history.h',
        'resources/platform_color.h',
        'resources/release_callback.h',
        'resources/resource.h',
        'resources/resource_format.cc',
        'resources/resource_format.h',
        'resources/resource_pool.cc',
        'resources/resource_pool.h',
        'resources/resource_provider.cc',
        'resources/resource_provider.h',
        'resources/resource_util.h',
        'resources/returned_resource.h',
        'resources/scoped_resource.cc',
        'resources/scoped_resource.h',
        'resources/scoped_ui_resource.cc',
        'resources/scoped_ui_resource.h',
        'resources/shared_bitmap.cc',
        'resources/shared_bitmap.h',
        'resources/shared_bitmap_manager.h',
        'resources/single_release_callback.cc',
        'resources/single_release_callback.h',
        'resources/single_release_callback_impl.cc',
        'resources/single_release_callback_impl.h',
        'resources/texture_mailbox.cc',
        'resources/texture_mailbox.h',
        'resources/transferable_resource.cc',
        'resources/transferable_resource.h',
        'resources/ui_resource_bitmap.cc',
        'resources/ui_resource_bitmap.h',
        'resources/ui_resource_client.h',
        'resources/ui_resource_request.cc',
        'resources/ui_resource_request.h',
        'resources/video_resource_updater.cc',
        'resources/video_resource_updater.h',
        'scheduler/begin_frame_source.cc',
        'scheduler/begin_frame_source.h',
        'scheduler/begin_frame_tracker.cc',
        'scheduler/begin_frame_tracker.h',
        'scheduler/commit_earlyout_reason.h',
        'scheduler/compositor_timing_history.cc',
        'scheduler/compositor_timing_history.h',
        'scheduler/delay_based_time_source.cc',
        'scheduler/delay_based_time_source.h',
        'scheduler/draw_result.h',
        'scheduler/scheduler.cc',
        'scheduler/scheduler.h',
        'scheduler/scheduler_settings.cc',
        'scheduler/scheduler_settings.h',
        'scheduler/scheduler_state_machine.cc',
        'scheduler/scheduler_state_machine.h',
        'scheduler/video_frame_controller.h',
        'tiles/eviction_tile_priority_queue.cc',
        'tiles/eviction_tile_priority_queue.h',
        'tiles/image_decode_controller.cc',
        'tiles/image_decode_controller.h',
        'tiles/picture_layer_tiling.cc',
        'tiles/picture_layer_tiling.h',
        'tiles/picture_layer_tiling_set.cc',
        'tiles/picture_layer_tiling_set.h',
        'tiles/prioritized_tile.cc',
        'tiles/prioritized_tile.h',
        'tiles/raster_tile_priority_queue.cc',
        'tiles/raster_tile_priority_queue.h',
        'tiles/raster_tile_priority_queue_all.cc',
        'tiles/raster_tile_priority_queue_all.h',
        'tiles/raster_tile_priority_queue_required.cc',
        'tiles/raster_tile_priority_queue_required.h',
        'tiles/tile.cc',
        'tiles/tile.h',
        'tiles/tile_draw_info.cc',
        'tiles/tile_draw_info.h',
        'tiles/tile_manager.cc',
        'tiles/tile_manager.h',
        'tiles/tile_priority.cc',
        'tiles/tile_priority.h',
        'tiles/tiling_set_eviction_queue.cc',
        'tiles/tiling_set_eviction_queue.h',
        'tiles/tiling_set_raster_queue_all.cc',
        'tiles/tiling_set_raster_queue_all.h',
        'tiles/tiling_set_raster_queue_required.cc',
        'tiles/tiling_set_raster_queue_required.h',
        'trees/blocking_task_runner.cc',
        'trees/blocking_task_runner.h',
        'trees/damage_tracker.cc',
        'trees/damage_tracker.h',
        'trees/draw_property_utils.cc',
        'trees/draw_property_utils.h',
        'trees/latency_info_swap_promise_monitor.cc',
        'trees/latency_info_swap_promise_monitor.h',
        'trees/layer_tree_host.cc',
        'trees/layer_tree_host.h',
        'trees/layer_tree_host_client.h',
        'trees/layer_tree_host_common.cc',
        'trees/layer_tree_host_common.h',
        'trees/layer_tree_host_impl.cc',
        'trees/layer_tree_host_impl.h',
        'trees/layer_tree_host_single_thread_client.h',
        'trees/layer_tree_impl.cc',
        'trees/layer_tree_impl.h',
        'trees/layer_tree_settings.cc',
        'trees/layer_tree_settings.h',
        'trees/mutator_host_client.h',
        'trees/occlusion.cc',
        'trees/occlusion.h',
        'trees/occlusion_tracker.cc',
        'trees/occlusion_tracker.h',
        'trees/property_tree.cc',
        'trees/property_tree.h',
        'trees/property_tree_builder.cc',
        'trees/property_tree_builder.h',
        'trees/proxy.cc',
        'trees/proxy.h',
        'trees/scoped_abort_remaining_swap_promises.h',
        'trees/single_thread_proxy.cc',
        'trees/single_thread_proxy.h',
        'trees/swap_promise_monitor.cc',
        'trees/swap_promise_monitor.h',
        'trees/thread_proxy.cc',
        'trees/thread_proxy.h',
        'trees/tree_synchronizer.cc',
        'trees/tree_synchronizer.h',
      ],
      'includes': [
        '../build/android/increase_size_for_speed.gypi',
      ],
      'conditions': [
        ['target_arch == "ia32" or target_arch == "x64"', {
          'sources': [
            'raster/texture_compressor_etc1_sse.cc',
            'raster/texture_compressor_etc1_sse.h',
          ],
        }],
      ],
    },
    {
      # GN version: //cc/surfaces
      'target_name': 'cc_surfaces',
      'type': '<(component)',
      'dependencies': [
        'cc',
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '<(DEPTH)/gpu/gpu.gyp:gpu',
        '<(DEPTH)/skia/skia.gyp:skia',
        '<(DEPTH)/ui/events/events.gyp:events_base',
        '<(DEPTH)/ui/gfx/gfx.gyp:gfx',
        '<(DEPTH)/ui/gfx/gfx.gyp:gfx_geometry',
      ],
      'defines': [
        'CC_SURFACES_IMPLEMENTATION=1',
      ],
      'sources': [
        # Note: file list duplicated in GN build.
        'surfaces/display.cc',
        'surfaces/display.h',
        'surfaces/display_client.h',
        'surfaces/display_scheduler.cc',
        'surfaces/display_scheduler.h',
        'surfaces/onscreen_display_client.cc',
        'surfaces/onscreen_display_client.h',
        'surfaces/surface.cc',
        'surfaces/surface.h',
        'surfaces/surface_aggregator.cc',
        'surfaces/surface_aggregator.h',
        'surfaces/surface_display_output_surface.cc',
        'surfaces/surface_display_output_surface.h',
        'surfaces/surface_factory.cc',
        'surfaces/surface_factory.h',
        'surfaces/surface_factory_client.h',
        'surfaces/surface_hittest.cc',
        'surfaces/surface_hittest.h',
        'surfaces/surface_id.h',
        'surfaces/surface_id_allocator.cc',
        'surfaces/surface_id_allocator.h',
        'surfaces/surface_manager.cc',
        'surfaces/surface_manager.h',
        'surfaces/surface_resource_holder.cc',
        'surfaces/surface_resource_holder.h',
        'surfaces/surfaces_export.h',
      ],
      'includes': [
        '../build/android/increase_size_for_speed.gypi',
      ],
    },
  ],
}
