// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "chrome/browser/banners/app_banner_settings_helper.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "ui/base/page_transition_types.h"

namespace {

const char kTestURL[] = "https://www.google.com";
const char kSameOriginTestURL[] = "https://www.google.com/foo.html";
const char kTestPackageName[] = "test.package";

base::Time GetReferenceTime() {
  base::Time::Exploded exploded_reference_time;
  exploded_reference_time.year = 2015;
  exploded_reference_time.month = 1;
  exploded_reference_time.day_of_month = 30;
  exploded_reference_time.day_of_week = 5;
  exploded_reference_time.hour = 11;
  exploded_reference_time.minute = 0;
  exploded_reference_time.second = 0;
  exploded_reference_time.millisecond = 0;

  return base::Time::FromLocalExploded(exploded_reference_time);
}

bool IsWithinDay(base::Time time1, base::Time time2) {
  return time1 - time2 < base::TimeDelta::FromDays(1) ||
         time2 - time1 < base::TimeDelta::FromDays(1);
}

bool IsWithinHour(base::Time time1, base::Time time2) {
  return time1 - time2 < base::TimeDelta::FromHours(1) ||
         time2 - time1 < base::TimeDelta::FromHours(1);
}

class AppBannerSettingsHelperTest : public ChromeRenderViewHostTestHarness {};

}  // namespace

TEST_F(AppBannerSettingsHelperTest, CouldShowEvents) {
  AppBannerSettingsHelper::SetEngagementWeights(1, 1);
  GURL url(kTestURL);
  NavigateAndCommit(url);

  // Check that by default, there are no events recorded.
  std::vector<AppBannerSettingsHelper::BannerEvent> events =
      AppBannerSettingsHelper::GetCouldShowBannerEvents(web_contents(), url,
                                                        kTestPackageName);
  EXPECT_TRUE(events.empty());

  base::Time reference_time = GetReferenceTime();
  base::Time same_day = reference_time + base::TimeDelta::FromHours(2);
  base::Time three_days_prior = reference_time - base::TimeDelta::FromDays(3);
  base::Time previous_fortnight =
      reference_time - base::TimeDelta::FromDays(14);

  // Test adding the first date.
  AppBannerSettingsHelper::RecordBannerCouldShowEvent(
      web_contents(), url, kTestPackageName, previous_fortnight,
      ui::PAGE_TRANSITION_TYPED);

  // It should be the only date recorded.
  events = AppBannerSettingsHelper::GetCouldShowBannerEvents(
      web_contents(), url, kTestPackageName);
  EXPECT_EQ(1u, events.size());
  EXPECT_TRUE(IsWithinDay(events[0].time, previous_fortnight));
  EXPECT_EQ(events[0].engagement, 1);

  // Now add the next date.
  AppBannerSettingsHelper::RecordBannerCouldShowEvent(
      web_contents(), url, kTestPackageName, three_days_prior,
      ui::PAGE_TRANSITION_GENERATED);

  // Now there should be two days.
  events = AppBannerSettingsHelper::GetCouldShowBannerEvents(
      web_contents(), url, kTestPackageName);
  EXPECT_EQ(2u, events.size());
  EXPECT_TRUE(IsWithinDay(events[0].time, previous_fortnight));
  EXPECT_TRUE(IsWithinDay(events[1].time, three_days_prior));
  EXPECT_EQ(events[0].engagement, 1);
  EXPECT_EQ(events[1].engagement, 1);

  // Now add the reference date.
  AppBannerSettingsHelper::RecordBannerCouldShowEvent(
      web_contents(), url, kTestPackageName, reference_time,
      ui::PAGE_TRANSITION_LINK);

  // Now there should still be two days, but the first date should have been
  // removed.
  events = AppBannerSettingsHelper::GetCouldShowBannerEvents(
      web_contents(), url, kTestPackageName);
  EXPECT_EQ(2u, events.size());
  EXPECT_TRUE(IsWithinDay(events[0].time, three_days_prior));
  EXPECT_TRUE(IsWithinDay(events[1].time, reference_time));
  EXPECT_EQ(events[0].engagement, 1);
  EXPECT_EQ(events[1].engagement, 1);

  // Now add the the other day on the reference date.
  AppBannerSettingsHelper::RecordBannerCouldShowEvent(
      web_contents(), url, kTestPackageName, same_day,
      ui::PAGE_TRANSITION_RELOAD);

  // Now there should still be the same two days.
  events = AppBannerSettingsHelper::GetCouldShowBannerEvents(
      web_contents(), url, kTestPackageName);
  EXPECT_EQ(2u, events.size());
  EXPECT_TRUE(IsWithinDay(events[0].time, three_days_prior));
  EXPECT_TRUE(IsWithinDay(events[1].time, reference_time));
  EXPECT_EQ(events[0].engagement, 1);
  EXPECT_EQ(events[1].engagement, 1);
}

