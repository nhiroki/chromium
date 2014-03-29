// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/run_loop.h"
#include "content/browser/service_worker/embedded_worker_instance.h"
#include "content/browser/service_worker/embedded_worker_registry.h"
#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"
#include "content/browser/service_worker/service_worker_registration.h"
#include "content/browser/service_worker/service_worker_test_utils.h"
#include "content/browser/service_worker/service_worker_version.h"
#include "content/common/service_worker/service_worker_messages.h"
#include "content/common/service_worker/service_worker_status_code.h"
#include "content/common/service_worker/service_worker_types.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace content {

namespace {

struct FetchResult {
  ServiceWorkerStatusCode status;
  ServiceWorkerFetchEventResult result;
  ServiceWorkerResponse response;
};

void RunAndQuit(const base::Closure& closure,
                const base::Closure& quit,
                base::MessageLoopProxy* original_message_loop) {
  closure.Run();
  original_message_loop->PostTask(FROM_HERE, quit);
}

void RunOnIOThread(const base::Closure& closure) {
  base::RunLoop run_loop;
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&RunAndQuit, closure, run_loop.QuitClosure(),
                 base::MessageLoopProxy::current()));
  run_loop.Run();
}

void RunOnIOThread(
    const base::Callback<void(const base::Closure& continuation)>& closure) {
  base::RunLoop run_loop;
  base::Closure quit_on_original_thread =
      base::Bind(base::IgnoreResult(&base::MessageLoopProxy::PostTask),
                 base::MessageLoopProxy::current().get(),
                 FROM_HERE,
                 run_loop.QuitClosure());
  BrowserThread::PostTask(BrowserThread::IO,
                          FROM_HERE,
                          base::Bind(closure, quit_on_original_thread));
  run_loop.Run();
}

// Contrary to the style guide, the output parameter of this function comes
// before input parameters so Bind can be used on it to create a FetchCallback
// to pass to DispatchFetchEvent.
void ReceiveFetchResult(BrowserThread::ID run_quit_thread,
                        const base::Closure& quit,
                        FetchResult* out_result,
                        ServiceWorkerStatusCode actual_status,
                        ServiceWorkerFetchEventResult actual_result,
                        const ServiceWorkerResponse& actual_response) {
  out_result->status = actual_status;
  out_result->result = actual_result;
  out_result->response = actual_response;
  if (!quit.is_null())
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, quit);
}

ServiceWorkerVersion::FetchCallback CreateResponseReceiver(
    BrowserThread::ID run_quit_thread,
    const base::Closure& quit,
    FetchResult* result) {
  return base::Bind(&ReceiveFetchResult, run_quit_thread, quit, result);
}

}  // namespace

class ServiceWorkerBrowserTest : public ContentBrowserTest {
 protected:
  typedef ServiceWorkerBrowserTest self;

  virtual void SetUpCommandLine(CommandLine* command_line) OVERRIDE {
    command_line->AppendSwitch(switches::kEnableServiceWorker);
  }

  virtual void SetUpOnMainThread() OVERRIDE {
    ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());
    StoragePartition* partition = BrowserContext::GetDefaultStoragePartition(
        shell()->web_contents()->GetBrowserContext());
    wrapper_ = static_cast<ServiceWorkerContextWrapper*>(
        partition->GetServiceWorkerContext());

    // Navigate to the page to set up a renderer page (where we can embed
    // a worker).
    NavigateToURLBlockUntilNavigationsComplete(
        shell(),
        embedded_test_server()->GetURL("/service_worker/empty.html"), 1);

    RunOnIOThread(base::Bind(&self::SetUpOnIOThread, this));
  }

  virtual void TearDownOnMainThread() OVERRIDE {
    RunOnIOThread(base::Bind(&self::TearDownOnIOThread, this));
    wrapper_ = NULL;
  }

  virtual void SetUpOnIOThread() {}
  virtual void TearDownOnIOThread() {}

  ServiceWorkerContextWrapper* wrapper() { return wrapper_.get(); }
  ServiceWorkerContext* public_context() { return wrapper(); }

  void AssociateRendererProcessToWorker(EmbeddedWorkerInstance* worker) {
    worker->AddProcessReference(
        shell()->web_contents()->GetRenderProcessHost()->GetID());
  }

 private:
  scoped_refptr<ServiceWorkerContextWrapper> wrapper_;
};

