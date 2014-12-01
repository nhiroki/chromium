// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/web_request/web_request_api_helpers.h"

#include <algorithm>
#include <cmath>

#include "base/bind.h"
#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/web_cache/browser/web_cache_manager.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "extensions/browser/api/web_request/web_request_api_constants.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extensions_browser_client.h"
#include "extensions/browser/runtime_data.h"
#include "extensions/browser/warning_set.h"
#include "extensions/common/extension_messages.h"
#include "net/base/net_log.h"
#include "net/cookies/cookie_util.h"
#include "net/cookies/parsed_cookie.h"
#include "net/http/http_util.h"
#include "net/url_request/url_request.h"
#include "url/url_constants.h"

// TODO(battre): move all static functions into an anonymous namespace at the
// top of this file.

using base::Time;
using content::ResourceType;
using net::cookie_util::ParsedRequestCookie;
using net::cookie_util::ParsedRequestCookies;

namespace keys = extension_web_request_api_constants;

namespace extension_web_request_api_helpers {

namespace {

static const char* kResourceTypeStrings[] = {
  "main_frame",
  "sub_frame",
  "stylesheet",
  "script",
  "image",
  "object",
  "xmlhttprequest",
  "other",
  "other",
};

const size_t kResourceTypeStringsLength = arraysize(kResourceTypeStrings);

static ResourceType kResourceTypeValues[] = {
  content::RESOURCE_TYPE_MAIN_FRAME,
  content::RESOURCE_TYPE_SUB_FRAME,
  content::RESOURCE_TYPE_STYLESHEET,
  content::RESOURCE_TYPE_SCRIPT,
  content::RESOURCE_TYPE_IMAGE,
  content::RESOURCE_TYPE_OBJECT,
  content::RESOURCE_TYPE_XHR,
  content::RESOURCE_TYPE_LAST_TYPE,  // represents "other"
  // TODO(jochen): We duplicate the last entry, so the array's size is not a
  // power of two. If it is, this triggers a bug in gcc 4.4 in Release builds
  // (http://gcc.gnu.org/bugzilla/show_bug.cgi?id=43949). Once we use a version
  // of gcc with this bug fixed, or the array is changed so this duplicate
  // entry is no longer required, this should be removed.
  content::RESOURCE_TYPE_LAST_TYPE,
};

const size_t kResourceTypeValuesLength = arraysize(kResourceTypeValues);

typedef std::vector<linked_ptr<net::ParsedCookie> > ParsedResponseCookies;

void ClearCacheOnNavigationOnUI() {
  web_cache::WebCacheManager::GetInstance()->ClearCacheOnNavigation();
}

bool ParseCookieLifetime(net::ParsedCookie* cookie,
                         int64* seconds_till_expiry) {
  // 'Max-Age' is processed first because according to:
  // http://tools.ietf.org/html/rfc6265#section-5.3 'Max-Age' attribute
  // overrides 'Expires' attribute.
  if (cookie->HasMaxAge() &&
      base::StringToInt64(cookie->MaxAge(), seconds_till_expiry)) {
    return true;
  }

  Time parsed_expiry_time;
  if (cookie->HasExpires())
    parsed_expiry_time = net::cookie_util::ParseCookieTime(cookie->Expires());

  if (!parsed_expiry_time.is_null()) {
    *seconds_till_expiry =
        ceil((parsed_expiry_time - Time::Now()).InSecondsF());
    return *seconds_till_expiry >= 0;
  }
  return false;
}

bool NullableEquals(const int* a, const int* b) {
  if ((a && !b) || (!a && b))
    return false;
  return (!a) || (*a == *b);
}

bool NullableEquals(const bool* a, const bool* b) {
  if ((a && !b) || (!a && b))
    return false;
  return (!a) || (*a == *b);
}

bool NullableEquals(const std::string* a, const std::string* b) {
  if ((a && !b) || (!a && b))
    return false;
  return (!a) || (*a == *b);
}

}  // namespace

RequestCookie::RequestCookie() {}
RequestCookie::~RequestCookie() {}

bool NullableEquals(const RequestCookie* a, const RequestCookie* b) {
  if ((a && !b) || (!a && b))
    return false;
  if (!a)
    return true;
  return NullableEquals(a->name.get(), b->name.get()) &&
         NullableEquals(a->value.get(), b->value.get());
}

ResponseCookie::ResponseCookie() {}
ResponseCookie::~ResponseCookie() {}

bool NullableEquals(const ResponseCookie* a, const ResponseCookie* b) {
  if ((a && !b) || (!a && b))
    return false;
  if (!a)
    return true;
  return NullableEquals(a->name.get(), b->name.get()) &&
         NullableEquals(a->value.get(), b->value.get()) &&
         NullableEquals(a->expires.get(), b->expires.get()) &&
         NullableEquals(a->max_age.get(), b->max_age.get()) &&
         NullableEquals(a->domain.get(), b->domain.get()) &&
         NullableEquals(a->path.get(), b->path.get()) &&
         NullableEquals(a->secure.get(), b->secure.get()) &&
         NullableEquals(a->http_only.get(), b->http_only.get());
}

FilterResponseCookie::FilterResponseCookie() {}
FilterResponseCookie::~FilterResponseCookie() {}

bool NullableEquals(const FilterResponseCookie* a,
                    const FilterResponseCookie* b) {
  if ((a && !b) || (!a && b))
    return false;
  if (!a)
    return true;
  return NullableEquals(a->age_lower_bound.get(), b->age_lower_bound.get()) &&
         NullableEquals(a->age_upper_bound.get(), b->age_upper_bound.get()) &&
         NullableEquals(a->session_cookie.get(), b->session_cookie.get());
}

RequestCookieModification::RequestCookieModification() {}
RequestCookieModification::~RequestCookieModification() {}

bool NullableEquals(const RequestCookieModification* a,
                    const RequestCookieModification* b) {
  if ((a && !b) || (!a && b))
    return false;
  if (!a)
    return true;
  return NullableEquals(a->filter.get(), b->filter.get()) &&
         NullableEquals(a->modification.get(), b->modification.get());
}

ResponseCookieModification::ResponseCookieModification() : type(ADD) {}
ResponseCookieModification::~ResponseCookieModification() {}

bool NullableEquals(const ResponseCookieModification* a,
                    const ResponseCookieModification* b) {
  if ((a && !b) || (!a && b))
    return false;
  if (!a)
    return true;
  return a->type == b->type &&
         NullableEquals(a->filter.get(), b->filter.get()) &&
         NullableEquals(a->modification.get(), b->modification.get());
}

EventResponseDelta::EventResponseDelta(
    const std::string& extension_id, const base::Time& extension_install_time)
    : extension_id(extension_id),
      extension_install_time(extension_install_time),
      cancel(false) {
}

EventResponseDelta::~EventResponseDelta() {
}


// Creates a NetLog callback the returns a Value with the ID of the extension
// that caused an event.  |delta| must remain valid for the lifetime of the
// callback.
net::NetLog::ParametersCallback CreateNetLogExtensionIdCallback(
    const EventResponseDelta* delta) {
  return net::NetLog::StringCallback("extension_id", &delta->extension_id);
}

// Creates NetLog parameters to indicate that an extension modified a request.
// Caller takes ownership of returned value.
base::Value* NetLogModificationCallback(
    const EventResponseDelta* delta,
    net::NetLog::LogLevel log_level) {
  base::DictionaryValue* dict = new base::DictionaryValue();
  dict->SetString("extension_id", delta->extension_id);

  base::ListValue* modified_headers = new base::ListValue();
  net::HttpRequestHeaders::Iterator modification(
      delta->modified_request_headers);
  while (modification.GetNext()) {
    std::string line = modification.name() + ": " + modification.value();
    modified_headers->Append(new base::StringValue(line));
  }
  dict->Set("modified_headers", modified_headers);

  base::ListValue* deleted_headers = new base::ListValue();
  for (std::vector<std::string>::const_iterator key =
           delta->deleted_request_headers.begin();
       key != delta->deleted_request_headers.end();
       ++key) {
    deleted_headers->Append(new base::StringValue(*key));
  }
  dict->Set("deleted_headers", deleted_headers);
  return dict;
}

bool InDecreasingExtensionInstallationTimeOrder(
    const linked_ptr<EventResponseDelta>& a,
    const linked_ptr<EventResponseDelta>& b) {
  return a->extension_install_time > b->extension_install_time;
}

base::ListValue* StringToCharList(const std::string& s) {
  base::ListValue* result = new base::ListValue;
  for (size_t i = 0, n = s.size(); i < n; ++i) {
    result->Append(
        new base::FundamentalValue(
            *reinterpret_cast<const unsigned char*>(&s[i])));
  }
  return result;
}

bool CharListToString(const base::ListValue* list, std::string* out) {
  if (!list)
    return false;
  const size_t list_length = list->GetSize();
  out->resize(list_length);
  int value = 0;
  for (size_t i = 0; i < list_length; ++i) {
    if (!list->GetInteger(i, &value) || value < 0 || value > 255)
      return false;
    unsigned char tmp = static_cast<unsigned char>(value);
    (*out)[i] = *reinterpret_cast<char*>(&tmp);
  }
  return true;
}

EventResponseDelta* CalculateOnBeforeRequestDelta(
    const std::string& extension_id,
    const base::Time& extension_install_time,
    bool cancel,
    const GURL& new_url) {
  EventResponseDelta* result =
      new EventResponseDelta(extension_id, extension_install_time);
  result->cancel = cancel;
  result->new_url = new_url;
  return result;
}

EventResponseDelta* CalculateOnBeforeSendHeadersDelta(
    const std::string& extension_id,
    const base::Time& extension_install_time,
    bool cancel,
    net::HttpRequestHeaders* old_headers,
    net::HttpRequestHeaders* new_headers) {
  EventResponseDelta* result =
      new EventResponseDelta(extension_id, extension_install_time);
  result->cancel = cancel;

  // The event listener might not have passed any new headers if he
  // just wanted to cancel the request.
  if (new_headers) {
    // Find deleted headers.
    {
      net::HttpRequestHeaders::Iterator i(*old_headers);
      while (i.GetNext()) {
        if (!new_headers->HasHeader(i.name())) {
          result->deleted_request_headers.push_back(i.name());
        }
      }
    }

    // Find modified headers.
    {
      net::HttpRequestHeaders::Iterator i(*new_headers);
      while (i.GetNext()) {
        std::string value;
        if (!old_headers->GetHeader(i.name(), &value) || i.value() != value) {
          result->modified_request_headers.SetHeader(i.name(), i.value());
        }
      }
    }
  }
  return result;
}

EventResponseDelta* CalculateOnHeadersReceivedDelta(
    const std::string& extension_id,
    const base::Time& extension_install_time,
    bool cancel,
    const GURL& new_url,
    const net::HttpResponseHeaders* old_response_headers,
    ResponseHeaders* new_response_headers) {
  EventResponseDelta* result =
      new EventResponseDelta(extension_id, extension_install_time);
  result->cancel = cancel;
  result->new_url = new_url;

  if (!new_response_headers)
    return result;

  // Find deleted headers (header keys are treated case insensitively).
  {
    void* iter = NULL;
    std::string name;
    std::string value;
    while (old_response_headers->EnumerateHeaderLines(&iter, &name, &value)) {
      std::string name_lowercase(name);
      base::StringToLowerASCII(&name_lowercase);

      bool header_found = false;
      for (ResponseHeaders::const_iterator i = new_response_headers->begin();
           i != new_response_headers->end(); ++i) {
        if (LowerCaseEqualsASCII(i->first, name_lowercase.c_str()) &&
            value == i->second) {
          header_found = true;
          break;
        }
      }
      if (!header_found)
        result->deleted_response_headers.push_back(ResponseHeader(name, value));
    }
  }

  // Find added headers (header keys are treated case insensitively).
  {
    for (ResponseHeaders::const_iterator i = new_response_headers->begin();
         i != new_response_headers->end(); ++i) {
      void* iter = NULL;
      std::string value;
      bool header_found = false;
      while (old_response_headers->EnumerateHeader(&iter, i->first, &value) &&
             !header_found) {
        header_found = (value == i->second);
      }
      if (!header_found)
        result->added_response_headers.push_back(*i);
    }
  }

  return result;
}

EventResponseDelta* CalculateOnAuthRequiredDelta(
    const std::string& extension_id,
    const base::Time& extension_install_time,
    bool cancel,
    scoped_ptr<net::AuthCredentials>* auth_credentials) {
  EventResponseDelta* result =
      new EventResponseDelta(extension_id, extension_install_time);
  result->cancel = cancel;
  result->auth_credentials.swap(*auth_credentials);
  return result;
}

void MergeCancelOfResponses(
    const EventResponseDeltas& deltas,
    bool* canceled,
    const net::BoundNetLog* net_log) {
  for (EventResponseDeltas::const_iterator i = deltas.begin();
       i != deltas.end(); ++i) {
    if ((*i)->cancel) {
      *canceled = true;
      net_log->AddEvent(
          net::NetLog::TYPE_CHROME_EXTENSION_ABORTED_REQUEST,
          CreateNetLogExtensionIdCallback(i->get()));
      break;
    }
  }
}

// Helper function for MergeRedirectUrlOfResponses() that allows ignoring
// all redirects but those to data:// urls and about:blank. This is important
// to treat these URLs as "cancel urls", i.e. URLs that extensions redirect
// to if they want to express that they want to cancel a request. This reduces
// the number of conflicts that we need to flag, as canceling is considered
// a higher precedence operation that redirects.
// Returns whether a redirect occurred.
static bool MergeRedirectUrlOfResponsesHelper(
    const EventResponseDeltas& deltas,
    GURL* new_url,
    extensions::WarningSet* conflicting_extensions,
    const net::BoundNetLog* net_log,
    bool consider_only_cancel_scheme_urls) {
  bool redirected = false;

  // Extension that determines the |new_url|.
  std::string winning_extension_id;
  EventResponseDeltas::const_iterator delta;
  for (delta = deltas.begin(); delta != deltas.end(); ++delta) {
    if ((*delta)->new_url.is_empty())
      continue;
    if (consider_only_cancel_scheme_urls &&
        !(*delta)->new_url.SchemeIs(url::kDataScheme) &&
        (*delta)->new_url.spec() != "about:blank") {
      continue;
    }

    if (!redirected || *new_url == (*delta)->new_url) {
      *new_url = (*delta)->new_url;
      winning_extension_id = (*delta)->extension_id;
      redirected = true;
      net_log->AddEvent(
          net::NetLog::TYPE_CHROME_EXTENSION_REDIRECTED_REQUEST,
          CreateNetLogExtensionIdCallback(delta->get()));
    } else {
      conflicting_extensions->insert(
          extensions::Warning::CreateRedirectConflictWarning(
              (*delta)->extension_id,
              winning_extension_id,
              (*delta)->new_url,
              *new_url));
      net_log->AddEvent(
          net::NetLog::TYPE_CHROME_EXTENSION_IGNORED_DUE_TO_CONFLICT,
          CreateNetLogExtensionIdCallback(delta->get()));
    }
  }
  return redirected;
}

void MergeRedirectUrlOfResponses(
    const EventResponseDeltas& deltas,
    GURL* new_url,
    extensions::WarningSet* conflicting_extensions,
    const net::BoundNetLog* net_log) {

  // First handle only redirects to data:// URLs and about:blank. These are a
  // special case as they represent a way of cancelling a request.
  if (MergeRedirectUrlOfResponsesHelper(
          deltas, new_url, conflicting_extensions, net_log, true)) {
    // If any extension cancelled a request by redirecting to a data:// URL or
    // about:blank, we don't consider the other redirects.
    return;
  }

  // Handle all other redirects.
  MergeRedirectUrlOfResponsesHelper(
      deltas, new_url, conflicting_extensions, net_log, false);
}

void MergeOnBeforeRequestResponses(
    const EventResponseDeltas& deltas,
    GURL* new_url,
    extensions::WarningSet* conflicting_extensions,
    const net::BoundNetLog* net_log) {
  MergeRedirectUrlOfResponses(deltas, new_url, conflicting_extensions, net_log);
}

static bool DoesRequestCookieMatchFilter(
    const ParsedRequestCookie& cookie,
    RequestCookie* filter) {
  if (!filter) return true;
  if (filter->name.get() && cookie.first != *filter->name) return false;
  if (filter->value.get() && cookie.second != *filter->value) return false;
  return true;
}

// Applies all CookieModificationType::ADD operations for request cookies of
// |deltas| to |cookies|. Returns whether any cookie was added.
static bool MergeAddRequestCookieModifications(
    const EventResponseDeltas& deltas,
    ParsedRequestCookies* cookies) {
  bool modified = false;
  // We assume here that the deltas are sorted in decreasing extension
  // precedence (i.e. decreasing extension installation time).
  EventResponseDeltas::const_reverse_iterator delta;
  for (delta = deltas.rbegin(); delta != deltas.rend(); ++delta) {
    const RequestCookieModifications& modifications =
        (*delta)->request_cookie_modifications;
    for (RequestCookieModifications::const_iterator mod = modifications.begin();
         mod != modifications.end(); ++mod) {
      if ((*mod)->type != ADD || !(*mod)->modification.get())
        continue;
      std::string* new_name = (*mod)->modification->name.get();
      std::string* new_value = (*mod)->modification->value.get();
      if (!new_name || !new_value)
        continue;

      bool cookie_with_same_name_found = false;
      for (ParsedRequestCookies::iterator cookie = cookies->begin();
           cookie != cookies->end() && !cookie_with_same_name_found; ++cookie) {
        if (cookie->first == *new_name) {
          if (cookie->second != *new_value) {
            cookie->second = *new_value;
            modified = true;
          }
          cookie_with_same_name_found = true;
        }
      }
      if (!cookie_with_same_name_found) {
        cookies->push_back(std::make_pair(base::StringPiece(*new_name),
                                          base::StringPiece(*new_value)));
        modified = true;
      }
    }
  }
  return modified;
}

// Applies all CookieModificationType::EDIT operations for request cookies of
// |deltas| to |cookies|. Returns whether any cookie was modified.
static bool MergeEditRequestCookieModifications(
    const EventResponseDeltas& deltas,
    ParsedRequestCookies* cookies) {
  bool modified = false;
  // We assume here that the deltas are sorted in decreasing extension
  // precedence (i.e. decreasing extension installation time).
  EventResponseDeltas::const_reverse_iterator delta;
  for (delta = deltas.rbegin(); delta != deltas.rend(); ++delta) {
    const RequestCookieModifications& modifications =
        (*delta)->request_cookie_modifications;
    for (RequestCookieModifications::const_iterator mod = modifications.begin();
         mod != modifications.end(); ++mod) {
      if ((*mod)->type != EDIT || !(*mod)->modification.get())
        continue;

      std::string* new_value = (*mod)->modification->value.get();
      RequestCookie* filter = (*mod)->filter.get();
      for (ParsedRequestCookies::iterator cookie = cookies->begin();
           cookie != cookies->end(); ++cookie) {
        if (!DoesRequestCookieMatchFilter(*cookie, filter))
          continue;
        // If the edit operation tries to modify the cookie name, we just ignore
        // this. We only modify the cookie value.
        if (new_value && cookie->second != *new_value) {
          cookie->second = *new_value;
          modified = true;
        }
      }
    }
  }
  return modified;
}

// Applies all CookieModificationType::REMOVE operations for request cookies of
// |deltas| to |cookies|. Returns whether any cookie was deleted.
static bool MergeRemoveRequestCookieModifications(
    const EventResponseDeltas& deltas,
    ParsedRequestCookies* cookies) {
  bool modified = false;
  // We assume here that the deltas are sorted in decreasing extension
  // precedence (i.e. decreasing extension installation time).
  EventResponseDeltas::const_reverse_iterator delta;
  for (delta = deltas.rbegin(); delta != deltas.rend(); ++delta) {
    const RequestCookieModifications& modifications =
        (*delta)->request_cookie_modifications;
    for (RequestCookieModifications::const_iterator mod = modifications.begin();
         mod != modifications.end(); ++mod) {
      if ((*mod)->type != REMOVE)
        continue;

      RequestCookie* filter = (*mod)->filter.get();
      ParsedRequestCookies::iterator i = cookies->begin();
      while (i != cookies->end()) {
        if (DoesRequestCookieMatchFilter(*i, filter)) {
          i = cookies->erase(i);
          modified = true;
        } else {
          ++i;
        }
      }
    }
  }
  return modified;
}

void MergeCookiesInOnBeforeSendHeadersResponses(
    const EventResponseDeltas& deltas,
    net::HttpRequestHeaders* request_headers,
    extensions::WarningSet* conflicting_extensions,
    const net::BoundNetLog* net_log) {
  // Skip all work if there are no registered cookie modifications.
  bool cookie_modifications_exist = false;
  EventResponseDeltas::const_iterator delta;
  for (delta = deltas.begin(); delta != deltas.end(); ++delta) {
    cookie_modifications_exist |=
        !(*delta)->request_cookie_modifications.empty();
  }
  if (!cookie_modifications_exist)
    return;

  // Parse old cookie line.
  std::string cookie_header;
  request_headers->GetHeader(net::HttpRequestHeaders::kCookie, &cookie_header);
  ParsedRequestCookies cookies;
  net::cookie_util::ParseRequestCookieLine(cookie_header, &cookies);

  // Modify cookies.
  bool modified = false;
  modified |= MergeAddRequestCookieModifications(deltas, &cookies);
  modified |= MergeEditRequestCookieModifications(deltas, &cookies);
  modified |= MergeRemoveRequestCookieModifications(deltas, &cookies);

  // Reassemble and store new cookie line.
  if (modified) {
    std::string new_cookie_header =
        net::cookie_util::SerializeRequestCookieLine(cookies);
    request_headers->SetHeader(net::HttpRequestHeaders::kCookie,
                               new_cookie_header);
  }
}

// Returns the extension ID of the first extension in |deltas| that sets the
// request header identified by |key| to |value|.
static std::string FindSetRequestHeader(
    const EventResponseDeltas& deltas,
    const std::string& key,
    const std::string& value) {
  EventResponseDeltas::const_iterator delta;
  for (delta = deltas.begin(); delta != deltas.end(); ++delta) {
    net::HttpRequestHeaders::Iterator modification(
        (*delta)->modified_request_headers);
    while (modification.GetNext()) {
      if (key == modification.name() && value == modification.value())
        return (*delta)->extension_id;
    }
  }
  return std::string();
}

// Returns the extension ID of the first extension in |deltas| that removes the
// request header identified by |key|.
static std::string FindRemoveRequestHeader(
    const EventResponseDeltas& deltas,
    const std::string& key) {
  EventResponseDeltas::const_iterator delta;
  for (delta = deltas.begin(); delta != deltas.end(); ++delta) {
    std::vector<std::string>::iterator i;
    for (i = (*delta)->deleted_request_headers.begin();
         i != (*delta)->deleted_request_headers.end();
         ++i) {
      if (*i == key)
        return (*delta)->extension_id;
    }
  }
  return std::string();
}

void MergeOnBeforeSendHeadersResponses(
    const EventResponseDeltas& deltas,
    net::HttpRequestHeaders* request_headers,
    extensions::WarningSet* conflicting_extensions,
    const net::BoundNetLog* net_log) {
  EventResponseDeltas::const_iterator delta;

  // Here we collect which headers we have removed or set to new values
  // so far due to extensions of higher precedence.
  std::set<std::string> removed_headers;
  std::set<std::string> set_headers;

  // We assume here that the deltas are sorted in decreasing extension
  // precedence (i.e. decreasing extension installation time).
  for (delta = deltas.begin(); delta != deltas.end(); ++delta) {
    if ((*delta)->modified_request_headers.IsEmpty() &&
        (*delta)->deleted_request_headers.empty()) {
      continue;
    }

    // Check whether any modification affects a request header that
    // has been modified differently before. As deltas is sorted by decreasing
    // extension installation order, this takes care of precedence.
    bool extension_conflicts = false;
    std::string winning_extension_id;
    std::string conflicting_header;
    {
      net::HttpRequestHeaders::Iterator modification(
          (*delta)->modified_request_headers);
      while (modification.GetNext() && !extension_conflicts) {
        // This modification sets |key| to |value|.
        const std::string& key = modification.name();
        const std::string& value = modification.value();

        // We must not delete anything that has been modified before.
        if (removed_headers.find(key) != removed_headers.end() &&
            !extension_conflicts) {
          winning_extension_id = FindRemoveRequestHeader(deltas, key);
          conflicting_header = key;
          extension_conflicts = true;
        }

        // We must not modify anything that has been set to a *different*
        // value before.
        if (set_headers.find(key) != set_headers.end() &&
            !extension_conflicts) {
          std::string current_value;
          if (!request_headers->GetHeader(key, &current_value) ||
              current_value != value) {
            winning_extension_id =
                FindSetRequestHeader(deltas, key, current_value);
            conflicting_header = key;
            extension_conflicts = true;
          }
        }
      }
    }

    // Check whether any deletion affects a request header that has been
    // modified before.
    {
      std::vector<std::string>::iterator key;
      for (key = (*delta)->deleted_request_headers.begin();
           key != (*delta)->deleted_request_headers.end() &&
               !extension_conflicts;
           ++key) {
        if (set_headers.find(*key) != set_headers.end()) {
          std::string current_value;
          request_headers->GetHeader(*key, &current_value);
          winning_extension_id =
              FindSetRequestHeader(deltas, *key, current_value);
          conflicting_header = *key;
          extension_conflicts = true;
        }
      }
    }

    // Now execute the modifications if there were no conflicts.
    if (!extension_conflicts) {
      // Copy all modifications into the original headers.
      request_headers->MergeFrom((*delta)->modified_request_headers);
      {
        // Record which keys were changed.
        net::HttpRequestHeaders::Iterator modification(
            (*delta)->modified_request_headers);
        while (modification.GetNext())
          set_headers.insert(modification.name());
      }

      // Perform all deletions and record which keys were deleted.
      {
        std::vector<std::string>::iterator key;
        for (key = (*delta)->deleted_request_headers.begin();
             key != (*delta)->deleted_request_headers.end();
             ++key) {
          request_headers->RemoveHeader(*key);
          removed_headers.insert(*key);
        }
      }
      net_log->AddEvent(
          net::NetLog::TYPE_CHROME_EXTENSION_MODIFIED_HEADERS,
          base::Bind(&NetLogModificationCallback, delta->get()));
    } else {
      conflicting_extensions->insert(
          extensions::Warning::CreateRequestHeaderConflictWarning(
              (*delta)->extension_id, winning_extension_id,
              conflicting_header));
      net_log->AddEvent(
          net::NetLog::TYPE_CHROME_EXTENSION_IGNORED_DUE_TO_CONFLICT,
          CreateNetLogExtensionIdCallback(delta->get()));
    }
  }

  MergeCookiesInOnBeforeSendHeadersResponses(deltas, request_headers,
      conflicting_extensions, net_log);
}

// Retrives all cookies from |override_response_headers|.
static ParsedResponseCookies GetResponseCookies(
    scoped_refptr<net::HttpResponseHeaders> override_response_headers) {
  ParsedResponseCookies result;

  void* iter = NULL;
  std::string value;
  while (override_response_headers->EnumerateHeader(&iter, "Set-Cookie",
                                                    &value)) {
    result.push_back(make_linked_ptr(new net::ParsedCookie(value)));
  }
  return result;
}

// Stores all |cookies| in |override_response_headers| deleting previously
// existing cookie definitions.
static void StoreResponseCookies(
    const ParsedResponseCookies& cookies,
    scoped_refptr<net::HttpResponseHeaders> override_response_headers) {
  override_response_headers->RemoveHeader("Set-Cookie");
  for (ParsedResponseCookies::const_iterator i = cookies.begin();
       i != cookies.end(); ++i) {
    override_response_headers->AddHeader("Set-Cookie: " + (*i)->ToCookieLine());
  }
}

// Modifies |cookie| according to |modification|. Each value that is set in
// |modification| is applied to |cookie|.
static bool ApplyResponseCookieModification(ResponseCookie* modification,
                                            net::ParsedCookie* cookie) {
  bool modified = false;
  if (modification->name.get())
    modified |= cookie->SetName(*modification->name);
  if (modification->value.get())
    modified |= cookie->SetValue(*modification->value);
  if (modification->expires.get())
    modified |= cookie->SetExpires(*modification->expires);
  if (modification->max_age.get())
    modified |= cookie->SetMaxAge(base::IntToString(*modification->max_age));
  if (modification->domain.get())
    modified |= cookie->SetDomain(*modification->domain);
  if (modification->path.get())
    modified |= cookie->SetPath(*modification->path);
  if (modification->secure.get())
    modified |= cookie->SetIsSecure(*modification->secure);
  if (modification->http_only.get())
    modified |= cookie->SetIsHttpOnly(*modification->http_only);
  return modified;
}

static bool DoesResponseCookieMatchFilter(net::ParsedCookie* cookie,
                                          FilterResponseCookie* filter) {
  if (!cookie->IsValid()) return false;
  if (!filter) return true;
  if (filter->name && cookie->Name() != *filter->name)
    return false;
  if (filter->value && cookie->Value() != *filter->value)
    return false;
  if (filter->expires) {
    std::string actual_value =
        cookie->HasExpires() ? cookie->Expires() : std::string();
    if (actual_value != *filter->expires)
      return false;
  }
  if (filter->max_age) {
    std::string actual_value =
        cookie->HasMaxAge() ? cookie->MaxAge() : std::string();
    if (actual_value != base::IntToString(*filter->max_age))
      return false;
  }
  if (filter->domain) {
    std::string actual_value =
        cookie->HasDomain() ? cookie->Domain() : std::string();
    if (actual_value != *filter->domain)
      return false;
  }
  if (filter->path) {
    std::string actual_value =
        cookie->HasPath() ? cookie->Path() : std::string();
    if (actual_value != *filter->path)
      return false;
  }
  if (filter->secure && cookie->IsSecure() != *filter->secure)
    return false;
  if (filter->http_only && cookie->IsHttpOnly() != *filter->http_only)
    return false;
  if (filter->age_upper_bound || filter->age_lower_bound ||
      (filter->session_cookie && *filter->session_cookie)) {
    int64 seconds_to_expiry;
    bool lifetime_parsed = ParseCookieLifetime(cookie, &seconds_to_expiry);
    if (filter->age_upper_bound && seconds_to_expiry > *filter->age_upper_bound)
      return false;
    if (filter->age_lower_bound && seconds_to_expiry < *filter->age_lower_bound)
      return false;
    if (filter->session_cookie && *filter->session_cookie && lifetime_parsed)
      return false;
  }
  return true;
}

// Applies all CookieModificationType::ADD operations for response cookies of
// |deltas| to |cookies|. Returns whether any cookie was added.
static bool MergeAddResponseCookieModifications(
    const EventResponseDeltas& deltas,
    ParsedResponseCookies* cookies) {
  bool modified = false;
  // We assume here that the deltas are sorted in decreasing extension
  // precedence (i.e. decreasing extension installation time).
  EventResponseDeltas::const_reverse_iterator delta;
  for (delta = deltas.rbegin(); delta != deltas.rend(); ++delta) {
    const ResponseCookieModifications& modifications =
        (*delta)->response_cookie_modifications;
    for (ResponseCookieModifications::const_iterator mod =
             modifications.begin(); mod != modifications.end(); ++mod) {
      if ((*mod)->type != ADD || !(*mod)->modification.get())
        continue;
      // Cookie names are not unique in response cookies so we always append
      // and never override.
      linked_ptr<net::ParsedCookie> cookie(
          new net::ParsedCookie(std::string()));
      ApplyResponseCookieModification((*mod)->modification.get(), cookie.get());
      cookies->push_back(cookie);
      modified = true;
    }
  }
  return modified;
}

// Applies all CookieModificationType::EDIT operations for response cookies of
// |deltas| to |cookies|. Returns whether any cookie was modified.
static bool MergeEditResponseCookieModifications(
    const EventResponseDeltas& deltas,
    ParsedResponseCookies* cookies) {
  bool modified = false;
  // We assume here that the deltas are sorted in decreasing extension
  // precedence (i.e. decreasing extension installation time).
  EventResponseDeltas::const_reverse_iterator delta;
  for (delta = deltas.rbegin(); delta != deltas.rend(); ++delta) {
    const ResponseCookieModifications& modifications =
        (*delta)->response_cookie_modifications;
    for (ResponseCookieModifications::const_iterator mod =
             modifications.begin(); mod != modifications.end(); ++mod) {
      if ((*mod)->type != EDIT || !(*mod)->modification.get())
        continue;

      for (ParsedResponseCookies::iterator cookie = cookies->begin();
           cookie != cookies->end(); ++cookie) {
        if (DoesResponseCookieMatchFilter(cookie->get(),
                                          (*mod)->filter.get())) {
          modified |= ApplyResponseCookieModification(
              (*mod)->modification.get(), cookie->get());
        }
      }
    }
  }
  return modified;
}

// Applies all CookieModificationType::REMOVE operations for response cookies of
// |deltas| to |cookies|. Returns whether any cookie was deleted.
static bool MergeRemoveResponseCookieModifications(
    const EventResponseDeltas& deltas,
    ParsedResponseCookies* cookies) {
  bool modified = false;
  // We assume here that the deltas are sorted in decreasing extension
  // precedence (i.e. decreasing extension installation time).
  EventResponseDeltas::const_reverse_iterator delta;
  for (delta = deltas.rbegin(); delta != deltas.rend(); ++delta) {
    const ResponseCookieModifications& modifications =
        (*delta)->response_cookie_modifications;
    for (ResponseCookieModifications::const_iterator mod =
             modifications.begin(); mod != modifications.end(); ++mod) {
      if ((*mod)->type != REMOVE)
        continue;

      ParsedResponseCookies::iterator i = cookies->begin();
      while (i != cookies->end()) {
        if (DoesResponseCookieMatchFilter(i->get(),
                                          (*mod)->filter.get())) {
          i = cookies->erase(i);
          modified = true;
        } else {
          ++i;
        }
      }
    }
  }
  return modified;
}

void MergeCookiesInOnHeadersReceivedResponses(
    const EventResponseDeltas& deltas,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    extensions::WarningSet* conflicting_extensions,
    const net::BoundNetLog* net_log) {
  // Skip all work if there are no registered cookie modifications.
  bool cookie_modifications_exist = false;
  EventResponseDeltas::const_reverse_iterator delta;
  for (delta = deltas.rbegin(); delta != deltas.rend(); ++delta) {
    cookie_modifications_exist |=
        !(*delta)->response_cookie_modifications.empty();
  }
  if (!cookie_modifications_exist)
    return;

  // Only create a copy if we really want to modify the response headers.
  if (override_response_headers->get() == NULL) {
    *override_response_headers = new net::HttpResponseHeaders(
        original_response_headers->raw_headers());
  }

  ParsedResponseCookies cookies =
      GetResponseCookies(*override_response_headers);

  bool modified = false;
  modified |= MergeAddResponseCookieModifications(deltas, &cookies);
  modified |= MergeEditResponseCookieModifications(deltas, &cookies);
  modified |= MergeRemoveResponseCookieModifications(deltas, &cookies);

  // Store new value.
  if (modified)
    StoreResponseCookies(cookies, *override_response_headers);
}

// Converts the key of the (key, value) pair to lower case.
static ResponseHeader ToLowerCase(const ResponseHeader& header) {
  std::string lower_key(header.first);
  base::StringToLowerASCII(&lower_key);
  return ResponseHeader(lower_key, header.second);
}

// Returns the extension ID of the first extension in |deltas| that removes the
// request header identified by |key|.
static std::string FindRemoveResponseHeader(
    const EventResponseDeltas& deltas,
    const std::string& key) {
  std::string lower_key = base::StringToLowerASCII(key);
  EventResponseDeltas::const_iterator delta;
  for (delta = deltas.begin(); delta != deltas.end(); ++delta) {
    ResponseHeaders::const_iterator i;
    for (i = (*delta)->deleted_response_headers.begin();
         i != (*delta)->deleted_response_headers.end(); ++i) {
      if (base::StringToLowerASCII(i->first) == lower_key)
        return (*delta)->extension_id;
    }
  }
  return std::string();
}

void MergeOnHeadersReceivedResponses(
    const EventResponseDeltas& deltas,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url,
    extensions::WarningSet* conflicting_extensions,
    const net::BoundNetLog* net_log) {
  EventResponseDeltas::const_iterator delta;

  // Here we collect which headers we have removed or added so far due to
  // extensions of higher precedence. Header keys are always stored as
  // lower case.
  std::set<ResponseHeader> removed_headers;
  std::set<ResponseHeader> added_headers;

  // We assume here that the deltas are sorted in decreasing extension
  // precedence (i.e. decreasing extension installation time).
  for (delta = deltas.begin(); delta != deltas.end(); ++delta) {
    if ((*delta)->added_response_headers.empty() &&
        (*delta)->deleted_response_headers.empty()) {
      continue;
    }

    // Only create a copy if we really want to modify the response headers.
    if (override_response_headers->get() == NULL) {
      *override_response_headers = new net::HttpResponseHeaders(
          original_response_headers->raw_headers());
    }

    // We consider modifications as pairs of (delete, add) operations.
    // If a header is deleted twice by different extensions we assume that the
    // intention was to modify it to different values and consider this a
    // conflict. As deltas is sorted by decreasing extension installation order,
    // this takes care of precedence.
    bool extension_conflicts = false;
    std::string conflicting_header;
    std::string winning_extension_id;
    ResponseHeaders::const_iterator i;
    for (i = (*delta)->deleted_response_headers.begin();
         i != (*delta)->deleted_response_headers.end(); ++i) {
      if (removed_headers.find(ToLowerCase(*i)) != removed_headers.end()) {
        winning_extension_id = FindRemoveResponseHeader(deltas, i->first);
        conflicting_header = i->first;
        extension_conflicts = true;
        break;
      }
    }

    // Now execute the modifications if there were no conflicts.
    if (!extension_conflicts) {
      // Delete headers
      {
        for (i = (*delta)->deleted_response_headers.begin();
             i != (*delta)->deleted_response_headers.end(); ++i) {
          (*override_response_headers)->RemoveHeaderLine(i->first, i->second);
          removed_headers.insert(ToLowerCase(*i));
        }
      }

      // Add headers.
      {
        for (i = (*delta)->added_response_headers.begin();
             i != (*delta)->added_response_headers.end(); ++i) {
          ResponseHeader lowercase_header(ToLowerCase(*i));
          if (added_headers.find(lowercase_header) != added_headers.end())
            continue;
          added_headers.insert(lowercase_header);
          (*override_response_headers)->AddHeader(i->first + ": " + i->second);
        }
      }
      net_log->AddEvent(
          net::NetLog::TYPE_CHROME_EXTENSION_MODIFIED_HEADERS,
          CreateNetLogExtensionIdCallback(delta->get()));
    } else {
      conflicting_extensions->insert(
          extensions::Warning::CreateResponseHeaderConflictWarning(
              (*delta)->extension_id, winning_extension_id,
              conflicting_header));
      net_log->AddEvent(
          net::NetLog::TYPE_CHROME_EXTENSION_IGNORED_DUE_TO_CONFLICT,
          CreateNetLogExtensionIdCallback(delta->get()));
    }
  }

  MergeCookiesInOnHeadersReceivedResponses(deltas, original_response_headers,
      override_response_headers, conflicting_extensions, net_log);

  GURL new_url;
  MergeRedirectUrlOfResponses(
      deltas, &new_url, conflicting_extensions, net_log);
  if (new_url.is_valid()) {
    // Only create a copy if we really want to modify the response headers.
    if (override_response_headers->get() == NULL) {
      *override_response_headers = new net::HttpResponseHeaders(
          original_response_headers->raw_headers());
    }
    (*override_response_headers)->ReplaceStatusLine("HTTP/1.1 302 Found");
    (*override_response_headers)->RemoveHeader("location");
    (*override_response_headers)->AddHeader("Location: " + new_url.spec());
    // Explicitly mark the URL as safe for redirection, to prevent the request
    // from being blocked because of net::ERR_UNSAFE_REDIRECT.
    *allowed_unsafe_redirect_url = new_url;
  }
}

bool MergeOnAuthRequiredResponses(
    const EventResponseDeltas& deltas,
    net::AuthCredentials* auth_credentials,
    extensions::WarningSet* conflicting_extensions,
    const net::BoundNetLog* net_log) {
  CHECK(auth_credentials);
  bool credentials_set = false;
  std::string winning_extension_id;

  for (EventResponseDeltas::const_iterator delta = deltas.begin();
       delta != deltas.end();
       ++delta) {
    if (!(*delta)->auth_credentials.get())
      continue;
    bool different =
        auth_credentials->username() !=
            (*delta)->auth_credentials->username() ||
        auth_credentials->password() != (*delta)->auth_credentials->password();
    if (credentials_set && different) {
      conflicting_extensions->insert(
          extensions::Warning::CreateCredentialsConflictWarning(
              (*delta)->extension_id, winning_extension_id));
      net_log->AddEvent(
          net::NetLog::TYPE_CHROME_EXTENSION_IGNORED_DUE_TO_CONFLICT,
          CreateNetLogExtensionIdCallback(delta->get()));
    } else {
      net_log->AddEvent(
          net::NetLog::TYPE_CHROME_EXTENSION_PROVIDE_AUTH_CREDENTIALS,
          CreateNetLogExtensionIdCallback(delta->get()));
      *auth_credentials = *(*delta)->auth_credentials;
      credentials_set = true;
      winning_extension_id = (*delta)->extension_id;
    }
  }
  return credentials_set;
}

void ClearCacheOnNavigation() {
  if (content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
    ClearCacheOnNavigationOnUI();
  } else {
    content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
                                     base::Bind(&ClearCacheOnNavigationOnUI));
  }
}

