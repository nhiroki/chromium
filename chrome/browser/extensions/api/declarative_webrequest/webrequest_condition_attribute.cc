// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/declarative_webrequest/webrequest_condition_attribute.h"

#include <algorithm>

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "chrome/browser/extensions/api/declarative_webrequest/webrequest_condition.h"
#include "chrome/browser/extensions/api/web_request/web_request_api_helpers.h"
#include "content/public/browser/resource_request_info.h"
#include "extensions/browser/api/declarative/deduping_factory.h"
#include "extensions/browser/api/declarative_webrequest/request_stage.h"
#include "extensions/browser/api/declarative_webrequest/webrequest_constants.h"
#include "extensions/common/error_utils.h"
#include "net/base/net_errors.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/base/static_cookie_policy.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_util.h"
#include "net/url_request/url_request.h"

using base::CaseInsensitiveCompareASCII;
using base::DictionaryValue;
using base::ListValue;
using base::StringValue;
using base::Value;
using content::ResourceType;

namespace helpers = extension_web_request_api_helpers;
namespace keys = extensions::declarative_webrequest_constants;

namespace extensions {

namespace {
// Error messages.
const char kInvalidValue[] = "Condition '*' has an invalid value";

struct WebRequestConditionAttributeFactory {
  DedupingFactory<WebRequestConditionAttribute> factory;

