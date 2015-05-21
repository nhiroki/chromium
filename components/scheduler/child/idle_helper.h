// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SCHEDULER_CHILD_IDLE_HELPER_H_
#define COMPONENTS_SCHEDULER_CHILD_IDLE_HELPER_H_

#include "components/scheduler/child/cancelable_closure_holder.h"
#include "components/scheduler/child/prioritizing_task_queue_selector.h"
#include "components/scheduler/child/single_thread_idle_task_runner.h"
#include "components/scheduler/scheduler_export.h"

namespace scheduler {

class SchedulerHelper;

// Common scheduler functionality for Idle tasks.
class SCHEDULER_EXPORT IdleHelper {
 public:
  // Used to by scheduler implementations to customize idle behaviour.
  class SCHEDULER_EXPORT Delegate {
   public:
    Delegate();
    virtual ~Delegate();

    // If it's ok to enter a long idle period, return true.  Otherwise return
    // false and set next_long_idle_period_delay_out so we know when to try
    // again.
    virtual bool CanEnterLongIdlePeriod(
        base::TimeTicks now,
        base::TimeDelta* next_long_idle_period_delay_out) = 0;

    // Signals that the Long Idle Period hasn't started yet because the system
    // isn't quiescent.
    virtual void IsNotQuiescent() = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(Delegate);
  };

  // Keep IdleHelper::IdlePeriodStateToString in sync with this enum.
  enum class IdlePeriodState {
    NOT_IN_IDLE_PERIOD,
    IN_SHORT_IDLE_PERIOD,
    IN_LONG_IDLE_PERIOD,
    IN_LONG_IDLE_PERIOD_WITH_MAX_DEADLINE,
    ENDING_LONG_IDLE_PERIOD,
    // Must be the last entry.
    IDLE_PERIOD_STATE_COUNT,
    FIRST_IDLE_PERIOD_STATE = NOT_IN_IDLE_PERIOD,
  };

  // The maximum length of an idle period.
  static const int kMaximumIdlePeriodMillis = 50;

  // |helper| and |delegate| are not owned by IdleHelper object and must
  // outlive it.
  IdleHelper(
      SchedulerHelper* helper,
      Delegate* delegate,
      size_t idle_queue_index,
      const char* tracing_category,
      const char* disabled_by_default_tracing_category,
      const char* idle_period_tracing_name,
      base::TimeDelta required_quiescence_duration_before_long_idle_period);
  ~IdleHelper();

  // Returns the idle task runner. Tasks posted to this runner may be reordered
  // relative to other task types and may be starved for an arbitrarily long
  // time if no idle time is available.
  scoped_refptr<SingleThreadIdleTaskRunner> IdleTaskRunner();

  // If |required_quiescence_duration_before_long_idle_period_| is zero then
  // immediately initiate a long idle period, otherwise check if any tasks have
  // run recently and if so, check again after a delay of
  // |required_quiescence_duration_before_long_idle_period_|.
  // Calling this function will end any previous idle period immediately, and
  // potentially again later if
  // |required_quiescence_duration_before_long_idle_period_| is non-zero.
  // NOTE EndIdlePeriod will disable the long idle periods.
  void EnableLongIdlePeriod();

  // Start and end an idle period. If |post_end_idle_period| is true, it will
  // post a delayed EndIdlePeriod scheduled to occur at |idle_period_deadline|.
  void StartIdlePeriod(IdlePeriodState new_idle_period_state,
                       base::TimeTicks now,
                       base::TimeTicks idle_period_deadline,
                       bool post_end_idle_period);

  // This will end an idle period either started with StartIdlePeriod or
  // EnableLongIdlePeriod.
  void EndIdlePeriod();

  // Returns true if a currently running idle task could exceed its deadline
  // without impacting user experience too much. This should only be used if
  // there is a task which cannot be pre-empted and is likely to take longer
  // than the largest expected idle task deadline. It should NOT be polled to
  // check whether more work can be performed on the current idle task after
  // its deadline has expired - post a new idle task for the continuation of the
  // work in this case.
  // Must be called from the thread this class was created on.
  bool CanExceedIdleDeadlineIfRequired() const;

  IdlePeriodState SchedulerIdlePeriodState() const;

  // IdleTaskDeadlineSupplier Implementation:
  void CurrentIdleTaskDeadlineCallback(base::TimeTicks* deadline_out) const;

  static const char* IdlePeriodStateToString(IdlePeriodState state);

 private:
  friend class IdleHelperTest;

  // The minimum delay to wait between retrying to initiate a long idle time.
  static const int kRetryEnableLongIdlePeriodDelayMillis = 1;

  // Returns the new idle period state for the next long idle period. Fills in
  // |next_long_idle_period_delay_out| with the next time we should try to
  // initiate the next idle period.
  IdlePeriodState ComputeNewLongIdlePeriodState(
      const base::TimeTicks now,
      base::TimeDelta* next_long_idle_period_delay_out);

  bool ShouldWaitForQuiescence();
  void EnableLongIdlePeriodAfterWakeup();

  // Returns true if |state| represents being within an idle period state.
  static bool IsInIdlePeriod(IdlePeriodState state);

  SchedulerHelper* helper_;  // NOT OWNED
  Delegate* delegate_;       // NOT OWNED
  size_t idle_queue_index_;
  scoped_refptr<SingleThreadIdleTaskRunner> idle_task_runner_;

  CancelableClosureHolder end_idle_period_closure_;
  CancelableClosureHolder enable_next_long_idle_period_closure_;
  CancelableClosureHolder enable_next_long_idle_period_after_wakeup_closure_;

  IdlePeriodState idle_period_state_;

  // A bitmap which controls the set of queues that are checked for quiescence
  // before triggering a long idle period.
  uint64 quiescence_monitored_task_queue_mask_;
  base::TimeDelta required_quiescence_duration_before_long_idle_period_;

  base::TimeTicks idle_period_deadline_;

  const char* tracing_category_;
  const char* disabled_by_default_tracing_category_;
  const char* idle_period_tracing_name_;

  base::WeakPtr<IdleHelper> weak_idle_helper_ptr_;
  base::WeakPtrFactory<IdleHelper> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(IdleHelper);
};

}  // namespace scheduler

#endif  // COMPONENTS_SCHEDULER_CHILD_IDLE_HELPER_H_
