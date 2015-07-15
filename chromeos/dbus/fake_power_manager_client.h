// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_FAKE_POWER_MANAGER_CLIENT_H_
#define CHROMEOS_DBUS_FAKE_POWER_MANAGER_CLIENT_H_

#include <string>

#include "base/basictypes.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/power_manager/policy.pb.h"
#include "chromeos/dbus/power_manager/power_supply_properties.pb.h"
#include "chromeos/dbus/power_manager/suspend.pb.h"
#include "chromeos/dbus/power_manager_client.h"

namespace chromeos {

// A fake implementation of PowerManagerClient. This remembers the policy passed
// to SetPolicy() and the user of this class can inspect the last set policy by
// get_policy().
class CHROMEOS_EXPORT FakePowerManagerClient : public PowerManagerClient {
 public:
  FakePowerManagerClient();
  ~FakePowerManagerClient() override;

  power_manager::PowerManagementPolicy& policy() { return policy_; }
  const power_manager::PowerSupplyProperties& props() const { return props_; }
  int num_request_restart_calls() const { return num_request_restart_calls_; }
  int num_request_shutdown_calls() const { return num_request_shutdown_calls_; }
  int num_set_policy_calls() const { return num_set_policy_calls_; }
  int num_set_is_projecting_calls() const {
    return num_set_is_projecting_calls_;
  }
  bool is_projecting() const { return is_projecting_; }

  // PowerManagerClient overrides
  void Init(dbus::Bus* bus) override;
  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;
  bool HasObserver(const Observer* observer) const override;
  void SetRenderProcessManagerDelegate(
      base::WeakPtr<RenderProcessManagerDelegate> delegate) override;
  void DecreaseScreenBrightness(bool allow_off) override;
  void IncreaseScreenBrightness() override;
  void SetScreenBrightnessPercent(double percent, bool gradual) override;
  void GetScreenBrightnessPercent(
      const GetScreenBrightnessPercentCallback& callback) override;
  void DecreaseKeyboardBrightness() override;
  void IncreaseKeyboardBrightness() override;
  void RequestStatusUpdate() override;
  void RequestSuspend() override;
  void RequestRestart() override;
  void RequestShutdown() override;
  void NotifyUserActivity(power_manager::UserActivityType type) override;
  void NotifyVideoActivity(bool is_fullscreen) override;
  void SetPolicy(const power_manager::PowerManagementPolicy& policy) override;
  void SetIsProjecting(bool is_projecting) override;
  base::Closure GetSuspendReadinessCallback() override;
  int GetNumPendingSuspendReadinessCallbacks() override;

  // Emulates the power manager announcing that the system is starting or
  // completing a suspend attempt.
  void SendSuspendImminent();
  void SendSuspendDone();
  void SendDarkSuspendImminent();

  // Notifies observers that the power button has been pressed or released.
  void SendPowerButtonEvent(bool down, const base::TimeTicks& timestamp);

  // Updates |props_| and notifies observers of its changes.
  void UpdatePowerProperties(
      const power_manager::PowerSupplyProperties& power_props);

 private:
  // Callback that will be run by asynchronous suspend delays to report
  // readiness.
  void HandleSuspendReadiness();

  // Notifies |observers_| that |props_| has been updated.
  void NotifyObservers();

  base::ObserverList<Observer> observers_;

  // Last policy passed to SetPolicy().
  power_manager::PowerManagementPolicy policy_;

  // Power status received from the power manager.
  power_manager::PowerSupplyProperties props_;

  // Number of times that various methods have been called.
  int num_request_restart_calls_;
  int num_request_shutdown_calls_;
  int num_set_policy_calls_;
  int num_set_is_projecting_calls_;

  // Number of pending suspend readiness callbacks.
  int num_pending_suspend_readiness_callbacks_;

  // Last projecting state set in SetIsProjecting().
  bool is_projecting_;

  // Delegate for managing power consumption of Chrome's renderer processes.
  base::WeakPtr<RenderProcessManagerDelegate> render_process_manager_delegate_;

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<FakePowerManagerClient> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(FakePowerManagerClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_FAKE_POWER_MANAGER_CLIENT_H_