  WebRequestConditionAttributeFactory() : factory(5) {
    factory.RegisterFactoryMethod(
        keys::kResourceTypeKey,
        DedupingFactory<WebRequestConditionAttribute>::IS_PARAMETERIZED,
        &WebRequestConditionAttributeResourceType::Create);

    factory.RegisterFactoryMethod(
        keys::kContentTypeKey,
        DedupingFactory<WebRequestConditionAttribute>::IS_PARAMETERIZED,
        &WebRequestConditionAttributeContentType::Create);
    factory.RegisterFactoryMethod(
        keys::kExcludeContentTypeKey,
        DedupingFactory<WebRequestConditionAttribute>::IS_PARAMETERIZED,
        &WebRequestConditionAttributeContentType::Create);

    factory.RegisterFactoryMethod(
        keys::kRequestHeadersKey,
        DedupingFactory<WebRequestConditionAttribute>::IS_PARAMETERIZED,
        &WebRequestConditionAttributeRequestHeaders::Create);
    factory.RegisterFactoryMethod(
        keys::kExcludeRequestHeadersKey,
        DedupingFactory<WebRequestConditionAttribute>::IS_PARAMETERIZED,
        &WebRequestConditionAttributeRequestHeaders::Create);

    factory.RegisterFactoryMethod(
        keys::kResponseHeadersKey,
        DedupingFactory<WebRequestConditionAttribute>::IS_PARAMETERIZED,
        &WebRequestConditionAttributeResponseHeaders::Create);
    factory.RegisterFactoryMethod(
        keys::kExcludeResponseHeadersKey,
        DedupingFactory<WebRequestConditionAttribute>::IS_PARAMETERIZED,
        &WebRequestConditionAttributeResponseHeaders::Create);

    factory.RegisterFactoryMethod(
        keys::kThirdPartyKey,
        DedupingFactory<WebRequestConditionAttribute>::IS_PARAMETERIZED,
        &WebRequestConditionAttributeThirdParty::Create);

    factory.RegisterFactoryMethod(
        keys::kStagesKey,
        DedupingFactory<WebRequestConditionAttribute>::IS_PARAMETERIZED,
        &WebRequestConditionAttributeStages::Create);
  }
};

base::LazyInstance<WebRequestConditionAttributeFactory>::Leaky
    g_web_request_condition_attribute_factory = LAZY_INSTANCE_INITIALIZER;

}  // namespace

//
// WebRequestConditionAttribute
//

WebRequestConditionAttribute::WebRequestConditionAttribute() {}

WebRequestConditionAttribute::~WebRequestConditionAttribute() {}

bool WebRequestConditionAttribute::Equals(
    const WebRequestConditionAttribute* other) const {
  return GetType() == other->GetType();
}

// static
scoped_refptr<const WebRequestConditionAttribute>
WebRequestConditionAttribute::Create(
    const std::string& name,
    const base::Value* value,
    std::string* error) {
  CHECK(value != NULL && error != NULL);
  bool bad_message = false;
  return g_web_request_condition_attribute_factory.Get().factory.Instantiate(
      name, value, error, &bad_message);
}

//
// WebRequestConditionAttributeResourceType
//

WebRequestConditionAttributeResourceType::
WebRequestConditionAttributeResourceType(
    const std::vector<ResourceType>& types)
    : types_(types) {}

WebRequestConditionAttributeResourceType::
~WebRequestConditionAttributeResourceType() {}

// static
scoped_refptr<const WebRequestConditionAttribute>
WebRequestConditionAttributeResourceType::Create(
    const std::string& instance_type,
    const base::Value* value,
    std::string* error,
    bool* bad_message) {
  DCHECK(instance_type == keys::kResourceTypeKey);
  const base::ListValue* value_as_list = NULL;
  if (!value->GetAsList(&value_as_list)) {
    *error = ErrorUtils::FormatErrorMessage(kInvalidValue,
                                            keys::kResourceTypeKey);
    return scoped_refptr<const WebRequestConditionAttribute>(NULL);
  }

  size_t number_types = value_as_list->GetSize();

  std::vector<ResourceType> passed_types;
  passed_types.reserve(number_types);
  for (size_t i = 0; i < number_types; ++i) {
    std::string resource_type_string;
    ResourceType type = content::RESOURCE_TYPE_LAST_TYPE;
    if (!value_as_list->GetString(i, &resource_type_string) ||
        !helpers::ParseResourceType(resource_type_string, &type)) {
      *error = ErrorUtils::FormatErrorMessage(kInvalidValue,
                                              keys::kResourceTypeKey);
      return scoped_refptr<const WebRequestConditionAttribute>(NULL);
    }
    passed_types.push_back(type);
  }

  return scoped_refptr<const WebRequestConditionAttribute>(
      new WebRequestConditionAttributeResourceType(passed_types));
}

int WebRequestConditionAttributeResourceType::GetStages() const {
  return ON_BEFORE_REQUEST | ON_BEFORE_SEND_HEADERS | ON_SEND_HEADERS |
      ON_HEADERS_RECEIVED | ON_AUTH_REQUIRED | ON_BEFORE_REDIRECT |
      ON_RESPONSE_STARTED | ON_COMPLETED | ON_ERROR;
}

bool WebRequestConditionAttributeResourceType::IsFulfilled(
    const WebRequestData& request_data) const {
  if (!(request_data.stage & GetStages()))
    return false;
  const content::ResourceRequestInfo* info =
      content::ResourceRequestInfo::ForRequest(request_data.request);
  if (!info)
    return false;
  return std::find(types_.begin(), types_.end(), info->GetResourceType()) !=
      types_.end();
}

WebRequestConditionAttribute::Type
WebRequestConditionAttributeResourceType::GetType() const {
  return CONDITION_RESOURCE_TYPE;
}

std::string WebRequestConditionAttributeResourceType::GetName() const {
  return keys::kResourceTypeKey;
}

bool WebRequestConditionAttributeResourceType::Equals(
    const WebRequestConditionAttribute* other) const {
  if (!WebRequestConditionAttribute::Equals(other))
    return false;
  const WebRequestConditionAttributeResourceType* casted_other =
      static_cast<const WebRequestConditionAttributeResourceType*>(other);
  return types_ == casted_other->types_;
}

//
// WebRequestConditionAttributeContentType
//

WebRequestConditionAttributeContentType::
WebRequestConditionAttributeContentType(
    const std::vector<std::string>& content_types,
    bool inclusive)
    : content_types_(content_types),
      inclusive_(inclusive) {}

WebRequestConditionAttributeContentType::
~WebRequestConditionAttributeContentType() {}

// static
scoped_refptr<const WebRequestConditionAttribute>
WebRequestConditionAttributeContentType::Create(
      const std::string& name,
      const base::Value* value,
      std::string* error,
      bool* bad_message) {
  DCHECK(name == keys::kContentTypeKey || name == keys::kExcludeContentTypeKey);

  const base::ListValue* value_as_list = NULL;
  if (!value->GetAsList(&value_as_list)) {
    *error = ErrorUtils::FormatErrorMessage(kInvalidValue, name);
    return scoped_refptr<const WebRequestConditionAttribute>(NULL);
  }
  std::vector<std::string> content_types;
  for (base::ListValue::const_iterator it = value_as_list->begin();
       it != value_as_list->end(); ++it) {
    std::string content_type;
    if (!(*it)->GetAsString(&content_type)) {
      *error = ErrorUtils::FormatErrorMessage(kInvalidValue, name);
      return scoped_refptr<const WebRequestConditionAttribute>(NULL);
    }
    content_types.push_back(content_type);
  }

  return scoped_refptr<const WebRequestConditionAttribute>(
      new WebRequestConditionAttributeContentType(
          content_types, name == keys::kContentTypeKey));
}

int WebRequestConditionAttributeContentType::GetStages() const {
  return ON_HEADERS_RECEIVED;
}

bool WebRequestConditionAttributeContentType::IsFulfilled(
    const WebRequestData& request_data) const {
  if (!(request_data.stage & GetStages()))
    return false;
  std::string content_type;
  request_data.original_response_headers->GetNormalizedHeader(
      net::HttpRequestHeaders::kContentType, &content_type);
  std::string mime_type;
  std::string charset;
  bool had_charset = false;
  net::HttpUtil::ParseContentType(
      content_type, &mime_type, &charset, &had_charset, NULL);

  if (inclusive_) {
    return std::find(content_types_.begin(), content_types_.end(),
                     mime_type) != content_types_.end();
  } else {
    return std::find(content_types_.begin(), content_types_.end(),
                     mime_type) == content_types_.end();
  }
}

WebRequestConditionAttribute::Type
WebRequestConditionAttributeContentType::GetType() const {
  return CONDITION_CONTENT_TYPE;
}

std::string WebRequestConditionAttributeContentType::GetName() const {
  return (inclusive_ ? keys::kContentTypeKey : keys::kExcludeContentTypeKey);
}

bool WebRequestConditionAttributeContentType::Equals(
    const WebRequestConditionAttribute* other) const {
  if (!WebRequestConditionAttribute::Equals(other))
    return false;
  const WebRequestConditionAttributeContentType* casted_other =
      static_cast<const WebRequestConditionAttributeContentType*>(other);
  return content_types_ == casted_other->content_types_ &&
         inclusive_ == casted_other->inclusive_;
}

// Manages a set of tests to be applied to name-value pairs representing
// headers. This is a helper class to header-related condition attributes.
// It contains a set of test groups. A name-value pair satisfies the whole
// set of test groups iff it passes at least one test group.
class HeaderMatcher {
 public:
  ~HeaderMatcher();

