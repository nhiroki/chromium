// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/extensions/extension_settings_handler.h"

#include "apps/app_load_service.h"
#include "apps/app_restore_service.h"
#include "apps/saved_files_service.h"
#include "apps/shell_window.h"
#include "apps/shell_window_registry.h"
#include "base/auto_reset.h"
#include "base/base64.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/message_loop/message_loop.h"
#include "base/prefs/pref_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "base/version.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/devtools/devtools_window.h"
#include "chrome/browser/extensions/api/extension_action/extension_action_api.h"
#include "chrome/browser/extensions/component_loader.h"
#include "chrome/browser/extensions/crx_installer.h"
#include "chrome/browser/extensions/devtools_util.h"
#include "chrome/browser/extensions/error_console/error_console.h"
#include "chrome/browser/extensions/extension_action_manager.h"
#include "chrome/browser/extensions/extension_disabled_ui.h"
#include "chrome/browser/extensions/extension_error_reporter.h"
#include "chrome/browser/extensions/extension_host.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_system.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/extensions/extension_warning_set.h"
#include "chrome/browser/extensions/unpacked_installer.h"
#include "chrome/browser/extensions/updater/extension_updater.h"
#include "chrome/browser/google/google_util.h"
#include "chrome/browser/managed_mode/managed_user_service.h"
#include "chrome/browser/managed_mode/managed_user_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/tab_contents/background_contents.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/chrome_select_file_policy.h"
#include "chrome/browser/ui/extensions/application_launch.h"
#include "chrome/browser/ui/webui/extensions/extension_basic_info.h"
#include "chrome/browser/ui/webui/extensions/extension_icon_source.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/common/extensions/extension_icon_set.h"
#include "chrome/common/extensions/extension_set.h"
#include "chrome/common/extensions/manifest_url_handler.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "components/user_prefs/pref_registry_syncable.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/user_metrics.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_view.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "extensions/browser/extension_error.h"
#include "extensions/browser/lazy_background_task_queue.h"
#include "extensions/browser/management_policy.h"
#include "extensions/browser/view_type_utils.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/feature_switch.h"
#include "extensions/common/manifest_handlers/background_info.h"
#include "extensions/common/manifest_handlers/incognito_info.h"
#include "grit/browser_resources.h"
#include "grit/chromium_strings.h"
#include "grit/generated_resources.h"
#include "grit/theme_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

using base::DictionaryValue;
using base::ListValue;
using content::RenderViewHost;
using content::WebContents;

namespace extensions {

ExtensionPage::ExtensionPage(const GURL& url,
                             int render_process_id,
                             int render_view_id,
                             bool incognito,
                             bool generated_background_page)
    : url(url),
      render_process_id(render_process_id),
      render_view_id(render_view_id),
      incognito(incognito),
      generated_background_page(generated_background_page) {
}

// On Mac, the install prompt is not modal. This means that the user can
// navigate while the dialog is up, causing the dialog handler to outlive the
// ExtensionSettingsHandler. That's a problem because the dialog framework will
// try to contact us back once the dialog is closed, which causes a crash.
// This class is designed to broker the message between the two objects, while
// managing its own lifetime so that it can outlive the ExtensionSettingsHandler
// and (when doing so) gracefully ignore the message from the dialog.
class BrokerDelegate : public ExtensionInstallPrompt::Delegate {
 public:
  explicit BrokerDelegate(
      const base::WeakPtr<ExtensionSettingsHandler>& delegate)
      : delegate_(delegate) {}

  // ExtensionInstallPrompt::Delegate implementation.
  virtual void InstallUIProceed() OVERRIDE {
    if (delegate_)
      delegate_->InstallUIProceed();
    delete this;
  };

  virtual void InstallUIAbort(bool user_initiated) OVERRIDE {
    if (delegate_)
      delegate_->InstallUIAbort(user_initiated);
    delete this;
  };

 private:
  base::WeakPtr<ExtensionSettingsHandler> delegate_;

