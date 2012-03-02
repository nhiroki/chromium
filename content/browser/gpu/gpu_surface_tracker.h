// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_GPU_GPU_SURFACE_TRACKER_H_
#define CONTENT_BROWSER_GPU_GPU_SURFACE_TRACKER_H_

#include <map>

#include "base/basictypes.h"
#include "base/memory/singleton.h"
#include "base/synchronization/lock.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/size.h"

// This class is responsible for managing rendering surfaces exposed to the
// GPU process. Every surface gets registered to this class, and gets an ID.
// All calls to and from the GPU process, with the exception of
// CreateViewCommandBuffer, refer to the rendering surface by its ID.
// This class is thread safe.
//
// Note: The ID can exist before the actual native handle for the surface is
// created, for example to allow giving a reference to it to a renderer, so that
// it is unamibiguously identified.
class GpuSurfaceTracker {
 public:
  // Gets the global instance of the surface tracker.
  static GpuSurfaceTracker* Get() { return GetInstance(); }

  // Adds a surface for a given RenderWidgetHost. |renderer_id| is the renderer
  // process ID, |render_widget_id| is the RenderWidgetHost route id within that
  // renderer. Returns the surface ID.
  int AddSurfaceForRenderer(int renderer_id, int render_widget_id);

  // Looks up a surface for a given RenderWidgetHost. Returns the surface
  // ID, or 0 if not found.
  // Note: This is an O(N) lookup.
  int LookupSurfaceForRenderer(int renderer_id, int render_widget_id);

  // Adds a surface for a native widget. Returns the surface ID.
  int AddSurfaceForNativeWidget(gfx::AcceleratedWidget widget);

  // Removes a given existing surface.
  void RemoveSurface(int surface_id);

  // Gets the renderer process ID and RenderWidgetHost route id for a given
  // surface, returning true if the surface is found (and corresponds to a
  // RenderWidgetHost), or false if not.
  bool GetRenderWidgetIDForSurface(int surface_id,
                                   int* renderer_id,
                                   int* render_widget_id);

  // Sets the native handle for the given surface.
  // Note: This is an O(log N) lookup.
  void SetSurfaceHandle(int surface_id, const gfx::GLSurfaceHandle& handle);

  // Gets the native handle for the given surface.
  // Note: This is an O(log N) lookup.
  gfx::GLSurfaceHandle GetSurfaceHandle(int surface_id);

#if defined(OS_WIN) && !defined(USE_AURA)
  // This is a member of GpuSurfaceTracker because it holds the lock for its
  // duration. This prevents the AcceleratedSurface that it posts to from being
  // destroyed by the main thread during that time. This function is only called
  // on the IO thread. This function only posts tasks asynchronously. If it
  // were to synchronously call the UI thread, there would be a possiblity of
  // deadlock.
  void AsyncPresentAndAcknowledge(int surface_id,
                                  const gfx::Size& size,
                                  int64 surface_handle,
                                  const base::Closure& completion_task);
#endif

  // Gets the global instance of the surface tracker. Identical to Get(), but
  // named that way for the implementation of Singleton.
  static GpuSurfaceTracker* GetInstance();

 private:
  struct SurfaceInfo {
    int renderer_id;
    int render_widget_id;
    gfx::AcceleratedWidget native_widget;
    gfx::GLSurfaceHandle handle;
  };
  typedef std::map<int, SurfaceInfo> SurfaceMap;

  friend struct DefaultSingletonTraits<GpuSurfaceTracker>;

  GpuSurfaceTracker();
  ~GpuSurfaceTracker();

  base::Lock lock_;
  SurfaceMap surface_map_;
  int next_surface_id_;

  DISALLOW_COPY_AND_ASSIGN(GpuSurfaceTracker);
};

#endif  // CONTENT_BROWSER_GPU_GPU_SURFACE_TRACKER_H_
