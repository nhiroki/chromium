// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_SERIAL_SERIAL_CONNECTION_H_
#define DEVICE_SERIAL_SERIAL_CONNECTION_H_

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "device/serial/serial.mojom.h"
#include "mojo/public/cpp/bindings/interface_impl.h"

namespace device {

class DataSinkReceiver;
class DataSourceSender;
class ReadOnlyBuffer;
class SerialIoHandler;
class WritableBuffer;

class SerialConnection : public mojo::InterfaceImpl<serial::Connection> {
 public:
  SerialConnection(scoped_refptr<SerialIoHandler> io_handler,
                   mojo::InterfaceRequest<serial::DataSink> sink,
                   mojo::InterfaceRequest<serial::DataSource> source);
  virtual ~SerialConnection();

  // mojo::InterfaceImpl<serial::Connection> overrides.
  virtual void GetInfo(
      const mojo::Callback<void(serial::ConnectionInfoPtr)>& callback) OVERRIDE;
  virtual void SetOptions(serial::ConnectionOptionsPtr options,
                          const mojo::Callback<void(bool)>& callback) OVERRIDE;
  virtual void SetControlSignals(
      serial::HostControlSignalsPtr signals,
      const mojo::Callback<void(bool)>& callback) OVERRIDE;
  virtual void GetControlSignals(const mojo::Callback<
      void(serial::DeviceControlSignalsPtr)>& callback) OVERRIDE;
  virtual void Flush(const mojo::Callback<void(bool)>& callback) OVERRIDE;

 private:
  void OnSendPipeReady(scoped_ptr<ReadOnlyBuffer> buffer);
  void OnSendCancelled(int32_t error);
  void OnReceivePipeReady(scoped_ptr<WritableBuffer> buffer);

  scoped_refptr<SerialIoHandler> io_handler_;
  scoped_refptr<DataSinkReceiver> receiver_;
  scoped_refptr<DataSourceSender> sender_;

  DISALLOW_COPY_AND_ASSIGN(SerialConnection);
};

}  // namespace device

#endif  // DEVICE_SERIAL_SERIAL_CONNECTION_H_