TEST_F(AppBannerSettingsHelperTest, SingleEvents) {
  AppBannerSettingsHelper::SetEngagementWeights(1, 1);
  GURL url(kTestURL);
  NavigateAndCommit(url);

  base::Time reference_time = GetReferenceTime();
  base::Time other_time = reference_time - base::TimeDelta::FromDays(3);
  for (int event = AppBannerSettingsHelper::APP_BANNER_EVENT_DID_SHOW;
       event < AppBannerSettingsHelper::APP_BANNER_EVENT_NUM_EVENTS; ++event) {
    // Check that by default, there is no event.
    base::Time event_time = AppBannerSettingsHelper::GetSingleBannerEvent(
        web_contents(), url, kTestPackageName,
        AppBannerSettingsHelper::AppBannerEvent(event));
    EXPECT_TRUE(event_time.is_null());

    // Check that a time can be recorded.
    AppBannerSettingsHelper::RecordBannerEvent(
        web_contents(), url, kTestPackageName,
        AppBannerSettingsHelper::AppBannerEvent(event), reference_time);

    event_time = AppBannerSettingsHelper::GetSingleBannerEvent(
        web_contents(), url, kTestPackageName,
        AppBannerSettingsHelper::AppBannerEvent(event));
    EXPECT_EQ(reference_time, event_time);

    // Check that another time can be recorded.
    AppBannerSettingsHelper::RecordBannerEvent(
        web_contents(), url, kTestPackageName,
        AppBannerSettingsHelper::AppBannerEvent(event), other_time);

    event_time = AppBannerSettingsHelper::GetSingleBannerEvent(
        web_contents(), url, kTestPackageName,
        AppBannerSettingsHelper::AppBannerEvent(event));
    EXPECT_EQ(other_time, event_time);
  }
}

TEST_F(AppBannerSettingsHelperTest, CouldShowEventReplacedWithHigherWeight) {
  // Set direct engagement to be worth 4 and indirect to be worth 2.
  AppBannerSettingsHelper::SetEngagementWeights(4, 2);
  GURL url(kTestURL);
  NavigateAndCommit(url);

  base::Time reference_time = GetReferenceTime();
  base::Time later_same_day = reference_time + base::TimeDelta::FromHours(2);
  base::Time later_again_same_day =
      reference_time + base::TimeDelta::FromHours(6);
  base::Time next_day = reference_time + base::TimeDelta::FromDays(1);
  base::Time later_next_day = next_day + base::TimeDelta::FromHours(3);

  // Ensure there are no events recorded by default.
  std::vector<AppBannerSettingsHelper::BannerEvent> events =
      AppBannerSettingsHelper::GetCouldShowBannerEvents(web_contents(), url,
                                                        kTestPackageName);
  EXPECT_TRUE(events.empty());

  // Record an indirect engagement type.
  AppBannerSettingsHelper::RecordBannerCouldShowEvent(
      web_contents(), url, kTestPackageName, reference_time,
      ui::PAGE_TRANSITION_LINK);

  events = AppBannerSettingsHelper::GetCouldShowBannerEvents(
      web_contents(), url, kTestPackageName);

  EXPECT_EQ(1u, events.size());
  EXPECT_TRUE(IsWithinHour(events[0].time, reference_time));
  EXPECT_EQ(2, events[0].engagement);

  // Record a direct engagement type. This should override the previous value.
  AppBannerSettingsHelper::RecordBannerCouldShowEvent(
      web_contents(), url, kTestPackageName,
      later_same_day, ui::PAGE_TRANSITION_TYPED);

  events = AppBannerSettingsHelper::GetCouldShowBannerEvents(
      web_contents(), url, kTestPackageName);

  EXPECT_EQ(1u, events.size());
  EXPECT_TRUE(IsWithinHour(events[0].time, later_same_day));
  EXPECT_EQ(4, events[0].engagement);

  // Record an indirect engagement type. This should be ignored.
  AppBannerSettingsHelper::RecordBannerCouldShowEvent(
      web_contents(), url, kTestPackageName,
      later_again_same_day, ui::PAGE_TRANSITION_RELOAD);

  events = AppBannerSettingsHelper::GetCouldShowBannerEvents(
      web_contents(), url, kTestPackageName);

  EXPECT_EQ(1u, events.size());
  EXPECT_TRUE(IsWithinHour(events[0].time, later_same_day));
  EXPECT_EQ(4, events[0].engagement);

  // Record an indirect engagement type one day later. This should appear.
  AppBannerSettingsHelper::RecordBannerCouldShowEvent(
      web_contents(), url, kTestPackageName, next_day,
      ui::PAGE_TRANSITION_AUTO_BOOKMARK);

  events = AppBannerSettingsHelper::GetCouldShowBannerEvents(
      web_contents(), url, kTestPackageName);

  EXPECT_EQ(2u, events.size());
  EXPECT_TRUE(IsWithinHour(events[0].time, later_same_day));
  EXPECT_EQ(4, events[0].engagement);
  EXPECT_TRUE(IsWithinHour(events[1].time, next_day));
  EXPECT_EQ(2, events[1].engagement);

  // Record a direct engagement type later on the next day. This should override
  // the previous value.
  AppBannerSettingsHelper::RecordBannerCouldShowEvent(
      web_contents(), url, kTestPackageName, later_next_day,
      ui::PAGE_TRANSITION_GENERATED);

  events = AppBannerSettingsHelper::GetCouldShowBannerEvents(
      web_contents(), url, kTestPackageName);

  EXPECT_EQ(2u, events.size());
  EXPECT_TRUE(IsWithinHour(events[0].time, later_same_day));
  EXPECT_EQ(4, events[0].engagement);
  EXPECT_TRUE(IsWithinHour(events[1].time, later_next_day));
  EXPECT_EQ(4, events[1].engagement);
}