class EmbeddedWorkerBrowserTest : public ServiceWorkerBrowserTest,
                                  public EmbeddedWorkerInstance::Observer {
 public:
  typedef EmbeddedWorkerBrowserTest self;

  EmbeddedWorkerBrowserTest()
      : last_worker_status_(EmbeddedWorkerInstance::STOPPED) {}
  virtual ~EmbeddedWorkerBrowserTest() {}

  virtual void TearDownOnIOThread() OVERRIDE {
    if (worker_) {
      worker_->RemoveObserver(this);
      worker_.reset();
    }
  }

  void StartOnIOThread() {
    ASSERT_TRUE(BrowserThread::CurrentlyOn(BrowserThread::IO));
    worker_ = wrapper()->context()->embedded_worker_registry()->CreateWorker();
    EXPECT_EQ(EmbeddedWorkerInstance::STOPPED, worker_->status());
    worker_->AddObserver(this);

    AssociateRendererProcessToWorker(worker_.get());

    const int64 service_worker_version_id = 33L;
    const GURL script_url = embedded_test_server()->GetURL(
        "/service_worker/worker.js");
    ServiceWorkerStatusCode status = worker_->Start(
        service_worker_version_id, script_url);

    last_worker_status_ = worker_->status();
    EXPECT_EQ(SERVICE_WORKER_OK, status);
    EXPECT_EQ(EmbeddedWorkerInstance::STARTING, last_worker_status_);

    if (status != SERVICE_WORKER_OK && !done_closure_.is_null())
      done_closure_.Run();
  }

  void StopOnIOThread() {
    ASSERT_TRUE(BrowserThread::CurrentlyOn(BrowserThread::IO));
    EXPECT_EQ(EmbeddedWorkerInstance::RUNNING, worker_->status());

    ServiceWorkerStatusCode status = worker_->Stop();

    last_worker_status_ = worker_->status();
    EXPECT_EQ(SERVICE_WORKER_OK, status);
    EXPECT_EQ(EmbeddedWorkerInstance::STOPPING, last_worker_status_);

    if (status != SERVICE_WORKER_OK && !done_closure_.is_null())
      done_closure_.Run();
  }

 protected:
  // EmbeddedWorkerInstance::Observer overrides:
  virtual void OnStarted() OVERRIDE {
    ASSERT_TRUE(worker_ != NULL);
    ASSERT_FALSE(done_closure_.is_null());
    last_worker_status_ = worker_->status();
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, done_closure_);
  }
  virtual void OnStopped() OVERRIDE {
    ASSERT_TRUE(worker_ != NULL);
    ASSERT_FALSE(done_closure_.is_null());
    last_worker_status_ = worker_->status();
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, done_closure_);
  }
  virtual void OnMessageReceived(
      int request_id, const IPC::Message& message) OVERRIDE {
    NOTREACHED();
  }

  scoped_ptr<EmbeddedWorkerInstance> worker_;
  EmbeddedWorkerInstance::Status last_worker_status_;

  // Called by EmbeddedWorkerInstance::Observer overrides so that
  // test code can wait for the worker status notifications.
  base::Closure done_closure_;
};

class ServiceWorkerVersionBrowserTest : public ServiceWorkerBrowserTest {
 public:
  typedef ServiceWorkerVersionBrowserTest self;

  ServiceWorkerVersionBrowserTest() : next_registration_id_(1) {}
  virtual ~ServiceWorkerVersionBrowserTest() {}

  virtual void TearDownOnIOThread() OVERRIDE {
    if (registration_) {
      registration_->Shutdown();
      registration_ = NULL;
    }
    if (version_) {
      version_->Shutdown();
      version_ = NULL;
    }
  }

