// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web/public/user_agent.h"

#import <UIKit/UIKit.h>

#include <sys/sysctl.h>
#include <string>

#include "base/mac/scoped_nsobject.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "base/sys_info.h"

namespace {

struct UAVersions {
  const char* safari_version_string;
  const char* webkit_version_string;
};

struct OSVersionMap {
  int32 major_os_version;
  int32 minor_os_version;
  UAVersions ua_versions;
};

const UAVersions& GetUAVersionsForCurrentOS() {
  // The WebKit version can be extracted dynamically from UIWebView, but the
  // Safari version can't be, so a lookup table is used instead (for both, since
  // the reported versions should stay in sync).
  static const OSVersionMap version_map[] = {
    { 8, 0, { "600.1.4",   "600.1.4" } },
    { 7, 1, { "9537.53",   "537.51.2" } },
    { 7, 0, { "9537.53",   "537.51.1" } },
    // 6.1 has the same values as 6.0.
    { 6, 0, { "8536.25",   "536.26" } },
  };

  int32 os_major_version = 0;
  int32 os_minor_version = 0;
  int32 os_bugfix_version = 0;
  base::SysInfo::OperatingSystemVersionNumbers(&os_major_version,
                                               &os_minor_version,
                                               &os_bugfix_version);

  // Return the versions corresponding to the first (and thus highest) OS
  // version less than or equal to the given OS version.
  for (unsigned int i = 0; i < arraysize(version_map); ++i) {
    if (os_major_version > version_map[i].major_os_version ||
        (os_major_version == version_map[i].major_os_version &&
         os_minor_version >= version_map[i].minor_os_version))
      return version_map[i].ua_versions;
  }
  NOTREACHED();
  return version_map[arraysize(version_map) - 1].ua_versions;
}

std::string BuildOSCpuInfo() {
  int32 os_major_version = 0;
  int32 os_minor_version = 0;
  int32 os_bugfix_version = 0;
  base::SysInfo::OperatingSystemVersionNumbers(&os_major_version,
                                               &os_minor_version,
                                               &os_bugfix_version);
  std::string os_version;
  if (os_bugfix_version == 0) {
    base::StringAppendF(&os_version,
                        "%d_%d",
                        os_major_version,
                        os_minor_version);
  } else {
    base::StringAppendF(&os_version,
                        "%d_%d_%d",
                        os_major_version,
                        os_minor_version,
                        os_bugfix_version);
  }

  // Remove the end of the platform name. For example "iPod touch" becomes
  // "iPod".
  std::string platform = base::SysNSStringToUTF8(
      [[UIDevice currentDevice] model]);
  size_t position = platform.find_first_of(" ");
  if (position != std::string::npos)
    platform = platform.substr(0, position);

  std::string os_cpu;
  base::StringAppendF(
      &os_cpu,
      "%s; CPU %s %s like Mac OS X",
      platform.c_str(),
      (platform == "iPad") ? "OS" : "iPhone OS",
      os_version.c_str());

  return os_cpu;
}

}  // namespace

namespace web {

std::string BuildUserAgentFromProduct(const std::string& product) {
  // Retrieve the kernel build number.
  int mib[2] = {CTL_KERN, KERN_OSVERSION};
  unsigned int namelen = sizeof(mib) / sizeof(mib[0]);
  size_t bufferSize = 0;
  sysctl(mib, namelen, NULL, &bufferSize, NULL, 0);
  char kernel_version[bufferSize];
  int result = sysctl(mib, namelen, kernel_version, &bufferSize, NULL, 0);
  DCHECK(result == 0);

  UAVersions ua_versions = GetUAVersionsForCurrentOS();

  std::string user_agent;
  base::StringAppendF(&user_agent,
                      "Mozilla/5.0 (%s) AppleWebKit/%s"
                      " (KHTML, like Gecko) %s Mobile/%s Safari/%s",
                      BuildOSCpuInfo().c_str(),
                      ua_versions.webkit_version_string,
                      product.c_str(),
                      kernel_version,
                      ua_versions.safari_version_string);

  return user_agent;
}

}  // namespace web