  DISALLOW_COPY_AND_ASSIGN(BrokerDelegate);
};

///////////////////////////////////////////////////////////////////////////////
//
// ExtensionSettingsHandler
//
///////////////////////////////////////////////////////////////////////////////

ExtensionSettingsHandler::ExtensionSettingsHandler()
    : extension_service_(NULL),
      management_policy_(NULL),
      ignore_notifications_(false),
      deleting_rvh_(NULL),
      deleting_rwh_id_(-1),
      deleting_rph_id_(-1),
      registered_for_notifications_(false),
      warning_service_observer_(this),
      error_console_observer_(this),
      should_do_verification_check_(false) {
}

ExtensionSettingsHandler::~ExtensionSettingsHandler() {
  // There may be pending file dialogs, we need to tell them that we've gone
  // away so they don't try and call back to us.
  if (load_extension_dialog_.get())
    load_extension_dialog_->ListenerDestroyed();
}

ExtensionSettingsHandler::ExtensionSettingsHandler(ExtensionService* service,
                                                   ManagementPolicy* policy)
    : extension_service_(service),
      management_policy_(policy),
      ignore_notifications_(false),
      deleting_rvh_(NULL),
      deleting_rwh_id_(-1),
      deleting_rph_id_(-1),
      registered_for_notifications_(false),
      warning_service_observer_(this),
      error_console_observer_(this),
      should_do_verification_check_(false) {
}

// static
void ExtensionSettingsHandler::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(
      prefs::kExtensionsUIDeveloperMode,
      false,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
}

base::DictionaryValue* ExtensionSettingsHandler::CreateExtensionDetailValue(
    const Extension* extension,
    const std::vector<ExtensionPage>& pages,
    const ExtensionWarningService* warning_service) {
  base::DictionaryValue* extension_data = new base::DictionaryValue();
  bool enabled = extension_service_->IsExtensionEnabled(extension->id());
  GetExtensionBasicInfo(extension, enabled, extension_data);

  ExtensionPrefs* prefs = extension_service_->extension_prefs();
  int disable_reasons = prefs->GetDisableReasons(extension->id());

  bool suspicious_install =
      (disable_reasons & Extension::DISABLE_NOT_VERIFIED) != 0;
  extension_data->SetBoolean("suspiciousInstall", suspicious_install);
  if (suspicious_install)
    should_do_verification_check_ = true;

  bool managed_install =
      !management_policy_->UserMayModifySettings(extension, NULL);
  extension_data->SetBoolean("managedInstall", managed_install);

  // We should not get into a state where both are true.
  DCHECK(managed_install == false || suspicious_install == false);

  GURL icon =
      ExtensionIconSource::GetIconURL(extension,
                                      extension_misc::EXTENSION_ICON_MEDIUM,
                                      ExtensionIconSet::MATCH_BIGGER,
                                      !enabled, NULL);
  if (Manifest::IsUnpackedLocation(extension->location()))
    extension_data->SetString("path", extension->path().value());
  extension_data->SetString("icon", icon.spec());
  extension_data->SetBoolean("isUnpacked",
      Manifest::IsUnpackedLocation(extension->location()));
  extension_data->SetBoolean("terminated",
      extension_service_->terminated_extensions()->Contains(extension->id()));
  extension_data->SetBoolean("enabledIncognito",
      extension_util::IsIncognitoEnabled(extension->id(), extension_service_));
  extension_data->SetBoolean("incognitoCanBeToggled",
                             extension->can_be_incognito_enabled() &&
                             !extension->force_incognito_enabled());
  extension_data->SetBoolean("wantsFileAccess", extension->wants_file_access());
  extension_data->SetBoolean("allowFileAccess",
      extension_util::AllowFileAccess(extension, extension_service_));
  extension_data->SetBoolean("allow_reload",
      Manifest::IsUnpackedLocation(extension->location()));
  extension_data->SetBoolean("is_hosted_app", extension->is_hosted_app());
  extension_data->SetBoolean("is_platform_app", extension->is_platform_app());
  extension_data->SetBoolean("homepageProvided",
      ManifestURL::GetHomepageURL(extension).is_valid());

  base::string16 location_text;
  if (Manifest::IsPolicyLocation(extension->location())) {
    location_text = l10n_util::GetStringUTF16(
        IDS_OPTIONS_INSTALL_LOCATION_ENTERPRISE);
  } else if (extension->location() == Manifest::INTERNAL &&
      !ManifestURL::UpdatesFromGallery(extension)) {
    location_text = l10n_util::GetStringUTF16(
        IDS_OPTIONS_INSTALL_LOCATION_UNKNOWN);
  } else if (extension->location() == Manifest::EXTERNAL_REGISTRY) {
    location_text = l10n_util::GetStringUTF16(
        IDS_OPTIONS_INSTALL_LOCATION_3RD_PARTY);
  }
  extension_data->SetString("locationText", location_text);

  // Force unpacked extensions to show at the top.
  if (Manifest::IsUnpackedLocation(extension->location()))
    extension_data->SetInteger("order", 1);
  else
    extension_data->SetInteger("order", 2);

  if (!ExtensionActionAPI::GetBrowserActionVisibility(
          extension_service_->extension_prefs(), extension->id())) {
    extension_data->SetBoolean("enable_show_button", true);
  }

  // Add views
  base::ListValue* views = new base::ListValue;
  for (std::vector<ExtensionPage>::const_iterator iter = pages.begin();
       iter != pages.end(); ++iter) {
    base::DictionaryValue* view_value = new base::DictionaryValue;
    if (iter->url.scheme() == kExtensionScheme) {
      // No leading slash.
      view_value->SetString("path", iter->url.path().substr(1));
    } else {
      // For live pages, use the full URL.
      view_value->SetString("path", iter->url.spec());
    }
    view_value->SetInteger("renderViewId", iter->render_view_id);
    view_value->SetInteger("renderProcessId", iter->render_process_id);
    view_value->SetBoolean("incognito", iter->incognito);
    view_value->SetBoolean("generatedBackgroundPage",
                           iter->generated_background_page);
    views->Append(view_value);
  }
  extension_data->Set("views", views);
  ExtensionActionManager* extension_action_manager =
      ExtensionActionManager::Get(extension_service_->profile());
  extension_data->SetBoolean(
      "hasPopupAction",
      extension_action_manager->GetBrowserAction(*extension) ||
      extension_action_manager->GetPageAction(*extension));

  // Add warnings.
  if (warning_service) {
    std::vector<std::string> warnings =
        warning_service->GetWarningMessagesForExtension(extension->id());

    if (!warnings.empty()) {
      base::ListValue* warnings_list = new base::ListValue;
      for (std::vector<std::string>::const_iterator iter = warnings.begin();
           iter != warnings.end(); ++iter) {
        warnings_list->Append(base::Value::CreateStringValue(*iter));
      }
      extension_data->Set("warnings", warnings_list);
    }
  }

  // If the ErrorConsole is enabled, get the errors for the extension and add
  // them to the list. Otherwise, use the install warnings (using both is
  // redundant).
  ErrorConsole* error_console =
      ErrorConsole::Get(extension_service_->profile());
  if (error_console->enabled()) {
    const ErrorConsole::ErrorList& errors =
        error_console->GetErrorsForExtension(extension->id());
    if (!errors.empty()) {
      scoped_ptr<ListValue> manifest_errors(new ListValue);
      scoped_ptr<ListValue> runtime_errors(new ListValue);
      for (ErrorConsole::ErrorList::const_iterator iter = errors.begin();
           iter != errors.end(); ++iter) {
        if ((*iter)->type() == ExtensionError::MANIFEST_ERROR) {
          manifest_errors->Append((*iter)->ToValue().release());
        } else {  // Handle runtime error.
          const RuntimeError* error = static_cast<const RuntimeError*>(*iter);
          scoped_ptr<DictionaryValue> value = error->ToValue();
          bool can_inspect =
              !(deleting_rwh_id_ == error->render_view_id() &&
                deleting_rph_id_ == error->render_process_id()) &&
              RenderViewHost::FromID(error->render_process_id(),
                                     error->render_view_id()) != NULL;
          value->SetBoolean("canInspect", can_inspect);
          runtime_errors->Append(value.release());
        }
      }
      if (!manifest_errors->empty())
        extension_data->Set("manifestErrors", manifest_errors.release());
      if (!runtime_errors->empty())
        extension_data->Set("runtimeErrors", runtime_errors.release());
    }
  } else if (Manifest::IsUnpackedLocation(extension->location())) {
    const std::vector<InstallWarning>& install_warnings =
        extension->install_warnings();
    if (!install_warnings.empty()) {
      scoped_ptr<base::ListValue> list(new base::ListValue());
      for (std::vector<InstallWarning>::const_iterator it =
               install_warnings.begin(); it != install_warnings.end(); ++it) {
        base::DictionaryValue* item = new base::DictionaryValue();
        item->SetString("message", it->message);
        list->Append(item);
      }
      extension_data->Set("installWarnings", list.release());
    }
  }

  return extension_data;
}

void ExtensionSettingsHandler::GetLocalizedValues(
    content::WebUIDataSource* source) {
  source->AddString("extensionSettings",
      l10n_util::GetStringUTF16(IDS_MANAGE_EXTENSIONS_SETTING_WINDOWS_TITLE));

  source->AddString("extensionSettingsDeveloperMode",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_DEVELOPER_MODE_LINK));
  source->AddString("extensionSettingsNoExtensions",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_NONE_INSTALLED));
  source->AddString("extensionSettingsSuggestGallery",
      l10n_util::GetStringFUTF16(IDS_EXTENSIONS_NONE_INSTALLED_SUGGEST_GALLERY,
          ASCIIToUTF16(google_util::AppendGoogleLocaleParam(
              GURL(extension_urls::GetExtensionGalleryURL())).spec())));
  source->AddString("extensionSettingsGetMoreExtensions",
      l10n_util::GetStringUTF16(IDS_GET_MORE_EXTENSIONS));
  source->AddString("extensionSettingsGetMoreExtensionsUrl",
      ASCIIToUTF16(google_util::AppendGoogleLocaleParam(
          GURL(extension_urls::GetExtensionGalleryURL())).spec()));
  source->AddString("extensionSettingsExtensionId",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_ID));
  source->AddString("extensionSettingsExtensionPath",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_PATH));
  source->AddString("extensionSettingsInspectViews",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_INSPECT_VIEWS));
  source->AddString("extensionSettingsInstallWarnings",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_INSTALL_WARNINGS));
  source->AddString("viewIncognito",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_VIEW_INCOGNITO));
  source->AddString("viewInactive",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_VIEW_INACTIVE));
  source->AddString("backgroundPage",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_BACKGROUND_PAGE));
  source->AddString("extensionSettingsEnable",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_ENABLE));
  source->AddString("extensionSettingsEnabled",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_ENABLED));
  source->AddString("extensionSettingsRemove",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_REMOVE));
  source->AddString("extensionSettingsEnableIncognito",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_ENABLE_INCOGNITO));
  source->AddString("extensionSettingsAllowFileAccess",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_ALLOW_FILE_ACCESS));
  source->AddString("extensionSettingsIncognitoWarning",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_INCOGNITO_WARNING));
  source->AddString("extensionSettingsReloadTerminated",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_RELOAD_TERMINATED));
  source->AddString("extensionSettingsLaunch",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_LAUNCH));
  source->AddString("extensionSettingsReloadUnpacked",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_RELOAD_UNPACKED));
  source->AddString("extensionSettingsOptions",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_OPTIONS_LINK));
  source->AddString("extensionSettingsPermissions",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_PERMISSIONS_LINK));
  source->AddString("extensionSettingsVisitWebsite",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_VISIT_WEBSITE));
  source->AddString("extensionSettingsVisitWebStore",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_VISIT_WEBSTORE));
  source->AddString("extensionSettingsPolicyControlled",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_POLICY_CONTROLLED));
  source->AddString("extensionSettingsManagedMode",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_LOCKED_MANAGED_USER));
  source->AddString("extensionSettingsSuspiciousInstall",
      l10n_util::GetStringFUTF16(
          IDS_EXTENSIONS_ADDED_WITHOUT_KNOWLEDGE,
          l10n_util::GetStringUTF16(IDS_EXTENSION_WEB_STORE_TITLE)));
  source->AddString("extensionSettingsSuspiciousInstallLearnMore",
      l10n_util::GetStringUTF16(IDS_LEARN_MORE));
  source->AddString("extensionSettingsSuspiciousInstallHelpUrl",
      ASCIIToUTF16(google_util::AppendGoogleLocaleParam(
          GURL(chrome::kRemoveNonCWSExtensionURL)).spec()));
  source->AddString("extensionSettingsUseAppsDevTools",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_USE_APPS_DEV_TOOLS));
  source->AddString("extensionSettingsOpenAppsDevTools",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_OPEN_APPS_DEV_TOOLS));
  source->AddString("extensionSettingsShowButton",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_SHOW_BUTTON));
  source->AddString("extensionSettingsLoadUnpackedButton",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_LOAD_UNPACKED_BUTTON));
  source->AddString("extensionSettingsPackButton",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_PACK_BUTTON));
  source->AddString("extensionSettingsCommandsLink",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_COMMANDS_CONFIGURE));
  source->AddString("extensionSettingsUpdateButton",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_UPDATE_BUTTON));
  source->AddString("extensionSettingsCrashMessage",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_CRASHED_EXTENSION));
  source->AddString("extensionSettingsInDevelopment",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_IN_DEVELOPMENT));
  source->AddString("extensionSettingsWarningsTitle",
      l10n_util::GetStringUTF16(IDS_EXTENSION_WARNINGS_TITLE));
  source->AddString("extensionSettingsShowDetails",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_SHOW_DETAILS));
  source->AddString("extensionSettingsHideDetails",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_HIDE_DETAILS));

  // TODO(estade): comb through the above strings to find ones no longer used in
  // uber extensions.
  source->AddString("extensionUninstall",
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_UNINSTALL));
}