TEST_F(AppBannerSettingsHelperTest, IndirectEngagementWithLowerWeight) {
  AppBannerSettingsHelper::SetEngagementWeights(2, 0.5);
  GURL url(kTestURL);
  NavigateAndCommit(url);

  base::Time reference_time = GetReferenceTime();
  base::Time second_day = reference_time + base::TimeDelta::FromDays(1);
  base::Time third_day = reference_time + base::TimeDelta::FromDays(2);
  base::Time fourth_day = reference_time + base::TimeDelta::FromDays(3);

  // By default the banner should not be shown.
  EXPECT_FALSE(AppBannerSettingsHelper::ShouldShowBanner(
      web_contents(), url, kTestPackageName, reference_time));

  // It should take four indirect visits with a weight of 0.5 to trigger the
  // banner.
  AppBannerSettingsHelper::RecordBannerCouldShowEvent(
      web_contents(), url, kTestPackageName, reference_time,
      ui::PAGE_TRANSITION_LINK);
  EXPECT_FALSE(AppBannerSettingsHelper::ShouldShowBanner(
      web_contents(), url, kTestPackageName, reference_time));

  AppBannerSettingsHelper::RecordBannerCouldShowEvent(
      web_contents(), url, kTestPackageName, second_day,
      ui::PAGE_TRANSITION_LINK);
  EXPECT_FALSE(AppBannerSettingsHelper::ShouldShowBanner(
      web_contents(), url, kTestPackageName, second_day));

  AppBannerSettingsHelper::RecordBannerCouldShowEvent(
      web_contents(), url, kTestPackageName, third_day,
      ui::PAGE_TRANSITION_FORM_SUBMIT);
  EXPECT_FALSE(AppBannerSettingsHelper::ShouldShowBanner(
      web_contents(), url, kTestPackageName, third_day));

  // Visit the site again; now it should be shown.
  AppBannerSettingsHelper::RecordBannerCouldShowEvent(
      web_contents(), url, kTestPackageName, fourth_day,
      ui::PAGE_TRANSITION_MANUAL_SUBFRAME);
  EXPECT_TRUE(AppBannerSettingsHelper::ShouldShowBanner(
      web_contents(), url, kTestPackageName, fourth_day));
}

TEST_F(AppBannerSettingsHelperTest, DirectEngagementWithHigherWeight) {
  AppBannerSettingsHelper::SetEngagementWeights(2, 0.5);
  GURL url(kTestURL);
  NavigateAndCommit(url);

  base::Time reference_time = GetReferenceTime();

  // By default the banner should not be shown.
  EXPECT_FALSE(AppBannerSettingsHelper::ShouldShowBanner(
      web_contents(), url, kTestPackageName, reference_time));

  // It should take one direct visit with a weight of 2 to trigger the banner.
  AppBannerSettingsHelper::RecordBannerCouldShowEvent(
      web_contents(), url, kTestPackageName, reference_time,
      ui::PAGE_TRANSITION_TYPED);
  EXPECT_TRUE(AppBannerSettingsHelper::ShouldShowBanner(
      web_contents(), url, kTestPackageName, reference_time));
}

