// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/render_thread_impl.h"

#include <algorithm>
#include <limits>
#include <map>
#include <vector>

#include "base/allocator/allocator_extension.h"
#include "base/command_line.h"
#include "base/debug/trace_event.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/memory/discardable_memory.h"
#include "base/memory/shared_memory.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram.h"
#include "base/metrics/stats_table.h"
#include "base/path_service.h"
#include "base/strings/string16.h"
#include "base/strings/string_tokenizer.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_local.h"
#include "base/threading/thread_restrictions.h"
#include "base/values.h"
#include "content/child/appcache_dispatcher.h"
#include "content/child/child_histogram_message_filter.h"
#include "content/child/db_message_filter.h"
#include "content/child/indexed_db/indexed_db_dispatcher.h"
#include "content/child/indexed_db/indexed_db_message_filter.h"
#include "content/child/npapi/npobject_util.h"
#include "content/child/plugin_messages.h"
#include "content/child/resource_dispatcher.h"
#include "content/child/runtime_features.h"
#include "content/child/thread_safe_sender.h"
#include "content/child/web_database_observer_impl.h"
#include "content/common/child_process_messages.h"
#include "content/common/content_constants_internal.h"
#include "content/common/database_messages.h"
#include "content/common/dom_storage_messages.h"
#include "content/common/gpu/client/context_provider_command_buffer.h"
#include "content/common/gpu/client/gpu_channel_host.h"
#include "content/common/gpu/gpu_messages.h"
#include "content/common/resource_messages.h"
#include "content/common/view_messages.h"
#include "content/public/common/content_constants.h"
#include "content/public/common/content_paths.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/renderer_preferences.h"
#include "content/public/common/url_constants.h"
#include "content/public/renderer/content_renderer_client.h"
#include "content/public/renderer/render_process_observer.h"
#include "content/public/renderer/render_view_visitor.h"
#include "content/renderer/devtools/devtools_agent_filter.h"
#include "content/renderer/dom_storage/dom_storage_dispatcher.h"
#include "content/renderer/dom_storage/webstoragearea_impl.h"
#include "content/renderer/dom_storage/webstoragenamespace_impl.h"
#include "content/renderer/gpu/compositor_output_surface.h"
#include "content/renderer/gpu/gpu_benchmarking_extension.h"
#include "content/renderer/gpu/input_event_filter.h"
#include "content/renderer/gpu/input_handler_manager.h"
#include "content/renderer/media/audio_input_message_filter.h"
#include "content/renderer/media/audio_message_filter.h"
#include "content/renderer/media/audio_renderer_mixer_manager.h"
#include "content/renderer/media/media_stream_center.h"
#include "content/renderer/media/media_stream_dependency_factory.h"
#include "content/renderer/media/midi_message_filter.h"
#include "content/renderer/media/peer_connection_tracker.h"
#include "content/renderer/media/video_capture_impl_manager.h"
#include "content/renderer/media/video_capture_message_filter.h"
#include "content/renderer/media/webrtc_identity_service.h"
#include "content/renderer/memory_benchmarking_extension.h"
#include "content/renderer/p2p/socket_dispatcher.h"
#include "content/renderer/plugin_channel_host.h"
#include "content/renderer/render_process_impl.h"
#include "content/renderer/render_process_visibility_manager.h"
#include "content/renderer/render_view_impl.h"
#include "content/renderer/renderer_webkitplatformsupport_impl.h"
#include "content/renderer/skia_benchmarking_extension.h"
#include "grit/content_resources.h"
#include "ipc/ipc_channel_handle.h"
#include "ipc/ipc_forwarding_message_filter.h"
#include "ipc/ipc_platform_file.h"
#include "media/base/audio_hardware_config.h"
#include "media/base/media.h"
#include "media/filters/gpu_video_decoder_factories.h"
#include "net/base/net_errors.h"
#include "net/base/net_util.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/web/WebColorName.h"
#include "third_party/WebKit/public/web/WebDatabase.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebImageCache.h"
#include "third_party/WebKit/public/web/WebKit.h"
#include "third_party/WebKit/public/web/WebNetworkStateNotifier.h"
#include "third_party/WebKit/public/web/WebPopupMenu.h"
#include "third_party/WebKit/public/web/WebRuntimeFeatures.h"
#include "third_party/WebKit/public/web/WebScriptController.h"
#include "third_party/WebKit/public/web/WebSecurityPolicy.h"
#include "third_party/WebKit/public/web/WebSharedWorkerRepository.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "ui/base/layout.h"
#include "ui/base/ui_base_switches.h"
#include "v8/include/v8.h"
#include "webkit/child/worker_task_runner.h"
#include "webkit/glue/webkit_glue.h"
#include "webkit/renderer/appcache/appcache_frontend_impl.h"
#include "webkit/renderer/compositor_bindings/web_external_bitmap_impl.h"

#if defined(OS_WIN)
#include <windows.h>
#include <objbase.h>
#include "base/win/scoped_com_initializer.h"
#else
// TODO(port)
#include "base/memory/scoped_handle.h"
#include "content/child/npapi/np_channel_base.h"
#endif

#if defined(OS_POSIX)
#include "ipc/ipc_channel_posix.h"
#endif

#if defined(OS_ANDROID)
#include <cpu-features.h>
#include "content/renderer/android/synchronous_compositor_factory.h"
#endif

using base::ThreadRestrictions;
using WebKit::WebDocument;
using WebKit::WebFrame;
using WebKit::WebNetworkStateNotifier;
using WebKit::WebRuntimeFeatures;
using WebKit::WebScriptController;
using WebKit::WebSecurityPolicy;
using WebKit::WebString;
using WebKit::WebView;

namespace content {

namespace {

const int64 kInitialIdleHandlerDelayMs = 1000;
const int64 kShortIdleHandlerDelayMs = 1000;
const int64 kLongIdleHandlerDelayMs = 30*1000;
const int kIdleCPUUsageThresholdInPercents = 3;

// Keep the global RenderThreadImpl in a TLS slot so it is impossible to access
// incorrectly from the wrong thread.
base::LazyInstance<base::ThreadLocalPointer<RenderThreadImpl> >
    lazy_tls = LAZY_INSTANCE_INITIALIZER;

class RenderViewZoomer : public RenderViewVisitor {
 public:
  RenderViewZoomer(const std::string& scheme,
                   const std::string& host,
                   double zoom_level) : scheme_(scheme),
                                        host_(host),
                                        zoom_level_(zoom_level) {
  }