  void InstallTestHelper(const std::string& worker_url) {
    RunOnIOThread(base::Bind(&self::SetUpRegistrationOnIOThread, this,
                             worker_url));

    // Dispatch install on a worker.
    ServiceWorkerStatusCode status = SERVICE_WORKER_ERROR_FAILED;
    base::RunLoop install_run_loop;
    BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                            base::Bind(&self::InstallOnIOThread, this,
                                       install_run_loop.QuitClosure(),
                                       &status));
    install_run_loop.Run();
    ASSERT_EQ(SERVICE_WORKER_OK, status);

    // Stop the worker.
    status = SERVICE_WORKER_ERROR_FAILED;
    base::RunLoop stop_run_loop;
    BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                            base::Bind(&self::StopOnIOThread, this,
                                       stop_run_loop.QuitClosure(),
                                       &status));
    stop_run_loop.Run();
    ASSERT_EQ(SERVICE_WORKER_OK, status);
  }

  void FetchTestHelper(const std::string& worker_url,
                       ServiceWorkerFetchEventResult* result,
                       ServiceWorkerResponse* response) {
    RunOnIOThread(
        base::Bind(&self::SetUpRegistrationOnIOThread, this, worker_url));

    FetchResult fetch_result;
    fetch_result.status = SERVICE_WORKER_ERROR_FAILED;
    base::RunLoop fetch_run_loop;
    BrowserThread::PostTask(BrowserThread::IO,
                            FROM_HERE,
                            base::Bind(&self::FetchOnIOThread,
                                       this,
                                       fetch_run_loop.QuitClosure(),
                                       &fetch_result));
    fetch_run_loop.Run();
    *result = fetch_result.result;
    *response = fetch_result.response;
    ASSERT_EQ(SERVICE_WORKER_OK, fetch_result.status);
  }

  void SetUpRegistrationOnIOThread(const std::string& worker_url) {
    const int64 version_id = 1L;
    registration_ = new ServiceWorkerRegistration(
        embedded_test_server()->GetURL("/*"),
        embedded_test_server()->GetURL(worker_url),
        next_registration_id_++);
    version_ = new ServiceWorkerVersion(
        registration_,
        wrapper()->context()->embedded_worker_registry(),
        version_id);
    AssociateRendererProcessToWorker(version_->embedded_worker());
  }

  void StartOnIOThread(const base::Closure& done,
                       ServiceWorkerStatusCode* result) {
    ASSERT_TRUE(BrowserThread::CurrentlyOn(BrowserThread::IO));
    version_->StartWorker(CreateReceiver(BrowserThread::UI, done, result));
  }

  void InstallOnIOThread(const base::Closure& done,
                         ServiceWorkerStatusCode* result) {
    ASSERT_TRUE(BrowserThread::CurrentlyOn(BrowserThread::IO));
    version_->DispatchInstallEvent(
        -1, CreateReceiver(BrowserThread::UI, done, result));
  }

  void FetchOnIOThread(const base::Closure& done, FetchResult* result) {
    ASSERT_TRUE(BrowserThread::CurrentlyOn(BrowserThread::IO));
    ServiceWorkerFetchRequest request(
        embedded_test_server()->GetURL("/service_worker/empty.html"),
        "GET",
        std::map<std::string, std::string>());
    version_->SetStatus(ServiceWorkerVersion::ACTIVE);
    version_->DispatchFetchEvent(
        request, CreateResponseReceiver(BrowserThread::UI, done, result));
  }

  void StopOnIOThread(const base::Closure& done,
                      ServiceWorkerStatusCode* result) {
    ASSERT_TRUE(version_);
    version_->StopWorker(CreateReceiver(BrowserThread::UI, done, result));
  }

 protected:
  int64 next_registration_id_;
  scoped_refptr<ServiceWorkerRegistration> registration_;
  scoped_refptr<ServiceWorkerVersion> version_;
};