void ExtensionSettingsHandler::RenderViewDeleted(
    RenderViewHost* render_view_host) {
  deleting_rvh_ = render_view_host;
  Profile* source_profile = Profile::FromBrowserContext(
      render_view_host->GetSiteInstance()->GetBrowserContext());
  if (!Profile::FromWebUI(web_ui())->IsSameProfile(source_profile))
    return;
  MaybeUpdateAfterNotification();
}

void ExtensionSettingsHandler::DidStartNavigationToPendingEntry(
    const GURL& url,
    content::NavigationController::ReloadType reload_type) {
  if (reload_type != content::NavigationController::NO_RELOAD)
    ReloadUnpackedExtensions();
}

void ExtensionSettingsHandler::RegisterMessages() {
  // Don't override an |extension_service_| or |management_policy_| injected
  // for testing.
  if (!extension_service_) {
    extension_service_ = Profile::FromWebUI(web_ui())->GetOriginalProfile()->
        GetExtensionService();
  }
  if (!management_policy_) {
    management_policy_ = ExtensionSystem::Get(
        extension_service_->profile())->management_policy();
  }

  web_ui()->RegisterMessageCallback("extensionSettingsRequestExtensionsData",
      base::Bind(&ExtensionSettingsHandler::HandleRequestExtensionsData,
                 AsWeakPtr()));
  web_ui()->RegisterMessageCallback("extensionSettingsToggleDeveloperMode",
      base::Bind(&ExtensionSettingsHandler::HandleToggleDeveloperMode,
                 AsWeakPtr()));
  web_ui()->RegisterMessageCallback("extensionSettingsInspect",
      base::Bind(&ExtensionSettingsHandler::HandleInspectMessage,
                 AsWeakPtr()));
  web_ui()->RegisterMessageCallback("extensionSettingsLaunch",
      base::Bind(&ExtensionSettingsHandler::HandleLaunchMessage,
                 AsWeakPtr()));
  web_ui()->RegisterMessageCallback("extensionSettingsReload",
      base::Bind(&ExtensionSettingsHandler::HandleReloadMessage,
                 AsWeakPtr()));
  web_ui()->RegisterMessageCallback("extensionSettingsEnable",
      base::Bind(&ExtensionSettingsHandler::HandleEnableMessage,
                 AsWeakPtr()));
  web_ui()->RegisterMessageCallback("extensionSettingsEnableIncognito",
      base::Bind(&ExtensionSettingsHandler::HandleEnableIncognitoMessage,
                 AsWeakPtr()));
  web_ui()->RegisterMessageCallback("extensionSettingsAllowFileAccess",
      base::Bind(&ExtensionSettingsHandler::HandleAllowFileAccessMessage,
                 AsWeakPtr()));
  web_ui()->RegisterMessageCallback("extensionSettingsUninstall",
      base::Bind(&ExtensionSettingsHandler::HandleUninstallMessage,
                 AsWeakPtr()));
  web_ui()->RegisterMessageCallback("extensionSettingsOptions",
      base::Bind(&ExtensionSettingsHandler::HandleOptionsMessage,
                 AsWeakPtr()));
  web_ui()->RegisterMessageCallback("extensionSettingsPermissions",
      base::Bind(&ExtensionSettingsHandler::HandlePermissionsMessage,
                 AsWeakPtr()));
  web_ui()->RegisterMessageCallback("extensionSettingsShowButton",
      base::Bind(&ExtensionSettingsHandler::HandleShowButtonMessage,
                 AsWeakPtr()));
  web_ui()->RegisterMessageCallback("extensionSettingsAutoupdate",
      base::Bind(&ExtensionSettingsHandler::HandleAutoUpdateMessage,
                 AsWeakPtr()));
  web_ui()->RegisterMessageCallback("extensionSettingsLoadUnpackedExtension",
      base::Bind(&ExtensionSettingsHandler::HandleLoadUnpackedExtensionMessage,
                 AsWeakPtr()));
}

