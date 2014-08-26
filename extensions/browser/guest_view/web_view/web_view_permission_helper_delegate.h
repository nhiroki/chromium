// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_GUEST_VIEW_WEB_VIEW_WEB_VIWE_PERMISSION_HELPER_DELEGATE_H_
#define EXTENSIONS_BROWSER_GUEST_VIEW_WEB_VIEW_WEB_VIWE_PERMISSION_HELPER_DELEGATE_H_

#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/media_stream_request.h"

namespace extensions {

// A delegate class of WebViewPermissionHelper to request permissions that are
// not a part of extensions.
class WebViewPermissionHelperDelegate : public content::WebContentsObserver {
 public:
  explicit WebViewPermissionHelperDelegate(content::WebContents* contents);
  virtual ~WebViewPermissionHelperDelegate();

  virtual void RequestMediaAccessPermission(
      content::WebContents* source,
      const content::MediaStreamRequest& request,
      const content::MediaResponseCallback& callback) {}

  virtual void CanDownload(
      content::RenderViewHost* render_view_host,
      const GURL& url,
      const std::string& request_method,
      const base::Callback<void(bool)>& callback) {}

  virtual void RequestPointerLockPermission(
      bool user_gesture,
      bool last_unlocked_by_target,
      const base::Callback<void(bool)>& callback) {}

  // Requests Geolocation Permission from the embedder.
  virtual void RequestGeolocationPermission(
      int bridge_id,
      const GURL& requesting_frame,
      bool user_gesture,
      const base::Callback<void(bool)>& callback) {}

  virtual void CancelGeolocationPermissionRequest(int bridge_id) {}

  virtual void RequestFileSystemPermission(
      const GURL& url,
      bool allowed_by_default,
      const base::Callback<void(bool)>& callback) {}

  // Called when file system access is requested by the guest content using the
  // asynchronous HTML5 file system API. The request is plumbed through the
  // <webview> permission request API. The request will be:
  // - Allowed if the embedder explicitly allowed it.
  // - Denied if the embedder explicitly denied.
  // - Determined by the guest's content settings if the embedder does not
  // perform an explicit action.
  // If access was blocked due to the page's content settings,
  // |blocked_by_policy| should be true, and this function should invoke
  // OnContentBlocked.
  virtual void FileSystemAccessedAsync(
      int render_process_id,
      int render_frame_id,
      int request_id,
      const GURL& url,
      bool blocked_by_policy) {}

  // Called when file system access is requested by the guest content using the
  // synchronous HTML5 file system API in a worker thread or shared worker. The
  // request is plumbed through the <webview> permission request API. The
  // request will be:
  // - Allowed if the embedder explicitly allowed it.
  // - Denied if the embedder explicitly denied.
  // - Determined by the guest's content settings if the embedder does not
  // perform an explicit action.
  // If access was blocked due to the page's content settings,
  // |blocked_by_policy| should be true, and this function should invoke
  // OnContentBlocked.
  virtual void FileSystemAccessedSync(
      int render_process_id,
      int render_frame_id,
      const GURL& url,
      bool blocked_by_policy,
      IPC::Message* reply_msg) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(WebViewPermissionHelperDelegate);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_GUEST_VIEW_WEB_VIEW_WEB_VIWE_PERMISSION_HELPER_DELEGATE_H_
