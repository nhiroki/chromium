// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_RESOURCES_PIXEL_BUFFER_RASTER_WORKER_POOL_H_
#define CC_RESOURCES_PIXEL_BUFFER_RASTER_WORKER_POOL_H_

#include <deque>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "cc/resources/raster_worker_pool.h"
#include "cc/resources/rasterizer.h"

namespace cc {
class ResourceProvider;

class CC_EXPORT PixelBufferRasterWorkerPool
    : public RasterWorkerPool,
      public Rasterizer,
      public internal::RasterizerTaskClient {
 public:
  virtual ~PixelBufferRasterWorkerPool();

  static scoped_ptr<RasterWorkerPool> Create(
      base::SequencedTaskRunner* task_runner,
      internal::TaskGraphRunner* task_graph_runner,
      ResourceProvider* resource_provider,
      size_t max_transfer_buffer_usage_bytes);

  // Overridden from RasterWorkerPool:
  virtual Rasterizer* AsRasterizer() OVERRIDE;

  // Overridden from Rasterizer:
  virtual void SetClient(RasterizerClient* client) OVERRIDE;
  virtual void Shutdown() OVERRIDE;
  virtual void ScheduleTasks(RasterTaskQueue* queue) OVERRIDE;
  virtual void CheckForCompletedTasks() OVERRIDE;

  // Overridden from internal::RasterizerTaskClient:
  virtual SkCanvas* AcquireCanvasForRaster(internal::RasterTask* task) OVERRIDE;
  virtual void ReleaseCanvasForRaster(internal::RasterTask* task) OVERRIDE;

 private:
  struct RasterTaskState {
    class TaskComparator {
     public:
      explicit TaskComparator(const internal::RasterTask* task) : task_(task) {}

      bool operator()(const RasterTaskState& state) const {
        return state.task == task_;
      }

     private:
      const internal::RasterTask* task_;
    };

    typedef std::vector<RasterTaskState> Vector;

    RasterTaskState(internal::RasterTask* task, bool required_for_activation)
        : type(UNSCHEDULED),
          task(task),
          required_for_activation(required_for_activation) {}

    enum { UNSCHEDULED, SCHEDULED, UPLOADING, COMPLETED } type;
    internal::RasterTask* task;
    bool required_for_activation;
  };

  typedef std::deque<scoped_refptr<internal::RasterTask> > RasterTaskDeque;

  PixelBufferRasterWorkerPool(base::SequencedTaskRunner* task_runner,
                              internal::TaskGraphRunner* task_graph_runner,
                              ResourceProvider* resource_provider,
                              size_t max_transfer_buffer_usage_bytes);

  void OnRasterFinished();
  void OnRasterRequiredForActivationFinished();
  void FlushUploads();
  void CheckForCompletedUploads();
  void ScheduleCheckForCompletedRasterTasks();
  void OnCheckForCompletedRasterTasks();
  void CheckForCompletedRasterTasks();
  void ScheduleMoreTasks();
  unsigned PendingRasterTaskCount() const;
  bool HasPendingTasks() const;
  bool HasPendingTasksRequiredForActivation() const;
  void CheckForCompletedRasterizerTasks();

  const char* StateName() const;
  scoped_ptr<base::Value> StateAsValue() const;
  scoped_ptr<base::Value> ThrottleStateAsValue() const;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  internal::TaskGraphRunner* task_graph_runner_;
  const internal::NamespaceToken namespace_token_;
  RasterizerClient* client_;
  ResourceProvider* resource_provider_;

  bool shutdown_;

  RasterTaskQueue raster_tasks_;
  RasterTaskState::Vector raster_task_states_;
  RasterTaskDeque raster_tasks_with_pending_upload_;
  internal::RasterTask::Vector completed_raster_tasks_;
  internal::RasterizerTask::Vector completed_image_decode_tasks_;

  size_t scheduled_raster_task_count_;
  size_t raster_tasks_required_for_activation_count_;
  size_t bytes_pending_upload_;
  size_t max_bytes_pending_upload_;
  bool has_performed_uploads_since_last_flush_;
  base::TimeTicks check_for_completed_raster_tasks_time_;
  bool check_for_completed_raster_tasks_pending_;

  bool should_notify_client_if_no_tasks_are_pending_;
  bool should_notify_client_if_no_tasks_required_for_activation_are_pending_;
  bool raster_finished_task_pending_;
  bool raster_required_for_activation_finished_task_pending_;

  base::WeakPtrFactory<PixelBufferRasterWorkerPool>
      raster_finished_weak_ptr_factory_;

  scoped_refptr<internal::RasterizerTask> raster_finished_task_;
  scoped_refptr<internal::RasterizerTask>
      raster_required_for_activation_finished_task_;

  // Task graph used when scheduling tasks and vector used to gather
  // completed tasks.
  internal::TaskGraph graph_;
  internal::Task::Vector completed_tasks_;

  base::WeakPtrFactory<PixelBufferRasterWorkerPool> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PixelBufferRasterWorkerPool);
};

}  // namespace cc

#endif  // CC_RESOURCES_PIXEL_BUFFER_RASTER_WORKER_POOL_H_
