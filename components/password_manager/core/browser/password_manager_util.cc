// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/password_manager_util.h"

#include <algorithm>

#include "components/autofill/core/common/password_form.h"
#include "components/sync_driver/sync_service.h"

namespace password_manager_util {

password_manager::PasswordSyncState GetPasswordSyncState(
    const sync_driver::SyncService* sync_service) {
  if (sync_service && sync_service->HasSyncSetupCompleted() &&
      sync_service->IsSyncActive() &&
      sync_service->GetActiveDataTypes().Has(syncer::PASSWORDS)) {
    return sync_service->IsUsingSecondaryPassphrase()
               ? password_manager::SYNCING_WITH_CUSTOM_PASSPHRASE
               : password_manager::SYNCING_NORMAL_ENCRYPTION;
  }
  return password_manager::NOT_SYNCING_PASSWORDS;
}

void FindDuplicates(
    ScopedVector<autofill::PasswordForm>* forms,
    ScopedVector<autofill::PasswordForm>* duplicates,
    std::vector<std::vector<autofill::PasswordForm*>>* tag_groups) {
  if (forms->empty())
    return;

  // Linux backends used to treat the first form as a prime oneamong the
  // duplicates. Therefore, the caller should try to preserve it.
  std::stable_sort(forms->begin(), forms->end(), autofill::LessThanUniqueKey());

  ScopedVector<autofill::PasswordForm> unique_forms;
  unique_forms.push_back(forms->front());
  forms->front() = nullptr;
  if (tag_groups) {
    tag_groups->clear();
    tag_groups->push_back(std::vector<autofill::PasswordForm*>());
    tag_groups->front().push_back(unique_forms.front());
  }
  for (auto it = forms->begin() + 1; it != forms->end(); ++it) {
    if (ArePasswordFormUniqueKeyEqual(**it, *unique_forms.back())) {
      duplicates->push_back(*it);
      if (tag_groups)
        tag_groups->back().push_back(*it);
    } else {
      unique_forms.push_back(*it);
      if (tag_groups)
        tag_groups->push_back(std::vector<autofill::PasswordForm*>(1, *it));
    }
    *it = nullptr;
  }
  forms->weak_clear();
  forms->swap(unique_forms);
}

}  // namespace password_manager_util