  virtual bool Visit(RenderView* render_view) OVERRIDE {
    WebView* webview = render_view->GetWebView();
    WebDocument document = webview->mainFrame()->document();

    // Don't set zoom level for full-page plugin since they don't use the same
    // zoom settings.
    if (document.isPluginDocument())
      return true;
    GURL url(document.url());
    // Empty scheme works as wildcard that matches any scheme,
    if ((net::GetHostOrSpecFromURL(url) == host_) &&
        (scheme_.empty() || scheme_ == url.scheme())) {
      webview->setZoomLevel(false, zoom_level_);
    }
    return true;
  }

 private:
  const std::string scheme_;
  const std::string host_;
  const double zoom_level_;

  DISALLOW_COPY_AND_ASSIGN(RenderViewZoomer);
};

std::string HostToCustomHistogramSuffix(const std::string& host) {
  if (host == "mail.google.com")
    return ".gmail";
  if (host == "docs.google.com" || host == "drive.google.com")
    return ".docs";
  if (host == "plus.google.com")
    return ".plus";
  return std::string();
}

void* CreateHistogram(
    const char *name, int min, int max, size_t buckets) {
  if (min <= 0)
    min = 1;
  std::string histogram_name;
  RenderThreadImpl* render_thread_impl = RenderThreadImpl::current();
  if (render_thread_impl) {  // Can be null in tests.
    histogram_name = render_thread_impl->
        histogram_customizer()->ConvertToCustomHistogramName(name);
  } else {
    histogram_name = std::string(name);
  }
  base::HistogramBase* histogram = base::Histogram::FactoryGet(
      histogram_name, min, max, buckets,
      base::Histogram::kUmaTargetedHistogramFlag);
  return histogram;
}

void AddHistogramSample(void* hist, int sample) {
  base::Histogram* histogram = static_cast<base::Histogram*>(hist);
  histogram->Add(sample);
}

scoped_ptr<base::SharedMemory> AllocateSharedMemoryFunction(size_t size) {
  return RenderThreadImpl::Get()->HostAllocateSharedMemoryBuffer(size);
}

void EnableWebCoreLogChannels(const std::string& channels) {
  if (channels.empty())
    return;
  base::StringTokenizer t(channels, ", ");
  while (t.GetNext())
    WebKit::enableLogChannel(t.token().c_str());
}

}  // namespace

class RenderThreadImpl::GpuVDAContextLostCallback
    : public WebKit::WebGraphicsContext3D::WebGraphicsContextLostCallback {
 public:
  GpuVDAContextLostCallback()
      : main_message_loop_(base::MessageLoopProxy::current()) {}
  virtual ~GpuVDAContextLostCallback() {}
  virtual void onContextLost() {
    main_message_loop_->PostTask(FROM_HERE, base::Bind(
        &RenderThreadImpl::OnGpuVDAContextLoss));
  }

 private:
  scoped_refptr<base::MessageLoopProxy> main_message_loop_;
};

class RenderThreadImpl::RendererContextProviderCommandBuffer
    : public ContextProviderCommandBuffer {
 public:
  static scoped_refptr<RendererContextProviderCommandBuffer> Create() {
    scoped_refptr<RendererContextProviderCommandBuffer> provider =
        new RendererContextProviderCommandBuffer();
    if (!provider->InitializeOnMainThread())
      return NULL;
    return provider;
  }

 protected:
  virtual ~RendererContextProviderCommandBuffer() {}

  virtual scoped_ptr<WebGraphicsContext3DCommandBufferImpl>
  CreateOffscreenContext3d() OVERRIDE {
    RenderThreadImpl* self = RenderThreadImpl::current();
    DCHECK(self);
    return self->CreateOffscreenContext3d().Pass();
  }
};

RenderThreadImpl::HistogramCustomizer::HistogramCustomizer() {
  custom_histograms_.insert("V8.MemoryExternalFragmentationTotal");
  custom_histograms_.insert("V8.MemoryHeapSampleTotalCommitted");
  custom_histograms_.insert("V8.MemoryHeapSampleTotalUsed");
}

RenderThreadImpl::HistogramCustomizer::~HistogramCustomizer() {}

void RenderThreadImpl::HistogramCustomizer::RenderViewNavigatedToHost(
    const std::string& host, size_t view_count) {
  if (CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableHistogramCustomizer)) {
    return;
  }
  // Check if all RenderViews are displaying a page from the same host. If there
  // is only one RenderView, the common host is this view's host. If there are
  // many, check if this one shares the common host of the other
  // RenderViews. It's ok to not detect some cases where the RenderViews share a
  // common host. This information is only used for producing custom histograms.
  if (view_count == 1)
    SetCommonHost(host);
  else if (host != common_host_)
    SetCommonHost(std::string());
}

std::string RenderThreadImpl::HistogramCustomizer::ConvertToCustomHistogramName(
    const char* histogram_name) const {
  std::string name(histogram_name);
  if (!common_host_histogram_suffix_.empty() &&
      custom_histograms_.find(name) != custom_histograms_.end())
    name += common_host_histogram_suffix_;
  return name;
}

void RenderThreadImpl::HistogramCustomizer::SetCommonHost(
    const std::string& host) {
  if (host != common_host_) {
    common_host_ = host;
    common_host_histogram_suffix_ = HostToCustomHistogramSuffix(host);
    v8::V8::SetCreateHistogramFunction(CreateHistogram);
  }
}

RenderThreadImpl* RenderThreadImpl::current() {
  return lazy_tls.Pointer()->Get();
}

// When we run plugins in process, we actually run them on the render thread,
// which means that we need to make the render thread pump UI events.
RenderThreadImpl::RenderThreadImpl() {
  Init();
}

RenderThreadImpl::RenderThreadImpl(const std::string& channel_name)
    : ChildThread(channel_name) {
  Init();
}

