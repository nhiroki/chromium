// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_BLUETOOTH_INTERNALS_BLUETOOTH_INTERNALS_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_BLUETOOTH_INTERNALS_BLUETOOTH_INTERNALS_HANDLER_H_

#include "base/macros.h"
#include "chrome/browser/ui/webui/bluetooth_internals/bluetooth_internals.mojom.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

// Handles API requests from chrome://bluetooth-internals page by implementing
// mojom::BluetoothInternalsHandler.
class BluetoothInternalsHandler : public mojom::BluetoothInternalsHandler {
 public:
  explicit BluetoothInternalsHandler(
      mojo::PendingReceiver<mojom::BluetoothInternalsHandler> receiver);
  ~BluetoothInternalsHandler() override;

  // mojom::BluetoothInternalsHandler overrides:
  void GetAdapter(GetAdapterCallback callback) override;

 private:
  void OnGetAdapter(GetAdapterCallback callback,
                    scoped_refptr<device::BluetoothAdapter> adapter);

  mojo::Receiver<mojom::BluetoothInternalsHandler> receiver_;
  base::WeakPtrFactory<BluetoothInternalsHandler> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(BluetoothInternalsHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_BLUETOOTH_INTERNALS_BLUETOOTH_INTERNALS_HANDLER_H_
