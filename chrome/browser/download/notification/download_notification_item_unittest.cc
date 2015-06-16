// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/download/notification/download_notification_item.h"

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/test/test_simple_task_runner.h"
#include "base/thread_task_runner_handle.h"
#include "chrome/browser/download/notification/download_notification_manager.h"
#include "chrome/browser/notifications/notification_test_util.h"
#include "chrome/browser/notifications/platform_notification_service_impl.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/test/mock_download_item.h"
#include "content/public/test/mock_download_manager.h"
#include "content/public/test/test_browser_thread.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/message_center/fake_message_center.h"

using testing::NiceMock;
using testing::Return;
using testing::_;

namespace test {

class DownloadNotificationItemTest : public testing::Test {
 public:
  DownloadNotificationItemTest()
      : ui_thread_(content::BrowserThread::UI, &message_loop_),
        profile_(nullptr) {}

  void SetUp() override {
    testing::Test::SetUp();
    message_center::MessageCenter::Initialize();

    profile_manager_.reset(
        new TestingProfileManager(TestingBrowserProcess::GetGlobal()));
    ASSERT_TRUE(profile_manager_->SetUp());
    profile_ = profile_manager_->CreateTestingProfile("test-user");

    scoped_ptr<NotificationUIManager> ui_manager(new StubNotificationUIManager);
    TestingBrowserProcess::GetGlobal()->
        SetNotificationUIManager(ui_manager.Pass());

    download_notification_manager_.reset(
        new DownloadNotificationManagerForProfile(profile_, nullptr));

    download_item_.reset(new NiceMock<content::MockDownloadItem>());
    ON_CALL(*download_item_, GetId()).WillByDefault(Return(12345));
    ON_CALL(*download_item_, GetState())
        .WillByDefault(Return(content::DownloadItem::IN_PROGRESS));
    ON_CALL(*download_item_, IsDangerous()).WillByDefault(Return(false));
    ON_CALL(*download_item_, GetFileNameToReportUser())
        .WillByDefault(Return(base::FilePath("TITLE.bin")));
    ON_CALL(*download_item_, GetDangerType())
        .WillByDefault(Return(content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS));
    ON_CALL(*download_item_, IsDone()).WillByDefault(Return(false));
    ON_CALL(*download_item_, GetBrowserContext())
        .WillByDefault(Return(profile_));
  }

  void TearDown() override {
    download_notification_item_ = nullptr;  // will be free'd in the manager.
    download_notification_manager_.reset();
    profile_manager_.reset();
    message_center::MessageCenter::Shutdown();
    testing::Test::TearDown();
  }

 protected:
  message_center::MessageCenter* message_center() const {
    return message_center::MessageCenter::Get();
  }

  NotificationUIManager* ui_manager() const {
    return TestingBrowserProcess::GetGlobal()->notification_ui_manager();
  }

  std::string notification_id() const {
    return download_notification_item_->notification_->id();
  }

  const Notification* notification() const {
    return ui_manager()->FindById(
        download_notification_item_->watcher()->id(),
        NotificationUIManager::GetProfileID(profile_));
  }

  size_t NotificationCount() const {
    return ui_manager()
        ->GetAllIdsByProfileAndSourceOrigin(
              NotificationUIManager::GetProfileID(profile_),
              GURL(DownloadNotificationItem::kDownloadNotificationOrigin))
        .size();
  }

  void RemoveNotification() {
    ui_manager()->CancelById(download_notification_item_->watcher()->id(),
                             NotificationUIManager::GetProfileID(profile_));

    // Waits, since removing a notification may cause an async job.
    base::RunLoop().RunUntilIdle();
  }

  // Trampoline methods to access a private method in DownloadNotificationItem.
  void NotificationItemClick() {
    return download_notification_item_->OnNotificationClick();
  }
  void NotificationItemButtonClick(int index) {
    return download_notification_item_->OnNotificationButtonClick(index);
  }

  bool ShownAsPopUp() {
    return !download_notification_item_->notification_->shown_as_popup();
  }

  void CreateDownloadNotificationItem() {
    download_notification_manager_->OnNewDownloadReady(download_item_.get());
    download_notification_item_ =
        download_notification_manager_->items_[download_item_.get()];
  }

  base::MessageLoopForUI message_loop_;
  content::TestBrowserThread ui_thread_;

  scoped_ptr<TestingProfileManager> profile_manager_;
  Profile* profile_;

