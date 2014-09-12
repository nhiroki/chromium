// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/message_loop/message_loop.h"
#include "base/threading/thread.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/application_runner_chromium.h"
#include "mojo/public/cpp/application/interface_factory_impl.h"
#include "mojo/services/html_viewer/blink_platform_impl.h"
#include "mojo/services/html_viewer/html_document_view.h"
#include "mojo/services/public/interfaces/content_handler/content_handler.mojom.h"
#include "third_party/WebKit/public/web/WebKit.h"

namespace mojo {

class HTMLViewer;

class ContentHandlerImpl : public InterfaceImpl<ContentHandler> {
 public:
  ContentHandlerImpl(Shell* shell,
                     scoped_refptr<base::MessageLoopProxy> compositor_thread)
      : shell_(shell), compositor_thread_(compositor_thread) {}
  virtual ~ContentHandlerImpl() {}

 private:
  // Overridden from ContentHandler:
  virtual void OnConnect(
      const mojo::String& url,
      URLResponsePtr response,
      InterfaceRequest<ServiceProvider> service_provider_request) OVERRIDE {
    new HTMLDocumentView(response.Pass(),
                         service_provider_request.Pass(),
                         shell_,
                         compositor_thread_);
  }

  Shell* shell_;
  scoped_refptr<base::MessageLoopProxy> compositor_thread_;

  DISALLOW_COPY_AND_ASSIGN(ContentHandlerImpl);
};

class HTMLViewer : public ApplicationDelegate,
                   public InterfaceFactory<ContentHandler> {
 public:
  HTMLViewer() : compositor_thread_("compositor thread") {}

  virtual ~HTMLViewer() { blink::shutdown(); }

 private:
  // Overridden from ApplicationDelegate:
  virtual void Initialize(ApplicationImpl* app) OVERRIDE {
    shell_ = app->shell();
    blink_platform_impl_.reset(new BlinkPlatformImpl(app));
    blink::initialize(blink_platform_impl_.get());
    compositor_thread_.Start();
  }

  virtual bool ConfigureIncomingConnection(ApplicationConnection* connection)
      OVERRIDE {
    connection->AddService(this);
    return true;
  }

  // Overridden from InterfaceFactory<ContentHandler>
  virtual void Create(ApplicationConnection* connection,
                      InterfaceRequest<ContentHandler> request) OVERRIDE {
    BindToRequest(
        new ContentHandlerImpl(shell_, compositor_thread_.message_loop_proxy()),
        &request);
  }

  scoped_ptr<BlinkPlatformImpl> blink_platform_impl_;
  Shell* shell_;
  base::Thread compositor_thread_;

  DISALLOW_COPY_AND_ASSIGN(HTMLViewer);
};

}  // namespace mojo

MojoResult MojoMain(MojoHandle shell_handle) {
  mojo::ApplicationRunnerChromium runner(new mojo::HTMLViewer);
  return runner.Run(shell_handle);
}
