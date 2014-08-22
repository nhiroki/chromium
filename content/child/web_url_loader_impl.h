// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CHILD_WEB_URL_LOADER_IMPL_H_
#define CONTENT_CHILD_WEB_URL_LOADER_IMPL_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "third_party/WebKit/public/platform/WebURLLoader.h"

class GURL;

namespace content {

class ResourceDispatcher;
struct ResourceResponseInfo;

class CONTENT_EXPORT WebURLLoaderImpl
    : public NON_EXPORTED_BASE(blink::WebURLLoader) {
 public:
  explicit WebURLLoaderImpl(ResourceDispatcher* resource_dispatcher);
  virtual ~WebURLLoaderImpl();

  static blink::WebURLError CreateError(const blink::WebURL& unreachable_url,
                                        bool stale_copy_in_cache,
                                        int reason);
  static void PopulateURLResponse(
      const GURL& url,
      const ResourceResponseInfo& info,
      blink::WebURLResponse* response);

  // WebURLLoader methods:
  virtual void loadSynchronously(
      const blink::WebURLRequest& request,
      blink::WebURLResponse& response,
      blink::WebURLError& error,
      blink::WebData& data) OVERRIDE;
  virtual void loadAsynchronously(
      const blink::WebURLRequest& request,
      blink::WebURLLoaderClient* client) OVERRIDE;
  virtual void cancel() OVERRIDE;
  virtual void setDefersLoading(bool value) OVERRIDE;
  virtual void didChangePriority(blink::WebURLRequest::Priority new_priority,
                                 int intra_priority_value) OVERRIDE;
  virtual bool attachThreadedDataReceiver(
      blink::WebThreadedDataReceiver* threaded_data_receiver) OVERRIDE;

 private:
  class Context;
  scoped_refptr<Context> context_;

  DISALLOW_COPY_AND_ASSIGN(WebURLLoaderImpl);
};

}  // namespace content

#endif  // CONTENT_CHILD_WEB_URL_LOADER_IMPL_H_