  scoped_ptr<NiceMock<content::MockDownloadItem>> download_item_;
  scoped_ptr<DownloadNotificationManagerForProfile>
      download_notification_manager_;
  DownloadNotificationItem* download_notification_item_;
};

TEST_F(DownloadNotificationItemTest, ShowAndCloseNotification) {
  EXPECT_EQ(0u, NotificationCount());

  // Shows a notification
  CreateDownloadNotificationItem();
  download_item_->NotifyObserversDownloadOpened();

  // Confirms that the notification is shown as a popup.
  EXPECT_TRUE(ShownAsPopUp());
  EXPECT_EQ(1u, NotificationCount());

  // Makes sure the DownloadItem::Cancel() is not called.
  EXPECT_CALL(*download_item_, Cancel(_)).Times(0);
  // Closes it once.
  RemoveNotification();

  // Confirms that the notification is closed.
  EXPECT_EQ(0u, NotificationCount());

  // Makes sure the DownloadItem::Cancel() is never called.
  EXPECT_CALL(*download_item_, Cancel(_)).Times(0);
}

TEST_F(DownloadNotificationItemTest, PauseAndResumeNotification) {
  // Shows a notification
  CreateDownloadNotificationItem();
  download_item_->NotifyObserversDownloadOpened();

  // Confirms that the notification is shown as a popup.
  EXPECT_EQ(1u, NotificationCount());

  // Pauses and makes sure the DownloadItem::Pause() is called.
  EXPECT_CALL(*download_item_, Pause()).Times(1);
  EXPECT_CALL(*download_item_, IsPaused()).WillRepeatedly(Return(true));
  NotificationItemButtonClick(0);
  download_item_->NotifyObserversDownloadUpdated();

  // Resumes and makes sure the DownloadItem::Resume() is called.
  EXPECT_CALL(*download_item_, Resume()).Times(1);
  EXPECT_CALL(*download_item_, IsPaused()).WillRepeatedly(Return(false));
  NotificationItemButtonClick(0);
  download_item_->NotifyObserversDownloadUpdated();
}

TEST_F(DownloadNotificationItemTest, OpenDownload) {
  EXPECT_CALL(*download_item_, GetState())
      .WillRepeatedly(Return(content::DownloadItem::COMPLETE));
  EXPECT_CALL(*download_item_, IsDone()).WillRepeatedly(Return(true));

  // Shows a notification.
  CreateDownloadNotificationItem();
  download_item_->NotifyObserversDownloadOpened();
  download_item_->NotifyObserversDownloadUpdated();

  // Clicks and confirms that the OpenDownload() is called.
  EXPECT_CALL(*download_item_, OpenDownload()).Times(1);
  EXPECT_CALL(*download_item_, SetOpenWhenComplete(_)).Times(0);
  NotificationItemClick();
}

TEST_F(DownloadNotificationItemTest, OpenWhenComplete) {
  // Shows a notification
  CreateDownloadNotificationItem();
  download_item_->NotifyObserversDownloadOpened();

  EXPECT_CALL(*download_item_, OpenDownload()).Times(0);

  // Toggles open-when-complete (new value: true).
  EXPECT_CALL(*download_item_, SetOpenWhenComplete(true))
      .Times(1)
      .WillOnce(Return());
  NotificationItemClick();
  EXPECT_CALL(*download_item_, GetOpenWhenComplete())
      .WillRepeatedly(Return(true));

  // Toggles open-when-complete (new value: false).
  EXPECT_CALL(*download_item_, SetOpenWhenComplete(false))
      .Times(1)
      .WillOnce(Return());
  NotificationItemClick();
  EXPECT_CALL(*download_item_, GetOpenWhenComplete())
      .WillRepeatedly(Return(false));

  // Sets open-when-complete.
  EXPECT_CALL(*download_item_, SetOpenWhenComplete(true))
      .Times(1)
      .WillOnce(Return());
  NotificationItemClick();
  EXPECT_CALL(*download_item_, GetOpenWhenComplete())
      .WillRepeatedly(Return(true));

  // Downloading is completed.
  EXPECT_CALL(*download_item_, GetState())
      .WillRepeatedly(Return(content::DownloadItem::COMPLETE));
  EXPECT_CALL(*download_item_, IsDone()).WillRepeatedly(Return(true));
  download_item_->NotifyObserversDownloadUpdated();

  // DownloadItem::OpenDownload must not be called since the file opens
  // automatically due to the open-when-complete flag.
}

}  // namespace test
