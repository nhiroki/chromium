// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/gcm_driver/gcm_account_mapper.h"

#include "base/bind.h"
#include "base/guid.h"
#include "base/time/clock.h"
#include "base/time/default_clock.h"
#include "components/gcm_driver/gcm_driver_desktop.h"
#include "google_apis/gcm/engine/gcm_store.h"

namespace gcm {

namespace {

const char kGCMAccountMapperSenderId[] = "745476177629";
const char kGCMAccountMapperAppId[] = "com.google.android.gms";
const int kGCMAddMappingMessageTTL = 30 * 60;  // 0.5 hours in seconds.
const int kGCMRemoveMappingMessageTTL = 24 * 60 * 60;  // 1 day in seconds.
const int kGCMUpdateIntervalHours = 24;
// Because adding an account mapping dependents on a fresh OAuth2 token, we
// allow the update to happen earlier than update due time, if it is within
// the early start time to take advantage of that token.
const int kGCMUpdateEarlyStartHours = 6;
const char kRegistrationIdMessgaeKey[] = "id";
const char kTokenMessageKey[] = "t";
const char kAccountMessageKey[] = "a";
const char kRemoveAccountKey[] = "r";
const char kRemoveAccountValue[] = "1";

std::string GenerateMessageID() {
  return base::GenerateGUID();
}

}  // namespace

GCMAccountMapper::GCMAccountMapper(GCMDriver* gcm_driver)
    : gcm_driver_(gcm_driver),
      clock_(new base::DefaultClock),
      initialized_(false),
      weak_ptr_factory_(this) {
}

GCMAccountMapper::~GCMAccountMapper() {
}

void GCMAccountMapper::Initialize(
    const std::vector<AccountMapping>& account_mappings,
    const std::string& registration_id) {
  DCHECK(!initialized_);
  initialized_ = true;
  registration_id_ = registration_id;

  accounts_ = account_mappings;

  gcm_driver_->AddAppHandler(kGCMAccountMapperAppId, this);

  // TODO(fgorski): if no registration ID, get registration ID.
}

void GCMAccountMapper::SetAccountTokens(
    const std::vector<GCMClient::AccountTokenInfo> account_tokens) {
  DCHECK(initialized_);

  // Start from removing the old tokens, from all of the known accounts.
  for (AccountMappings::iterator iter = accounts_.begin();
       iter != accounts_.end();
       ++iter) {
    iter->access_token.clear();
  }

  // Update the internal collection of mappings with the new tokens.
  for (std::vector<GCMClient::AccountTokenInfo>::const_iterator token_iter =
           account_tokens.begin();
       token_iter != account_tokens.end();
       ++token_iter) {
    AccountMapping* account_mapping =
        FindMappingByAccountId(token_iter->account_id);
    if (!account_mapping) {
      AccountMapping new_mapping;
      new_mapping.status = AccountMapping::NEW;
      new_mapping.account_id = token_iter->account_id;
      new_mapping.access_token = token_iter->access_token;
      new_mapping.email = token_iter->email;
      accounts_.push_back(new_mapping);
    } else {
      // Since we got a token for an account, drop the remove message and treat
      // it as mapped.
      if (account_mapping->status == AccountMapping::REMOVING) {
        account_mapping->status = AccountMapping::MAPPED;
        account_mapping->status_change_timestamp = base::Time();
        account_mapping->last_message_id.clear();
      }

      account_mapping->email = token_iter->email;
      account_mapping->access_token = token_iter->access_token;
    }
  }

  // Decide what to do with each account (either start mapping, or start
  // removing).
  for (AccountMappings::iterator mappings_iter = accounts_.begin();
       mappings_iter != accounts_.end();
       ++mappings_iter) {
    if (mappings_iter->access_token.empty()) {
      // Send a remove message if the account was not previously being removed,
      // or it doesn't have a pending message, or the pending message is
      // already expired, but OnSendError event was lost.
      if (mappings_iter->status != AccountMapping::REMOVING ||
          mappings_iter->last_message_id.empty() ||
          IsLastStatusChangeOlderThanTTL(*mappings_iter)) {
        SendRemoveMappingMessage(*mappings_iter);
      }
    } else {
      // A message is sent for all of the mappings considered NEW, or mappings
      // that are ADDING, but have expired message (OnSendError event lost), or
      // for those mapped accounts that can be refreshed.
      if (mappings_iter->status == AccountMapping::NEW ||
          (mappings_iter->status == AccountMapping::ADDING &&
           IsLastStatusChangeOlderThanTTL(*mappings_iter)) ||
          (mappings_iter->status == AccountMapping::MAPPED &&
           CanTriggerUpdate(mappings_iter->status_change_timestamp))) {
        mappings_iter->last_message_id.clear();
        SendAddMappingMessage(*mappings_iter);
      }
    }
  }
}

void GCMAccountMapper::ShutdownHandler() {
  gcm_driver_->RemoveAppHandler(kGCMAccountMapperAppId);
}

void GCMAccountMapper::OnMessage(const std::string& app_id,
                                 const GCMClient::IncomingMessage& message) {
  // Account message does not expect messages right now.
}

void GCMAccountMapper::OnMessagesDeleted(const std::string& app_id) {
  // Account message does not expect messages right now.
}

void GCMAccountMapper::OnSendError(
    const std::string& app_id,
    const GCMClient::SendErrorDetails& send_error_details) {
  DCHECK_EQ(app_id, kGCMAccountMapperAppId);

  AccountMappings::iterator account_mapping_it =
      FindMappingByMessageId(send_error_details.message_id);

  if (account_mapping_it == accounts_.end())
    return;

  if (send_error_details.result != GCMClient::TTL_EXCEEDED) {
    DVLOG(1) << "Send error result different than TTL EXCEEDED: "
             << send_error_details.result << ". "
             << "Postponing the retry until a new batch of tokens arrives.";
    return;
  }

  if (account_mapping_it->status == AccountMapping::REMOVING) {
    // Another message to remove mapping can be sent immediately, because TTL
    // for those is one day. No need to back off.
    SendRemoveMappingMessage(*account_mapping_it);
  } else {
    if (account_mapping_it->status == AccountMapping::ADDING) {
      // There is no mapping established, so we can remove the entry.
      // Getting a fresh token will trigger a new attempt.
      gcm_driver_->RemoveAccountMapping(account_mapping_it->account_id);
      accounts_.erase(account_mapping_it);
    } else {
      // Account is already MAPPED, we have to wait for another token.
      account_mapping_it->last_message_id.clear();
      gcm_driver_->UpdateAccountMapping(*account_mapping_it);
    }
  }
}

void GCMAccountMapper::OnSendAcknowledged(const std::string& app_id,
                                          const std::string& message_id) {
  DCHECK_EQ(app_id, kGCMAccountMapperAppId);
  AccountMappings::iterator account_mapping_it =
      FindMappingByMessageId(message_id);

  DVLOG(1) << "OnSendAcknowledged with message ID: " << message_id;

  if (account_mapping_it == accounts_.end())
    return;

  // Here is where we advance a status of a mapping and persist or remove.
  if (account_mapping_it->status == AccountMapping::REMOVING) {
    // Message removing the account has been confirmed by the GCM, we can remove
    // all the information related to the account (from memory and store).
    gcm_driver_->RemoveAccountMapping(account_mapping_it->account_id);
    accounts_.erase(account_mapping_it);
  } else {
    // Mapping status is ADDING only when it is a first time mapping.
    DCHECK(account_mapping_it->status == AccountMapping::ADDING ||
           account_mapping_it->status == AccountMapping::MAPPED);

    // Account is marked as mapped with the current time.
    account_mapping_it->status = AccountMapping::MAPPED;
    account_mapping_it->status_change_timestamp = clock_->Now();
    // There is no pending message for the account.
    account_mapping_it->last_message_id.clear();

    gcm_driver_->UpdateAccountMapping(*account_mapping_it);
  }
}

bool GCMAccountMapper::CanHandle(const std::string& app_id) const {
  return app_id.compare(kGCMAccountMapperAppId) == 0;
}

void GCMAccountMapper::SendAddMappingMessage(AccountMapping& account_mapping) {
  CreateAndSendMessage(account_mapping);
}

void GCMAccountMapper::SendRemoveMappingMessage(
    AccountMapping& account_mapping) {
  // We want to persist an account that is being removed as quickly as possible
  // as well as clean up the last message information.
  if (account_mapping.status != AccountMapping::REMOVING) {
    account_mapping.status = AccountMapping::REMOVING;
    account_mapping.status_change_timestamp = clock_->Now();
  }

  account_mapping.last_message_id.clear();

  gcm_driver_->UpdateAccountMapping(account_mapping);

  CreateAndSendMessage(account_mapping);
}

void GCMAccountMapper::CreateAndSendMessage(
    const AccountMapping& account_mapping) {
  GCMClient::OutgoingMessage outgoing_message;
  outgoing_message.id = GenerateMessageID();
  outgoing_message.data[kRegistrationIdMessgaeKey] = registration_id_;
  outgoing_message.data[kAccountMessageKey] = account_mapping.email;

  if (account_mapping.status == AccountMapping::REMOVING) {
    outgoing_message.time_to_live = kGCMRemoveMappingMessageTTL;
    outgoing_message.data[kRemoveAccountKey] = kRemoveAccountValue;
  } else {
    outgoing_message.data[kTokenMessageKey] = account_mapping.access_token;
    outgoing_message.time_to_live = kGCMAddMappingMessageTTL;
  }

  gcm_driver_->Send(kGCMAccountMapperAppId,
                    kGCMAccountMapperSenderId,
                    outgoing_message,
                    base::Bind(&GCMAccountMapper::OnSendFinished,
                               weak_ptr_factory_.GetWeakPtr(),
                               account_mapping.account_id));
}


void GCMAccountMapper::OnSendFinished(const std::string& account_id,
                                      const std::string& message_id,
                                      GCMClient::Result result) {
  // TODO(fgorski): Add another attempt, in case the QUEUE is not full.
  if (result != GCMClient::SUCCESS)
    return;

  AccountMapping* account_mapping = FindMappingByAccountId(account_id);
  DCHECK(account_mapping);

  // If we are dealing with account with status NEW, it is the first time
  // mapping, and we should mark it as ADDING.
  if (account_mapping->status == AccountMapping::NEW) {
    account_mapping->status = AccountMapping::ADDING;
    account_mapping->status_change_timestamp = clock_->Now();
  }

  account_mapping->last_message_id = message_id;

  gcm_driver_->UpdateAccountMapping(*account_mapping);
}

bool GCMAccountMapper::CanTriggerUpdate(
    const base::Time& last_update_time) const {
  return last_update_time +
             base::TimeDelta::FromHours(kGCMUpdateIntervalHours -
                                        kGCMUpdateEarlyStartHours) <
         clock_->Now();
}

bool GCMAccountMapper::IsLastStatusChangeOlderThanTTL(
    const AccountMapping& account_mapping) const {
  int ttl_seconds = account_mapping.status == AccountMapping::REMOVING ?
      kGCMRemoveMappingMessageTTL : kGCMAddMappingMessageTTL;
  return account_mapping.status_change_timestamp +
      base::TimeDelta::FromSeconds(ttl_seconds) < clock_->Now();
}

AccountMapping* GCMAccountMapper::FindMappingByAccountId(
    const std::string& account_id) {
  for (AccountMappings::iterator iter = accounts_.begin();
       iter != accounts_.end();
       ++iter) {
    if (iter->account_id == account_id)
      return &*iter;
  }

  return NULL;
}

GCMAccountMapper::AccountMappings::iterator
GCMAccountMapper::FindMappingByMessageId(const std::string& message_id) {
  for (std::vector<AccountMapping>::iterator iter = accounts_.begin();
       iter != accounts_.end();
       ++iter) {
    if (iter->last_message_id == message_id)
      return iter;
  }

  return accounts_.end();
}

void GCMAccountMapper::SetClockForTesting(scoped_ptr<base::Clock> clock) {
  clock_ = clock.Pass();
}

}  // namespace gcm
