// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_CHROME_CONTENT_RENDERER_CLIENT_H_
#define CHROME_RENDERER_CHROME_CONTENT_RENDERER_CLIENT_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/string16.h"
#include "content/public/renderer/content_renderer_client.h"
#include "ipc/ipc_channel_proxy.h"
#include "v8/include/v8.h"

class ChromeExtensionsDispatcherDelegate;
class ChromeRenderProcessObserver;
#if defined(ENABLE_PRINT_PREVIEW)
class ChromePDFPrintClient;
#endif
class PrescientNetworkingDispatcher;
#if defined(ENABLE_SPELLCHECK)
class SpellCheck;
class SpellCheckProvider;
#endif

struct ChromeViewHostMsg_GetPluginInfo_Output;

namespace content {
class BrowserPluginDelegate;
struct WebPluginInfo;
}

namespace network_hints {
class PrescientNetworkingDispatcher;
}

namespace extensions {
class Dispatcher;
class Extension;
class ExtensionSet;
class ExtensionsGuestViewContainerDispatcher;
class RendererPermissionsPolicyDelegate;
class ResourceRequestPolicy;
}

namespace prerender {
class PrerenderDispatcher;
}

namespace safe_browsing {
class PhishingClassifierFilter;
}

namespace visitedlink {
class VisitedLinkSlave;
}

namespace web_cache {
class WebCacheRenderProcessObserver;
}

namespace blink {
class WebSecurityOrigin;
}

namespace password_manager {
class CredentialManagerClient;
}

#if defined(ENABLE_WEBRTC)
class WebRtcLoggingMessageFilter;
#endif

class ChromeContentRendererClient : public content::ContentRendererClient {
 public:
  ChromeContentRendererClient();
  ~ChromeContentRendererClient() override;

  void RenderThreadStarted() override;
  void RenderFrameCreated(content::RenderFrame* render_frame) override;
  void RenderViewCreated(content::RenderView* render_view) override;
  SkBitmap* GetSadPluginBitmap() override;
  SkBitmap* GetSadWebViewBitmap() override;
  bool OverrideCreatePlugin(content::RenderFrame* render_frame,
                            blink::WebLocalFrame* frame,
                            const blink::WebPluginParams& params,
                            blink::WebPlugin** plugin) override;
  blink::WebPlugin* CreatePluginReplacement(
      content::RenderFrame* render_frame,
      const base::FilePath& plugin_path) override;
  bool HasErrorPage(int http_status_code, std::string* error_domain) override;
  bool ShouldSuppressErrorPage(content::RenderFrame* render_frame,
                               const GURL& url) override;
  void GetNavigationErrorStrings(content::RenderView* render_view,
                                 blink::WebFrame* frame,
                                 const blink::WebURLRequest& failed_request,
                                 const blink::WebURLError& error,
                                 std::string* error_html,
                                 base::string16* error_description) override;
  void DeferMediaLoad(content::RenderFrame* render_frame,
                      bool has_played_media_before,
                      const base::Closure& closure) override;
  bool RunIdleHandlerWhenWidgetsHidden() override;
  bool AllowTimerSuspensionWhenProcessBackgrounded() override;
  bool AllowPopup() override;
  bool ShouldFork(blink::WebLocalFrame* frame,
                  const GURL& url,
                  const std::string& http_method,
                  bool is_initial_navigation,
                  bool is_server_redirect,
                  bool* send_referrer) override;
  bool WillSendRequest(blink::WebFrame* frame,
                       ui::PageTransition transition_type,
                       const GURL& url,
                       const GURL& first_party_for_cookies,
                       GURL* new_url) override;
  unsigned long long VisitedLinkHash(const char* canonical_url,
                                     size_t length) override;
  bool IsLinkVisited(unsigned long long link_hash) override;
  blink::WebPrescientNetworking* GetPrescientNetworking() override;
  bool ShouldOverridePageVisibilityState(
      const content::RenderFrame* render_frame,
      blink::WebPageVisibilityState* override_state) override;
  const void* CreatePPAPIInterface(const std::string& interface_name) override;
  bool IsExternalPepperPlugin(const std::string& module_name) override;
  blink::WebSpeechSynthesizer* OverrideSpeechSynthesizer(
      blink::WebSpeechSynthesizerClient* client) override;
  bool ShouldReportDetailedMessageForSource(
      const base::string16& source) const override;
  bool ShouldGatherSiteIsolationStats() const override;
  blink::WebWorkerContentSettingsClientProxy*
  CreateWorkerContentSettingsClientProxy(content::RenderFrame* render_frame,
                                         blink::WebFrame* frame) override;
  bool AllowPepperMediaStreamAPI(const GURL& url) override;
  void AddKeySystems(std::vector<media::KeySystemInfo>* key_systems) override;
  bool IsPluginAllowedToUseDevChannelAPIs() override;
  bool IsPluginAllowedToUseCameraDeviceAPI(const GURL& url) override;
  bool IsPluginAllowedToUseCompositorAPI(const GURL& url) override;
  content::BrowserPluginDelegate* CreateBrowserPluginDelegate(
      content::RenderFrame* render_frame,
      const std::string& mime_type,
      const GURL& original_url) override;
  void RecordRappor(const std::string& metric,
                    const std::string& sample) override;
  void RecordRapporURL(const std::string& metric, const GURL& url) override;
  scoped_ptr<blink::WebAppBannerClient> CreateAppBannerClient(
      content::RenderFrame* render_frame) override;
  void AddImageContextMenuProperties(
      const blink::WebURLResponse& response,
      std::map<std::string, std::string>* properties) override;
  void DidInitializeServiceWorkerContextOnWorkerThread(
      v8::Local<v8::Context> context,
      const GURL& url) override;
  void WillDestroyServiceWorkerContextOnWorkerThread(
      v8::Local<v8::Context> context,
      const GURL& url) override;
#if defined(ENABLE_EXTENSIONS)
  // Takes ownership.
  void SetExtensionDispatcherForTest(
      extensions::Dispatcher* extension_dispatcher);
  extensions::Dispatcher* GetExtensionDispatcherForTest();
#endif

#if defined(ENABLE_SPELLCHECK)
  // Sets a new |spellcheck|. Used for testing only.
  // Takes ownership of |spellcheck|.
  void SetSpellcheck(SpellCheck* spellcheck);
#endif

#if defined(ENABLE_PLUGINS)
  static blink::WebPlugin* CreatePlugin(
      content::RenderFrame* render_frame,
      blink::WebLocalFrame* frame,
      const blink::WebPluginParams& params,
      const ChromeViewHostMsg_GetPluginInfo_Output& output);
#endif

#if defined(ENABLE_PLUGINS) && defined(ENABLE_EXTENSIONS)
  static bool IsExtensionOrSharedModuleWhitelisted(
      const GURL& url, const std::set<std::string>& whitelist);
#endif

