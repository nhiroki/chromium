// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_NOTIFICATION_WEB_NOTIFICATION_TRAY_H_
#define ASH_SYSTEM_NOTIFICATION_WEB_NOTIFICATION_TRAY_H_

#include "ash/ash_export.h"
#include "ash/system/tray/tray_background_view.h"
#include "ash/system/user/login_status.h"
#include "base/gtest_prod_util.h"
#include "ui/aura/event_filter.h"

namespace aura {
class LocatedEvent;
}

namespace gfx {
class ImageSkia;
}

namespace views {
class Label;
}

namespace ash {

namespace internal {
class StatusAreaWidget;
class WebNotificationButtonView;
class WebNotificationList;
class WebNotificationMenuModel;
class WebNotificationView;
}

// Status area tray for showing browser and app notifications. The client
// (e.g. Chrome) calls [Add|Remove|Update]Notification to create and update
// notifications in the tray. It can also implement Delegate to receive
// callbacks when a notification is removed (closed), clicked on, or a menu
// item is triggered.
//
// Note: These are not related to system notifications (i.e NotificationView
// generated by SystemTrayItem). Visibility of one notification type or other
// is controlled by StatusAreaWidget.

class ASH_EXPORT WebNotificationTray : public internal::TrayBackgroundView {
 public:
  class Delegate {
   public:
    virtual ~Delegate() {}

    // Called when the notification associated with |notification_id| is
    // removed (i.e. closed by the user).
    virtual void NotificationRemoved(const std::string& notifcation_id) = 0;

    // Request to disable the extension associated with |notification_id|.
    virtual void DisableExtension(const std::string& notifcation_id) = 0;

    // Request to disable notifications from the source of |notification_id|.
    virtual void DisableNotificationsFromSource(
        const std::string& notifcation_id) = 0;

    // Request to show the notification settings (|notification_id| is used
    // to identify the requesting browser context).
    virtual void ShowSettings(const std::string& notifcation_id) = 0;

    // Called when the notification body is clicked on.
    virtual void OnClicked(const std::string& notifcation_id) = 0;
  };

  explicit WebNotificationTray(internal::StatusAreaWidget* status_area_widget);
  virtual ~WebNotificationTray();

  // Called once to set the delegate.
  void SetDelegate(Delegate* delegate);

  // Add a new notification. |id| is a unique identifier, used to update or
  // remove notifications. |title| and |meesage| describe the notification text.
  // Use SetNotificationImage to set the icon image. If |extension_id| is
  // provided then 'Disable extension' will appear in a dropdown menu and the
  // id will be used to disable notifications from the extension. Otherwise if
  // |display_source| is provided, a menu item showing the source and allowing
  // notifications from that source to be disabled will be shown. All actual
  // disabling is handled by the Delegate.
  void AddNotification(const std::string& id,
                       const string16& title,
                       const string16& message,
                       const string16& display_source,
                       const std::string& extension_id);

  // Update an existing notification with id = old_id and set its id to new_id.
  void UpdateNotification(const std::string& old_id,
                          const std::string& new_id,
                          const string16& title,
                          const string16& message);

  // Remove an existing notification.
  void RemoveNotification(const std::string& id);

  // Set the notification image.
  void SetNotificationImage(const std::string& id,
                            const gfx::ImageSkia& image);

  // Show the message center bubble. Should only be called by StatusAreaWidget.
  void ShowMessageCenterBubble();

  // Hide the message center bubble. Should only be called by StatusAreaWidget.
  void HideMessageCenterBubble();

  // Show a single notification bubble for the most recent notification.
  void ShowNotificationBubble();

  // Hide the single notification bubble if visible.
  void HideNotificationBubble();

  // Updates tray visibility login status of the system changes.
  void UpdateAfterLoginStatusChange(user::LoginStatus login_status);

  // Overridden from TrayBackgroundView.
  virtual void SetShelfAlignment(ShelfAlignment alignment) OVERRIDE;

  // Overridden from internal::ActionableView.
  virtual bool PerformAction(const views::Event& event) OVERRIDE;

 protected:
  // Send a remove request to the delegate.
  void SendRemoveNotification(const std::string& id);

  // Send a remove request for all notifications to the delegate.
  void SendRemoveAllNotifications();

  // Disable all notifications matching notification |id|.
  void DisableByExtension(const std::string& id);
  void DisableByUrl(const std::string& id);

  // Request the Delegate to the settings dialog.
  void ShowSettings(const std::string& id);

  // Called when a notification is clicked on. Event is passed to the Delegate.
  void OnClicked(const std::string& id);

 private:
  class Bubble;
  friend class internal::WebNotificationButtonView;
  friend class internal::WebNotificationMenuModel;
  friend class internal::WebNotificationView;
  FRIEND_TEST_ALL_PREFIXES(WebNotificationTrayTest, WebNotifications);
  FRIEND_TEST_ALL_PREFIXES(WebNotificationTrayTest, WebNotificationBubble);

  int GetNotificationCount() const;
  void UpdateTray();
  void UpdateTrayAndBubble();
  void HideBubble(Bubble* bubble);
  bool HasNotificationForTest(const std::string& id) const;

  const internal::WebNotificationList* notification_list() const {
    return notification_list_.get();
  }
  Bubble* message_center_bubble() const { return message_center_bubble_.get(); }
  Bubble* notification_bubble() const { return notification_bubble_.get(); }

  scoped_ptr<internal::WebNotificationList> notification_list_;
  scoped_ptr<Bubble> message_center_bubble_;
  scoped_ptr<Bubble> notification_bubble_;
  views::Label* count_label_;
  Delegate* delegate_;
  bool show_message_center_on_unlock_;

  DISALLOW_COPY_AND_ASSIGN(WebNotificationTray);
};

}  // namespace ash

#endif  // ASH_SYSTEM_NOTIFICATION_WEB_NOTIFICATION_TRAY_H_