  // Creates an instance based on a list |tests| of test groups, encoded as
  // dictionaries of the type declarativeWebRequest.HeaderFilter (see
  // declarative_web_request.json).
  static scoped_ptr<const HeaderMatcher> Create(const base::ListValue* tests);

  // Does |this| match the header "|name|: |value|"?
  bool TestNameValue(const std::string& name, const std::string& value) const;

 private:
  // Represents a single string-matching test.
  class StringMatchTest {
   public:
    enum MatchType { kPrefix, kSuffix, kEquals, kContains };

    // |data| is the pattern to be matched in the position given by |type|.
    // Note that |data| must point to a StringValue object.
    static scoped_ptr<StringMatchTest> Create(const base::Value* data,
                                              MatchType type,
                                              bool case_sensitive);
    ~StringMatchTest();

    // Does |str| pass |this| StringMatchTest?
    bool Matches(const std::string& str) const;

   private:
    StringMatchTest(const std::string& data,
                    MatchType type,
                    bool case_sensitive);

    const std::string data_;
    const MatchType type_;
    const bool case_sensitive_;
    DISALLOW_COPY_AND_ASSIGN(StringMatchTest);
  };

  // Represents a test group -- a set of string matching tests to be applied to
  // both the header name and value.
  class HeaderMatchTest {
   public:
    ~HeaderMatchTest();