void RenderThreadImpl::Init() {
  TRACE_EVENT_BEGIN_ETW("RenderThreadImpl::Init", 0, "");

  base::debug::TraceLog::GetInstance()->SetThreadSortIndex(
      base::PlatformThread::CurrentId(),
      kTraceEventRendererMainThreadSortIndex);

  v8::V8::SetCounterFunction(base::StatsTable::FindLocation);
  v8::V8::SetCreateHistogramFunction(CreateHistogram);
  v8::V8::SetAddHistogramSampleFunction(AddHistogramSample);

#if defined(OS_MACOSX) || defined(OS_ANDROID)
  // On Mac and Android, the select popups are rendered by the browser.
  WebKit::WebView::setUseExternalPopupMenus(true);
#endif

  lazy_tls.Pointer()->Set(this);

#if defined(OS_WIN)
  // If you are running plugins in this thread you need COM active but in
  // the normal case you don't.
  if (RenderProcessImpl::InProcessPlugins())
    initialize_com_.reset(new base::win::ScopedCOMInitializer());
#endif

  // Register this object as the main thread.
  ChildProcess::current()->set_main_thread(this);

  // In single process the single process is all there is.
  suspend_webkit_shared_timer_ = true;
  notify_webkit_of_modal_loop_ = true;
  widget_count_ = 0;
  hidden_widget_count_ = 0;
  idle_notification_delay_in_ms_ = kInitialIdleHandlerDelayMs;
  idle_notifications_to_skip_ = 0;
  layout_test_mode_ = false;
  shutdown_event_ = NULL;

  appcache_dispatcher_.reset(
      new AppCacheDispatcher(Get(), new appcache::AppCacheFrontendImpl()));
  dom_storage_dispatcher_.reset(new DomStorageDispatcher());
  main_thread_indexed_db_dispatcher_.reset(new IndexedDBDispatcher(
      thread_safe_sender()));

  media_stream_center_ = NULL;

  db_message_filter_ = new DBMessageFilter();
  AddFilter(db_message_filter_.get());

#if defined(ENABLE_WEBRTC)
  peer_connection_tracker_.reset(new PeerConnectionTracker());
  AddObserver(peer_connection_tracker_.get());

  p2p_socket_dispatcher_ =
      new P2PSocketDispatcher(GetIOMessageLoopProxy().get());
  AddFilter(p2p_socket_dispatcher_.get());

  webrtc_identity_service_.reset(new WebRTCIdentityService());
#endif  // defined(ENABLE_WEBRTC)
  vc_manager_ = new VideoCaptureImplManager();
  AddFilter(vc_manager_->video_capture_message_filter());

  audio_input_message_filter_ =
      new AudioInputMessageFilter(GetIOMessageLoopProxy());
  AddFilter(audio_input_message_filter_.get());

  audio_message_filter_ = new AudioMessageFilter(GetIOMessageLoopProxy());
  AddFilter(audio_message_filter_.get());

  midi_message_filter_ = new MIDIMessageFilter(GetIOMessageLoopProxy());
  AddFilter(midi_message_filter_.get());

  AddFilter(new IndexedDBMessageFilter(thread_safe_sender()));

  GetContentClient()->renderer()->RenderThreadStarted();

  const CommandLine& command_line = *CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kEnableGpuBenchmarking))
      RegisterExtension(GpuBenchmarkingExtension::Get());

#if defined(USE_TCMALLOC) && (defined(OS_LINUX) || defined(OS_ANDROID))
  if (command_line.HasSwitch(switches::kEnableMemoryBenchmarking))
    RegisterExtension(MemoryBenchmarkingExtension::Get());
#endif  // USE_TCMALLOC

  if (command_line.HasSwitch(switches::kEnableSkiaBenchmarking)) {
    LOG(WARNING) << "Enabling unsafe Skia benchmarking extension.";
    RegisterExtension(SkiaBenchmarkingExtension::Get());
  }

  context_lost_cb_.reset(new GpuVDAContextLostCallback());

  // Note that under Linux, the media library will normally already have
  // been initialized by the Zygote before this instance became a Renderer.
  base::FilePath media_path;
  PathService::Get(DIR_MEDIA_LIBS, &media_path);
  if (!media_path.empty())
    media::InitializeMediaLibrary(media_path);

  memory_pressure_listener_.reset(new base::MemoryPressureListener(
      base::Bind(&RenderThreadImpl::OnMemoryPressure, base::Unretained(this))));

  TRACE_EVENT_END_ETW("RenderThreadImpl::Init", 0, "");
}

RenderThreadImpl::~RenderThreadImpl() {
}

void RenderThreadImpl::Shutdown() {
  FOR_EACH_OBSERVER(
      RenderProcessObserver, observers_, OnRenderProcessShutdown());

  ChildThread::Shutdown();

  // Wait for all databases to be closed.
  if (web_database_observer_impl_)
    web_database_observer_impl_->WaitForAllDatabasesToClose();

  // Shutdown in reverse of the initialization order.
  if (devtools_agent_message_filter_.get()) {
    RemoveFilter(devtools_agent_message_filter_.get());
    devtools_agent_message_filter_ = NULL;
  }

  RemoveFilter(audio_input_message_filter_.get());
  audio_input_message_filter_ = NULL;

  RemoveFilter(audio_message_filter_.get());
  audio_message_filter_ = NULL;

  RemoveFilter(vc_manager_->video_capture_message_filter());

  RemoveFilter(db_message_filter_.get());
  db_message_filter_ = NULL;

  // Shutdown the file thread if it's running.
  if (file_thread_)
    file_thread_->Stop();

  if (compositor_output_surface_filter_.get()) {
    RemoveFilter(compositor_output_surface_filter_.get());
    compositor_output_surface_filter_ = NULL;
  }

  compositor_thread_.reset();
  input_handler_manager_.reset();
  if (input_event_filter_.get()) {
    RemoveFilter(input_event_filter_.get());
    input_event_filter_ = NULL;
  }

  if (webkit_platform_support_)
    WebKit::shutdown();

  lazy_tls.Pointer()->Set(NULL);

  // TODO(port)
#if defined(OS_WIN)
  // Clean up plugin channels before this thread goes away.
  NPChannelBase::CleanupChannels();
#endif

  // Leak shared contexts on other threads, as we can not get to the correct
  // thread to destroy them.
  if (shared_contexts_compositor_thread_.get())
    shared_contexts_compositor_thread_->set_leak_on_destroy();
}

