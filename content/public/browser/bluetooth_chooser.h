// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_BLUETOOTH_CHOOSER_H_
#define CONTENT_PUBLIC_BROWSER_BLUETOOTH_CHOOSER_H_

#include <string>

#include "base/callback.h"
#include "base/strings/string16.h"
#include "content/common/content_export.h"

namespace content {

// Represents a way to ask the user to select a Bluetooth device from a list of
// options.
class CONTENT_EXPORT BluetoothChooser {
 public:
  enum class Event {
    // The user cancelled the chooser instead of selecting a device.
    CANCELLED,
    // The user selected device |opt_device_id|.
    SELECTED,

    // As the dialog implementations grow more user-visible buttons and knobs,
    // we'll add enumerators here to support them.
  };

  // Chooser implementations are constructed with an |EventHandler| and report
  // user interaction with the chooser through it. |opt_device_id| is an empty
  // string except for Event::SELECTED.
  //
  // The EventHandler won't be called after the chooser object is destroyed.
  //
  // After the EventHandler is called with Event::CANCELLED or Event::SELECTED,
  // it won't be called again, and users must not call any more BluetoothChooser
  // methods.
  typedef base::Callback<void(Event, const std::string& opt_device_id)>
      EventHandler;

  BluetoothChooser() {}
  virtual ~BluetoothChooser();

  // Lets the chooser tell the user the state of the Bluetooth adapter. This
  // defaults to POWERED_ON.
  enum class AdapterPresence { ABSENT, POWERED_OFF, POWERED_ON };
  virtual void SetAdapterPresence(AdapterPresence presence) {}

  // Lets the chooser tell the user whether discovery is happening. This
  // defaults to DISCOVERING.
  enum class DiscoveryState { FAILED_TO_START, DISCOVERING, IDLE };
  virtual void ShowDiscoveryState(DiscoveryState state) {}

  // Shows a new device in the chooser.
  virtual void AddDevice(const std::string& device_id,
                         const base::string16& device_name) {}

  // Tells the chooser that a device is no longer available. The chooser should
  // not call DeviceSelected() for a device that's been removed.
  virtual void RemoveDevice(const std::string& device_id) {}
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_BLUETOOTH_CHOOSER_H_
