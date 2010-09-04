// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVICE_CLOUD_PRINT_CLOUD_PRINT_CONSTS_H_
#define CHROME_SERVICE_CLOUD_PRINT_CLOUD_PRINT_CONSTS_H_
#pragma once

#include "base/basictypes.h"

// Constant defines used in the cloud print proxy code
extern const char kProxyIdValue[];
extern const char kPrinterNameValue[];
extern const char kPrinterDescValue[];
extern const char kPrinterCapsValue[];
extern const char kPrinterDefaultsValue[];
extern const char kPrinterStatusValue[];
extern const char kPrinterTagValue[];
// Values in the respone JSON from the cloud print server
extern const char kPrinterListValue[];
extern const char kSuccessValue[];
extern const char kNameValue[];
extern const char kIdValue[];
extern const char kTicketUrlValue[];
extern const char kFileUrlValue[];
extern const char kJobListValue[];
extern const char kTitleValue[];
extern const char kPrinterCapsHashValue[];

extern const char kDefaultCloudPrintServerUrl[];
extern const char kCloudPrintTalkServiceUrl[];
extern const char kGaiaUrl[];
extern const char kCloudPrintGaiaServiceId[];
extern const char kSyncGaiaServiceId[];

// Max interval between retrying connection to the server
const int64 kMaxRetryInterval = 5*60*1000;  // 5 minutes in millseconds
const int64 kBaseRetryInterval = 5*1000;  // 5 seconds
const int kMaxRetryCount = 2;
const int64 kJobStatusUpdateInterval = 10*1000;  // 10 seconds

#endif  // CHROME_SERVICE_CLOUD_PRINT_CLOUD_PRINT_CONSTS_H_

