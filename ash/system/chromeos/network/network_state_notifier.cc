// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/chromeos/network/network_state_notifier.h"

#include "ash/shell.h"
#include "ash/system/chromeos/network/network_connect.h"
#include "ash/system/chromeos/network/network_observer.h"
#include "ash/system/tray/system_tray_notifier.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chromeos/network/network_configuration_handler.h"
#include "chromeos/network/network_connection_handler.h"
#include "chromeos/network/network_event_log.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "grit/ash_strings.h"
#include "third_party/cros_system_api/dbus/service_constants.h"
#include "ui/base/l10n/l10n_util.h"

using chromeos::NetworkConnectionHandler;
using chromeos::NetworkHandler;
using chromeos::NetworkState;
using chromeos::NetworkStateHandler;

namespace {

const int kMinTimeBetweenOutOfCreditsNotifySeconds = 10 * 60;

// Error messages based on |error_name|, not network_state->error().
string16 GetConnectErrorString(const std::string& error_name) {
  if (error_name == NetworkConnectionHandler::kErrorNotFound)
    return l10n_util::GetStringUTF16(IDS_CHROMEOS_NETWORK_ERROR_CONNECT_FAILED);
  if (error_name == NetworkConnectionHandler::kErrorConfigureFailed)
    return l10n_util::GetStringUTF16(
        IDS_CHROMEOS_NETWORK_ERROR_CONFIGURE_FAILED);
  if (error_name == NetworkConnectionHandler::kErrorActivateFailed)
    return l10n_util::GetStringUTF16(
        IDS_CHROMEOS_NETWORK_ERROR_ACTIVATION_FAILED);
  return string16();
}

}  // namespace