bool RenderThreadImpl::Send(IPC::Message* msg) {
  // Certain synchronous messages cannot always be processed synchronously by
  // the browser, e.g., Chrome frame communicating with the embedding browser.
  // This could cause a complete hang of Chrome if a windowed plug-in is trying
  // to communicate with the renderer thread since the browser's UI thread
  // could be stuck (within a Windows API call) trying to synchronously
  // communicate with the plug-in.  The remedy is to pump messages on this
  // thread while the browser is processing this request. This creates an
  // opportunity for re-entrancy into WebKit, so we need to take care to disable
  // callbacks, timers, and pending network loads that could trigger such
  // callbacks.
  bool pumping_events = false;
  if (msg->is_sync()) {
    if (msg->is_caller_pumping_messages()) {
      pumping_events = true;
    } else {
      if ((msg->type() == ViewHostMsg_GetCookies::ID ||
           msg->type() == ViewHostMsg_GetRawCookies::ID ||
           msg->type() == ViewHostMsg_CookiesEnabled::ID) &&
          GetContentClient()->renderer()->
              ShouldPumpEventsDuringCookieMessage()) {
        pumping_events = true;
      }
    }
  }

  bool suspend_webkit_shared_timer = true;  // default value
  std::swap(suspend_webkit_shared_timer, suspend_webkit_shared_timer_);

  bool notify_webkit_of_modal_loop = true;  // default value
  std::swap(notify_webkit_of_modal_loop, notify_webkit_of_modal_loop_);

  int render_view_id = MSG_ROUTING_NONE;

  if (pumping_events) {
    if (suspend_webkit_shared_timer)
      webkit_platform_support_->SuspendSharedTimer();

    if (notify_webkit_of_modal_loop)
      WebView::willEnterModalLoop();

    RenderViewImpl* render_view =
        RenderViewImpl::FromRoutingID(msg->routing_id());
    if (render_view) {
      render_view_id = msg->routing_id();
      PluginChannelHost::Broadcast(
          new PluginMsg_SignalModalDialogEvent(render_view_id));
    }
  }

  bool rv = ChildThread::Send(msg);

  if (pumping_events) {
    if (render_view_id != MSG_ROUTING_NONE) {
      PluginChannelHost::Broadcast(
          new PluginMsg_ResetModalDialogEvent(render_view_id));
    }

    if (notify_webkit_of_modal_loop)
      WebView::didExitModalLoop();

    if (suspend_webkit_shared_timer)
      webkit_platform_support_->ResumeSharedTimer();
  }

  return rv;
}

base::MessageLoop* RenderThreadImpl::GetMessageLoop() {
  return message_loop();
}

IPC::SyncChannel* RenderThreadImpl::GetChannel() {
  return channel();
}

std::string RenderThreadImpl::GetLocale() {
  // The browser process should have passed the locale to the renderer via the
  // --lang command line flag.
  const CommandLine& parsed_command_line = *CommandLine::ForCurrentProcess();
  const std::string& lang =
      parsed_command_line.GetSwitchValueASCII(switches::kLang);
  DCHECK(!lang.empty());
  return lang;
}

IPC::SyncMessageFilter* RenderThreadImpl::GetSyncMessageFilter() {
  return sync_message_filter();
}

scoped_refptr<base::MessageLoopProxy>
    RenderThreadImpl::GetIOMessageLoopProxy() {
  return ChildProcess::current()->io_message_loop_proxy();
}

void RenderThreadImpl::AddRoute(int32 routing_id, IPC::Listener* listener) {
  widget_count_++;
  return ChildThread::AddRoute(routing_id, listener);
}

void RenderThreadImpl::RemoveRoute(int32 routing_id) {
  widget_count_--;
  return ChildThread::RemoveRoute(routing_id);
}

int RenderThreadImpl::GenerateRoutingID() {
  int routing_id = MSG_ROUTING_NONE;
  Send(new ViewHostMsg_GenerateRoutingID(&routing_id));
  return routing_id;
}

void RenderThreadImpl::AddFilter(IPC::ChannelProxy::MessageFilter* filter) {
  channel()->AddFilter(filter);
}

void RenderThreadImpl::RemoveFilter(IPC::ChannelProxy::MessageFilter* filter) {
  channel()->RemoveFilter(filter);
}

void RenderThreadImpl::SetOutgoingMessageFilter(
    IPC::ChannelProxy::OutgoingMessageFilter* filter) {
}

void RenderThreadImpl::AddObserver(RenderProcessObserver* observer) {
  observers_.AddObserver(observer);
}

void RenderThreadImpl::RemoveObserver(RenderProcessObserver* observer) {
  observers_.RemoveObserver(observer);
}

void RenderThreadImpl::SetResourceDispatcherDelegate(
    ResourceDispatcherDelegate* delegate) {
  resource_dispatcher()->set_delegate(delegate);
}

void RenderThreadImpl::WidgetHidden() {
  DCHECK(hidden_widget_count_ < widget_count_);
  hidden_widget_count_++;

  RenderProcessVisibilityManager* manager =
      RenderProcessVisibilityManager::GetInstance();
  manager->WidgetVisibilityChanged(false);

  if (!GetContentClient()->renderer()->RunIdleHandlerWhenWidgetsHidden()) {
    return;
  }

  if (widget_count_ && hidden_widget_count_ == widget_count_)
    ScheduleIdleHandler(kInitialIdleHandlerDelayMs);
}

void RenderThreadImpl::WidgetRestored() {
  DCHECK_GT(hidden_widget_count_, 0);
  hidden_widget_count_--;

  RenderProcessVisibilityManager* manager =
      RenderProcessVisibilityManager::GetInstance();
  manager->WidgetVisibilityChanged(true);

  if (!GetContentClient()->renderer()->RunIdleHandlerWhenWidgetsHidden()) {
    return;
  }

  ScheduleIdleHandler(kLongIdleHandlerDelayMs);
}