IN_PROC_BROWSER_TEST_F(EmbeddedWorkerBrowserTest, StartAndStop) {
  // Start a worker and wait until OnStarted() is called.
  base::RunLoop start_run_loop;
  done_closure_ = start_run_loop.QuitClosure();
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::Bind(&self::StartOnIOThread, this));
  start_run_loop.Run();

  ASSERT_EQ(EmbeddedWorkerInstance::RUNNING, last_worker_status_);

  // Stop a worker and wait until OnStopped() is called.
  base::RunLoop stop_run_loop;
  done_closure_ = stop_run_loop.QuitClosure();
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::Bind(&self::StopOnIOThread, this));
  stop_run_loop.Run();

  ASSERT_EQ(EmbeddedWorkerInstance::STOPPED, last_worker_status_);
}

IN_PROC_BROWSER_TEST_F(ServiceWorkerVersionBrowserTest, StartAndStop) {
  RunOnIOThread(base::Bind(&self::SetUpRegistrationOnIOThread, this,
                           "/service_worker/worker.js"));

  // Start a worker.
  ServiceWorkerStatusCode status = SERVICE_WORKER_ERROR_FAILED;
  base::RunLoop start_run_loop;
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::Bind(&self::StartOnIOThread, this,
                                     start_run_loop.QuitClosure(),
                                     &status));
  start_run_loop.Run();
  ASSERT_EQ(SERVICE_WORKER_OK, status);

  // Stop the worker.
  status = SERVICE_WORKER_ERROR_FAILED;
  base::RunLoop stop_run_loop;
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::Bind(&self::StopOnIOThread, this,
                                     stop_run_loop.QuitClosure(),
                                     &status));
  stop_run_loop.Run();
  ASSERT_EQ(SERVICE_WORKER_OK, status);
}

IN_PROC_BROWSER_TEST_F(ServiceWorkerVersionBrowserTest, StartNotFound) {
  RunOnIOThread(base::Bind(&self::SetUpRegistrationOnIOThread, this,
                           "/service_worker/nonexistent.js"));

  // Start a worker for nonexistent URL.
  ServiceWorkerStatusCode status = SERVICE_WORKER_ERROR_FAILED;
  base::RunLoop start_run_loop;
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::Bind(&self::StartOnIOThread, this,
                                     start_run_loop.QuitClosure(),
                                     &status));
  start_run_loop.Run();
  ASSERT_EQ(SERVICE_WORKER_ERROR_START_WORKER_FAILED, status);
}

IN_PROC_BROWSER_TEST_F(ServiceWorkerVersionBrowserTest, Install) {
  InstallTestHelper("/service_worker/worker.js");
}

IN_PROC_BROWSER_TEST_F(ServiceWorkerVersionBrowserTest,
                       InstallWithWaitUntil_Fulfilled) {
  InstallTestHelper("/service_worker/worker_install_fulfilled.js");
}

IN_PROC_BROWSER_TEST_F(ServiceWorkerVersionBrowserTest,
                       InstallWithWaitUntil_Rejected) {
  // TODO(kinuko): This should also report back an error, but we
  // don't have plumbing for it yet.
  InstallTestHelper("/service_worker/worker_install_rejected.js");
}

IN_PROC_BROWSER_TEST_F(ServiceWorkerVersionBrowserTest, FetchEvent_Response) {
  ServiceWorkerFetchEventResult result;
  ServiceWorkerResponse response;
  FetchTestHelper("/service_worker/fetch_event.js", &result, &response);
  ASSERT_EQ(SERVICE_WORKER_FETCH_EVENT_RESULT_RESPONSE, result);
  EXPECT_EQ(200, response.status_code);
  EXPECT_EQ("OK", response.status_text);
  EXPECT_EQ("GET", response.method);
  std::map<std::string, std::string> expected_headers;
  EXPECT_EQ(expected_headers, response.headers);
}

IN_PROC_BROWSER_TEST_F(ServiceWorkerVersionBrowserTest,
                       FetchEvent_FallbackToNative) {
  ServiceWorkerFetchEventResult result;
  ServiceWorkerResponse response;
  FetchTestHelper(
      "/service_worker/fetch_event_fallback.js", &result, &response);
  ASSERT_EQ(SERVICE_WORKER_FETCH_EVENT_RESULT_FALLBACK, result);
}

