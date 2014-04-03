// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_GATT_CHARACTERISTIC_H_
#define DEVICE_BLUETOOTH_GATT_CHARACTERISTIC_H_

#include <vector>

#include "base/basictypes.h"
#include "base/callback.h"
#include "device/bluetooth/bluetooth_utils.h"

namespace device {

class BluetoothGattDescriptor;
class BluetoothGattService;

// BluetoothGattCharacteristic represents a local or remote GATT characteristic.
// A GATT characteristic is a basic data element used to construct a GATT
// service. Hence, instances of a BluetoothGattCharacteristic are associated
// with a BluetoothGattService. There are two ways in which this class is used:
//
//   1. To represent GATT characteristics that belong to a service hosted by a
//      a remote device. In this case the characteristic will be constructed by
//      the subsystem.
//   2. To represent GATT characteristics that belong to a locally hosted
//      service. To achieve this, users can construct instances of
//      BluetoothGattCharacteristic directly and add it to the desired
//      BluetoothGattService instance that represents a local service.
class BluetoothGattCharacteristic {
 public:
  // Values representing the possible properties of a characteristic, which
  // define how the characteristic can be used. Each of these properties serve
  // a role as defined in the Bluetooth Specification.
  // |kPropertyExtendedProperties| is a special property that, if present,
  // indicates that there is a characteristic descriptor (namely the
  // "Characteristic Extended Properties Descriptor" with UUID 0x2900) that
  // contains additional properties pertaining to the characteristic.
  enum Property {
    kPropertyNone = 0,
    kPropertyBroadcast = 1 << 0,
    kPropertyRead = 1 << 1,
    kPropertyWriteWithoutResponse = 1 << 2,
    kPropertyWrite = 1 << 3,
    kPropertyNotify = 1 << 4,
    kPropertyIndicate = 1 << 5,
    kPropertyAuthenticatedSignedWrites = 1 << 6,
    kPropertyExtendedProperties = 1 << 7
  };
  typedef uint32 Properties;

  // Values representing read, write, and encryption permissions for a
  // characteristic's value. While attribute permissions for all GATT
  // definitions have been set by the Bluetooth specification, characteristic
  // value permissions are left up to the higher-level profile.
  //
  // Attribute permissions are distinct from the characteristic properties. For
  // example, a characteristic may have the property |kPropertyRead| to make
  // clients know that it is possible to read the characteristic value and have
  // the permission |kPermissionReadEncrypted| to require a secure connection.
  // It is up to the application to properly specify the permissions and
  // properties for a local characteristic.
  enum Permission {
    kPermissionNone = 0,
    kPermissionRead = 1 << 0,
    kPermissionWrite = 1 << 1,
    kPermissionReadEncrypted = 1 << 2,
    kPermissionWriteEncrypted = 1 << 3
  };
  typedef uint32 Permissions;

  // Interface for observing changes from a BluetoothGattCharacteristic.
  // Properties of remote characteristics are received asynchonously. The
  // Observer interface can be used to be notified when the initial values of a
  // characteristic are received as well as when successive changes occur during
  // its life cycle.
  class Observer {
   public:
    // Called when the UUID of |characteristic| has changed.
    virtual void UuidChanged(
        BluetoothGattCharacteristic* characteristic,
        const bluetooth_utils::UUID& uuid) {}

    // Called when the current value of |characteristic| has changed.
    virtual void ValueChanged(
        BluetoothGattCharacteristic* characteristic,
        const std::vector<uint8>& value) {}

    // Called when the descriptors that are associated with |characteristic|
    // have changed.
    virtual void DescriptorsChanged(
        BluetoothGattCharacteristic* characteristic,
        const std::vector<BluetoothGattDescriptor*>& descriptors) {}
  };

  // The ErrorCallback is used by methods to asynchronously report errors.
  typedef base::Callback<void(const std::string&)> ErrorCallback;

  // The ValueCallback is used to return the value of a remote characteristic
  // upon a read request.
  typedef base::Callback<void(const std::vector<uint8>&)> ValueCallback;