void RenderThreadImpl::EnsureWebKitInitialized() {
  if (webkit_platform_support_)
    return;

  webkit_platform_support_.reset(new RendererWebKitPlatformSupportImpl);
  WebKit::initialize(webkit_platform_support_.get());
  WebKit::setSharedWorkerRepository(
      webkit_platform_support_.get()->sharedWorkerRepository());

  const CommandLine& command_line = *CommandLine::ForCurrentProcess();

  bool enable = command_line.HasSwitch(switches::kEnableThreadedCompositing);
  if (enable) {
#if defined(OS_ANDROID)
    if (SynchronousCompositorFactory* factory =
        SynchronousCompositorFactory::GetInstance())
      compositor_message_loop_proxy_ =
          factory->GetCompositorMessageLoop();
#endif
    if (!compositor_message_loop_proxy_.get()) {
      compositor_thread_.reset(new base::Thread("Compositor"));
      compositor_thread_->Start();
#if defined(OS_ANDROID)
      compositor_thread_->SetPriority(base::kThreadPriority_Display);
#endif
      compositor_message_loop_proxy_ =
          compositor_thread_->message_loop_proxy();
      compositor_message_loop_proxy_->PostTask(
          FROM_HERE,
          base::Bind(base::IgnoreResult(&ThreadRestrictions::SetIOAllowed),
                     false));
    }

    InputHandlerManagerClient* input_handler_manager_client = NULL;
#if defined(OS_ANDROID)
    if (SynchronousCompositorFactory* factory =
        SynchronousCompositorFactory::GetInstance()) {
      input_handler_manager_client = factory->GetInputHandlerManagerClient();
    }
#endif
    if (!input_handler_manager_client) {
      input_event_filter_ =
          new InputEventFilter(this, compositor_message_loop_proxy_);
      AddFilter(input_event_filter_.get());
      input_handler_manager_client = input_event_filter_.get();
    }
    input_handler_manager_.reset(
        new InputHandlerManager(compositor_message_loop_proxy_,
                                input_handler_manager_client));
  }

  scoped_refptr<base::MessageLoopProxy> output_surface_loop;
  if (enable)
    output_surface_loop = compositor_message_loop_proxy_;
  else
    output_surface_loop = base::MessageLoopProxy::current();

  compositor_output_surface_filter_ =
      CompositorOutputSurface::CreateFilter(output_surface_loop.get());
  AddFilter(compositor_output_surface_filter_.get());

  WebScriptController::enableV8SingleThreadMode();

  RenderThreadImpl::RegisterSchemes();

  EnableWebCoreLogChannels(
      command_line.GetSwitchValueASCII(switches::kWebCoreLogChannels));

  web_database_observer_impl_.reset(
      new WebDatabaseObserverImpl(sync_message_filter()));
  WebKit::WebDatabase::setObserver(web_database_observer_impl_.get());

  SetRuntimeFeaturesDefaultsAndUpdateFromArgs(command_line);

  if (!media::IsMediaLibraryInitialized()) {
    WebRuntimeFeatures::enableMediaPlayer(false);
    WebRuntimeFeatures::enableWebAudio(false);
  }

  FOR_EACH_OBSERVER(RenderProcessObserver, observers_, WebKitInitialized());

  devtools_agent_message_filter_ = new DevToolsAgentFilter();
  AddFilter(devtools_agent_message_filter_.get());

  if (GetContentClient()->renderer()->RunIdleHandlerWhenWidgetsHidden())
    ScheduleIdleHandler(kLongIdleHandlerDelayMs);

  webkit::SetSharedMemoryAllocationFunction(AllocateSharedMemoryFunction);
}

void RenderThreadImpl::RegisterSchemes() {
  // swappedout: pages should not be accessible, and should also
  // be treated as empty documents that can commit synchronously.
  WebString swappedout_scheme(ASCIIToUTF16(chrome::kSwappedOutScheme));
  WebSecurityPolicy::registerURLSchemeAsDisplayIsolated(swappedout_scheme);
  WebSecurityPolicy::registerURLSchemeAsEmptyDocument(swappedout_scheme);
}

void RenderThreadImpl::RecordUserMetrics(const std::string& action) {
  Send(new ViewHostMsg_UserMetricsRecordAction(action));
}

scoped_ptr<base::SharedMemory>
    RenderThreadImpl::HostAllocateSharedMemoryBuffer(size_t size) {
  if (size > static_cast<size_t>(std::numeric_limits<int>::max()))
    return scoped_ptr<base::SharedMemory>();

  base::SharedMemoryHandle handle;
  bool success;
  IPC::Message* message =
      new ChildProcessHostMsg_SyncAllocateSharedMemory(size, &handle);

  // Allow calling this from the compositor thread.
  if (base::MessageLoop::current() == message_loop())
    success = ChildThread::Send(message);
  else
    success = sync_message_filter()->Send(message);

  if (!success)
    return scoped_ptr<base::SharedMemory>();

  if (!base::SharedMemory::IsHandleValid(handle))
    return scoped_ptr<base::SharedMemory>();

  return scoped_ptr<base::SharedMemory>(new base::SharedMemory(handle, false));
}

void RenderThreadImpl::RegisterExtension(v8::Extension* extension) {
  WebScriptController::registerExtension(extension);
}

void RenderThreadImpl::ScheduleIdleHandler(int64 initial_delay_ms) {
  idle_notification_delay_in_ms_ = initial_delay_ms;
  idle_timer_.Stop();
  idle_timer_.Start(FROM_HERE,
      base::TimeDelta::FromMilliseconds(initial_delay_ms),
      this, &RenderThreadImpl::IdleHandler);
}

void RenderThreadImpl::IdleHandler() {
  bool run_in_foreground_tab = (widget_count_ > hidden_widget_count_) &&
                               GetContentClient()->renderer()->
                                   RunIdleHandlerWhenWidgetsHidden();
  if (run_in_foreground_tab) {
    IdleHandlerInForegroundTab();
    return;
  }

  base::allocator::ReleaseFreeMemory();

  v8::V8::IdleNotification();

  // Schedule next invocation.
  // Dampen the delay using the algorithm (if delay is in seconds):
  //    delay = delay + 1 / (delay + 2)
  // Using floor(delay) has a dampening effect such as:
  //    1s, 1, 1, 2, 2, 2, 2, 3, 3, ...
  // If the delay is in milliseconds, the above formula is equivalent to:
  //    delay_ms / 1000 = delay_ms / 1000 + 1 / (delay_ms / 1000 + 2)
  // which is equivalent to
  //    delay_ms = delay_ms + 1000*1000 / (delay_ms + 2000).
  // Note that idle_notification_delay_in_ms_ would be reset to
  // kInitialIdleHandlerDelayMs in RenderThreadImpl::WidgetHidden.
  ScheduleIdleHandler(idle_notification_delay_in_ms_ +
                      1000000 / (idle_notification_delay_in_ms_ + 2000));

  FOR_EACH_OBSERVER(RenderProcessObserver, observers_, IdleNotification());
}

