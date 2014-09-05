// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STORAGE_BROWSER_DATABASE_DATABASE_TRACKER_H_
#define STORAGE_BROWSER_DATABASE_DATABASE_TRACKER_H_

#include <map>
#include <set>
#include <utility>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/gtest_prod_util.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/observer_list.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "net/base/completion_callback.h"
#include "storage/browser/storage_browser_export.h"
#include "storage/common/database/database_connections.h"

namespace base {
class MessageLoopProxy;
}

namespace content {
class DatabaseTracker_TestHelper_Test;
class MockDatabaseTracker;
}

namespace sql {
class Connection;
class MetaTable;
}

namespace storage {
class QuotaManagerProxy;
class SpecialStoragePolicy;
}

namespace storage {

STORAGE_EXPORT extern const base::FilePath::CharType
    kDatabaseDirectoryName[];
STORAGE_EXPORT extern const base::FilePath::CharType
    kTrackerDatabaseFileName[];

class DatabasesTable;

// This class is used to store information about all databases in an origin.
class STORAGE_EXPORT OriginInfo {
 public:
  OriginInfo();
  OriginInfo(const OriginInfo& origin_info);
  ~OriginInfo();

  const std::string& GetOriginIdentifier() const { return origin_identifier_; }
  int64 TotalSize() const { return total_size_; }
  void GetAllDatabaseNames(std::vector<base::string16>* databases) const;
  int64 GetDatabaseSize(const base::string16& database_name) const;
  base::string16 GetDatabaseDescription(
      const base::string16& database_name) const;

 protected:
  typedef std::map<base::string16, std::pair<int64, base::string16> >
      DatabaseInfoMap;

  OriginInfo(const std::string& origin_identifier, int64 total_size);

