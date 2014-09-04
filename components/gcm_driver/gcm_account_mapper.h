// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_GCM_DRIVER_GCM_ACCOUNT_MAPPER_H_
#define COMPONENTS_GCM_DRIVER_GCM_ACCOUNT_MAPPER_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "components/gcm_driver/gcm_app_handler.h"
#include "components/gcm_driver/gcm_client.h"
#include "google_apis/gcm/engine/account_mapping.h"

namespace base {
class Clock;
}

namespace gcm {

class GCMDriver;

// Class for mapping signed-in GAIA accounts to the GCM Device ID.
class GCMAccountMapper : public GCMAppHandler {
 public:
  // List of account mappings.
  typedef std::vector<AccountMapping> AccountMappings;

  explicit GCMAccountMapper(GCMDriver* gcm_driver);
  virtual ~GCMAccountMapper();

  void Initialize(const AccountMappings& account_mappings,
                  const std::string& registration_id);

  // Called by AccountTracker, when a new list of account tokens is available.
  // This will cause a refresh of account mappings and sending updates to GCM.
  void SetAccountTokens(
      const std::vector<GCMClient::AccountTokenInfo> account_tokens);

  // Implementation of GCMAppHandler:
  virtual void ShutdownHandler() OVERRIDE;
  virtual void OnMessage(const std::string& app_id,
                         const GCMClient::IncomingMessage& message) OVERRIDE;
  virtual void OnMessagesDeleted(const std::string& app_id) OVERRIDE;
  virtual void OnSendError(
      const std::string& app_id,
      const GCMClient::SendErrorDetails& send_error_details) OVERRIDE;
  virtual void OnSendAcknowledged(const std::string& app_id,
                                  const std::string& message_id) OVERRIDE;
  virtual bool CanHandle(const std::string& app_id) const OVERRIDE;

 private:
  friend class GCMAccountMapperTest;

  typedef std::map<std::string, GCMClient::OutgoingMessage> OutgoingMessages;

  // Informs GCM of an added or refreshed account mapping.
  void SendAddMappingMessage(AccountMapping& account_mapping);

  // Informs GCM of a removed account mapping.
  void SendRemoveMappingMessage(AccountMapping& account_mapping);

  void CreateAndSendMessage(const AccountMapping& account_mapping);

  // Callback for sending a message.
  void OnSendFinished(const std::string& account_id,
                      const std::string& message_id,
                      GCMClient::Result result);

  // Checks whether the update can be triggered now. If the current time is
  // within reasonable time (6 hours) of when the update is due, we want to
  // trigger the update immediately to take advantage of a fresh OAuth2 token.
  bool CanTriggerUpdate(const base::Time& last_update_time) const;

  // Checks whether last status change is older than a TTL of a message.
  bool IsLastStatusChangeOlderThanTTL(
      const AccountMapping& account_mapping) const;

  // Finds an account mapping in |accounts_| by |account_id|.
  AccountMapping* FindMappingByAccountId(const std::string& account_id);
  // Finds an account mapping in |accounts_| by |message_id|.
  // Returns iterator that can be used to delete the account.
  AccountMappings::iterator FindMappingByMessageId(
      const std::string& message_id);

  // Sets the clock for testing.
  void SetClockForTesting(scoped_ptr<base::Clock> clock);

  // GCMDriver owns GCMAccountMapper.
  GCMDriver* gcm_driver_;

  // Clock for timestamping status changes.
  scoped_ptr<base::Clock> clock_;

  // Currnetly tracked account mappings.
  AccountMappings accounts_;

  // GCM Registration ID of the account mapper.
  std::string registration_id_;

  bool initialized_;

  base::WeakPtrFactory<GCMAccountMapper> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(GCMAccountMapper);
};

}  // namespace gcm

#endif  // COMPONENTS_GCM_DRIVER_GCM_ACCOUNT_MAPPER_H_