void ExtensionSettingsHandler::FileSelected(const base::FilePath& path,
                                            int index,
                                            void* params) {
  last_unpacked_directory_ = base::FilePath(path);
  UnpackedInstaller::Create(extension_service_)->Load(path);
}

void ExtensionSettingsHandler::MultiFilesSelected(
    const std::vector<base::FilePath>& files, void* params) {
  NOTREACHED();
}

void ExtensionSettingsHandler::FileSelectionCanceled(void* params) {
}

void ExtensionSettingsHandler::OnErrorAdded(const ExtensionError* error) {
  MaybeUpdateAfterNotification();
}

void ExtensionSettingsHandler::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  Profile* profile = Profile::FromWebUI(web_ui());
  Profile* source_profile = NULL;
  switch (type) {
    // We listen for notifications that will result in the page being
    // repopulated with data twice for the same event in certain cases.
    // For instance, EXTENSION_LOADED & EXTENSION_HOST_CREATED because
    // we don't know about the views for an extension at EXTENSION_LOADED, but
    // if we only listen to EXTENSION_HOST_CREATED, we'll miss extensions
    // that don't have a process at startup.
    //
    // Doing it this way gets everything but causes the page to be rendered
    // more than we need. It doesn't seem to result in any noticeable flicker.
    case chrome::NOTIFICATION_BACKGROUND_CONTENTS_DELETED:
      deleting_rvh_ = content::Details<BackgroundContents>(details)->
          web_contents()->GetRenderViewHost();
      // Fall through.
    case chrome::NOTIFICATION_BACKGROUND_CONTENTS_NAVIGATED:
    case chrome::NOTIFICATION_EXTENSION_HOST_CREATED:
      source_profile = content::Source<Profile>(source).ptr();
      if (!profile->IsSameProfile(source_profile))
          return;
      MaybeUpdateAfterNotification();
      break;
    case content::NOTIFICATION_RENDER_WIDGET_HOST_DESTROYED: {
      content::RenderWidgetHost* rwh =
          content::Source<content::RenderWidgetHost>(source).ptr();
      deleting_rwh_id_ = rwh->GetRoutingID();
      deleting_rph_id_ = rwh->GetProcess()->GetID();
      MaybeUpdateAfterNotification();
      break;
    }
    case chrome::NOTIFICATION_EXTENSION_LOADED:
    case chrome::NOTIFICATION_EXTENSION_UNLOADED:
    case chrome::NOTIFICATION_EXTENSION_UNINSTALLED:
    case chrome::NOTIFICATION_EXTENSION_UPDATE_DISABLED:
    case chrome::NOTIFICATION_EXTENSION_BROWSER_ACTION_VISIBILITY_CHANGED:
      MaybeUpdateAfterNotification();
      break;
    case chrome::NOTIFICATION_EXTENSION_HOST_DESTROYED:
       // This notification is sent when the extension host destruction begins,
       // not when it finishes. We use PostTask to delay the update until after
       // the destruction finishes.
       base::MessageLoop::current()->PostTask(
           FROM_HERE,
           base::Bind(&ExtensionSettingsHandler::MaybeUpdateAfterNotification,
                      AsWeakPtr()));
       break;
    default:
      NOTREACHED();
  }
}

