// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/autocomplete_input.h"

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/metrics/proto/omnibox_event.pb.h"
#include "components/omnibox/browser/autocomplete_scheme_classifier.h"
#include "components/omnibox/browser/omnibox_field_trial.h"
#include "components/url_formatter/url_fixer.h"
#include "components/url_formatter/url_formatter.h"
#include "net/base/net_util.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/url_canon_ip.h"
#include "url/url_util.h"

namespace {

// Hardcode constant to avoid any dependencies on content/.
const char kViewSourceScheme[] = "view-source";

void AdjustCursorPositionIfNecessary(size_t num_leading_chars_removed,
                                     size_t* cursor_position) {
  if (*cursor_position == base::string16::npos)
    return;
  if (num_leading_chars_removed < *cursor_position)
    *cursor_position -= num_leading_chars_removed;
  else
    *cursor_position = 0;
}

// Finds all terms in |text| that start with http:// or https:// plus at least
// one more character and puts the text after the prefix in
// |terms_prefixed_by_http_or_https|.
void PopulateTermsPrefixedByHttpOrHttps(
    const base::string16& text,
    std::vector<base::string16>* terms_prefixed_by_http_or_https) {
  // Split on whitespace rather than use ICU's word iterator because, for
  // example, ICU's iterator may break on punctuation (such as ://) or decide
  // to split a single term in a hostname (if it seems to think that the
  // hostname is multiple words).  Neither of these behaviors is desirable.
  const std::string separator(url::kStandardSchemeSeparator);
  for (const auto& term :
       base::SplitString(text, base::ASCIIToUTF16(" "),
                         base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL)) {
    const std::string term_utf8(base::UTF16ToUTF8(term));
    static const char* kSchemes[2] = { url::kHttpScheme, url::kHttpsScheme };
    for (const char* scheme : kSchemes) {
      const std::string prefix(scheme + separator);
      // Doing an ASCII comparison is okay because prefix is ASCII.
      if (base::StartsWith(term_utf8, prefix,
                           base::CompareCase::INSENSITIVE_ASCII) &&
          (term_utf8.length() > prefix.length())) {
        terms_prefixed_by_http_or_https->push_back(
            term.substr(prefix.length()));
      }
    }
  }
}

}  // namespace

AutocompleteInput::AutocompleteInput()
    : cursor_position_(base::string16::npos),
      current_page_classification_(metrics::OmniboxEventProto::INVALID_SPEC),
      type_(metrics::OmniboxInputType::INVALID),
      prevent_inline_autocomplete_(false),
      prefer_keyword_(false),
      allow_exact_keyword_match_(true),
      want_asynchronous_matches_(true),
      from_omnibox_focus_(false) {
}

AutocompleteInput::AutocompleteInput(
    const base::string16& text,
    size_t cursor_position,
    const std::string& desired_tld,
    const GURL& current_url,
    metrics::OmniboxEventProto::PageClassification current_page_classification,
    bool prevent_inline_autocomplete,
    bool prefer_keyword,
    bool allow_exact_keyword_match,
    bool want_asynchronous_matches,
    bool from_omnibox_focus,
    const AutocompleteSchemeClassifier& scheme_classifier)
    : cursor_position_(cursor_position),
      current_url_(current_url),
      current_page_classification_(current_page_classification),
      prevent_inline_autocomplete_(prevent_inline_autocomplete),
      prefer_keyword_(prefer_keyword),
      allow_exact_keyword_match_(allow_exact_keyword_match),
      want_asynchronous_matches_(want_asynchronous_matches),
      from_omnibox_focus_(from_omnibox_focus) {
  DCHECK(cursor_position <= text.length() ||
         cursor_position == base::string16::npos)
      << "Text: '" << text << "', cp: " << cursor_position;
  // None of the providers care about leading white space so we always trim it.
  // Providers that care about trailing white space handle trimming themselves.
  if ((base::TrimWhitespace(text, base::TRIM_LEADING, &text_) &
       base::TRIM_LEADING) != 0)
    AdjustCursorPositionIfNecessary(text.length() - text_.length(),
                                    &cursor_position_);

  GURL canonicalized_url;
  type_ = Parse(text_, desired_tld, scheme_classifier, &parts_, &scheme_,
                &canonicalized_url);
  PopulateTermsPrefixedByHttpOrHttps(text_, &terms_prefixed_by_http_or_https_);

  if (type_ == metrics::OmniboxInputType::INVALID)
    return;

  if (((type_ == metrics::OmniboxInputType::UNKNOWN) ||
       (type_ == metrics::OmniboxInputType::URL)) &&
      canonicalized_url.is_valid() &&
      (!canonicalized_url.IsStandard() || canonicalized_url.SchemeIsFile() ||
       canonicalized_url.SchemeIsFileSystem() ||
       !canonicalized_url.host().empty()))
    canonicalized_url_ = canonicalized_url;

  size_t chars_removed = RemoveForcedQueryStringIfNecessary(type_, &text_);
  AdjustCursorPositionIfNecessary(chars_removed, &cursor_position_);
  if (chars_removed) {
    // Remove spaces between opening question mark and first actual character.
    base::string16 trimmed_text;
    if ((base::TrimWhitespace(text_, base::TRIM_LEADING, &trimmed_text) &
         base::TRIM_LEADING) != 0) {
      AdjustCursorPositionIfNecessary(text_.length() - trimmed_text.length(),
                                      &cursor_position_);
      text_ = trimmed_text;
    }
  }
}

