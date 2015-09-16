// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data/browsing_data_counter.h"

#include "base/prefs/pref_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"

BrowsingDataCounter::BrowsingDataCounter() {}

BrowsingDataCounter::~BrowsingDataCounter() {
}

void BrowsingDataCounter::Init(
    Profile* profile,
    const Callback& callback) {
  DCHECK(!initialized_);
  profile_ = profile;
  callback_ = callback;
  pref_.Init(
      GetPrefName(),
      profile_->GetPrefs(),
      base::Bind(&BrowsingDataCounter::Restart,
                 base::Unretained(this)));
  period_.Init(
      prefs::kDeleteTimePeriod,
      profile_->GetPrefs(),
      base::Bind(&BrowsingDataCounter::Restart,
                 base::Unretained(this)));

  initialized_ = true;
  OnInitialized();
}

Profile* BrowsingDataCounter::GetProfile() const {
  return profile_;
}

void BrowsingDataCounter::OnInitialized() {
}

base::Time BrowsingDataCounter::GetPeriodStart() {
  return BrowsingDataRemover::CalculateBeginDeleteTime(
      static_cast<BrowsingDataRemover::TimePeriod>(*period_));
}

void BrowsingDataCounter::Restart() {
  DCHECK(initialized_);

  // If this data type was unchecked for deletion, we do not need to count it.
  if (!profile_->GetPrefs()->GetBoolean(GetPrefName()))
    return;

  callback_.Run(false, 0u);

  Count();
}

void BrowsingDataCounter::ReportResult(uint32 value) {
  DCHECK(initialized_);
  callback_.Run(true, value);
}