namespace ash {

NetworkStateNotifier::NetworkStateNotifier()
    : cellular_out_of_credits_(false),
      weak_ptr_factory_(this) {
  if (!NetworkHandler::IsInitialized())
    return;
  NetworkHandler::Get()->network_state_handler()->AddObserver(this, FROM_HERE);

  // Initialize |last_active_network_|.
  const NetworkState* default_network =
      NetworkHandler::Get()->network_state_handler()->DefaultNetwork();
  if (default_network && default_network->IsConnectedState())
    last_active_network_ = default_network->path();
}

NetworkStateNotifier::~NetworkStateNotifier() {
  if (!NetworkHandler::IsInitialized())
    return;
  NetworkHandler::Get()->network_state_handler()->RemoveObserver(
      this, FROM_HERE);
}

void NetworkStateNotifier::DefaultNetworkChanged(const NetworkState* network) {
  if (!network || !network->IsConnectedState())
    return;
  if (network->path() != last_active_network_) {
    last_active_network_ = network->path();
    // Reset state for new connected network
    cellular_out_of_credits_ = false;
    NetworkPropertiesUpdated(network);
  }
}

void NetworkStateNotifier::NetworkPropertiesUpdated(
    const NetworkState* network) {
  // Trigger "Out of credits" notification if the cellular network is the most
  // recent default network (i.e. we have not switched to another network).
  if (network->type() == flimflam::kTypeCellular &&
      network->path() == last_active_network_) {
    cellular_network_ = network->path();
    if (network->cellular_out_of_credits() &&
        !cellular_out_of_credits_) {
      cellular_out_of_credits_ = true;
      base::TimeDelta dtime = base::Time::Now() - out_of_credits_notify_time_;
      if (dtime.InSeconds() > kMinTimeBetweenOutOfCreditsNotifySeconds) {
        out_of_credits_notify_time_ = base::Time::Now();
        std::vector<string16> links;
        links.push_back(
            l10n_util::GetStringFUTF16(IDS_NETWORK_OUT_OF_CREDITS_LINK,
                                       UTF8ToUTF16(network->name())));
        ash::Shell::GetInstance()->system_tray_notifier()->
            NotifySetNetworkMessage(
                this,
                NetworkObserver::ERROR_OUT_OF_CREDITS,
                NetworkObserver::GetNetworkTypeForNetworkState(network),
                l10n_util::GetStringUTF16(IDS_NETWORK_OUT_OF_CREDITS_TITLE),
                l10n_util::GetStringUTF16(IDS_NETWORK_OUT_OF_CREDITS_BODY),
                links);
      }
    }
  }
}

void NetworkStateNotifier::NotificationLinkClicked(
    NetworkObserver::MessageType message_type,
    size_t link_index) {
  if (message_type == NetworkObserver::ERROR_OUT_OF_CREDITS) {
    if (!cellular_network_.empty()) {
      // This will trigger the activation / portal code.
      Shell::GetInstance()->system_tray_delegate()->ConfigureNetwork(
          cellular_network_);
    }
    ash::Shell::GetInstance()->system_tray_notifier()->
        NotifyClearNetworkMessage(message_type);
  }
}

void NetworkStateNotifier::ShowNetworkConnectError(
    const std::string& error_name,
    const std::string& service_path) {
  // Get the up-to-date properties for the network and display the error.
  NetworkHandler::Get()->network_configuration_handler()->GetProperties(
      service_path,
      base::Bind(&NetworkStateNotifier::ConnectErrorPropertiesSucceeded,
                 weak_ptr_factory_.GetWeakPtr(), error_name),
      base::Bind(&NetworkStateNotifier::ConnectErrorPropertiesFailed,
                 weak_ptr_factory_.GetWeakPtr(), error_name, service_path));
}

void NetworkStateNotifier::ConnectErrorPropertiesSucceeded(
    const std::string& error_name,
    const std::string& service_path,
    const base::DictionaryValue& shill_properties) {
  ShowConnectErrorNotification(error_name, service_path, shill_properties);
}

void NetworkStateNotifier::ConnectErrorPropertiesFailed(
    const std::string& error_name,
    const std::string& service_path,
    const std::string& shill_error_name,
    scoped_ptr<base::DictionaryValue> shill_error_data) {
  base::DictionaryValue shill_properties;
  ShowConnectErrorNotification(error_name, service_path, shill_properties);
}

void NetworkStateNotifier::ShowConnectErrorNotification(
    const std::string& error_name,
    const std::string& service_path,
    const base::DictionaryValue& shill_properties) {
  string16 error = GetConnectErrorString(error_name);
  if (error.empty()) {
    std::string network_error;
    shill_properties.GetStringWithoutPathExpansion(
        flimflam::kErrorProperty, &network_error);
    error = network_connect::ErrorString(network_error);
    if (error.empty())
      error = l10n_util::GetStringUTF16(IDS_CHROMEOS_NETWORK_ERROR_UNKNOWN);
  }
  NET_LOG_ERROR("Connect error notification: " + UTF16ToUTF8(error),
                service_path);

  std::string network_name =
      NetworkState::GetNameFromProperties(service_path, shill_properties);
  std::string network_error_details;
  shill_properties.GetStringWithoutPathExpansion(
        shill::kErrorDetailsProperty, &network_error_details);

  string16 error_msg;
  if (!network_error_details.empty()) {
    error_msg = l10n_util::GetStringFUTF16(
        IDS_NETWORK_CONNECTION_ERROR_MESSAGE_WITH_SERVER_MESSAGE,
        UTF8ToUTF16(network_name), error,
        UTF8ToUTF16(network_error_details));
  } else {
    error_msg = l10n_util::GetStringFUTF16(
        IDS_NETWORK_CONNECTION_ERROR_MESSAGE_WITH_DETAILS,
        UTF8ToUTF16(network_name), error);
  }

  std::string network_type;
  shill_properties.GetStringWithoutPathExpansion(
      flimflam::kTypeProperty, &network_type);

  NetworkObserver::NetworkType type = NetworkObserver::NETWORK_UNKNOWN;
  if (network_type == flimflam::kTypeCellular) {
    std::string network_technology;
    shill_properties.GetStringWithoutPathExpansion(
        flimflam::kNetworkTechnologyProperty, &network_technology);
    if (network_technology == flimflam::kNetworkTechnologyLte ||
        network_technology == flimflam::kNetworkTechnologyLteAdvanced)
      type = NetworkObserver::NETWORK_CELLULAR_LTE;
    else
      type = NetworkObserver::NETWORK_CELLULAR;
  } else if (network_type == flimflam::kTypeEthernet) {
    type = NetworkObserver:: NETWORK_ETHERNET;
  } else if (network_type == flimflam::kTypeWifi) {
    type = NetworkObserver:: NETWORK_WIFI;
  } else {
    NOTREACHED();
  }

  std::vector<string16> no_links;
  ash::Shell::GetInstance()->system_tray_notifier()->NotifySetNetworkMessage(
      this,
      NetworkObserver::ERROR_CONNECT_FAILED,
      type,
      l10n_util::GetStringUTF16(IDS_NETWORK_CONNECTION_ERROR_TITLE),
      error_msg,
      no_links);
}

}  // namespace ash