  std::string origin_identifier_;
  int64 total_size_;
  DatabaseInfoMap database_info_;
};

// This class manages the main database and keeps track of open databases.
//
// The data in this class is not thread-safe, so all methods of this class
// should be called on the same thread. The only exceptions are the ctor(),
// the dtor() and the database_directory() and quota_manager_proxy() getters.
//
// Furthermore, some methods of this class have to read/write data from/to
// the disk. Therefore, in a multi-threaded application, all methods of this
// class should be called on the thread dedicated to file operations (file
// thread in the browser process, for example), if such a thread exists.
class STORAGE_EXPORT DatabaseTracker
    : public base::RefCountedThreadSafe<DatabaseTracker> {
 public:
  class Observer {
   public:
    virtual void OnDatabaseSizeChanged(const std::string& origin_identifier,
                                       const base::string16& database_name,
                                       int64 database_size) = 0;
    virtual void OnDatabaseScheduledForDeletion(
        const std::string& origin_identifier,
        const base::string16& database_name) = 0;

   protected:
    virtual ~Observer() {}
  };

  DatabaseTracker(const base::FilePath& profile_path,
                  bool is_incognito,
                  storage::SpecialStoragePolicy* special_storage_policy,
                  storage::QuotaManagerProxy* quota_manager_proxy,
                  base::MessageLoopProxy* db_tracker_thread);

  void DatabaseOpened(const std::string& origin_identifier,
                      const base::string16& database_name,
                      const base::string16& database_details,
                      int64 estimated_size,
                      int64* database_size);
  void DatabaseModified(const std::string& origin_identifier,
                        const base::string16& database_name);
  void DatabaseClosed(const std::string& origin_identifier,
                      const base::string16& database_name);
  void HandleSqliteError(const std::string& origin_identifier,
                         const base::string16& database_name,
                         int error);

  void CloseDatabases(const DatabaseConnections& connections);

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void CloseTrackerDatabaseAndClearCaches();

  const base::FilePath& DatabaseDirectory() const { return db_dir_; }
  base::FilePath GetFullDBFilePath(const std::string& origin_identifier,
                                   const base::string16& database_name);

  // virtual for unit-testing only
  virtual bool GetOriginInfo(const std::string& origin_id, OriginInfo* info);
  virtual bool GetAllOriginIdentifiers(std::vector<std::string>* origin_ids);
  virtual bool GetAllOriginsInfo(std::vector<OriginInfo>* origins_info);

  // Safe to call on any thread.
  storage::QuotaManagerProxy* quota_manager_proxy() const {
    return quota_manager_proxy_.get();
  }

  bool IsDatabaseScheduledForDeletion(const std::string& origin_identifier,
                                      const base::string16& database_name);

  // Deletes a single database. Returns net::OK on success, net::FAILED on
  // failure, or net::ERR_IO_PENDING and |callback| is invoked upon completion,
  // if non-NULL.
  int DeleteDatabase(const std::string& origin_identifier,
                     const base::string16& database_name,
                     const net::CompletionCallback& callback);

  // Delete any databases that have been touched since the cutoff date that's
  // supplied, omitting any that match IDs within |protected_origins|.
  // Returns net::OK on success, net::FAILED if not all databases could be
  // deleted, and net::ERR_IO_PENDING and |callback| is invoked upon completion,
  // if non-NULL. Protected origins, according the the SpecialStoragePolicy,
  // are not deleted by this method.
  int DeleteDataModifiedSince(const base::Time& cutoff,
                              const net::CompletionCallback& callback);

  // Delete all databases that belong to the given origin. Returns net::OK on
  // success, net::FAILED if not all databases could be deleted, and
  // net::ERR_IO_PENDING and |callback| is invoked upon completion, if non-NULL.
  // virtual for unit testing only
  virtual int DeleteDataForOrigin(const std::string& origin_identifier,
                                  const net::CompletionCallback& callback);

  bool IsIncognitoProfile() const { return is_incognito_; }

  const base::File* GetIncognitoFile(const base::string16& vfs_file_path) const;
  const base::File* SaveIncognitoFile(const base::string16& vfs_file_path,
                                      base::File file);
  void CloseIncognitoFileHandle(const base::string16& vfs_file_path);
  bool HasSavedIncognitoFileHandle(const base::string16& vfs_file_path) const;

  // Shutdown the database tracker, deleting database files if the tracker is
  // used for an incognito profile.
  void Shutdown();
  // Disables the exit-time deletion of session-only data.
  void SetForceKeepSessionState();

 private:
  friend class base::RefCountedThreadSafe<DatabaseTracker>;
  friend class content::DatabaseTracker_TestHelper_Test;
  friend class content::MockDatabaseTracker; // for testing

  typedef std::map<std::string, std::set<base::string16> > DatabaseSet;
  typedef std::vector<std::pair<net::CompletionCallback, DatabaseSet> >
      PendingDeletionCallbacks;
  typedef std::map<base::string16, base::File*> FileHandlesMap;
  typedef std::map<std::string, base::string16> OriginDirectoriesMap;

  class CachedOriginInfo : public OriginInfo {
   public:
    CachedOriginInfo() : OriginInfo(std::string(), 0) {}
    void SetOriginIdentifier(const std::string& origin_identifier) {
      origin_identifier_ = origin_identifier;
    }
    void SetDatabaseSize(const base::string16& database_name, int64 new_size) {
      int64 old_size = 0;
      if (database_info_.find(database_name) != database_info_.end())
        old_size = database_info_[database_name].first;
      database_info_[database_name].first = new_size;
      if (new_size != old_size)
        total_size_ += new_size - old_size;
    }
    void SetDatabaseDescription(const base::string16& database_name,
                                const base::string16& description) {
      database_info_[database_name].second = description;
    }
  };

  // virtual for unit-testing only.
  virtual ~DatabaseTracker();

  // Deletes the directory that stores all DBs in incognito mode, if it exists.
  void DeleteIncognitoDBDirectory();

  // Deletes session-only databases. Blocks databases from being created/opened.
  void ClearSessionOnlyOrigins();

  bool DeleteClosedDatabase(const std::string& origin_identifier,
                            const base::string16& database_name);

  // Delete all files belonging to the given origin given that no database
  // connections within this origin are open, or if |force| is true, delete
  // the meta data and rename the associated directory.
  bool DeleteOrigin(const std::string& origin_identifier, bool force);
  void DeleteDatabaseIfNeeded(const std::string& origin_identifier,
                              const base::string16& database_name);

  bool LazyInit();
  bool UpgradeToCurrentVersion();
  void InsertOrUpdateDatabaseDetails(const std::string& origin_identifier,
                                     const base::string16& database_name,
                                     const base::string16& database_details,
                                     int64 estimated_size);

  void ClearAllCachedOriginInfo();
  CachedOriginInfo* MaybeGetCachedOriginInfo(
      const std::string& origin_identifier,
      bool create_if_needed);
  CachedOriginInfo* GetCachedOriginInfo(
      const std::string& origin_identifier) {
    return MaybeGetCachedOriginInfo(origin_identifier, true);
  }

  int64 GetDBFileSize(const std::string& origin_identifier,
                      const base::string16& database_name);
  int64 SeedOpenDatabaseInfo(const std::string& origin_identifier,
                             const base::string16& database_name,
                             const base::string16& description);
  int64 UpdateOpenDatabaseInfoAndNotify(const std::string& origin_identifier,
                                        const base::string16& database_name,
                                        const base::string16* opt_description);
  int64 UpdateOpenDatabaseSizeAndNotify(const std::string& origin_identifier,
                                        const base::string16& database_name) {
    return UpdateOpenDatabaseInfoAndNotify(
        origin_identifier, database_name, NULL);
  }


  void ScheduleDatabaseForDeletion(const std::string& origin_identifier,
                                   const base::string16& database_name);
  // Schedule a set of open databases for deletion. If non-null, callback is
  // invoked upon completion.
  void ScheduleDatabasesForDeletion(const DatabaseSet& databases,
                                    const net::CompletionCallback& callback);

  // Returns the directory where all DB files for the given origin are stored.
  base::string16 GetOriginDirectory(const std::string& origin_identifier);

  bool is_initialized_;
  const bool is_incognito_;
  bool force_keep_session_state_;
  bool shutting_down_;
  const base::FilePath profile_path_;
  const base::FilePath db_dir_;
  scoped_ptr<sql::Connection> db_;
  scoped_ptr<DatabasesTable> databases_table_;
  scoped_ptr<sql::MetaTable> meta_table_;
  ObserverList<Observer, true> observers_;
  std::map<std::string, CachedOriginInfo> origins_info_map_;
  DatabaseConnections database_connections_;

  // The set of databases that should be deleted but are still opened
  DatabaseSet dbs_to_be_deleted_;
  PendingDeletionCallbacks deletion_callbacks_;

  // Apps and Extensions can have special rights.
  scoped_refptr<storage::SpecialStoragePolicy> special_storage_policy_;

  scoped_refptr<storage::QuotaManagerProxy> quota_manager_proxy_;

  // The database tracker thread we're supposed to run file IO on.
  scoped_refptr<base::MessageLoopProxy> db_tracker_thread_;

  // When in incognito mode, store a DELETE_ON_CLOSE handle to each
  // main DB and journal file that was accessed. When the incognito profile
  // goes away (or when the browser crashes), all these handles will be
  // closed, and the files will be deleted.
  FileHandlesMap incognito_file_handles_;

  // In a non-incognito profile, all DBs in an origin are stored in a directory
  // named after the origin. In an incognito profile though, we do not want the
  // directory structure to reveal the origins visited by the user (in case the
  // browser process crashes and those directories are not deleted). So we use
  // this map to assign directory names that do not reveal this information.
  OriginDirectoriesMap incognito_origin_directories_;
  int incognito_origin_directories_generator_;

  FRIEND_TEST_ALL_PREFIXES(DatabaseTracker, TestHelper);
};

}  // namespace storage

#endif  // STORAGE_BROWSER_DATABASE_DATABASE_TRACKER_H_
