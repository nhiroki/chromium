// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_DRIVE_JOB_SCHEDULER_H_
#define CHROME_BROWSER_CHROMEOS_DRIVE_JOB_SCHEDULER_H_

#include <list>
#include <vector>

#include "base/id_map.h"
#include "base/memory/scoped_ptr.h"
#include "base/observer_list.h"
#include "chrome/browser/chromeos/drive/file_system_interface.h"
#include "chrome/browser/chromeos/drive/job_list.h"
#include "chrome/browser/google_apis/drive_service_interface.h"
#include "chrome/browser/google_apis/drive_uploader.h"
#include "net/base/network_change_notifier.h"

class Profile;

namespace drive {

// The JobScheduler is responsible for queuing and scheduling drive
// operations.  It is responsible for handling retry logic, rate limiting, as
// concurrency as appropriate.
class JobScheduler
    : public net::NetworkChangeNotifier::ConnectionTypeObserver,
      public JobListInterface {
 public:
  JobScheduler(Profile* profile,
               google_apis::DriveServiceInterface* drive_service);
  virtual ~JobScheduler();

  // JobListInterface overrides.
  virtual std::vector<JobInfo> GetJobInfoList() OVERRIDE;
  virtual void AddObserver(JobListObserver* observer) OVERRIDE;
  virtual void RemoveObserver(JobListObserver* observer) OVERRIDE;
  virtual void CancelJob(JobID job_id) OVERRIDE;
  virtual void CancelAllJobs() OVERRIDE;

  // Adds a GetAppList operation to the queue.
  // |callback| must not be null.
  void GetAppList(const google_apis::GetAppListCallback& callback);

  // Adds a GetAboutResource operation to the queue.
  // |callback| must not be null.
  void GetAboutResource(const google_apis::GetAboutResourceCallback& callback);

  // Adds a GetAllResourceList operation to the queue.
  // |callback| must not be null.
  void GetAllResourceList(const google_apis::GetResourceListCallback& callback);

  // Adds a GetResourceListInDirectory operation to the queue.
  // |callback| must not be null.
  void GetResourceListInDirectory(
      const std::string& directory_resource_id,
      const google_apis::GetResourceListCallback& callback);

  // Adds a Search operation to the queue.
  // |callback| must not be null.
  void Search(const std::string& search_query,
              const google_apis::GetResourceListCallback& callback);

  // Adds a GetChangeList operation to the queue.
  // |callback| must not be null.
  void GetChangeList(int64 start_changestamp,
                     const google_apis::GetResourceListCallback& callback);

  // Adds ContinueGetResourceList operation to the queue.
  // |callback| must not be null.
  void ContinueGetResourceList(
      const GURL& feed_url,
      const google_apis::GetResourceListCallback& callback);

  // Adds a GetResourceEntry operation to the queue.
  void GetResourceEntry(const std::string& resource_id,
                        const DriveClientContext& context,
                        const google_apis::GetResourceEntryCallback& callback);


  // Adds a DeleteResource operation to the queue.
  void DeleteResource(const std::string& resource_id,
                      const google_apis::EntryActionCallback& callback);


  // Adds a CopyHostedDocument operation to the queue.
  void CopyHostedDocument(
      const std::string& resource_id,
      const std::string& new_name,
      const google_apis::GetResourceEntryCallback& callback);

  // Adds a RenameResource operation to the queue.
  void RenameResource(const std::string& resource_id,
                      const std::string& new_name,
                      const google_apis::EntryActionCallback& callback);

  // Adds a AddResourceToDirectory operation to the queue.
  void AddResourceToDirectory(const std::string& parent_resource_id,
                              const std::string& resource_id,
                              const google_apis::EntryActionCallback& callback);

  // Adds a RemoveResourceFromDirectory operation to the queue.
  void RemoveResourceFromDirectory(
      const std::string& parent_resource_id,
      const std::string& resource_id,
      const google_apis::EntryActionCallback& callback);

  // Adds a AddNewDirectory operation to the queue.
  void AddNewDirectory(const std::string& parent_resource_id,
                       const std::string& directory_name,
                       const google_apis::GetResourceEntryCallback& callback);

  // Adds a DownloadFile operation to the queue.
  JobID DownloadFile(
      const base::FilePath& virtual_path,
      const base::FilePath& local_cache_path,
      const GURL& download_url,
      const DriveClientContext& context,
      const google_apis::DownloadActionCallback& download_action_callback,
      const google_apis::GetContentCallback& get_content_callback);

  // Adds an UploadNewFile operation to the queue.
  void UploadNewFile(const std::string& parent_resource_id,
                     const base::FilePath& drive_file_path,
                     const base::FilePath& local_file_path,
                     const std::string& title,
                     const std::string& content_type,
                     const DriveClientContext& context,
                     const google_apis::UploadCompletionCallback& callback);

  // Adds an UploadExistingFile operation to the queue.
  void UploadExistingFile(
      const std::string& resource_id,
      const base::FilePath& drive_file_path,
      const base::FilePath& local_file_path,
      const std::string& content_type,
      const std::string& etag,
      const DriveClientContext& context,
      const google_apis::UploadCompletionCallback& upload_completion_callback);

  // Adds a CreateFile operation to the queue.
  void CreateFile(const std::string& parent_resource_id,
                  const base::FilePath& drive_file_path,
                  const std::string& title,
                  const std::string& content_type,
                  const DriveClientContext& context,
                  const google_apis::UploadCompletionCallback& callback);

 private:
  friend class JobSchedulerTest;

  enum QueueType {
    METADATA_QUEUE,
    FILE_QUEUE,
    NUM_QUEUES
  };

  static const int kMaxJobCount[NUM_QUEUES];

  // Represents a single entry in the job map.
  struct JobEntry {
    explicit JobEntry(JobType type);
    ~JobEntry();

    // Returns true when |left| is in higher priority than |right|.
    // Used for sorting entries from high priority to low priority.
    static bool Less(const JobEntry& left, const JobEntry& right);

    JobInfo job_info;

    // Context of the job.
    DriveClientContext context;

    base::Closure task;
  };

  // Creates a new job and add it to the job map.
  JobEntry* CreateNewJob(JobType type);

  // Adds the specified job to the queue and starts the job loop for the queue
  // if needed.
  void StartJob(JobEntry* job);

  // Adds the specified job to the queue.
  void QueueJob(JobID job_id);

  // Starts the job loop, if it is not already running.
  void StartJobLoop(QueueType queue_type);

  // Determines the next job that should run, and starts it.
  void DoJobLoop(QueueType queue_type);

  // Checks if operations should be suspended, such as if the network is
  // disconnected.
  //
  // Returns true when it should stop, and false if it should continue.
  bool ShouldStopJobLoop(QueueType queue_type,
                         const  DriveClientContext& context);

  // Increases the throttle delay if it's below the maximum value, and posts a
  // task to continue the loop after the delay.
  void ThrottleAndContinueJobLoop(QueueType queue_type);

  // Resets the throttle delay to the initial value, and continues the job loop.
  void ResetThrottleAndContinueJobLoop(QueueType queue_type);

  // Retries the job if needed and returns false. Otherwise returns true.
  bool OnJobDone(JobID job_id, google_apis::GDataErrorCode error);

  // Callback for job finishing with a GetResourceListCallback.
  void OnGetResourceListJobDone(
      JobID job_id,
      const google_apis::GetResourceListCallback& callback,
      google_apis::GDataErrorCode error,
      scoped_ptr<google_apis::ResourceList> resource_list);

  // Callback for job finishing with a GetResourceEntryCallback.
  void OnGetResourceEntryJobDone(
      JobID job_id,
      const google_apis::GetResourceEntryCallback& callback,
      google_apis::GDataErrorCode error,
      scoped_ptr<google_apis::ResourceEntry> entry);

  // Callback for job finishing with a GetAboutResourceCallback.
  void OnGetAboutResourceJobDone(
      JobID job_id,
      const google_apis::GetAboutResourceCallback& callback,
      google_apis::GDataErrorCode error,
      scoped_ptr<google_apis::AboutResource> about_resource);

  // Callback for job finishing with a GetAppListCallback.
  void OnGetAppListJobDone(
      JobID job_id,
      const google_apis::GetAppListCallback& callback,
      google_apis::GDataErrorCode error,
      scoped_ptr<google_apis::AppList> app_list);

  // Callback for job finishing with a EntryActionCallback.
  void OnEntryActionJobDone(JobID job_id,
                            const google_apis::EntryActionCallback& callback,
                            google_apis::GDataErrorCode error);

  // Callback for job finishing with a DownloadActionCallback.
  void OnDownloadActionJobDone(
      JobID job_id,
      const google_apis::DownloadActionCallback& callback,
      google_apis::GDataErrorCode error,
      const base::FilePath& temp_file);

  // Callback for job finishing with a UploadCompletionCallback.
  void OnUploadCompletionJobDone(
      JobID job_id,
      const google_apis::UploadCompletionCallback& callback,
      google_apis::GDataErrorCode error,
      const base::FilePath& drive_path,
      const base::FilePath& file_path,
      scoped_ptr<google_apis::ResourceEntry> resource_entry);

  // Updates the progress status of the specified job.
  void UpdateProgress(JobID job_id, int64 progress, int64 total);

  // net::NetworkChangeNotifier::ConnectionTypeObserver override.
  virtual void OnConnectionTypeChanged(
      net::NetworkChangeNotifier::ConnectionType type) OVERRIDE;

  // Get the type of queue the specified job should be put in.
  QueueType GetJobQueueType(JobType type);

  // For testing only.  Disables throttling so that testing is faster.
  void SetDisableThrottling(bool disable) { disable_throttling_ = disable; }

  // Notifies updates to observers.
  void NotifyJobAdded(const JobInfo& job_info);
  void NotifyJobDone(const JobInfo& job_info,
                     google_apis::GDataErrorCode error);
  void NotifyJobUpdated(const JobInfo& job_info);

  // Gets information of the queue of the given type as string.
  std::string GetQueueInfo(QueueType type) const;

  // Returns a string representation of QueueType.
  static std::string QueueTypeToString(QueueType type);

  // Number of jobs in flight for each queue.
  int jobs_running_[NUM_QUEUES];

  // The number of times operations have failed in a row, capped at
  // kMaxThrottleCount.  This is used to calculate the delay before running the
  // next task.
  int throttle_count_;

  // Disables throttling for testing.
  bool disable_throttling_;

  // The queues of jobs.
  std::list<JobID> queue_[NUM_QUEUES];

  // The list of unfinished (= queued or running) job info indexed by job IDs.
  typedef IDMap<JobEntry, IDMapOwnPointer> JobIDMap;
  JobIDMap job_map_;

  // The list of observers for the scheduler.
  ObserverList<JobListObserver> observer_list_;

  google_apis::DriveServiceInterface* drive_service_;
  scoped_ptr<google_apis::DriveUploaderInterface> uploader_;

  Profile* profile_;

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<JobScheduler> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(JobScheduler);
};

}  // namespace drive

#endif  // CHROME_BROWSER_CHROMEOS_DRIVE_JOB_SCHEDULER_H_