AutocompleteInput::~AutocompleteInput() {
}

// static
size_t AutocompleteInput::RemoveForcedQueryStringIfNecessary(
    metrics::OmniboxInputType::Type type,
    base::string16* text) {
  if ((type != metrics::OmniboxInputType::FORCED_QUERY) || text->empty() ||
      (*text)[0] != L'?')
    return 0;
  // Drop the leading '?'.
  text->erase(0, 1);
  return 1;
}

// static
std::string AutocompleteInput::TypeToString(
    metrics::OmniboxInputType::Type type) {
  switch (type) {
    case metrics::OmniboxInputType::INVALID:      return "invalid";
    case metrics::OmniboxInputType::UNKNOWN:      return "unknown";
    case metrics::OmniboxInputType::DEPRECATED_REQUESTED_URL:
      return "deprecated-requested-url";
    case metrics::OmniboxInputType::URL:          return "url";
    case metrics::OmniboxInputType::QUERY:        return "query";
    case metrics::OmniboxInputType::FORCED_QUERY: return "forced-query";
  }
  return std::string();
}

// static
metrics::OmniboxInputType::Type AutocompleteInput::Parse(
    const base::string16& text,
    const std::string& desired_tld,
    const AutocompleteSchemeClassifier& scheme_classifier,
    url::Parsed* parts,
    base::string16* scheme,
    GURL* canonicalized_url) {
  size_t first_non_white = text.find_first_not_of(base::kWhitespaceUTF16, 0);
  if (first_non_white == base::string16::npos)
    return metrics::OmniboxInputType::INVALID;  // All whitespace.

  if (text[first_non_white] == L'?') {
    // If the first non-whitespace character is a '?', we magically treat this
    // as a query.
    return metrics::OmniboxInputType::FORCED_QUERY;
  }

  // Ask our parsing back-end to help us understand what the user typed.  We
  // use the URLFixerUpper here because we want to be smart about what we
  // consider a scheme.  For example, we shouldn't consider www.google.com:80
  // to have a scheme.
  url::Parsed local_parts;
  if (!parts)
    parts = &local_parts;
  const base::string16 parsed_scheme(url_formatter::SegmentURL(text, parts));
  if (scheme)
    *scheme = parsed_scheme;
  const std::string parsed_scheme_utf8(base::UTF16ToUTF8(parsed_scheme));

  // If we can't canonicalize the user's input, the rest of the autocomplete
  // system isn't going to be able to produce a navigable URL match for it.
  // So we just return QUERY immediately in these cases.
  GURL placeholder_canonicalized_url;
  if (!canonicalized_url)
    canonicalized_url = &placeholder_canonicalized_url;
  *canonicalized_url =
      url_formatter::FixupURL(base::UTF16ToUTF8(text), desired_tld);
  if (!canonicalized_url->is_valid())
    return metrics::OmniboxInputType::QUERY;

  if (base::LowerCaseEqualsASCII(parsed_scheme_utf8, url::kFileScheme)) {
    // A user might or might not type a scheme when entering a file URL.  In
    // either case, |parsed_scheme_utf8| will tell us that this is a file URL,
    // but |parts->scheme| might be empty, e.g. if the user typed "C:\foo".
    return metrics::OmniboxInputType::URL;
  }

  // If the user typed a scheme, and it's HTTP or HTTPS, we know how to parse it
  // well enough that we can fall through to the heuristics below.  If it's
  // something else, we can just determine our action based on what we do with
  // any input of this scheme.  In theory we could do better with some schemes
  // (e.g. "ftp" or "view-source") but I'll wait to spend the effort on that
  // until I run into some cases that really need it.
  if (parts->scheme.is_nonempty() &&
      !base::LowerCaseEqualsASCII(parsed_scheme_utf8, url::kHttpScheme) &&
      !base::LowerCaseEqualsASCII(parsed_scheme_utf8, url::kHttpsScheme)) {
    metrics::OmniboxInputType::Type type =
        scheme_classifier.GetInputTypeForScheme(parsed_scheme_utf8);
    if (type != metrics::OmniboxInputType::INVALID)
      return type;

    // We don't know about this scheme.  It might be that the user typed a
    // URL of the form "username:password@foo.com".
    const base::string16 http_scheme_prefix =
        base::ASCIIToUTF16(std::string(url::kHttpScheme) +
                           url::kStandardSchemeSeparator);
    url::Parsed http_parts;
    base::string16 http_scheme;
    GURL http_canonicalized_url;
    metrics::OmniboxInputType::Type http_type =
        Parse(http_scheme_prefix + text, desired_tld, scheme_classifier,
              &http_parts, &http_scheme, &http_canonicalized_url);
    DCHECK_EQ(std::string(url::kHttpScheme),
              base::UTF16ToUTF8(http_scheme));

    if ((http_type == metrics::OmniboxInputType::URL) &&
        http_parts.username.is_nonempty() &&
        http_parts.password.is_nonempty()) {
      // Manually re-jigger the parsed parts to match |text| (without the
      // http scheme added).
      http_parts.scheme.reset();
      url::Component* components[] = {
        &http_parts.username,
        &http_parts.password,
        &http_parts.host,
        &http_parts.port,
        &http_parts.path,
        &http_parts.query,
        &http_parts.ref,
      };
      for (size_t i = 0; i < arraysize(components); ++i) {
        url_formatter::OffsetComponent(
            -static_cast<int>(http_scheme_prefix.length()), components[i]);
      }

      *parts = http_parts;
      if (scheme)
        scheme->clear();
      *canonicalized_url = http_canonicalized_url;

      return metrics::OmniboxInputType::URL;
    }

    // We don't know about this scheme and it doesn't look like the user
    // typed a username and password.  It's likely to be a search operator
    // like "site:" or "link:".  We classify it as UNKNOWN so the user has
    // the option of treating it as a URL if we're wrong.
    // Note that SegmentURL() is smart so we aren't tricked by "c:\foo" or
    // "www.example.com:81" in this case.
    return metrics::OmniboxInputType::UNKNOWN;
  }

  // Either the user didn't type a scheme, in which case we need to distinguish
  // between an HTTP URL and a query, or the scheme is HTTP or HTTPS, in which
  // case we should reject invalid formulations.

  // If we have an empty host it can't be a valid HTTP[S] URL.  (This should
  // only trigger for input that begins with a colon, which GURL will parse as a
  // valid, non-standard URL; for standard URLs, an empty host would have
  // resulted in an invalid |canonicalized_url| above.)
  if (!canonicalized_url->has_host())
    return metrics::OmniboxInputType::QUERY;

  // Determine the host family.  We get this information by (re-)canonicalizing
  // the already-canonicalized host rather than using the user's original input,
  // in case fixup affected the result here (e.g. an input that looks like an
  // IPv4 address but with a non-empty desired TLD would return IPV4 before
  // fixup and NEUTRAL afterwards, and we want to treat it as NEUTRAL).
  url::CanonHostInfo host_info;
  net::CanonicalizeHost(canonicalized_url->host(), &host_info);

  // Check if the canonicalized host has a known TLD, which we'll want to know
  // below.
  const size_t registry_length =
      net::registry_controlled_domains::GetRegistryLength(
          canonicalized_url->host(),
          net::registry_controlled_domains::EXCLUDE_UNKNOWN_REGISTRIES,
          net::registry_controlled_domains::EXCLUDE_PRIVATE_REGISTRIES);
  DCHECK_NE(std::string::npos, registry_length);
  const bool has_known_tld = registry_length != 0;

  // See if the hostname is valid.  While IE and GURL allow hostnames to contain
  // many other characters (perhaps for weird intranet machines), it's extremely
  // unlikely that a user would be trying to type those in for anything other
  // than a search query.
  const base::string16 original_host(
      text.substr(parts->host.begin, parts->host.len));
  if ((host_info.family == url::CanonHostInfo::NEUTRAL) &&
      !net::IsCanonicalizedHostCompliant(canonicalized_url->host())) {
    // Invalid hostname.  There are several possible cases:
    // * The user is typing a multi-word query.  If we see a space anywhere in
    //   the input host we assume this is a search and return QUERY.  (We check
    //   the input string instead of canonicalized_url->host() in case fixup
    //   escaped the space.)
    // * The user is typing some garbage string.  Return QUERY.
    // * Our checker is too strict and the user is typing a real-world URL
    //   that's "invalid" but resolves.  To catch these, we return UNKNOWN when
    //   the user explicitly typed a scheme or when the hostname has a known
    //   TLD, so we'll still search by default but we'll show the accidental
    //   search infobar if necessary.
    //
    // This means we would block the following kinds of navigation attempts:
    // * Navigations to a hostname with spaces
    // * Navigations to a hostname with invalid characters and an unknown TLD
    // These might be possible in intranets, but we're not going to support them
    // without concrete evidence that doing so is necessary.
    return (parts->scheme.is_nonempty() ||
        (has_known_tld && (original_host.find(' ') == base::string16::npos))) ?
        metrics::OmniboxInputType::UNKNOWN : metrics::OmniboxInputType::QUERY;
  }

  // For hostnames that look like IP addresses, distinguish between IPv6
  // addresses, which are basically guaranteed to be navigations, and IPv4
  // addresses, which are much fuzzier.
  if (host_info.family == url::CanonHostInfo::IPV6)
    return metrics::OmniboxInputType::URL;
  if (host_info.family == url::CanonHostInfo::IPV4) {
    // The host may be a real IP address, or something that looks a bit like it
    // (e.g. "1.2" or "3232235521").  We check whether it was convertible to an
    // IP with a non-zero first octet; IPs with first octet zero are "source
    // IPs" and are almost never navigable as destination addresses.
    //
    // The one exception to this is 0.0.0.0; on many systems, attempting to
    // navigate to this IP actually navigates to localhost.  To support this
    // case, when the converted IP is 0.0.0.0, we go ahead and run the "did the
    // user actually type four components" test in the conditional below, so
    // that we'll allow explicit attempts to navigate to "0.0.0.0".  If the
    // input was anything else (e.g. "0"), we'll fall through to returning QUERY
    // afterwards.
    if ((host_info.address[0] != 0) ||
        ((host_info.address[1] == 0) && (host_info.address[2] == 0) &&
         (host_info.address[3] == 0))) {
      // This is theoretically a navigable IP.  We have four cases.  The first
      // three are:
      // * If the user typed four distinct components, this is an IP for sure.
      // * If the user typed two or three components, this is almost certainly a
      //   query, especially for two components (as in "13.5/7.25"), but we'll
      //   allow navigation for an explicit scheme or trailing slash below.
      // * If the user typed one component, this is likely a query, but could be
      //   a non-dotted-quad version of an IP address.
      // Unfortunately, since we called CanonicalizeHost() on the
      // already-canonicalized host, all of these cases will have been changed
      // to have four components (e.g. 13.2 -> 13.0.0.2), so we have to call
      // CanonicalizeHost() again, this time on the original input, so that we
      // can get the correct number of IP components.
      //
      // The fourth case is that the user typed something ambiguous like ".1.2"
      // that fixup converted to an IP address ("1.0.0.2").  In this case the
      // call to CanonicalizeHost() will return NEUTRAL here.  Since it's not
      // clear what the user intended, we fall back to our other heuristics.
      net::CanonicalizeHost(base::UTF16ToUTF8(original_host), &host_info);
      if ((host_info.family == url::CanonHostInfo::IPV4) &&
          (host_info.num_ipv4_components == 4))
        return metrics::OmniboxInputType::URL;
    }

    // By this point, if we have an "IP" with first octet zero, we know it
    // wasn't "0.0.0.0", so mark it as non-navigable.
    if (host_info.address[0] == 0)
      return metrics::OmniboxInputType::QUERY;
  }

  // Now that we've ruled out all schemes other than http or https and done a
  // little more sanity checking, the presence of a scheme means this is likely
  // a URL.
  if (parts->scheme.is_nonempty())
    return metrics::OmniboxInputType::URL;

  // Trailing slashes force the input to be treated as a URL.
  if (parts->path.is_nonempty()) {
    base::char16 c = text[parts->path.end() - 1];
    if ((c == '\\') || (c == '/'))
      return metrics::OmniboxInputType::URL;
  }

  // Handle the cases we detected in the IPv4 code above as "almost certainly a
  // query" now that we know the user hasn't tried to force navigation via a
  // scheme/trailing slash.
  if ((host_info.family == url::CanonHostInfo::IPV4) &&
      (host_info.num_ipv4_components > 1))
    return metrics::OmniboxInputType::QUERY;

  // If there is more than one recognized non-host component, this is likely to
  // be a URL, even if the TLD is unknown (in which case this is likely an
  // intranet URL).
  if (NumNonHostComponents(*parts) > 1)
    return metrics::OmniboxInputType::URL;

  // If we reach here with a username, our input looks something like
  // "user@host".  Unless there is a desired TLD, we think this is more likely
  // an email address than an HTTP auth attempt, so we search by default.  (When
  // there _is_ a desired TLD, the user hit ctrl-enter, and we assume that
  // implies an attempted navigation.)
  if (canonicalized_url->has_username() && desired_tld.empty())
    return metrics::OmniboxInputType::UNKNOWN;

  // If the host has a known TLD or a port, it's probably a URL.  Note that we
  // special-case "localhost" as a known hostname.
  if (has_known_tld || (canonicalized_url->host() == "localhost") ||
      canonicalized_url->has_port())
    return metrics::OmniboxInputType::URL;

  // If the input looks like a word followed by a pound sign and possibly more
  // characters ("c#" or "c# foo"), this is almost certainly an attempt to
  // search.  We try to be conservative here by not firing on cases like "c/#"
  // or "c?#" that might actually indicate some cryptic attempt to access an
  // intranet host, and by placing this check late enough that other tests
  // (e.g., for a non-empty TLD or a non-empty scheme) will have already
  // returned URL.
  if (!OmniboxFieldTrial::PreventUWYTDefaultForNonURLInputs() &&
      !parts->path.is_valid() && !canonicalized_url->has_query() &&
      canonicalized_url->has_ref())
    return metrics::OmniboxInputType::QUERY;

  // No scheme, username, port, and no known TLD on the host.
  // This could be:
  // * A single word "foo"; possibly an intranet site, but more likely a search.
  //   This is ideally an UNKNOWN, and we can let the Alternate Nav URL code
  //   catch our mistakes.
  // * A URL with a valid TLD we don't know about yet.  If e.g. a registrar adds
  //   "xxx" as a TLD, then until we add it to our data file, Chrome won't know
  //   "foo.xxx" is a real URL.  So ideally this is a URL, but we can't really
  //   distinguish this case from:
  // * A "URL-like" string that's not really a URL (like
  //   "browser.tabs.closeButtons" or "java.awt.event.*").  This is ideally a
  //   QUERY.  Since this is indistinguishable from the case above, and this
  //   case is much more likely, claim these are UNKNOWN, which should default
  //   to the right thing and let users correct us on a case-by-case basis.
  return metrics::OmniboxInputType::UNKNOWN;
}

