// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INTENTS_WEB_INTENTS_DISPATCHER_IMPL_H_
#define CONTENT_BROWSER_INTENTS_WEB_INTENTS_DISPATCHER_IMPL_H_

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_intents_dispatcher.h"
#include "webkit/glue/web_intent_data.h"

class IntentInjector;

// Implements the coordinator object interface for Web Intents.
// Observes the source (client) tab to make sure messages sent back by the
// service can be delivered. Keeps a copy of triggering intent data to
// be delivered to the service and serves as a forwarder for sending reply
// messages back to the client page.
class WebIntentsDispatcherImpl : public content::WebIntentsDispatcher,
                                 public content::WebContentsObserver {
 public:
  // |source_tab| is the page which triggered the web intent.
  // |intent| is the intent payload created by that page.
  // |intent_id| is the identifier assigned by WebKit to direct replies back to
  // the correct Javascript callback.
  WebIntentsDispatcherImpl(TabContents* source_tab,
                           const webkit_glue::WebIntentData& intent,
                           int intent_id);
  virtual ~WebIntentsDispatcherImpl();

  // WebIntentsDispatcher implementation.
  virtual const webkit_glue::WebIntentData& GetIntent() OVERRIDE;
  virtual void DispatchIntent(content::WebContents* destination_tab) OVERRIDE;
  virtual void SendReplyMessage(webkit_glue::WebIntentReplyType reply_type,
                                const string16& data) OVERRIDE;
  virtual void RegisterReplyNotification(
      const base::Callback<void(webkit_glue::WebIntentReplyType)>&
          closure) OVERRIDE;

  // content::WebContentsObserver implementation.
  virtual void WebContentsDestroyed(content::WebContents* tab) OVERRIDE;

 private:
  webkit_glue::WebIntentData intent_;

  int intent_id_;

  // Weak pointer to the internal object which provides the intent to the
  // newly-created service tab contents. This object is self-deleting
  // (connected to the service TabContents).
  IntentInjector* intent_injector_;

  // A callback to be notified when SendReplyMessage is called.
  base::Callback<void(webkit_glue::WebIntentReplyType)> reply_notifier_;

  DISALLOW_COPY_AND_ASSIGN(WebIntentsDispatcherImpl);
};

#endif  // CONTENT_BROWSER_INTENTS_WEB_INTENTS_DISPATCHER_IMPL_H_