TEST_F(AppBannerSettingsHelperTest, ShouldShowFromEngagement) {
  AppBannerSettingsHelper::SetEngagementWeights(1, 1);
  GURL url(kTestURL);
  NavigateAndCommit(url);

  base::Time reference_time = GetReferenceTime();
  base::Time one_day_ago = reference_time - base::TimeDelta::FromDays(1);
  base::Time one_year_ago = reference_time - base::TimeDelta::FromDays(366);

  // By default the banner should not be shown.
  EXPECT_FALSE(AppBannerSettingsHelper::ShouldShowBanner(
      web_contents(), url, kTestPackageName, reference_time));

  // Visit the site once, it still should not be shown.
  AppBannerSettingsHelper::RecordBannerCouldShowEvent(
      web_contents(), url, kTestPackageName, one_year_ago,
      ui::PAGE_TRANSITION_TYPED);
  EXPECT_FALSE(AppBannerSettingsHelper::ShouldShowBanner(
      web_contents(), url, kTestPackageName, reference_time));

  // Visit the site again after a long delay, it still should not be shown.
  AppBannerSettingsHelper::RecordBannerCouldShowEvent(
      web_contents(), url, kTestPackageName, one_day_ago,
      ui::PAGE_TRANSITION_TYPED);
  EXPECT_FALSE(AppBannerSettingsHelper::ShouldShowBanner(
      web_contents(), url, kTestPackageName, reference_time));

  // Visit the site again; now it should be shown.
  AppBannerSettingsHelper::RecordBannerCouldShowEvent(
      web_contents(), url, kTestPackageName, reference_time,
      ui::PAGE_TRANSITION_TYPED);
  EXPECT_TRUE(AppBannerSettingsHelper::ShouldShowBanner(
      web_contents(), url, kTestPackageName, reference_time));
}

TEST_F(AppBannerSettingsHelperTest, ShouldNotShowAfterBlocking) {
  AppBannerSettingsHelper::SetEngagementWeights(1, 1);
  GURL url(kTestURL);
  NavigateAndCommit(url);

  base::Time reference_time = GetReferenceTime();
  base::Time one_day_ago = reference_time - base::TimeDelta::FromDays(1);
  base::Time two_months_ago = reference_time - base::TimeDelta::FromDays(60);
  base::Time one_year_ago = reference_time - base::TimeDelta::FromDays(366);

  // By default the banner should not be shown.
  EXPECT_FALSE(AppBannerSettingsHelper::ShouldShowBanner(
      web_contents(), url, kTestPackageName, reference_time));

  // Record events such that the banner should show.
  AppBannerSettingsHelper::RecordBannerCouldShowEvent(
      web_contents(), url, kTestPackageName, one_day_ago,
      ui::PAGE_TRANSITION_TYPED);
  AppBannerSettingsHelper::RecordBannerCouldShowEvent(
      web_contents(), url, kTestPackageName, reference_time,
      ui::PAGE_TRANSITION_TYPED);
  EXPECT_TRUE(AppBannerSettingsHelper::ShouldShowBanner(
      web_contents(), url, kTestPackageName, reference_time));

  // Block the site a long time ago. It should still be shown.
  AppBannerSettingsHelper::RecordBannerEvent(
      web_contents(), url, kTestPackageName,
      AppBannerSettingsHelper::APP_BANNER_EVENT_DID_BLOCK, one_year_ago);
  EXPECT_TRUE(AppBannerSettingsHelper::ShouldShowBanner(
      web_contents(), url, kTestPackageName, reference_time));

  // Block the site more recently. Now it should not be shown.
  AppBannerSettingsHelper::RecordBannerEvent(
      web_contents(), url, kTestPackageName,
      AppBannerSettingsHelper::APP_BANNER_EVENT_DID_BLOCK, two_months_ago);
  EXPECT_FALSE(AppBannerSettingsHelper::ShouldShowBanner(
      web_contents(), url, kTestPackageName, reference_time));
}