IN_PROC_BROWSER_TEST_F(ServiceWorkerVersionBrowserTest, FetchEvent_Rejected) {
  ServiceWorkerFetchEventResult result;
  ServiceWorkerResponse response;
  FetchTestHelper("/service_worker/fetch_event_error.js", &result, &response);
  ASSERT_EQ(SERVICE_WORKER_FETCH_EVENT_RESULT_FALLBACK, result);
}

class ServiceWorkerBlackBoxBrowserTest : public ServiceWorkerBrowserTest {
 public:
  typedef ServiceWorkerBlackBoxBrowserTest self;

  static void ExpectResultAndRun(bool expected,
                                 const base::Closure& continuation,
                                 bool actual) {
    EXPECT_EQ(expected, actual);
    continuation.Run();
  }

  int RenderProcessID() {
    return shell()->web_contents()->GetRenderProcessHost()->GetID();
  }

  void FindRegistrationOnIO(const GURL& document_url,
                            ServiceWorkerStatusCode* status,
                            GURL* script_url,
                            const base::Closure& continuation) {
    wrapper()->context()->storage()->FindRegistrationForDocument(
        document_url,
        base::Bind(&ServiceWorkerBlackBoxBrowserTest::FindRegistrationOnIO2,
                   this,
                   status,
                   script_url,
                   continuation));
  }

  void FindRegistrationOnIO2(
      ServiceWorkerStatusCode* out_status,
      GURL* script_url,
      const base::Closure& continuation,
      ServiceWorkerStatusCode status,
      const scoped_refptr<ServiceWorkerRegistration>& registration) {
    *out_status = status;
    if (registration) {
      *script_url = registration->script_url();
    } else {
      EXPECT_NE(SERVICE_WORKER_OK, status);
    }
    continuation.Run();
  }
};

IN_PROC_BROWSER_TEST_F(ServiceWorkerBlackBoxBrowserTest, Registration) {
  const std::string kWorkerUrl = "/service_worker/fetch_event.js";
  {
    base::RunLoop run_loop;
    public_context()->RegisterServiceWorker(
        embedded_test_server()->GetURL("/*"),
        embedded_test_server()->GetURL(kWorkerUrl),
        RenderProcessID(),
        base::Bind(&ServiceWorkerBlackBoxBrowserTest::ExpectResultAndRun,
                   true,
                   run_loop.QuitClosure()));
    run_loop.Run();
  }
  {
    ServiceWorkerStatusCode status = SERVICE_WORKER_ERROR_FAILED;
    GURL script_url;
    RunOnIOThread(
        base::Bind(&ServiceWorkerBlackBoxBrowserTest::FindRegistrationOnIO,
                   this,
                   embedded_test_server()->GetURL("/service_worker/empty.html"),
                   &status,
                   &script_url));
    EXPECT_EQ(SERVICE_WORKER_OK, status);
    EXPECT_EQ(embedded_test_server()->GetURL(kWorkerUrl), script_url);
  }
  {
    base::RunLoop run_loop;
    public_context()->UnregisterServiceWorker(
        embedded_test_server()->GetURL("/*"),
        RenderProcessID(),
        base::Bind(&ServiceWorkerBlackBoxBrowserTest::ExpectResultAndRun,
                   true,
                   run_loop.QuitClosure()));
    run_loop.Run();
  }
  {
    ServiceWorkerStatusCode status = SERVICE_WORKER_ERROR_FAILED;
    GURL script_url;
    RunOnIOThread(
        base::Bind(&ServiceWorkerBlackBoxBrowserTest::FindRegistrationOnIO,
                   this,
                   embedded_test_server()->GetURL("/service_worker/empty.html"),
                   &status,
                   &script_url));
    EXPECT_EQ(SERVICE_WORKER_ERROR_NOT_FOUND, status);
  }
}

}  // namespace content