void RenderThreadImpl::IdleHandlerInForegroundTab() {
  // Increase the delay in the same way as in IdleHandler,
  // but make it periodic by reseting it once it is too big.
  int64 new_delay_ms = idle_notification_delay_in_ms_ +
                       1000000 / (idle_notification_delay_in_ms_ + 2000);
  if (new_delay_ms >= kLongIdleHandlerDelayMs)
    new_delay_ms = kShortIdleHandlerDelayMs;

  if (idle_notifications_to_skip_ > 0) {
    idle_notifications_to_skip_--;
  } else  {
    int cpu_usage = 0;
    Send(new ViewHostMsg_GetCPUUsage(&cpu_usage));
    // Idle notification hint roughly specifies the expected duration of the
    // idle pause. We set it proportional to the idle timer delay.
    int idle_hint = static_cast<int>(new_delay_ms / 10);
    if (cpu_usage < kIdleCPUUsageThresholdInPercents) {
      base::allocator::ReleaseFreeMemory();
      if (v8::V8::IdleNotification(idle_hint)) {
        // V8 finished collecting garbage.
        new_delay_ms = kLongIdleHandlerDelayMs;
      }
    }
  }
  ScheduleIdleHandler(new_delay_ms);
}

int64 RenderThreadImpl::GetIdleNotificationDelayInMs() const {
  return idle_notification_delay_in_ms_;
}

void RenderThreadImpl::SetIdleNotificationDelayInMs(
    int64 idle_notification_delay_in_ms) {
  idle_notification_delay_in_ms_ = idle_notification_delay_in_ms;
}

void RenderThreadImpl::ToggleWebKitSharedTimer(bool suspend) {
  if (suspend_webkit_shared_timer_) {
    EnsureWebKitInitialized();
    if (suspend) {
      webkit_platform_support_->SuspendSharedTimer();
    } else {
      webkit_platform_support_->ResumeSharedTimer();
    }
  }
}

void RenderThreadImpl::UpdateHistograms(int sequence_number) {
  child_histogram_message_filter()->SendHistograms(sequence_number);
}

int RenderThreadImpl::PostTaskToAllWebWorkers(const base::Closure& closure) {
  return webkit_glue::WorkerTaskRunner::Instance()->PostTaskToAllThreads(
      closure);
}

bool RenderThreadImpl::ResolveProxy(const GURL& url, std::string* proxy_list) {
  bool result = false;
  Send(new ViewHostMsg_ResolveProxy(url, &result, proxy_list));
  return result;
}

void RenderThreadImpl::PostponeIdleNotification() {
  idle_notifications_to_skip_ = 2;
}

scoped_refptr<RendererGpuVideoDecoderFactories>
RenderThreadImpl::GetGpuFactories(
    const scoped_refptr<base::MessageLoopProxy>& factories_loop) {
  DCHECK(IsMainThread());

  const CommandLine* cmd_line = CommandLine::ForCurrentProcess();
  scoped_refptr<RendererGpuVideoDecoderFactories> gpu_factories;
  WebGraphicsContext3DCommandBufferImpl* context3d = NULL;
  if (!cmd_line->HasSwitch(switches::kDisableAcceleratedVideoDecode))
    context3d = GetGpuVDAContext3D();
  if (context3d) {
    GpuChannelHost* gpu_channel_host = GetGpuChannel();
    if (gpu_channel_host) {
      gpu_factories = new RendererGpuVideoDecoderFactories(
          gpu_channel_host, factories_loop, context3d);
    }
  }
  return gpu_factories;
}

/* static */
void RenderThreadImpl::OnGpuVDAContextLoss() {
  RenderThreadImpl* self = RenderThreadImpl::current();
  DCHECK(self);
  if (!self->gpu_vda_context3d_)
    return;
  if (self->compositor_message_loop_proxy().get()) {
    self->compositor_message_loop_proxy()
        ->DeleteSoon(FROM_HERE, self->gpu_vda_context3d_.release());
  } else {
    self->gpu_vda_context3d_.reset();
  }
}

WebGraphicsContext3DCommandBufferImpl*
RenderThreadImpl::GetGpuVDAContext3D() {
  if (!gpu_vda_context3d_) {
    gpu_vda_context3d_.reset(
        WebGraphicsContext3DCommandBufferImpl::CreateOffscreenContext(
            this, WebKit::WebGraphicsContext3D::Attributes(),
            GURL("chrome://gpu/RenderThreadImpl::GetGpuVDAContext3D")));
    if (gpu_vda_context3d_)
      gpu_vda_context3d_->setContextLostCallback(context_lost_cb_.get());
  }
  return gpu_vda_context3d_.get();
}

scoped_ptr<WebGraphicsContext3DCommandBufferImpl>
RenderThreadImpl::CreateOffscreenContext3d() {
  WebKit::WebGraphicsContext3D::Attributes attributes;
  attributes.shareResources = true;
  attributes.depth = false;
  attributes.stencil = false;
  attributes.antialias = false;
  attributes.noAutomaticFlushes = true;

  return make_scoped_ptr(
      WebGraphicsContext3DCommandBufferImpl::CreateOffscreenContext(
          this,
          attributes,
          GURL("chrome://gpu/RenderThreadImpl::CreateOffscreenContext3d")));
}