void ExtensionSettingsHandler::ExtensionUninstallAccepted() {
  DCHECK(!extension_id_prompting_.empty());

  bool was_terminated = false;

  // The extension can be uninstalled in another window while the UI was
  // showing. Do nothing in that case.
  const Extension* extension =
      extension_service_->GetExtensionById(extension_id_prompting_, true);
  if (!extension) {
    extension = extension_service_->GetTerminatedExtension(
        extension_id_prompting_);
    was_terminated = true;
  }
  if (!extension)
    return;

  extension_service_->UninstallExtension(extension_id_prompting_,
                                         false,  // External uninstall.
                                         NULL);  // Error.
  extension_id_prompting_ = "";

  // There will be no EXTENSION_UNLOADED notification for terminated
  // extensions as they were already unloaded.
  if (was_terminated)
    HandleRequestExtensionsData(NULL);
}

void ExtensionSettingsHandler::ExtensionUninstallCanceled() {
  extension_id_prompting_ = "";
}

void ExtensionSettingsHandler::ExtensionWarningsChanged() {
  MaybeUpdateAfterNotification();
}

// This is called when the user clicks "Revoke File Access."
void ExtensionSettingsHandler::InstallUIProceed() {
  Profile* profile = Profile::FromWebUI(web_ui());
  apps::SavedFilesService::Get(profile)->ClearQueue(
      extension_service_->GetExtensionById(extension_id_prompting_, true));
  if (apps::AppRestoreService::Get(profile)->
          IsAppRestorable(extension_id_prompting_)) {
    apps::AppLoadService::Get(profile)->RestartApplication(
        extension_id_prompting_);
  }
  extension_id_prompting_.clear();
}

void ExtensionSettingsHandler::InstallUIAbort(bool user_initiated) {
  extension_id_prompting_.clear();
}

void ExtensionSettingsHandler::ReloadUnpackedExtensions() {
  const ExtensionSet* extensions = extension_service_->extensions();
  std::vector<const Extension*> unpacked_extensions;
  for (ExtensionSet::const_iterator extension = extensions->begin();
       extension != extensions->end(); ++extension) {
    if (Manifest::IsUnpackedLocation((*extension)->location()))
      unpacked_extensions.push_back(extension->get());
  }

  for (std::vector<const Extension*>::iterator iter =
       unpacked_extensions.begin(); iter != unpacked_extensions.end(); ++iter) {
    extension_service_->ReloadExtension((*iter)->id());
  }
}

void ExtensionSettingsHandler::HandleRequestExtensionsData(
    const base::ListValue* args) {
  base::DictionaryValue results;

  Profile* profile = Profile::FromWebUI(web_ui());

  // Add the extensions to the results structure.
  base::ListValue* extensions_list = new base::ListValue();

  ExtensionWarningService* warnings =
      ExtensionSystem::Get(profile)->warning_service();

  const ExtensionSet* extensions = extension_service_->extensions();
  for (ExtensionSet::const_iterator extension = extensions->begin();
       extension != extensions->end(); ++extension) {
    if ((*extension)->ShouldDisplayInExtensionSettings()) {
      extensions_list->Append(CreateExtensionDetailValue(
          extension->get(),
          GetInspectablePagesForExtension(extension->get(), true),
          warnings));
    }
  }
  extensions = extension_service_->disabled_extensions();
  for (ExtensionSet::const_iterator extension = extensions->begin();
       extension != extensions->end(); ++extension) {
    if ((*extension)->ShouldDisplayInExtensionSettings()) {
      extensions_list->Append(CreateExtensionDetailValue(
          extension->get(),
          GetInspectablePagesForExtension(extension->get(), false),
          warnings));
    }
  }
  extensions = extension_service_->terminated_extensions();
  std::vector<ExtensionPage> empty_pages;
  for (ExtensionSet::const_iterator extension = extensions->begin();
       extension != extensions->end(); ++extension) {
    if ((*extension)->ShouldDisplayInExtensionSettings()) {
      extensions_list->Append(CreateExtensionDetailValue(
          extension->get(),
          empty_pages,  // Terminated process has no active pages.
          warnings));
    }
  }
  results.Set("extensions", extensions_list);

  bool is_managed = profile->IsManaged();
  bool developer_mode =
      !is_managed &&
      profile->GetPrefs()->GetBoolean(prefs::kExtensionsUIDeveloperMode);
  results.SetBoolean("profileIsManaged", is_managed);
  results.SetBoolean("developerMode", developer_mode);
  results.SetBoolean(
      "appsDevToolsEnabled",
      CommandLine::ForCurrentProcess()->HasSwitch(switches::kAppsDevtool));

  bool load_unpacked_disabled =
      extension_service_->extension_prefs()->ExtensionsBlacklistedByDefault();
  results.SetBoolean("loadUnpackedDisabled", load_unpacked_disabled);

  web_ui()->CallJavascriptFunction(
      "extensions.ExtensionSettings.returnExtensionsData", results);

  MaybeRegisterForNotifications();
  if (should_do_verification_check_) {
    should_do_verification_check_ = false;
    extension_service_->VerifyAllExtensions();
  }
}

void ExtensionSettingsHandler::HandleToggleDeveloperMode(
    const base::ListValue* args) {
  Profile* profile = Profile::FromWebUI(web_ui());
  if (profile->IsManaged())
    return;

  bool developer_mode =
      !profile->GetPrefs()->GetBoolean(prefs::kExtensionsUIDeveloperMode);
  profile->GetPrefs()->SetBoolean(prefs::kExtensionsUIDeveloperMode,
                                  developer_mode);

  if (!CommandLine::ForCurrentProcess()->HasSwitch(switches::kAppsDevtool))
    return;

  base::FilePath apps_debugger_path(FILE_PATH_LITERAL("apps_debugger"));
  if (developer_mode) {
    profile->GetExtensionService()->component_loader()->Add(
        IDR_APPS_DEBUGGER_MANIFEST,
        apps_debugger_path);
  } else {
    std::string extension_id =
        profile->GetExtensionService()->component_loader()->GetExtensionID(
            IDR_APPS_DEBUGGER_MANIFEST,
            apps_debugger_path);
    scoped_refptr<const Extension> extension(
        profile->GetExtensionService()->GetInstalledExtension(extension_id));
    profile->GetExtensionService()->component_loader()->Remove(extension_id);
  }
}