  static bool WasWebRequestUsedBySomeExtensions();

 private:
  FRIEND_TEST_ALL_PREFIXES(ChromeContentRendererClientTest, NaClRestriction);
  FRIEND_TEST_ALL_PREFIXES(ChromeContentRendererClientTest,
                           ShouldSuppressErrorPage);

#if defined(ENABLE_EXTENSIONS)
  // Gets extension by the given origin, regardless of whether the extension
  // is active in the current process.
  const extensions::Extension* GetExtensionByOrigin(
      const blink::WebSecurityOrigin& origin) const;

  // Returns true if the frame is navigating to an URL either into or out of an
  // extension app's extent.
  bool CrossesExtensionExtents(blink::WebLocalFrame* frame,
                               const GURL& new_url,
                               bool is_extension_url,
                               bool is_initial_navigation);
#endif

  static GURL GetNaClContentHandlerURL(const std::string& actual_mime_type,
                                       const content::WebPluginInfo& plugin);

#if !defined(DISABLE_NACL)
  // Determines if a NaCl app is allowed, and modifies params to pass the app's
  // permissions to the trusted NaCl plugin.
  static bool IsNaClAllowed(const GURL& manifest_url,
                            const GURL& app_url,
                            bool is_nacl_unrestricted,
                            const extensions::Extension* extension,
                            blink::WebPluginParams* params);
#endif

  scoped_ptr<ChromeRenderProcessObserver> chrome_observer_;
  scoped_ptr<web_cache::WebCacheRenderProcessObserver> web_cache_observer_;

// TODO(thestig): Extract into a separate file if possible. Cleanup
// ENABLE_EXTENSIONS ifdefs in the .cc file as well.
#if defined(ENABLE_EXTENSIONS)
  scoped_ptr<ChromeExtensionsDispatcherDelegate> extension_dispatcher_delegate_;
  scoped_ptr<extensions::Dispatcher> extension_dispatcher_;
  scoped_ptr<extensions::RendererPermissionsPolicyDelegate>
      permissions_policy_delegate_;
  scoped_ptr<extensions::ExtensionsGuestViewContainerDispatcher>
      guest_view_container_dispatcher_;
  scoped_ptr<extensions::ResourceRequestPolicy> resource_request_policy_;
#endif

  scoped_ptr<network_hints::PrescientNetworkingDispatcher>
      prescient_networking_dispatcher_;
  scoped_ptr<password_manager::CredentialManagerClient>
      credential_manager_client_;
#if defined(ENABLE_SPELLCHECK)
  scoped_ptr<SpellCheck> spellcheck_;
#endif
  scoped_ptr<visitedlink::VisitedLinkSlave> visited_link_slave_;
  scoped_ptr<safe_browsing::PhishingClassifierFilter> phishing_classifier_;
  scoped_ptr<prerender::PrerenderDispatcher> prerender_dispatcher_;
#if defined(ENABLE_WEBRTC)
  scoped_refptr<WebRtcLoggingMessageFilter> webrtc_logging_message_filter_;
#endif
#if defined(ENABLE_PRINT_PREVIEW)
  scoped_ptr<ChromePDFPrintClient> pdf_print_client_;
#endif
#if defined(ENABLE_PLUGINS)
  std::set<std::string> allowed_camera_device_origins_;
  std::set<std::string> allowed_compositor_origins_;
#endif
};

#endif  // CHROME_RENDERER_CHROME_CONTENT_RENDERER_CLIENT_H_