  // Adds and removes observers for events on this GATT characteristic. If
  // monitoring multiple characteristics, check the |characteristic| parameter
  // of observer methods to determine which characteristic is issuing the event.
  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;

  // Constructs a BluetoothGattCharacteristic that can be associated with a
  // local GATT service when the adapter is in the peripheral role. To
  // associate the returned characteristic with a service, add it to a local
  // service by calling BluetoothGattService::AddCharacteristic.
  //
  // This method constructs a characteristic with UUID |uuid|, initial cached
  // value |value|, properties |properties|, and permissions |permissions|.
  // |value| will be cached and returned for read requests and automatically set
  // for write requests by default, unless an instance of
  // BluetoothGattService::Delegate has been provided to the associated
  // BluetoothGattService instance, in which case the delegate will handle read
  // and write requests.
  //
  // NOTE: Don't explicitly set |kPropertyExtendedProperties| in |properties|.
  // Instead, create and add a BluetoothGattDescriptor that represents the
  // "Characteristic Extended Properties" descriptor and this will automatically
  // set the correspoding bit in the characteristic's properties field. If
  // |properties| has |kPropertyExtendedProperties| set, it will be ignored.
  static BluetoothGattCharacteristic* Create(const bluetooth_utils::UUID& uuid,
                                             const std::vector<uint8>& value,
                                             Properties properties,
                                             Permissions permissions);

  // The Bluetooth-specific UUID of the characteristic.
  virtual const bluetooth_utils::UUID& GetUuid() const = 0;

  // Returns true, if this characteristic is hosted locally. If false, then this
  // instance represents a remote GATT characteristic.
  virtual bool IsLocal() const = 0;

  // Returns a pointer to the GATT service this characteristic belongs to.
  virtual const BluetoothGattService* GetService() const = 0;

  // Returns the list of GATT characteristic descriptors that provide more
  // information about this characteristic.
  virtual const std::vector<BluetoothGattDescriptor*>
      GetDescriptors() const = 0;

  // Adds a characteristic descriptor to the locally hosted characteristic
  // represented by this instance. This method only makes sense for local
  // characteristics and won't have an effect if this instance represents a
  // remote GATT service and will return false. This method takes ownership
  // of |descriptor|.
  virtual bool AddDescriptor(BluetoothGattDescriptor* descriptor) = 0;

  // For locally hosted characteristics, updates the characteristic's value.
  // This will update the value that is visible to remote devices and send out
  // any notifications and indications that have been configured. This method
  // can be used in place of, and in conjunction with,
  // BluetoothGattService::Delegate methods to send updates to remote devices,
  // or simply to set update the cached value for read requests without having
  // to implement the delegate methods.
  //
  // This method only makes sense for local characteristics and does nothing and
  // returns false if this instance represents a remote characteristic.
  virtual bool UpdateValue(const std::vector<uint8>& value) = 0;

  // Sends a read request to a remote characteristic to read its value.
  // |callback| is called to return the read value on success and
  // |error_callback| is called for failures.
  virtual void ReadRemoteCharacteristic(
      const ValueCallback& callback,
      const ErrorCallback& error_callback) = 0;

  // Sends a write request to a remote characteristic, to modify the
  // characteristic's value starting at offset |offset| with the new value
  // |new_value|. |callback| is called to signal success and |error_callback|
  // for failures. This method only applies to remote characteristics and will
  // fail for those that are locally hosted.
  virtual void WriteRemoteCharacteristic(
      int offset,
      const std::vector<uint8>& new_value,
      const base::Closure& callback,
      const ErrorCallback& error_callback) = 0;

 protected:
  BluetoothGattCharacteristic();
  virtual ~BluetoothGattCharacteristic();

 private:
  DISALLOW_COPY_AND_ASSIGN(BluetoothGattCharacteristic);
};

}  // namespace device

#endif  // DEVICE_BLUETOOTH_GATT_CHARACTERISTIC_H_