void ExtensionSettingsHandler::HandleInspectMessage(
    const base::ListValue* args) {
  std::string extension_id;
  std::string render_process_id_str;
  std::string render_view_id_str;
  int render_process_id;
  int render_view_id;
  bool incognito;
  CHECK_EQ(4U, args->GetSize());
  CHECK(args->GetString(0, &extension_id));
  CHECK(args->GetString(1, &render_process_id_str));
  CHECK(args->GetString(2, &render_view_id_str));
  CHECK(args->GetBoolean(3, &incognito));
  CHECK(base::StringToInt(render_process_id_str, &render_process_id));
  CHECK(base::StringToInt(render_view_id_str, &render_view_id));

  if (render_process_id == -1) {
    // This message is for a lazy background page. Start the page if necessary.
    const Extension* extension =
        extension_service_->extensions()->GetByID(extension_id);
    DCHECK(extension);
    devtools_util::InspectBackgroundPage(extension,
                                         Profile::FromWebUI(web_ui()));
    return;
  }

  RenderViewHost* host = RenderViewHost::FromID(render_process_id,
                                                render_view_id);
  if (!host) {
    // This can happen if the host has gone away since the page was displayed.
    return;
  }

  DevToolsWindow::OpenDevToolsWindow(host);
}

void ExtensionSettingsHandler::HandleLaunchMessage(
    const base::ListValue* args) {
  CHECK_EQ(1U, args->GetSize());
  std::string extension_id;
  CHECK(args->GetString(0, &extension_id));
  const Extension* extension =
      extension_service_->GetExtensionById(extension_id, false);
  OpenApplication(AppLaunchParams(extension_service_->profile(), extension,
                                  extensions::LAUNCH_CONTAINER_WINDOW,
                                  NEW_WINDOW));
}

void ExtensionSettingsHandler::HandleReloadMessage(
    const base::ListValue* args) {
  std::string extension_id = UTF16ToUTF8(ExtractStringValue(args));
  CHECK(!extension_id.empty());
  extension_service_->ReloadExtension(extension_id);
}

void ExtensionSettingsHandler::HandleEnableMessage(
    const base::ListValue* args) {
  CHECK_EQ(2U, args->GetSize());
  std::string extension_id, enable_str;
  CHECK(args->GetString(0, &extension_id));
  CHECK(args->GetString(1, &enable_str));

  const Extension* extension =
      extension_service_->GetInstalledExtension(extension_id);
  if (!extension ||
      !management_policy_->UserMayModifySettings(extension, NULL)) {
    LOG(ERROR) << "Attempt to enable an extension that is non-usermanagable was"
               << "made. Extension id: " << extension->id();
    return;
  }

  if (enable_str == "true") {
    ExtensionPrefs* prefs = extension_service_->extension_prefs();
    if (prefs->DidExtensionEscalatePermissions(extension_id)) {
      ShowExtensionDisabledDialog(
          extension_service_, web_ui()->GetWebContents(), extension);
    } else if ((prefs->GetDisableReasons(extension_id) &
                   Extension::DISABLE_UNSUPPORTED_REQUIREMENT) &&
               !requirements_checker_.get()) {
      // Recheck the requirements.
      scoped_refptr<const Extension> extension =
          extension_service_->GetExtensionById(extension_id,
                                               true /* include disabled */);
      requirements_checker_.reset(new RequirementsChecker);
      requirements_checker_->Check(
          extension,
          base::Bind(&ExtensionSettingsHandler::OnRequirementsChecked,
                     AsWeakPtr(), extension_id));
    } else {
      extension_service_->EnableExtension(extension_id);
    }
  } else {
    extension_service_->DisableExtension(
        extension_id, Extension::DISABLE_USER_ACTION);
  }
}

void ExtensionSettingsHandler::HandleEnableIncognitoMessage(
    const base::ListValue* args) {
  CHECK_EQ(2U, args->GetSize());
  std::string extension_id, enable_str;
  CHECK(args->GetString(0, &extension_id));
  CHECK(args->GetString(1, &enable_str));
  const Extension* extension =
      extension_service_->GetInstalledExtension(extension_id);
  if (!extension)
    return;

  // Flipping the incognito bit will generate unload/load notifications for the
  // extension, but we don't want to reload the page, because a) we've already
  // updated the UI to reflect the change, and b) we want the yellow warning
  // text to stay until the user has left the page.
  //
  // TODO(aa): This creates crappiness in some cases. For example, in a main
  // window, when toggling this, the browser action will flicker because it gets
  // unloaded, then reloaded. It would be better to have a dedicated
  // notification for this case.
  //
  // Bug: http://crbug.com/41384
  base::AutoReset<bool> auto_reset_ignore_notifications(
      &ignore_notifications_, true);
  extension_util::SetIsIncognitoEnabled(extension->id(),
                                        extension_service_,
                                        enable_str == "true");
}

void ExtensionSettingsHandler::HandleAllowFileAccessMessage(
    const base::ListValue* args) {
  CHECK_EQ(2U, args->GetSize());
  std::string extension_id, allow_str;
  CHECK(args->GetString(0, &extension_id));
  CHECK(args->GetString(1, &allow_str));
  const Extension* extension =
      extension_service_->GetInstalledExtension(extension_id);
  if (!extension)
    return;

  if (!management_policy_->UserMayModifySettings(extension, NULL)) {
    LOG(ERROR) << "Attempt to change allow file access of an extension that is "
               << "non-usermanagable was made. Extension id : "
               << extension->id();
    return;
  }

  extension_util::SetAllowFileAccess(
      extension, extension_service_, allow_str == "true");
}

