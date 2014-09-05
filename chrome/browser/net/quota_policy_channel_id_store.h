// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_QUOTA_POLICY_CHANNEL_ID_STORE_H_
#define CHROME_BROWSER_NET_QUOTA_POLICY_CHANNEL_ID_STORE_H_

#include <set>
#include <string>

#include "base/callback_forward.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "net/extras/sqlite/sqlite_channel_id_store.h"
#include "net/ssl/default_channel_id_store.h"

namespace base {
class FilePath;
class SequencedTaskRunner;
}

namespace storage {
class SpecialStoragePolicy;
}

// Persistent ChannelID Store that takes into account SpecialStoragePolicy and
// removes ChannelIDs that are StorageSessionOnly when store is closed.
class QuotaPolicyChannelIDStore
    : public net::DefaultChannelIDStore::PersistentStore {
 public:
  // Create or open persistent store in file |path|. All I/O tasks are performed
  // in background using |background_task_runner|. If provided, a
  // |special_storage_policy| is consulted when the store is closed to decide
  // which certificates to keep.
  QuotaPolicyChannelIDStore(
      const base::FilePath& path,
      const scoped_refptr<base::SequencedTaskRunner>& background_task_runner,
      const scoped_refptr<storage::SpecialStoragePolicy>&
          special_storage_policy);

  // net::DefaultChannelIDStore::PersistentStore:
  virtual void Load(const LoadedCallback& loaded_callback) OVERRIDE;
  virtual void AddChannelID(
      const net::DefaultChannelIDStore::ChannelID& channel_id) OVERRIDE;
  virtual void DeleteChannelID(
      const net::DefaultChannelIDStore::ChannelID& channel_id) OVERRIDE;
  virtual void SetForceKeepSessionState() OVERRIDE;

 private:
  typedef ScopedVector<net::DefaultChannelIDStore::ChannelID> ChannelIDVector;

  virtual ~QuotaPolicyChannelIDStore();

  void OnLoad(const LoadedCallback& loaded_callback,
              scoped_ptr<ChannelIDVector> channel_ids);

  scoped_refptr<storage::SpecialStoragePolicy> special_storage_policy_;
  scoped_refptr<net::SQLiteChannelIDStore> persistent_store_;
  // Cache of server identifiers we have channel IDs stored for.
  std::set<std::string> server_identifiers_;

  DISALLOW_COPY_AND_ASSIGN(QuotaPolicyChannelIDStore);
};

#endif  // CHROME_BROWSER_NET_QUOTA_POLICY_CHANNEL_ID_STORE_H_
