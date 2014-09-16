// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_FAKE_EASY_UNLOCK_CLIENT_H_
#define CHROMEOS_DBUS_FAKE_EASY_UNLOCK_CLIENT_H_

#include <string>

#include "chromeos/dbus/easy_unlock_client.h"

namespace chromeos {

// A fake implemetation of EasyUnlockClient.
class CHROMEOS_EXPORT FakeEasyUnlockClient : public EasyUnlockClient {
 public:
  // Tests if the provided keys belong to the same (fake) EC P256 key pair
  // generated by |this|.
  static bool IsEcP256KeyPair(const std::string& private_key,
                              const std::string& public_key);

  FakeEasyUnlockClient();
  virtual ~FakeEasyUnlockClient();

  // EasyUnlockClient overrides
  virtual void Init(dbus::Bus* bus) OVERRIDE;
  virtual void GenerateEcP256KeyPair(const KeyPairCallback& callback) OVERRIDE;
  virtual void PerformECDHKeyAgreement(const std::string& private_key,
                                       const std::string& public_key,
                                       const DataCallback& callback) OVERRIDE;
  virtual void CreateSecureMessage(const std::string& payload,
                                   const CreateSecureMessageOptions& options,
                                   const DataCallback& callback) OVERRIDE;
  virtual void UnwrapSecureMessage(const std::string& message,
                                   const UnwrapSecureMessageOptions& options,
                                   const DataCallback& callback) OVERRIDE;

 private:
  int generated_keys_count_;

  DISALLOW_COPY_AND_ASSIGN(FakeEasyUnlockClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_FAKE_EASY_UNLOCK_CLIENT_H_