void ExtensionSettingsHandler::HandleUninstallMessage(
    const base::ListValue* args) {
  CHECK_EQ(1U, args->GetSize());
  std::string extension_id;
  CHECK(args->GetString(0, &extension_id));
  const Extension* extension =
      extension_service_->GetInstalledExtension(extension_id);
  if (!extension)
    return;

  if (!management_policy_->UserMayModifySettings(extension, NULL)) {
    LOG(ERROR) << "Attempt to uninstall an extension that is non-usermanagable "
               << "was made. Extension id : " << extension->id();
    return;
  }

  if (!extension_id_prompting_.empty())
    return;  // Only one prompt at a time.

  extension_id_prompting_ = extension_id;

  GetExtensionUninstallDialog()->ConfirmUninstall(extension);
}

void ExtensionSettingsHandler::HandleOptionsMessage(
    const base::ListValue* args) {
  const Extension* extension = GetActiveExtension(args);
  if (!extension || ManifestURL::GetOptionsPage(extension).is_empty())
    return;
  ExtensionTabUtil::OpenOptionsPage(extension,
      chrome::FindBrowserWithWebContents(web_ui()->GetWebContents()));
}

void ExtensionSettingsHandler::HandlePermissionsMessage(
    const base::ListValue* args) {
  std::string extension_id(UTF16ToUTF8(ExtractStringValue(args)));
  CHECK(!extension_id.empty());
  const Extension* extension =
      extension_service_->GetExtensionById(extension_id, true);
  if (!extension)
    extension = extension_service_->GetTerminatedExtension(extension_id);
  if (!extension)
    return;

  if (!extension_id_prompting_.empty())
    return;  // Only one prompt at a time.

  extension_id_prompting_ = extension->id();
  prompt_.reset(new ExtensionInstallPrompt(web_contents()));
  std::vector<base::FilePath> retained_file_paths;
  if (extension->HasAPIPermission(APIPermission::kFileSystem)) {
    std::vector<apps::SavedFileEntry> retained_file_entries =
        apps::SavedFilesService::Get(Profile::FromWebUI(
            web_ui()))->GetAllFileEntries(extension_id_prompting_);
    for (size_t i = 0; i < retained_file_entries.size(); ++i) {
      retained_file_paths.push_back(retained_file_entries[i].path);
    }
  }
  // The BrokerDelegate manages its own lifetime.
  prompt_->ReviewPermissions(
      new BrokerDelegate(AsWeakPtr()), extension, retained_file_paths);
}

void ExtensionSettingsHandler::HandleShowButtonMessage(
    const base::ListValue* args) {
  const Extension* extension = GetActiveExtension(args);
  if (!extension)
    return;
  ExtensionActionAPI::SetBrowserActionVisibility(
      extension_service_->extension_prefs(), extension->id(), true);
}

void ExtensionSettingsHandler::HandleAutoUpdateMessage(
    const base::ListValue* args) {
  ExtensionUpdater* updater = extension_service_->updater();
  if (updater) {
    ExtensionUpdater::CheckParams params;
    params.install_immediately = true;
    updater->CheckNow(params);
  }
}

void ExtensionSettingsHandler::HandleLoadUnpackedExtensionMessage(
    const base::ListValue* args) {
  DCHECK(args->empty());

  base::string16 select_title =
      l10n_util::GetStringUTF16(IDS_EXTENSION_LOAD_FROM_DIRECTORY);

  const int kFileTypeIndex = 0;  // No file type information to index.
  const ui::SelectFileDialog::Type kSelectType =
      ui::SelectFileDialog::SELECT_FOLDER;
  load_extension_dialog_ = ui::SelectFileDialog::Create(
      this, new ChromeSelectFilePolicy(web_ui()->GetWebContents()));
  load_extension_dialog_->SelectFile(
      kSelectType,
      select_title,
      last_unpacked_directory_,
      NULL,
      kFileTypeIndex,
      base::FilePath::StringType(),
      web_ui()->GetWebContents()->GetView()->GetTopLevelNativeWindow(),
      NULL);

  content::RecordComputedAction("Options_LoadUnpackedExtension");
}

void ExtensionSettingsHandler::ShowAlert(const std::string& message) {
  base::ListValue arguments;
  arguments.Append(base::Value::CreateStringValue(message));
  web_ui()->CallJavascriptFunction("alert", arguments);
}

const Extension* ExtensionSettingsHandler::GetActiveExtension(
    const base::ListValue* args) {
  std::string extension_id = UTF16ToUTF8(ExtractStringValue(args));
  CHECK(!extension_id.empty());
  return extension_service_->GetExtensionById(extension_id, false);
}

void ExtensionSettingsHandler::MaybeUpdateAfterNotification() {
  WebContents* contents = web_ui()->GetWebContents();
  if (!ignore_notifications_ && contents && contents->GetRenderViewHost())
    HandleRequestExtensionsData(NULL);
  deleting_rvh_ = NULL;
}

void ExtensionSettingsHandler::MaybeRegisterForNotifications() {
  if (registered_for_notifications_)
    return;

  registered_for_notifications_  = true;
  Profile* profile = Profile::FromWebUI(web_ui());

  // Register for notifications that we need to reload the page.
  registrar_.Add(this, chrome::NOTIFICATION_EXTENSION_LOADED,
                 content::Source<Profile>(profile));
  registrar_.Add(this, chrome::NOTIFICATION_EXTENSION_UNLOADED,
                 content::Source<Profile>(profile));
  registrar_.Add(this, chrome::NOTIFICATION_EXTENSION_UNINSTALLED,
                 content::Source<Profile>(profile));
  registrar_.Add(this, chrome::NOTIFICATION_EXTENSION_UPDATE_DISABLED,
                 content::Source<Profile>(profile));
  registrar_.Add(this, chrome::NOTIFICATION_EXTENSION_HOST_CREATED,
                 content::NotificationService::AllBrowserContextsAndSources());
  registrar_.Add(this,
                 chrome::NOTIFICATION_BACKGROUND_CONTENTS_NAVIGATED,
                 content::NotificationService::AllBrowserContextsAndSources());
  registrar_.Add(this,
                 chrome::NOTIFICATION_BACKGROUND_CONTENTS_DELETED,
                 content::NotificationService::AllBrowserContextsAndSources());
  registrar_.Add(
      this,
      chrome::NOTIFICATION_EXTENSION_BROWSER_ACTION_VISIBILITY_CHANGED,
      content::Source<ExtensionPrefs>(
          profile->GetExtensionService()->extension_prefs()));
  registrar_.Add(this,
                 chrome::NOTIFICATION_EXTENSION_HOST_DESTROYED,
                 content::NotificationService::AllBrowserContextsAndSources());

  registrar_.Add(this,
                 content::NOTIFICATION_RENDER_WIDGET_HOST_DESTROYED,
                 content::NotificationService::AllBrowserContextsAndSources());

  content::WebContentsObserver::Observe(web_ui()->GetWebContents());

  warning_service_observer_.Add(
      ExtensionSystem::Get(profile)->warning_service());

  error_console_observer_.Add(ErrorConsole::Get(profile));

  base::Closure callback = base::Bind(
      &ExtensionSettingsHandler::MaybeUpdateAfterNotification,
      AsWeakPtr());

  pref_registrar_.Init(profile->GetPrefs());
  pref_registrar_.Add(prefs::kExtensionInstallDenyList, callback);
}

