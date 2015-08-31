// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_driver/fake_sync_service.h"

#include "base/values.h"
#include "sync/internal_api/public/base_transaction.h"
#include "sync/internal_api/public/sessions/sync_session_snapshot.h"
#include "sync/internal_api/public/user_share.h"

namespace sync_driver {

FakeSyncService::FakeSyncService() : error_(GoogleServiceAuthError::NONE) {
}

FakeSyncService::~FakeSyncService() {
}

bool FakeSyncService::HasSyncSetupCompleted() const {
  return false;
}

bool FakeSyncService::IsSyncAllowed() const {
  return false;
}

bool FakeSyncService::IsSyncActive() const {
  return false;
}

syncer::ModelTypeSet FakeSyncService::GetActiveDataTypes() const {
  return syncer::ModelTypeSet();
}

void FakeSyncService::AddObserver(SyncServiceObserver* observer) {
}

void FakeSyncService::RemoveObserver(SyncServiceObserver* observer) {
}

bool FakeSyncService::HasObserver(const SyncServiceObserver* observer) const {
  return false;
}

bool FakeSyncService::CanSyncStart() const {
  return false;
}

void FakeSyncService::OnDataTypeRequestsSyncStartup(syncer::ModelType type) {
}

void FakeSyncService::RequestStop(
    sync_driver::SyncService::SyncStopDataFate data_fate) {
}

void FakeSyncService::RequestStart() {
}

syncer::ModelTypeSet FakeSyncService::GetPreferredDataTypes() const {
  return syncer::ModelTypeSet();
}

void FakeSyncService::OnUserChoseDatatypes(bool sync_everything,
                                           syncer::ModelTypeSet chosen_types) {
}

void FakeSyncService::SetSyncSetupCompleted() {
}

bool FakeSyncService::FirstSetupInProgress() const {
  return false;
}

void FakeSyncService::SetSetupInProgress(bool setup_in_progress) {
}

bool FakeSyncService::setup_in_progress() const {
  return false;
}

bool FakeSyncService::ConfigurationDone() const {
  return false;
}

const GoogleServiceAuthError& FakeSyncService::GetAuthError() const {
  return error_;
}

bool FakeSyncService::HasUnrecoverableError() const {
  return false;
}

bool FakeSyncService::backend_initialized() const {
  return false;
}

OpenTabsUIDelegate* FakeSyncService::GetOpenTabsUIDelegate() {
  return nullptr;
}

bool FakeSyncService::IsPassphraseRequiredForDecryption() const {
  return false;
}

base::Time FakeSyncService::GetExplicitPassphraseTime() const {
  return base::Time();
}

bool FakeSyncService::IsUsingSecondaryPassphrase() const {
  return false;
}

void FakeSyncService::EnableEncryptEverything() {
}

bool FakeSyncService::EncryptEverythingEnabled() const {
  return false;
}

void FakeSyncService::SetEncryptionPassphrase(const std::string& passphrase,
                                              PassphraseType type) {
}

bool FakeSyncService::SetDecryptionPassphrase(const std::string& passphrase) {
  return false;
}

bool FakeSyncService::IsCryptographerReady(
    const syncer::BaseTransaction* trans) const {
  return false;
}

syncer::UserShare* FakeSyncService::GetUserShare() const {
  return new syncer::UserShare();
}

LocalDeviceInfoProvider* FakeSyncService::GetLocalDeviceInfoProvider() const {
  return nullptr;
}

void FakeSyncService::RegisterDataTypeController(
      sync_driver::DataTypeController* data_type_controller) {
}

void FakeSyncService::ReenableDatatype(syncer::ModelType type) {}

void FakeSyncService::DeactivateDataType(syncer::ModelType type) {}

bool FakeSyncService::IsPassphraseRequired() const {
  return false;
}

syncer::ModelTypeSet FakeSyncService::GetEncryptedDataTypes() const {
  return syncer::ModelTypeSet();
}

FakeSyncService::SyncTokenStatus FakeSyncService::GetSyncTokenStatus() const {
  return FakeSyncService::SyncTokenStatus();
}

std::string FakeSyncService::QuerySyncStatusSummaryString() {
  return "";
}

bool FakeSyncService::QueryDetailedSyncStatus(syncer::SyncStatus* result) {
  return false;
}

base::string16 FakeSyncService::GetLastSyncedTimeString() const {
  return base::string16();
}

std::string FakeSyncService::GetBackendInitializationStateString() const {
  return std::string();
}

syncer::sessions::SyncSessionSnapshot FakeSyncService::GetLastSessionSnapshot()
    const {
  return syncer::sessions::SyncSessionSnapshot();
}

base::Value* FakeSyncService::GetTypeStatusMap() const {
  return new base::ListValue();
}

const GURL& FakeSyncService::sync_service_url() const {
  return sync_service_url_;
}

std::string FakeSyncService::unrecoverable_error_message() const {
  return unrecoverable_error_message_;
}

tracked_objects::Location FakeSyncService::unrecoverable_error_location()
    const {
  return tracked_objects::Location();
}

}  // namespace sync_driver