    // Gets the test group description in |tests| and creates the corresponding
    // HeaderMatchTest. On failure returns NULL.
    static scoped_ptr<const HeaderMatchTest> Create(
        const base::DictionaryValue* tests);

    // Does the header "|name|: |value|" match all tests in |this|?
    bool Matches(const std::string& name, const std::string& value) const;

   private:
    // Takes ownership of the content of both |name_match| and |value_match|.
    HeaderMatchTest(ScopedVector<const StringMatchTest>* name_match,
                    ScopedVector<const StringMatchTest>* value_match);

    // Tests to be passed by a header's name.
    const ScopedVector<const StringMatchTest> name_match_;
    // Tests to be passed by a header's value.
    const ScopedVector<const StringMatchTest> value_match_;
    DISALLOW_COPY_AND_ASSIGN(HeaderMatchTest);
  };

  explicit HeaderMatcher(ScopedVector<const HeaderMatchTest>* tests);

  const ScopedVector<const HeaderMatchTest> tests_;

  DISALLOW_COPY_AND_ASSIGN(HeaderMatcher);
};

// HeaderMatcher implementation.

HeaderMatcher::~HeaderMatcher() {}

// static
scoped_ptr<const HeaderMatcher> HeaderMatcher::Create(
    const base::ListValue* tests) {
  ScopedVector<const HeaderMatchTest> header_tests;
  for (base::ListValue::const_iterator it = tests->begin();
       it != tests->end(); ++it) {
    const base::DictionaryValue* tests = NULL;
    if (!(*it)->GetAsDictionary(&tests))
      return scoped_ptr<const HeaderMatcher>();

    scoped_ptr<const HeaderMatchTest> header_test(
        HeaderMatchTest::Create(tests));
    if (header_test.get() == NULL)
      return scoped_ptr<const HeaderMatcher>();
    header_tests.push_back(header_test.release());
  }

  return scoped_ptr<const HeaderMatcher>(new HeaderMatcher(&header_tests));
}

bool HeaderMatcher::TestNameValue(const std::string& name,
                                  const std::string& value) const {
  for (size_t i = 0; i < tests_.size(); ++i) {
    if (tests_[i]->Matches(name, value))
      return true;
  }
  return false;
}

HeaderMatcher::HeaderMatcher(ScopedVector<const HeaderMatchTest>* tests)
  : tests_(tests->Pass()) {}

// HeaderMatcher::StringMatchTest implementation.

// static
scoped_ptr<HeaderMatcher::StringMatchTest>
HeaderMatcher::StringMatchTest::Create(const base::Value* data,
                                       MatchType type,
                                       bool case_sensitive) {
  std::string str;
  CHECK(data->GetAsString(&str));
  return scoped_ptr<StringMatchTest>(
      new StringMatchTest(str, type, case_sensitive));
}

HeaderMatcher::StringMatchTest::~StringMatchTest() {}

bool HeaderMatcher::StringMatchTest::Matches(
    const std::string& str) const {
  switch (type_) {
    case kPrefix:
      return StartsWithASCII(str, data_, case_sensitive_);
    case kSuffix:
      return EndsWith(str, data_, case_sensitive_);
    case kEquals:
      return str.size() == data_.size() &&
             StartsWithASCII(str, data_, case_sensitive_);
    case kContains:
      if (!case_sensitive_) {
        return std::search(str.begin(), str.end(), data_.begin(), data_.end(),
                           CaseInsensitiveCompareASCII<char>()) != str.end();
      } else {
        return str.find(data_) != std::string::npos;
      }
  }
  // We never get past the "switch", but the compiler worries about no return.
  NOTREACHED();
  return false;
}

HeaderMatcher::StringMatchTest::StringMatchTest(const std::string& data,
                                                MatchType type,
                                                bool case_sensitive)
    : data_(data),
      type_(type),
      case_sensitive_(case_sensitive) {}

// HeaderMatcher::HeaderMatchTest implementation.

HeaderMatcher::HeaderMatchTest::HeaderMatchTest(
    ScopedVector<const StringMatchTest>* name_match,
    ScopedVector<const StringMatchTest>* value_match)
    : name_match_(name_match->Pass()),
      value_match_(value_match->Pass()) {}

HeaderMatcher::HeaderMatchTest::~HeaderMatchTest() {}

// static
scoped_ptr<const HeaderMatcher::HeaderMatchTest>
HeaderMatcher::HeaderMatchTest::Create(const base::DictionaryValue* tests) {
  ScopedVector<const StringMatchTest> name_match;
  ScopedVector<const StringMatchTest> value_match;

  for (base::DictionaryValue::Iterator it(*tests);
       !it.IsAtEnd(); it.Advance()) {
    bool is_name = false;  // Is this test for header name?
    StringMatchTest::MatchType match_type;
    if (it.key() == keys::kNamePrefixKey) {
      is_name = true;
      match_type = StringMatchTest::kPrefix;
    } else if (it.key() == keys::kNameSuffixKey) {
      is_name = true;
      match_type = StringMatchTest::kSuffix;
    } else if (it.key() == keys::kNameContainsKey) {
      is_name = true;
      match_type = StringMatchTest::kContains;
    } else if (it.key() == keys::kNameEqualsKey) {
      is_name = true;
      match_type = StringMatchTest::kEquals;
    } else if (it.key() == keys::kValuePrefixKey) {
      match_type = StringMatchTest::kPrefix;
    } else if (it.key() == keys::kValueSuffixKey) {
      match_type = StringMatchTest::kSuffix;
    } else if (it.key() == keys::kValueContainsKey) {
      match_type = StringMatchTest::kContains;
    } else if (it.key() == keys::kValueEqualsKey) {
      match_type = StringMatchTest::kEquals;
    } else {
      NOTREACHED();  // JSON schema type checking should prevent this.
      return scoped_ptr<const HeaderMatchTest>();
    }
    const base::Value* content = &it.value();

    ScopedVector<const StringMatchTest>* tests =
        is_name ? &name_match : &value_match;
    switch (content->GetType()) {
      case base::Value::TYPE_LIST: {
        const base::ListValue* list = NULL;
        CHECK(content->GetAsList(&list));
        for (base::ListValue::const_iterator it = list->begin();
             it != list->end(); ++it) {
          tests->push_back(
              StringMatchTest::Create(*it, match_type, !is_name).release());
        }
        break;
      }
      case base::Value::TYPE_STRING: {
        tests->push_back(
            StringMatchTest::Create(content, match_type, !is_name).release());
        break;
      }
      default: {
        NOTREACHED();  // JSON schema type checking should prevent this.
        return scoped_ptr<const HeaderMatchTest>();
      }
    }
  }

  return scoped_ptr<const HeaderMatchTest>(
      new HeaderMatchTest(&name_match, &value_match));
}

bool HeaderMatcher::HeaderMatchTest::Matches(const std::string& name,
                                             const std::string& value) const {
  for (size_t i = 0; i < name_match_.size(); ++i) {
    if (!name_match_[i]->Matches(name))
      return false;
  }

  for (size_t i = 0; i < value_match_.size(); ++i) {
    if (!value_match_[i]->Matches(value))
      return false;
  }

  return true;
}

//
// WebRequestConditionAttributeRequestHeaders
//

WebRequestConditionAttributeRequestHeaders::
WebRequestConditionAttributeRequestHeaders(
    scoped_ptr<const HeaderMatcher> header_matcher,
    bool positive)
    : header_matcher_(header_matcher.Pass()),
      positive_(positive) {}

WebRequestConditionAttributeRequestHeaders::
~WebRequestConditionAttributeRequestHeaders() {}

namespace {

scoped_ptr<const HeaderMatcher> PrepareHeaderMatcher(
    const std::string& name,
    const base::Value* value,
    std::string* error) {
  const base::ListValue* value_as_list = NULL;
  if (!value->GetAsList(&value_as_list)) {
    *error = ErrorUtils::FormatErrorMessage(kInvalidValue, name);
    return scoped_ptr<const HeaderMatcher>();
  }

  scoped_ptr<const HeaderMatcher> header_matcher(
      HeaderMatcher::Create(value_as_list));
  if (header_matcher.get() == NULL)
    *error = ErrorUtils::FormatErrorMessage(kInvalidValue, name);
  return header_matcher.Pass();
}

}  // namespace

// static
scoped_refptr<const WebRequestConditionAttribute>
WebRequestConditionAttributeRequestHeaders::Create(
    const std::string& name,
    const base::Value* value,
    std::string* error,
    bool* bad_message) {
  DCHECK(name == keys::kRequestHeadersKey ||
         name == keys::kExcludeRequestHeadersKey);

  scoped_ptr<const HeaderMatcher> header_matcher(
      PrepareHeaderMatcher(name, value, error));
  if (header_matcher.get() == NULL)
    return scoped_refptr<const WebRequestConditionAttribute>(NULL);

  return scoped_refptr<const WebRequestConditionAttribute>(
      new WebRequestConditionAttributeRequestHeaders(
          header_matcher.Pass(), name == keys::kRequestHeadersKey));
}

int WebRequestConditionAttributeRequestHeaders::GetStages() const {
  // Currently we only allow matching against headers in the before-send-headers
  // stage. The headers are accessible in other stages as well, but before
  // allowing to match against them in further stages, we should consider
  // caching the match result.
  return ON_BEFORE_SEND_HEADERS;
}

bool WebRequestConditionAttributeRequestHeaders::IsFulfilled(
    const WebRequestData& request_data) const {
  if (!(request_data.stage & GetStages()))
    return false;

  const net::HttpRequestHeaders& headers =
      request_data.request->extra_request_headers();

  bool passed = false;  // Did some header pass TestNameValue?
  net::HttpRequestHeaders::Iterator it(headers);
  while (!passed && it.GetNext())
    passed |= header_matcher_->TestNameValue(it.name(), it.value());

  return (positive_ ? passed : !passed);
}

WebRequestConditionAttribute::Type
WebRequestConditionAttributeRequestHeaders::GetType() const {
  return CONDITION_REQUEST_HEADERS;
}

std::string WebRequestConditionAttributeRequestHeaders::GetName() const {
  return (positive_ ? keys::kRequestHeadersKey
                    : keys::kExcludeRequestHeadersKey);
}

bool WebRequestConditionAttributeRequestHeaders::Equals(
    const WebRequestConditionAttribute* other) const {
  // Comparing headers is too heavy, so we skip it entirely.
  return false;
}

//
// WebRequestConditionAttributeResponseHeaders
//

WebRequestConditionAttributeResponseHeaders::
WebRequestConditionAttributeResponseHeaders(
    scoped_ptr<const HeaderMatcher> header_matcher,
    bool positive)
    : header_matcher_(header_matcher.Pass()),
      positive_(positive) {}

WebRequestConditionAttributeResponseHeaders::
~WebRequestConditionAttributeResponseHeaders() {}

// static
scoped_refptr<const WebRequestConditionAttribute>
WebRequestConditionAttributeResponseHeaders::Create(
    const std::string& name,
    const base::Value* value,
    std::string* error,
    bool* bad_message) {
  DCHECK(name == keys::kResponseHeadersKey ||
         name == keys::kExcludeResponseHeadersKey);

  scoped_ptr<const HeaderMatcher> header_matcher(
      PrepareHeaderMatcher(name, value, error));
  if (header_matcher.get() == NULL)
    return scoped_refptr<const WebRequestConditionAttribute>(NULL);

  return scoped_refptr<const WebRequestConditionAttribute>(
      new WebRequestConditionAttributeResponseHeaders(
          header_matcher.Pass(), name == keys::kResponseHeadersKey));
}

int WebRequestConditionAttributeResponseHeaders::GetStages() const {
  return ON_HEADERS_RECEIVED;
}

bool WebRequestConditionAttributeResponseHeaders::IsFulfilled(
    const WebRequestData& request_data) const {
  if (!(request_data.stage & GetStages()))
    return false;

  const net::HttpResponseHeaders* headers =
      request_data.original_response_headers;
  if (headers == NULL) {
    // Each header of an empty set satisfies (the negation of) everything;
    // OTOH, there is no header to satisfy even the most permissive test.
    return !positive_;
  }

  bool passed = false;  // Did some header pass TestNameValue?
  std::string name;
  std::string value;
  void* iter = NULL;
  while (!passed && headers->EnumerateHeaderLines(&iter, &name, &value)) {
    passed |= header_matcher_->TestNameValue(name, value);
  }

  return (positive_ ? passed : !passed);
}

WebRequestConditionAttribute::Type
WebRequestConditionAttributeResponseHeaders::GetType() const {
  return CONDITION_RESPONSE_HEADERS;
}

std::string WebRequestConditionAttributeResponseHeaders::GetName() const {
  return (positive_ ? keys::kResponseHeadersKey
                    : keys::kExcludeResponseHeadersKey);
}

bool WebRequestConditionAttributeResponseHeaders::Equals(
    const WebRequestConditionAttribute* other) const {
  return false;
}

//
// WebRequestConditionAttributeThirdParty
//

WebRequestConditionAttributeThirdParty::
WebRequestConditionAttributeThirdParty(bool match_third_party)
    : match_third_party_(match_third_party) {}

WebRequestConditionAttributeThirdParty::
~WebRequestConditionAttributeThirdParty() {}

// static
scoped_refptr<const WebRequestConditionAttribute>
WebRequestConditionAttributeThirdParty::Create(
    const std::string& name,
    const base::Value* value,
    std::string* error,
    bool* bad_message) {
  DCHECK(name == keys::kThirdPartyKey);

  bool third_party = false;  // Dummy value, gets overwritten.
  if (!value->GetAsBoolean(&third_party)) {
    *error = ErrorUtils::FormatErrorMessage(kInvalidValue,
                                                     keys::kThirdPartyKey);
    return scoped_refptr<const WebRequestConditionAttribute>(NULL);
  }

  return scoped_refptr<const WebRequestConditionAttribute>(
      new WebRequestConditionAttributeThirdParty(third_party));
}

int WebRequestConditionAttributeThirdParty::GetStages() const {
  return ON_BEFORE_REQUEST | ON_BEFORE_SEND_HEADERS | ON_SEND_HEADERS |
      ON_HEADERS_RECEIVED | ON_AUTH_REQUIRED | ON_BEFORE_REDIRECT |
      ON_RESPONSE_STARTED | ON_COMPLETED | ON_ERROR;
}

bool WebRequestConditionAttributeThirdParty::IsFulfilled(
    const WebRequestData& request_data) const {
  if (!(request_data.stage & GetStages()))
    return false;

  // Request is "1st party" if it gets cookies under 3rd party-blocking policy.
  const net::StaticCookiePolicy block_third_party_policy(
      net::StaticCookiePolicy::BLOCK_ALL_THIRD_PARTY_COOKIES);
  const int can_get_cookies = block_third_party_policy.CanGetCookies(
          request_data.request->url(),
          request_data.request->first_party_for_cookies());
  const bool is_first_party = (can_get_cookies == net::OK);

  return match_third_party_ ? !is_first_party : is_first_party;
}

WebRequestConditionAttribute::Type
WebRequestConditionAttributeThirdParty::GetType() const {
  return CONDITION_THIRD_PARTY;
}

std::string WebRequestConditionAttributeThirdParty::GetName() const {
  return keys::kThirdPartyKey;
}

bool WebRequestConditionAttributeThirdParty::Equals(
    const WebRequestConditionAttribute* other) const {
  if (!WebRequestConditionAttribute::Equals(other))
    return false;
  const WebRequestConditionAttributeThirdParty* casted_other =
      static_cast<const WebRequestConditionAttributeThirdParty*>(other);
  return match_third_party_ == casted_other->match_third_party_;
}

//
// WebRequestConditionAttributeStages
//

WebRequestConditionAttributeStages::
WebRequestConditionAttributeStages(int allowed_stages)
    : allowed_stages_(allowed_stages) {}

WebRequestConditionAttributeStages::
~WebRequestConditionAttributeStages() {}

namespace {

// Reads strings stored in |value|, which is expected to be a ListValue, and
// sets corresponding bits (see RequestStage) in |out_stages|. Returns true on
// success, false otherwise.
bool ParseListOfStages(const base::Value& value, int* out_stages) {
  const base::ListValue* list = NULL;
  if (!value.GetAsList(&list))
    return false;

  int stages = 0;
  std::string stage_name;
  for (base::ListValue::const_iterator it = list->begin();
       it != list->end(); ++it) {
    if (!((*it)->GetAsString(&stage_name)))
      return false;
    if (stage_name == keys::kOnBeforeRequestEnum) {
      stages |= ON_BEFORE_REQUEST;
    } else if (stage_name == keys::kOnBeforeSendHeadersEnum) {
      stages |= ON_BEFORE_SEND_HEADERS;
    } else if (stage_name == keys::kOnHeadersReceivedEnum) {
      stages |= ON_HEADERS_RECEIVED;
    } else if (stage_name == keys::kOnAuthRequiredEnum) {
      stages |= ON_AUTH_REQUIRED;
    } else {
      NOTREACHED();  // JSON schema checks prevent getting here.
      return false;
    }
  }

  *out_stages = stages;
  return true;
}

}  // namespace

// static
scoped_refptr<const WebRequestConditionAttribute>
WebRequestConditionAttributeStages::Create(const std::string& name,
                                           const base::Value* value,
                                           std::string* error,
                                           bool* bad_message) {
  DCHECK(name == keys::kStagesKey);

  int allowed_stages = 0;
  if (!ParseListOfStages(*value, &allowed_stages)) {
    *error = ErrorUtils::FormatErrorMessage(kInvalidValue,
                                                     keys::kStagesKey);
    return scoped_refptr<const WebRequestConditionAttribute>(NULL);
  }

  return scoped_refptr<const WebRequestConditionAttribute>(
      new WebRequestConditionAttributeStages(allowed_stages));
}

int WebRequestConditionAttributeStages::GetStages() const {
  return allowed_stages_;
}

bool WebRequestConditionAttributeStages::IsFulfilled(
    const WebRequestData& request_data) const {
  // Note: removing '!=' triggers warning C4800 on the VS compiler.
  return (request_data.stage & GetStages()) != 0;
}

WebRequestConditionAttribute::Type
WebRequestConditionAttributeStages::GetType() const {
  return CONDITION_STAGES;
}

std::string WebRequestConditionAttributeStages::GetName() const {
  return keys::kStagesKey;
}

bool WebRequestConditionAttributeStages::Equals(
    const WebRequestConditionAttribute* other) const {
  if (!WebRequestConditionAttribute::Equals(other))
    return false;
  const WebRequestConditionAttributeStages* casted_other =
      static_cast<const WebRequestConditionAttributeStages*>(other);
  return allowed_stages_ == casted_other->allowed_stages_;
}

}  // namespace extensions