// static
void AutocompleteInput::ParseForEmphasizeComponents(
    const base::string16& text,
    const AutocompleteSchemeClassifier& scheme_classifier,
    url::Component* scheme,
    url::Component* host) {
  url::Parsed parts;
  base::string16 scheme_str;
  Parse(text, std::string(), scheme_classifier, &parts, &scheme_str, NULL);

  *scheme = parts.scheme;
  *host = parts.host;

  int after_scheme_and_colon = parts.scheme.end() + 1;
  // For the view-source scheme, we should emphasize the scheme and host of the
  // URL qualified by the view-source prefix.
  if (base::LowerCaseEqualsASCII(scheme_str, kViewSourceScheme) &&
      (static_cast<int>(text.length()) > after_scheme_and_colon)) {
    // Obtain the URL prefixed by view-source and parse it.
    base::string16 real_url(text.substr(after_scheme_and_colon));
    url::Parsed real_parts;
    AutocompleteInput::Parse(real_url, std::string(), scheme_classifier,
                             &real_parts, NULL, NULL);
    if (real_parts.scheme.is_nonempty() || real_parts.host.is_nonempty()) {
      if (real_parts.scheme.is_nonempty()) {
        *scheme = url::Component(
            after_scheme_and_colon + real_parts.scheme.begin,
            real_parts.scheme.len);
      } else {
        scheme->reset();
      }
      if (real_parts.host.is_nonempty()) {
        *host = url::Component(after_scheme_and_colon + real_parts.host.begin,
                               real_parts.host.len);
      } else {
        host->reset();
      }
    }
  } else if (base::LowerCaseEqualsASCII(scheme_str, url::kFileSystemScheme) &&
             parts.inner_parsed() && parts.inner_parsed()->scheme.is_valid()) {
    *host = parts.inner_parsed()->host;
  }
}