void NotifyWebRequestAPIUsed(void* browser_context_id,
                             const std::string& extension_id) {
  DCHECK(!extension_id.empty());
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  content::BrowserContext* browser_context =
      reinterpret_cast<content::BrowserContext*>(browser_context_id);
  if (!extensions::ExtensionsBrowserClient::Get()->IsValidContext(
      browser_context))
    return;

  extensions::RuntimeData* runtime_data =
      extensions::ExtensionSystem::Get(browser_context)->runtime_data();
  if (runtime_data->HasUsedWebRequest(extension_id))
    return;
  runtime_data->SetHasUsedWebRequest(extension_id, true);

  for (content::RenderProcessHost::iterator it =
           content::RenderProcessHost::AllHostsIterator();
       !it.IsAtEnd(); it.Advance()) {
    content::RenderProcessHost* host = it.GetCurrentValue();
    if (host->GetBrowserContext() == browser_context)
      SendExtensionWebRequestStatusToHost(host);
  }
}

void SendExtensionWebRequestStatusToHost(content::RenderProcessHost* host) {
  content::BrowserContext* browser_context = host->GetBrowserContext();
  if (!browser_context)
    return;

  bool webrequest_used = false;
  const extensions::ExtensionSet& extensions =
      extensions::ExtensionRegistry::Get(browser_context)->enabled_extensions();
  extensions::RuntimeData* runtime_data =
      extensions::ExtensionSystem::Get(browser_context)->runtime_data();
  for (extensions::ExtensionSet::const_iterator it = extensions.begin();
       !webrequest_used && it != extensions.end();
       ++it) {
    webrequest_used |= runtime_data->HasUsedWebRequest((*it)->id());
  }

  host->Send(new ExtensionMsg_UsingWebRequestAPI(webrequest_used));
}