scoped_refptr<cc::ContextProvider>
RenderThreadImpl::OffscreenContextProviderForMainThread() {
  DCHECK(IsMainThread());

#if defined(OS_ANDROID)
  if (SynchronousCompositorFactory* factory =
      SynchronousCompositorFactory::GetInstance()) {
    return factory->GetOffscreenContextProviderForMainThread();
  }
#endif

  if (!shared_contexts_main_thread_.get() ||
      shared_contexts_main_thread_->DestroyedOnMainThread()) {
    shared_contexts_main_thread_ =
        RendererContextProviderCommandBuffer::Create();
    if (shared_contexts_main_thread_.get() &&
        !shared_contexts_main_thread_->BindToCurrentThread())
      shared_contexts_main_thread_ = NULL;
  }
  return shared_contexts_main_thread_;
}

scoped_refptr<cc::ContextProvider>
RenderThreadImpl::OffscreenContextProviderForCompositorThread() {
  DCHECK(IsMainThread());

#if defined(OS_ANDROID)
  if (SynchronousCompositorFactory* factory =
      SynchronousCompositorFactory::GetInstance()) {
    return factory->GetOffscreenContextProviderForCompositorThread();
  }
#endif

  if (!shared_contexts_compositor_thread_.get() ||
      shared_contexts_compositor_thread_->DestroyedOnMainThread()) {
    shared_contexts_compositor_thread_ =
        RendererContextProviderCommandBuffer::Create();
  }
  return shared_contexts_compositor_thread_;
}

AudioRendererMixerManager* RenderThreadImpl::GetAudioRendererMixerManager() {
  if (!audio_renderer_mixer_manager_) {
    audio_renderer_mixer_manager_.reset(new AudioRendererMixerManager(
        GetAudioHardwareConfig()));
  }

  return audio_renderer_mixer_manager_.get();
}

media::AudioHardwareConfig* RenderThreadImpl::GetAudioHardwareConfig() {
  if (!audio_hardware_config_) {
    media::AudioParameters input_params;
    media::AudioParameters output_params;
    Send(new ViewHostMsg_GetAudioHardwareConfig(
        &input_params, &output_params));

    audio_hardware_config_.reset(new media::AudioHardwareConfig(
        input_params, output_params));
    audio_message_filter_->SetAudioHardwareConfig(audio_hardware_config_.get());
  }

  return audio_hardware_config_.get();
}

#if defined(OS_WIN)
void RenderThreadImpl::PreCacheFontCharacters(const LOGFONT& log_font,
                                              const string16& str) {
  Send(new ViewHostMsg_PreCacheFontCharacters(log_font, str));
}

void RenderThreadImpl::PreCacheFont(const LOGFONT& log_font) {
  Send(new ChildProcessHostMsg_PreCacheFont(log_font));
}

void RenderThreadImpl::ReleaseCachedFonts() {
  Send(new ChildProcessHostMsg_ReleaseCachedFonts());
}

#endif  // OS_WIN

bool RenderThreadImpl::IsMainThread() {
  return !!current();
}

base::MessageLoop* RenderThreadImpl::GetMainLoop() {
  return message_loop();
}

scoped_refptr<base::MessageLoopProxy> RenderThreadImpl::GetIOLoopProxy() {
  return io_message_loop_proxy_;
}

base::WaitableEvent* RenderThreadImpl::GetShutDownEvent() {
  return shutdown_event_;
}

scoped_ptr<base::SharedMemory> RenderThreadImpl::AllocateSharedMemory(
    size_t size) {
  return scoped_ptr<base::SharedMemory>(
      HostAllocateSharedMemoryBuffer(size));
}

int32 RenderThreadImpl::CreateViewCommandBuffer(
      int32 surface_id, const GPUCreateCommandBufferConfig& init_params) {
  TRACE_EVENT1("gpu",
               "RenderThreadImpl::CreateViewCommandBuffer",
               "surface_id",
               surface_id);

  int32 route_id = MSG_ROUTING_NONE;
  IPC::Message* message = new GpuHostMsg_CreateViewCommandBuffer(
      surface_id,
      init_params,
      &route_id);

  // Allow calling this from the compositor thread.
  thread_safe_sender()->Send(message);

  return route_id;
}

void RenderThreadImpl::CreateImage(
    gfx::PluginWindowHandle window,
    int32 image_id,
    const CreateImageCallback& callback) {
  NOTREACHED();
}

void RenderThreadImpl::DeleteImage(int32 image_id, int32 sync_point) {
  NOTREACHED();
}

void RenderThreadImpl::DoNotSuspendWebKitSharedTimer() {
  suspend_webkit_shared_timer_ = false;
}

void RenderThreadImpl::DoNotNotifyWebKitOfModalLoop() {
  notify_webkit_of_modal_loop_ = false;
}

void RenderThreadImpl::OnSetZoomLevelForCurrentURL(const std::string& scheme,
                                                   const std::string& host,
                                                   double zoom_level) {
  RenderViewZoomer zoomer(scheme, host, zoom_level);
  RenderView::ForEach(&zoomer);
}