// static
base::string16 AutocompleteInput::FormattedStringWithEquivalentMeaning(
    const GURL& url,
    const base::string16& formatted_url,
    const AutocompleteSchemeClassifier& scheme_classifier) {
  if (!url_formatter::CanStripTrailingSlash(url))
    return formatted_url;
  const base::string16 url_with_path(formatted_url + base::char16('/'));
  return (AutocompleteInput::Parse(formatted_url, std::string(),
                                   scheme_classifier, NULL, NULL, NULL) ==
          AutocompleteInput::Parse(url_with_path, std::string(),
                                   scheme_classifier, NULL, NULL, NULL)) ?
      formatted_url : url_with_path;
}

// static
int AutocompleteInput::NumNonHostComponents(const url::Parsed& parts) {
  int num_nonhost_components = 0;
  if (parts.scheme.is_nonempty())
    ++num_nonhost_components;
  if (parts.username.is_nonempty())
    ++num_nonhost_components;
  if (parts.password.is_nonempty())
    ++num_nonhost_components;
  if (parts.port.is_nonempty())
    ++num_nonhost_components;
  if (parts.path.is_nonempty())
    ++num_nonhost_components;
  if (parts.query.is_nonempty())
    ++num_nonhost_components;
  if (parts.ref.is_nonempty())
    ++num_nonhost_components;
  return num_nonhost_components;
}

