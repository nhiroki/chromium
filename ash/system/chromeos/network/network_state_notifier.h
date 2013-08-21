// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_CHROMEOS_NETWORK_NETWORK_STATE_NOTIFIER_H_
#define ASH_SYSTEM_CHROMEOS_NETWORK_NETWORK_STATE_NOTIFIER_H_

#include <map>

#include "ash/ash_export.h"
#include "ash/system/chromeos/network/network_tray_delegate.h"
#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "chromeos/network/network_state_handler_observer.h"

namespace base {
class DictionaryValue;
}

namespace chromeos {
class NetworkState;
}

namespace ash {

// This class has two purposes:
// 1. ShowNetworkConnectError() gets called after any user initiated connect
//    failure. This will handle displaying an error notification.
//    TODO(stevenjb): convert this class to use the new MessageCenter
//    notification system.
// 2. It observes NetworkState changes to generate notifications when a
//    Cellular network is out of credits.
class ASH_EXPORT NetworkStateNotifier :
      public chromeos::NetworkStateHandlerObserver,
      public NetworkTrayDelegate {
 public:
  NetworkStateNotifier();
  virtual ~NetworkStateNotifier();

  // NetworkStateHandlerObserver
  virtual void DefaultNetworkChanged(
      const chromeos::NetworkState* network) OVERRIDE;
  virtual void NetworkPropertiesUpdated(
      const chromeos::NetworkState* network) OVERRIDE;

  // NetworkTrayDelegate
  virtual void NotificationLinkClicked(
      NetworkObserver::MessageType message_type,
      size_t link_index) OVERRIDE;

  // Show a connection error notification. If |error_name| matches an error
  // defined in NetworkConnectionHandler for connect, configure, or activation
  // failed, then the associated message is shown, otherwise the Shill
  // error for Service.Error is used (from network_connect::ErrorString), or
  // "Unknown network error".
  void ShowNetworkConnectError(const std::string& error_name,
                               const std::string& service_path);

 private:
  void ConnectErrorPropertiesSucceeded(
      const std::string& error_name,
      const std::string& service_path,
      const base::DictionaryValue& shill_properties);
  void ConnectErrorPropertiesFailed(
      const std::string& error_name,
      const std::string& service_path,
      const std::string& shill_error_name,
      scoped_ptr<base::DictionaryValue> shill_error_data);
  void ShowConnectErrorNotification(
      const std::string& error_name,
      const std::string& service_path,
      const base::DictionaryValue& shill_properties);

  std::string last_active_network_;
  std::string cellular_network_;
  bool cellular_out_of_credits_;
  base::Time out_of_credits_notify_time_;
  base::WeakPtrFactory<NetworkStateNotifier> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(NetworkStateNotifier);
};

}  // namespace ash

#endif  // ASH_SYSTEM_CHROMEOS_NETWORK_NETWORK_STATE_NOTIFIER_H_