bool RenderThreadImpl::OnControlMessageReceived(const IPC::Message& msg) {
  ObserverListBase<RenderProcessObserver>::Iterator it(observers_);
  RenderProcessObserver* observer;
  while ((observer = it.GetNext()) != NULL) {
    if (observer->OnControlMessageReceived(msg))
      return true;
  }

  // Some messages are handled by delegates.
  if (appcache_dispatcher_->OnMessageReceived(msg) ||
      dom_storage_dispatcher_->OnMessageReceived(msg)) {
    return true;
  }

  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(RenderThreadImpl, msg)
    IPC_MESSAGE_HANDLER(ViewMsg_SetZoomLevelForCurrentURL,
                        OnSetZoomLevelForCurrentURL)
    // TODO(port): removed from render_messages_internal.h;
    // is there a new non-windows message I should add here?
    IPC_MESSAGE_HANDLER(ViewMsg_New, OnCreateNewView)
    IPC_MESSAGE_HANDLER(ViewMsg_PurgePluginListCache, OnPurgePluginListCache)
    IPC_MESSAGE_HANDLER(ViewMsg_NetworkStateChanged, OnNetworkStateChanged)
    IPC_MESSAGE_HANDLER(ViewMsg_TempCrashWithData, OnTempCrashWithData)
    IPC_MESSAGE_HANDLER(ViewMsg_SetWebKitSharedTimersSuspended,
                        OnSetWebKitSharedTimersSuspended)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void RenderThreadImpl::OnCreateNewView(const ViewMsg_New_Params& params) {
  EnsureWebKitInitialized();
  // When bringing in render_view, also bring in webkit's glue and jsbindings.
  RenderViewImpl::Create(
      params.opener_route_id,
      params.renderer_preferences,
      params.web_preferences,
      new SharedRenderViewCounter(0),
      params.view_id,
      params.main_frame_routing_id,
      params.surface_id,
      params.session_storage_namespace_id,
      params.frame_name,
      false,
      params.swapped_out,
      params.next_page_id,
      params.screen_info,
      params.accessibility_mode,
      params.allow_partial_swap);
}

GpuChannelHost* RenderThreadImpl::EstablishGpuChannelSync(
    CauseForGpuLaunch cause_for_gpu_launch) {
  TRACE_EVENT0("gpu", "RenderThreadImpl::EstablishGpuChannelSync");

  if (gpu_channel_.get()) {
    // Do nothing if we already have a GPU channel or are already
    // establishing one.
    if (!gpu_channel_->IsLost())
      return gpu_channel_.get();

    // Recreate the channel if it has been lost.
    gpu_channel_ = NULL;
  }

  // Ask the browser for the channel name.
  int client_id = 0;
  IPC::ChannelHandle channel_handle;
  gpu::GPUInfo gpu_info;
  if (!Send(new GpuHostMsg_EstablishGpuChannel(cause_for_gpu_launch,
                                               &client_id,
                                               &channel_handle,
                                               &gpu_info)) ||
#if defined(OS_POSIX)
      channel_handle.socket.fd == -1 ||
#endif
      channel_handle.name.empty()) {
    // Otherwise cancel the connection.
    return NULL;
  }

  GetContentClient()->SetGpuInfo(gpu_info);

  // Cache some variables that are needed on the compositor thread for our
  // implementation of GpuChannelHostFactory.
  io_message_loop_proxy_ = ChildProcess::current()->io_message_loop_proxy();
  shutdown_event_ = ChildProcess::current()->GetShutDownEvent();

  gpu_channel_ = GpuChannelHost::Create(
      this, 0, client_id, gpu_info, channel_handle);
  return gpu_channel_.get();
}

WebKit::WebMediaStreamCenter* RenderThreadImpl::CreateMediaStreamCenter(
    WebKit::WebMediaStreamCenterClient* client) {
#if defined(OS_ANDROID)
  if (CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableWebRTC))
    return NULL;
#endif

#if defined(ENABLE_WEBRTC)
  if (!media_stream_center_) {
    media_stream_center_ = GetContentClient()->renderer()
        ->OverrideCreateWebMediaStreamCenter(client);
    if (!media_stream_center_) {
      scoped_ptr<MediaStreamCenter> media_stream_center(
          new MediaStreamCenter(client, GetMediaStreamDependencyFactory()));
      AddObserver(media_stream_center.get());
      media_stream_center_ = media_stream_center.release();
    }
  }
#endif
  return media_stream_center_;
}

MediaStreamDependencyFactory*
RenderThreadImpl::GetMediaStreamDependencyFactory() {
#if defined(ENABLE_WEBRTC)
  if (!media_stream_factory_) {
    media_stream_factory_.reset(new MediaStreamDependencyFactory(
        vc_manager_.get(), p2p_socket_dispatcher_.get()));
  }
#endif
  return media_stream_factory_.get();
}

GpuChannelHost* RenderThreadImpl::GetGpuChannel() {
  if (!gpu_channel_.get())
    return NULL;

  if (gpu_channel_->IsLost())
    return NULL;

  return gpu_channel_.get();
}

void RenderThreadImpl::OnPurgePluginListCache(bool reload_pages) {
  EnsureWebKitInitialized();
  // The call below will cause a GetPlugins call with refresh=true, but at this
  // point we already know that the browser has refreshed its list, so disable
  // refresh temporarily to prevent each renderer process causing the list to be
  // regenerated.
  webkit_platform_support_->set_plugin_refresh_allowed(false);
  WebKit::resetPluginCache(reload_pages);
  webkit_platform_support_->set_plugin_refresh_allowed(true);

  FOR_EACH_OBSERVER(RenderProcessObserver, observers_, PluginListChanged());
}

void RenderThreadImpl::OnNetworkStateChanged(bool online) {
  EnsureWebKitInitialized();
  WebNetworkStateNotifier::setOnLine(online);
}

void RenderThreadImpl::OnTempCrashWithData(const GURL& data) {
  GetContentClient()->SetActiveURL(data);
  CHECK(false);
}

void RenderThreadImpl::OnSetWebKitSharedTimersSuspended(bool suspend) {
  ToggleWebKitSharedTimer(suspend);
}

void RenderThreadImpl::OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
  base::allocator::ReleaseFreeMemory();

  if (memory_pressure_level ==
      base::MemoryPressureListener::MEMORY_PRESSURE_CRITICAL) {
    // Trigger full v8 garbage collection on critical memory notification.
    v8::V8::LowMemoryNotification();
    // Clear the image cache.
    WebKit::WebImageCache::clear();
  } else {
    // Otherwise trigger a couple of v8 GCs using IdleNotification.
    if (!v8::V8::IdleNotification())
      v8::V8::IdleNotification();
  }
}

scoped_refptr<base::MessageLoopProxy>
RenderThreadImpl::GetFileThreadMessageLoopProxy() {
  DCHECK(message_loop() == base::MessageLoop::current());
  if (!file_thread_) {
    file_thread_.reset(new base::Thread("Renderer::FILE"));
    file_thread_->Start();
  }
  return file_thread_->message_loop_proxy();
}

scoped_refptr<base::MessageLoopProxy>
RenderThreadImpl::GetMediaThreadMessageLoopProxy() {
  DCHECK(message_loop() == base::MessageLoop::current());
  if (!media_thread_) {
    media_thread_.reset(new base::Thread("Media"));
    media_thread_->Start();
  }
  return media_thread_->message_loop_proxy();
}

void RenderThreadImpl::SetFlingCurveParameters(
    const std::vector<float>& new_touchpad,
    const std::vector<float>& new_touchscreen) {
  webkit_platform_support_->SetFlingCurveParameters(new_touchpad,
                                                    new_touchscreen);

}

}  // namespace content