std::vector<ExtensionPage>
ExtensionSettingsHandler::GetInspectablePagesForExtension(
    const Extension* extension, bool extension_is_enabled) {
  std::vector<ExtensionPage> result;

  // Get the extension process's active views.
  extensions::ProcessManager* process_manager =
      ExtensionSystem::Get(extension_service_->profile())->process_manager();
  GetInspectablePagesForExtensionProcess(
      extension,
      process_manager->GetRenderViewHostsForExtension(extension->id()),
      &result);

  // Get shell window views
  GetShellWindowPagesForExtensionProfile(extension,
      extension_service_->profile(), &result);

  // Include a link to start the lazy background page, if applicable.
  if (BackgroundInfo::HasLazyBackgroundPage(extension) &&
      extension_is_enabled &&
      !process_manager->GetBackgroundHostForExtension(extension->id())) {
    result.push_back(ExtensionPage(
        BackgroundInfo::GetBackgroundURL(extension),
        -1,
        -1,
        false,
        BackgroundInfo::HasGeneratedBackgroundPage(extension)));
  }

  // Repeat for the incognito process, if applicable. Don't try to get
  // shell windows for incognito processes.
  if (extension_service_->profile()->HasOffTheRecordProfile() &&
      IncognitoInfo::IsSplitMode(extension)) {
    extensions::ProcessManager* process_manager =
        ExtensionSystem::Get(extension_service_->profile()->
            GetOffTheRecordProfile())->process_manager();
    GetInspectablePagesForExtensionProcess(
        extension,
        process_manager->GetRenderViewHostsForExtension(extension->id()),
        &result);

    if (BackgroundInfo::HasLazyBackgroundPage(extension) &&
        extension_is_enabled &&
        !process_manager->GetBackgroundHostForExtension(extension->id())) {
      result.push_back(ExtensionPage(
          BackgroundInfo::GetBackgroundURL(extension),
          -1,
          -1,
          true,
          BackgroundInfo::HasGeneratedBackgroundPage(extension)));
    }
  }

  return result;
}

void ExtensionSettingsHandler::GetInspectablePagesForExtensionProcess(
    const Extension* extension,
    const std::set<RenderViewHost*>& views,
    std::vector<ExtensionPage>* result) {
  bool has_generated_background_page =
      BackgroundInfo::HasGeneratedBackgroundPage(extension);
  for (std::set<RenderViewHost*>::const_iterator iter = views.begin();
       iter != views.end(); ++iter) {
    RenderViewHost* host = *iter;
    WebContents* web_contents = WebContents::FromRenderViewHost(host);
    ViewType host_type = GetViewType(web_contents);
    if (host == deleting_rvh_ ||
        VIEW_TYPE_EXTENSION_POPUP == host_type ||
        VIEW_TYPE_EXTENSION_DIALOG == host_type)
      continue;

    GURL url = web_contents->GetURL();
    content::RenderProcessHost* process = host->GetProcess();
    bool is_background_page =
        (url == BackgroundInfo::GetBackgroundURL(extension));
    result->push_back(
        ExtensionPage(url,
                      process->GetID(),
                      host->GetRoutingID(),
                      process->GetBrowserContext()->IsOffTheRecord(),
                      is_background_page && has_generated_background_page));
  }
}

void ExtensionSettingsHandler::GetShellWindowPagesForExtensionProfile(
    const Extension* extension,
    Profile* profile,
    std::vector<ExtensionPage>* result) {
  apps::ShellWindowRegistry* registry = apps::ShellWindowRegistry::Get(profile);
  if (!registry) return;

  const apps::ShellWindowRegistry::ShellWindowList windows =
      registry->GetShellWindowsForApp(extension->id());

  bool has_generated_background_page =
      BackgroundInfo::HasGeneratedBackgroundPage(extension);
  for (apps::ShellWindowRegistry::const_iterator it = windows.begin();
       it != windows.end(); ++it) {
    WebContents* web_contents = (*it)->web_contents();
    RenderViewHost* host = web_contents->GetRenderViewHost();
    content::RenderProcessHost* process = host->GetProcess();

    bool is_background_page =
        (web_contents->GetURL() == BackgroundInfo::GetBackgroundURL(extension));
    result->push_back(
        ExtensionPage(web_contents->GetURL(),
                      process->GetID(),
                      host->GetRoutingID(),
                      process->GetBrowserContext()->IsOffTheRecord(),
                      is_background_page && has_generated_background_page));
  }
}

ExtensionUninstallDialog*
ExtensionSettingsHandler::GetExtensionUninstallDialog() {
#if !defined(OS_ANDROID)
  if (!extension_uninstall_dialog_.get()) {
    Browser* browser = chrome::FindBrowserWithWebContents(
        web_ui()->GetWebContents());
    extension_uninstall_dialog_.reset(
        ExtensionUninstallDialog::Create(extension_service_->profile(),
                                         browser, this));
  }
  return extension_uninstall_dialog_.get();
#else
  return NULL;
#endif  // !defined(OS_ANDROID)
}

void ExtensionSettingsHandler::OnRequirementsChecked(
    std::string extension_id,
    std::vector<std::string> requirement_errors) {
  if (requirement_errors.empty()) {
    extension_service_->EnableExtension(extension_id);
  } else {
    ExtensionErrorReporter::GetInstance()->ReportError(
        UTF8ToUTF16(JoinString(requirement_errors, ' ')),
        true /* be noisy */);
  }
  requirements_checker_.reset();
}

}  // namespace extensions