// static
bool AutocompleteInput::HasHTTPScheme(const base::string16& input) {
  std::string utf8_input(base::UTF16ToUTF8(input));
  url::Component scheme;
  if (url::FindAndCompareScheme(utf8_input, kViewSourceScheme, &scheme)) {
    utf8_input.erase(0, scheme.end() + 1);
  }
  return url::FindAndCompareScheme(utf8_input, url::kHttpScheme, NULL);
}

void AutocompleteInput::UpdateText(const base::string16& text,
                                   size_t cursor_position,
                                   const url::Parsed& parts) {
  DCHECK(cursor_position <= text.length() ||
         cursor_position == base::string16::npos)
      << "Text: '" << text << "', cp: " << cursor_position;
  text_ = text;
  cursor_position_ = cursor_position;
  parts_ = parts;
}

void AutocompleteInput::Clear() {
  text_.clear();
  cursor_position_ = base::string16::npos;
  current_url_ = GURL();
  current_page_classification_ = metrics::OmniboxEventProto::INVALID_SPEC;
  type_ = metrics::OmniboxInputType::INVALID;
  parts_ = url::Parsed();
  scheme_.clear();
  canonicalized_url_ = GURL();
  prevent_inline_autocomplete_ = false;
  prefer_keyword_ = false;
  allow_exact_keyword_match_ = false;
  want_asynchronous_matches_ = true;
  from_omnibox_focus_ = false;
  terms_prefixed_by_http_or_https_.clear();
}