TEST_F(AppBannerSettingsHelperTest, ShouldNotShowAfterShowing) {
  AppBannerSettingsHelper::SetEngagementWeights(1, 1);
  GURL url(kTestURL);
  NavigateAndCommit(url);

  base::Time reference_time = GetReferenceTime();
  base::Time one_day_ago = reference_time - base::TimeDelta::FromDays(1);
  base::Time three_weeks_ago = reference_time - base::TimeDelta::FromDays(21);
  base::Time one_year_ago = reference_time - base::TimeDelta::FromDays(366);

  // By default the banner should not be shown.
  EXPECT_FALSE(AppBannerSettingsHelper::ShouldShowBanner(
      web_contents(), url, kTestPackageName, reference_time));

  // Record events such that the banner should show.
  AppBannerSettingsHelper::RecordBannerCouldShowEvent(
      web_contents(), url, kTestPackageName, one_day_ago,
      ui::PAGE_TRANSITION_TYPED);
  AppBannerSettingsHelper::RecordBannerCouldShowEvent(
      web_contents(), url, kTestPackageName, reference_time,
      ui::PAGE_TRANSITION_TYPED);
  EXPECT_TRUE(AppBannerSettingsHelper::ShouldShowBanner(
      web_contents(), url, kTestPackageName, reference_time));

  // Show the banner a long time ago. It should still be shown.
  AppBannerSettingsHelper::RecordBannerEvent(
      web_contents(), url, kTestPackageName,
      AppBannerSettingsHelper::APP_BANNER_EVENT_DID_SHOW, one_year_ago);
  EXPECT_TRUE(AppBannerSettingsHelper::ShouldShowBanner(
      web_contents(), url, kTestPackageName, reference_time));

  // Show the site more recently. Now it should not be shown.
  AppBannerSettingsHelper::RecordBannerEvent(
      web_contents(), url, kTestPackageName,
      AppBannerSettingsHelper::APP_BANNER_EVENT_DID_SHOW, three_weeks_ago);
  EXPECT_FALSE(AppBannerSettingsHelper::ShouldShowBanner(
      web_contents(), url, kTestPackageName, reference_time));
}

TEST_F(AppBannerSettingsHelperTest, ShouldNotShowAfterAdding) {
  AppBannerSettingsHelper::SetEngagementWeights(1, 1);
  GURL url(kTestURL);
  NavigateAndCommit(url);

  base::Time reference_time = GetReferenceTime();
  base::Time one_day_ago = reference_time - base::TimeDelta::FromDays(1);
  base::Time one_year_ago = reference_time - base::TimeDelta::FromDays(366);

  // By default the banner should not be shown.
  EXPECT_FALSE(AppBannerSettingsHelper::ShouldShowBanner(
      web_contents(), url, kTestPackageName, reference_time));

  // Record events such that the banner should show.
  AppBannerSettingsHelper::RecordBannerCouldShowEvent(
      web_contents(), url, kTestPackageName, one_day_ago,
      ui::PAGE_TRANSITION_TYPED);
  AppBannerSettingsHelper::RecordBannerCouldShowEvent(
      web_contents(), url, kTestPackageName, reference_time,
      ui::PAGE_TRANSITION_TYPED);
  EXPECT_TRUE(AppBannerSettingsHelper::ShouldShowBanner(
      web_contents(), url, kTestPackageName, reference_time));

  // Add the site a long time ago. It should not be shown.
  AppBannerSettingsHelper::RecordBannerEvent(
      web_contents(), url, kTestPackageName,
      AppBannerSettingsHelper::APP_BANNER_EVENT_DID_ADD_TO_HOMESCREEN,
      one_year_ago);
  EXPECT_FALSE(AppBannerSettingsHelper::ShouldShowBanner(
      web_contents(), url, kTestPackageName, reference_time));
}

TEST_F(AppBannerSettingsHelperTest, OperatesOnOrigins) {
  AppBannerSettingsHelper::SetEngagementWeights(1, 1);
  GURL url(kTestURL);
  NavigateAndCommit(url);

  base::Time reference_time = GetReferenceTime();
  base::Time one_day_ago = reference_time - base::TimeDelta::FromDays(1);

  // By default the banner should not be shown.
  EXPECT_FALSE(AppBannerSettingsHelper::ShouldShowBanner(
      web_contents(), url, kTestPackageName, reference_time));

  // Record events such that the banner should show.
  AppBannerSettingsHelper::RecordBannerCouldShowEvent(
      web_contents(), url, kTestPackageName, one_day_ago,
      ui::PAGE_TRANSITION_TYPED);
  AppBannerSettingsHelper::RecordBannerCouldShowEvent(
      web_contents(), url, kTestPackageName, reference_time,
      ui::PAGE_TRANSITION_TYPED);

  // Navigate to another page on the same origin.
  url = GURL(kSameOriginTestURL);
  NavigateAndCommit(url);

  // The banner should show as settings are per-origin.
  EXPECT_TRUE(AppBannerSettingsHelper::ShouldShowBanner(
      web_contents(), url, kTestPackageName, reference_time));
}