// Converts the |name|, |value| pair of a http header to a HttpHeaders
// dictionary. Ownership is passed to the caller.
base::DictionaryValue* CreateHeaderDictionary(
    const std::string& name, const std::string& value) {
  base::DictionaryValue* header = new base::DictionaryValue();
  header->SetString(keys::kHeaderNameKey, name);
  if (base::IsStringUTF8(value)) {
    header->SetString(keys::kHeaderValueKey, value);
  } else {
    header->Set(keys::kHeaderBinaryValueKey,
                StringToCharList(value));
  }
  return header;
}

#define ARRAYEND(array) (array + arraysize(array))

bool IsRelevantResourceType(ResourceType type) {
  ResourceType* iter =
      std::find(kResourceTypeValues,
                kResourceTypeValues + kResourceTypeValuesLength,
                type);
  return iter != (kResourceTypeValues + kResourceTypeValuesLength);
}

const char* ResourceTypeToString(ResourceType type) {
  ResourceType* iter =
      std::find(kResourceTypeValues,
                kResourceTypeValues + kResourceTypeValuesLength,
                type);
  if (iter == (kResourceTypeValues + kResourceTypeValuesLength))
    return "other";

  return kResourceTypeStrings[iter - kResourceTypeValues];
}

bool ParseResourceType(const std::string& type_str,
                       ResourceType* type) {
  const char** iter =
      std::find(kResourceTypeStrings,
                kResourceTypeStrings + kResourceTypeStringsLength,
                type_str);
  if (iter == (kResourceTypeStrings + kResourceTypeStringsLength))
    return false;
  *type = kResourceTypeValues[iter - kResourceTypeStrings];
  return true;
}

}  // namespace extension_web_request_api_helpers
