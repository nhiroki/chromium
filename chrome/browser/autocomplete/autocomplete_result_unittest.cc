// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/autocomplete/autocomplete_result.h"

#include "base/memory/scoped_ptr.h"
#include "base/metrics/field_trial.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/autocomplete/autocomplete_input.h"
#include "chrome/browser/autocomplete/autocomplete_match.h"
#include "chrome/browser/autocomplete/autocomplete_provider.h"
#include "chrome/browser/omnibox/omnibox_field_trial.h"
#include "chrome/browser/search_engines/template_url_prepopulate_data.h"
#include "chrome/browser/search_engines/template_url_service.h"
#include "chrome/browser/search_engines/template_url_service_test_util.h"
#include "chrome/common/autocomplete_match_type.h"
#include "chrome/common/metrics/variations/variations_util.h"
#include "chrome/test/base/testing_profile.h"
#include "components/variations/entropy_provider.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// Creates an AutocompleteMatch using |destination_url| and |type| and appends
// it to |matches|.
void AddMatch(const std::string& destination_url, AutocompleteMatch::Type type,
              ACMatches* matches) {
  ASSERT_TRUE(matches != NULL);
  AutocompleteMatch* last_match =
    !matches->empty() ? &((*matches)[matches->size() - 1]) : NULL;
  AutocompleteMatch match;
  match.destination_url = GURL(destination_url);
  match.relevance = last_match ? last_match->relevance - 100 : 1300;
  match.allowed_to_be_default_match = true;
  match.type = type;
  matches->push_back(match);
}

}  // namespace

class AutocompleteResultTest : public testing::Test  {
 public:
  struct TestData {
    // Used to build a url for the AutocompleteMatch. The URL becomes
    // "http://" + ('a' + |url_id|) (e.g. an ID of 2 yields "http://b").
    int url_id;

    // ID of the provider.
    int provider_id;

    // Relevance score.
    int relevance;
  };

  AutocompleteResultTest() {
    // Destroy the existing FieldTrialList before creating a new one to avoid
    // a DCHECK.
    field_trial_list_.reset();
    field_trial_list_.reset(new base::FieldTrialList(
        new metrics::SHA1EntropyProvider("foo")));
    chrome_variations::testing::ClearAllVariationParams();
  }

  virtual void SetUp() OVERRIDE {
#if defined(OS_ANDROID)
    TemplateURLPrepopulateData::InitCountryCode(
        std::string() /* unknown country code */);
#endif
    test_util_.SetUp();
    test_util_.VerifyLoad();
  }

  virtual void TearDown() OVERRIDE {
    test_util_.TearDown();
  }

  // Configures |match| from |data|.
  static void PopulateAutocompleteMatch(const TestData& data,
                                        AutocompleteMatch* match);

  // Adds |count| AutocompleteMatches to |matches|.
  static void PopulateAutocompleteMatches(const TestData* data,
                                          size_t count,
                                          ACMatches* matches);

  // Asserts that |result| has |expected_count| matches matching |expected|.
  void AssertResultMatches(const AutocompleteResult& result,
                           const TestData* expected,
                           size_t expected_count);

  // Creates an AutocompleteResult from |last| and |current|. The two are
  // merged by |CopyOldMatches| and compared by |AssertResultMatches|.
  void RunCopyOldMatchesTest(const TestData* last, size_t last_size,
                             const TestData* current, size_t current_size,
                             const TestData* expected, size_t expected_size);

 protected:
  TemplateURLServiceTestUtil test_util_;

 private:
  scoped_ptr<base::FieldTrialList> field_trial_list_;

  DISALLOW_COPY_AND_ASSIGN(AutocompleteResultTest);
};

// static
void AutocompleteResultTest::PopulateAutocompleteMatch(
    const TestData& data,
    AutocompleteMatch* match) {
  match->provider = reinterpret_cast<AutocompleteProvider*>(data.provider_id);
  match->fill_into_edit = base::IntToString16(data.url_id);
  std::string url_id(1, data.url_id + 'a');
  match->destination_url = GURL("http://" + url_id);
  match->relevance = data.relevance;
  match->allowed_to_be_default_match = true;
}

// static
void AutocompleteResultTest::PopulateAutocompleteMatches(
    const TestData* data,
    size_t count,
    ACMatches* matches) {
  for (size_t i = 0; i < count; ++i) {
    AutocompleteMatch match;
    PopulateAutocompleteMatch(data[i], &match);
    matches->push_back(match);
  }
}

void AutocompleteResultTest::AssertResultMatches(
    const AutocompleteResult& result,
    const TestData* expected,
    size_t expected_count) {
  ASSERT_EQ(expected_count, result.size());
  for (size_t i = 0; i < expected_count; ++i) {
    AutocompleteMatch expected_match;
    PopulateAutocompleteMatch(expected[i], &expected_match);
    const AutocompleteMatch& match = *(result.begin() + i);
    EXPECT_EQ(expected_match.provider, match.provider) << i;
    EXPECT_EQ(expected_match.relevance, match.relevance) << i;
    EXPECT_EQ(expected_match.destination_url.spec(),
              match.destination_url.spec()) << i;
  }
}

void AutocompleteResultTest::RunCopyOldMatchesTest(
    const TestData* last, size_t last_size,
    const TestData* current, size_t current_size,
    const TestData* expected, size_t expected_size) {
  AutocompleteInput input(ASCIIToUTF16("a"), base::string16::npos,
                          base::string16(), GURL(),
                          AutocompleteInput::INVALID_SPEC, false, false, false,
                          AutocompleteInput::ALL_MATCHES);

  ACMatches last_matches;
  PopulateAutocompleteMatches(last, last_size, &last_matches);
  AutocompleteResult last_result;
  last_result.AppendMatches(last_matches);
  last_result.SortAndCull(input, test_util_.profile());

  ACMatches current_matches;
  PopulateAutocompleteMatches(current, current_size, &current_matches);
  AutocompleteResult current_result;
  current_result.AppendMatches(current_matches);
  current_result.SortAndCull(input, test_util_.profile());
  current_result.CopyOldMatches(input, last_result, test_util_.profile());

  AssertResultMatches(current_result, expected, expected_size);
}

// Assertion testing for AutocompleteResult::Swap.
TEST_F(AutocompleteResultTest, Swap) {
  AutocompleteResult r1;
  AutocompleteResult r2;

  // Swap with empty shouldn't do anything interesting.
  r1.Swap(&r2);
  EXPECT_EQ(r1.end(), r1.default_match());
  EXPECT_EQ(r2.end(), r2.default_match());

  // Swap with a single match.
  ACMatches matches;
  AutocompleteMatch match;
  match.relevance = 1;
  match.allowed_to_be_default_match = true;
  AutocompleteInput input(ASCIIToUTF16("a"), base::string16::npos,
                          base::string16(), GURL(),
                          AutocompleteInput::INVALID_SPEC, false, false, false,
                          AutocompleteInput::ALL_MATCHES);
  matches.push_back(match);
  r1.AppendMatches(matches);
  r1.SortAndCull(input, test_util_.profile());
  EXPECT_EQ(r1.begin(), r1.default_match());
  EXPECT_EQ("http://a/", r1.alternate_nav_url().spec());
  r1.Swap(&r2);
  EXPECT_TRUE(r1.empty());
  EXPECT_EQ(r1.end(), r1.default_match());
  EXPECT_TRUE(r1.alternate_nav_url().is_empty());
  ASSERT_FALSE(r2.empty());
  EXPECT_EQ(r2.begin(), r2.default_match());
  EXPECT_EQ("http://a/", r2.alternate_nav_url().spec());
}

// Tests that if the new results have a lower max relevance score than last,
// any copied results have their relevance shifted down.
TEST_F(AutocompleteResultTest, CopyOldMatches) {
  TestData last[] = {
    { 0, 0, 1000 },
    { 1, 0, 500 },
  };
  TestData current[] = {
    { 2, 0, 400 },
  };
  TestData result[] = {
    { 2, 0, 400 },
    { 1, 0, 399 },
  };

  ASSERT_NO_FATAL_FAILURE(
      RunCopyOldMatchesTest(last, ARRAYSIZE_UNSAFE(last),
                            current, ARRAYSIZE_UNSAFE(current),
                            result, ARRAYSIZE_UNSAFE(result)));
}

// Tests that matches are copied correctly from two distinct providers.
TEST_F(AutocompleteResultTest, CopyOldMatches2) {
  TestData last[] = {
    { 0, 0, 1000 },
    { 1, 1, 500 },
    { 2, 0, 400 },
    { 3, 1, 300 },
  };
  TestData current[] = {
    { 4, 0, 1100 },
    { 5, 1, 550 },
  };
  TestData result[] = {
    { 4, 0, 1100 },
    { 5, 1, 550 },
    { 2, 0, 400 },
    { 3, 1, 300 },
  };

  ASSERT_NO_FATAL_FAILURE(
      RunCopyOldMatchesTest(last, ARRAYSIZE_UNSAFE(last),
                            current, ARRAYSIZE_UNSAFE(current),
                            result, ARRAYSIZE_UNSAFE(result)));
}

// Tests that matches with empty destination URLs aren't treated as duplicates
// and culled.
TEST_F(AutocompleteResultTest, SortAndCullEmptyDestinationURLs) {
  TestData data[] = {
    { 1, 0, 500 },
    { 0, 0, 1100 },
    { 1, 0, 1000 },
    { 0, 0, 1300 },
    { 0, 0, 1200 },
  };

  ACMatches matches;
  PopulateAutocompleteMatches(data, arraysize(data), &matches);
  matches[1].destination_url = GURL();
  matches[3].destination_url = GURL();
  matches[4].destination_url = GURL();

  AutocompleteResult result;
  result.AppendMatches(matches);
  AutocompleteInput input(base::string16(), base::string16::npos,
                          base::string16(), GURL(),
                          AutocompleteInput::INVALID_SPEC, false, false, false,
                          AutocompleteInput::ALL_MATCHES);
  result.SortAndCull(input, test_util_.profile());

  // Of the two results with the same non-empty destination URL, the
  // lower-relevance one should be dropped.  All of the results with empty URLs
  // should be kept.
  ASSERT_EQ(4U, result.size());
  EXPECT_TRUE(result.match_at(0)->destination_url.is_empty());
  EXPECT_EQ(1300, result.match_at(0)->relevance);
  EXPECT_TRUE(result.match_at(1)->destination_url.is_empty());
  EXPECT_EQ(1200, result.match_at(1)->relevance);
  EXPECT_TRUE(result.match_at(2)->destination_url.is_empty());
  EXPECT_EQ(1100, result.match_at(2)->relevance);
  EXPECT_EQ("http://b/", result.match_at(3)->destination_url.spec());
  EXPECT_EQ(1000, result.match_at(3)->relevance);
}

TEST_F(AutocompleteResultTest, SortAndCullDuplicateSearchURLs) {
  // Register a template URL that corresponds to 'foo' search engine.
  TemplateURLData url_data;
  url_data.short_name = ASCIIToUTF16("unittest");
  url_data.SetKeyword(ASCIIToUTF16("foo"));
  url_data.SetURL("http://www.foo.com/s?q={searchTerms}");
  test_util_.model()->Add(new TemplateURL(test_util_.profile(), url_data));

  TestData data[] = {
    { 0, 0, 1300 },
    { 1, 0, 1200 },
    { 2, 0, 1100 },
    { 3, 0, 1000 },
    { 4, 1, 900 },
  };

  ACMatches matches;
  PopulateAutocompleteMatches(data, arraysize(data), &matches);
  matches[0].destination_url = GURL("http://www.foo.com/s?q=foo");
  matches[1].destination_url = GURL("http://www.foo.com/s?q=foo2");
  matches[2].destination_url = GURL("http://www.foo.com/s?q=foo&oq=f");
  matches[3].destination_url = GURL("http://www.foo.com/s?q=foo&aqs=0");
  matches[4].destination_url = GURL("http://www.foo.com/");

  AutocompleteResult result;
  result.AppendMatches(matches);
  AutocompleteInput input(base::string16(), base::string16::npos,
                          base::string16(), GURL(),
                          AutocompleteInput::INVALID_SPEC, false, false, false,
                          AutocompleteInput::ALL_MATCHES);
  result.SortAndCull(input, test_util_.profile());

  // We expect the 3rd and 4th results to be removed.
  ASSERT_EQ(3U, result.size());
  EXPECT_EQ("http://www.foo.com/s?q=foo",
            result.match_at(0)->destination_url.spec());
  EXPECT_EQ(1300, result.match_at(0)->relevance);
  EXPECT_EQ("http://www.foo.com/s?q=foo2",
            result.match_at(1)->destination_url.spec());
  EXPECT_EQ(1200, result.match_at(1)->relevance);
  EXPECT_EQ("http://www.foo.com/",
            result.match_at(2)->destination_url.spec());
  EXPECT_EQ(900, result.match_at(2)->relevance);
}

TEST_F(AutocompleteResultTest, SortAndCullWithDemotionsByType) {
  // Add some matches.
  ACMatches matches;
  AddMatch("http://history-url/", AutocompleteMatchType::HISTORY_URL, &matches);
  AddMatch("http://search-what-you-typed/",
           AutocompleteMatchType::SEARCH_WHAT_YOU_TYPED, &matches);
  AddMatch("http://history-title/", AutocompleteMatchType::HISTORY_TITLE,
           &matches);

  // Add a search history type match and demote its relevance score.
  AddMatch("http://search-history/", AutocompleteMatchType::SEARCH_HISTORY,
           &matches);
  matches[matches.size() - 1].relevance = 500;

  // Add a rule demoting history-url and killing history-title.
  {
    std::map<std::string, std::string> params;
    params[std::string(OmniboxFieldTrial::kDemoteByTypeRule) + ":3:*"] =
        "1:50,7:100,2:0";  // 3 == HOME_PAGE
    ASSERT_TRUE(chrome_variations::AssociateVariationParams(
        OmniboxFieldTrial::kBundledExperimentFieldTrialName, "A", params));
  }
  base::FieldTrialList::CreateFieldTrial(
      OmniboxFieldTrial::kBundledExperimentFieldTrialName, "A");

  AutocompleteResult result;
  result.AppendMatches(matches);
  AutocompleteInput input(base::string16(), base::string16::npos,
                          base::string16(), GURL(),
                          AutocompleteInput::HOME_PAGE, false, false, false,
                          AutocompleteInput::ALL_MATCHES);
  result.SortAndCull(input, test_util_.profile());

  // Check the new ordering.  The history-title results should be omitted.
  // We cannot check relevance scores because the matches are sorted by
  // demoted relevance but the actual relevance scores are not modified.
  ASSERT_EQ(3u, result.size());
  EXPECT_EQ("http://search-what-you-typed/",
            result.match_at(0)->destination_url.spec());
  EXPECT_EQ("http://history-url/",
            result.match_at(1)->destination_url.spec());
  EXPECT_EQ("http://search-history/",
            result.match_at(2)->destination_url.spec());
}

TEST_F(AutocompleteResultTest, SortAndCullWithUndemotableTypes) {
  // Add some matches.
  ACMatches matches(3);
  matches[0].destination_url = GURL("http://top-history-url/");
  matches[0].relevance = 1400;
  matches[0].allowed_to_be_default_match = true;
  matches[0].type = AutocompleteMatchType::HISTORY_URL;
  matches[1].destination_url = GURL("http://history-url2/");
  matches[1].relevance = 1300;
  matches[1].allowed_to_be_default_match = true;
  matches[1].type = AutocompleteMatchType::HISTORY_URL;
  matches[2].destination_url = GURL("http://search-what-you-typed/");
  matches[2].relevance = 1200;
  matches[2].allowed_to_be_default_match = true;
  matches[2].type = AutocompleteMatchType::SEARCH_WHAT_YOU_TYPED;

  // Add a rule demoting history-url, but don't demote the top match.
  {
    std::map<std::string, std::string> params;
    // 3 == HOME_PAGE
    params[std::string(OmniboxFieldTrial::kDemoteByTypeRule) + ":3:*"] =
        "1:50";
    params[std::string(OmniboxFieldTrial::kUndemotableTopTypeRule) + ":3:*"] =
        "1,5";
    ASSERT_TRUE(chrome_variations::AssociateVariationParams(
        OmniboxFieldTrial::kBundledExperimentFieldTrialName, "B", params));
  }
  base::FieldTrialList::CreateFieldTrial(
      OmniboxFieldTrial::kBundledExperimentFieldTrialName, "B");

  AutocompleteResult result;
  result.AppendMatches(matches);
  AutocompleteInput input(base::string16(), base::string16::npos,
                          base::string16(), GURL(),
                          AutocompleteInput::HOME_PAGE, false, false, false,
                          AutocompleteInput::ALL_MATCHES);
  result.SortAndCull(input, test_util_.profile());

  // Check the new ordering.  The first history-url result should not be
  // demoted, but the second result should be.
  // We cannot check relevance scores because the matches are sorted by
  // demoted relevance but the actual relevance scores are not modified.
  ASSERT_EQ(3u, result.size());
  EXPECT_EQ("http://top-history-url/",
            result.match_at(0)->destination_url.spec());
  EXPECT_EQ("http://search-what-you-typed/",
            result.match_at(1)->destination_url.spec());
  EXPECT_EQ("http://history-url2/",
            result.match_at(2)->destination_url.spec());
}

TEST_F(AutocompleteResultTest, SortAndCullReorderForDefaultMatch) {
  TestData data[] = {
    { 0, 0, 1300 },
    { 1, 0, 1200 },
    { 2, 0, 1100 },
    { 3, 0, 1000 }
  };

  {
    // Check that reorder doesn't do anything if the top result
    // is already a legal default match (which is the default from
    // PopulateAutocompleteMatches()).
    ACMatches matches;
    PopulateAutocompleteMatches(data, arraysize(data), &matches);
    AutocompleteResult result;
    result.AppendMatches(matches);
    AutocompleteInput input(base::string16(), base::string16::npos,
                            base::string16(), GURL(),
                            AutocompleteInput::HOME_PAGE, false, false, false,
                            AutocompleteInput::ALL_MATCHES);
    result.SortAndCull(input, test_util_.profile());
    AssertResultMatches(result, data, 4);
  }

  {
    // Check that reorder swaps up a result appropriately.
    ACMatches matches;
    PopulateAutocompleteMatches(data, arraysize(data), &matches);
    matches[0].allowed_to_be_default_match = false;
    matches[1].allowed_to_be_default_match = false;
    AutocompleteResult result;
    result.AppendMatches(matches);
    AutocompleteInput input(base::string16(), base::string16::npos,
                            base::string16(), GURL(),
                            AutocompleteInput::HOME_PAGE, false, false, false,
                            AutocompleteInput::ALL_MATCHES);
    result.SortAndCull(input, test_util_.profile());
    ASSERT_EQ(4U, result.size());
    EXPECT_EQ("http://c/", result.match_at(0)->destination_url.spec());
    EXPECT_EQ("http://a/", result.match_at(1)->destination_url.spec());
    EXPECT_EQ("http://b/", result.match_at(2)->destination_url.spec());
    EXPECT_EQ("http://d/", result.match_at(3)->destination_url.spec());
  }
}

TEST_F(AutocompleteResultTest, ShouldHideTopMatch) {
  // Add some matches.
  ACMatches matches;
  AddMatch("http://search-what-you-typed/",
           AutocompleteMatchType::SEARCH_WHAT_YOU_TYPED, &matches);
  AddMatch("http://history-title/", AutocompleteMatchType::HISTORY_TITLE,
           &matches);
  AddMatch("http://search-history/", AutocompleteMatchType::SEARCH_HISTORY,
           &matches);

  base::FieldTrialList::CreateFieldTrial("InstantExtended",
                                         "Group1 hide_verbatim:1");
  AutocompleteResult result;
  result.AppendMatches(matches);
  EXPECT_TRUE(result.ShouldHideTopMatch());
}

TEST_F(AutocompleteResultTest, DoNotHideTopMatch) {
  ACMatches matches;
  AddMatch("http://search-what-you-typed/",
           AutocompleteMatchType::SEARCH_WHAT_YOU_TYPED, &matches);
  AddMatch("http://url-what-you-typed/",
           AutocompleteMatchType::URL_WHAT_YOU_TYPED, &matches);
  AddMatch("http://history-title/", AutocompleteMatchType::HISTORY_TITLE,
           &matches);
  AddMatch("http://search-history/", AutocompleteMatchType::SEARCH_HISTORY,
           &matches);

  base::FieldTrialList::CreateFieldTrial("InstantExtended",
                                         "Group1 hide_verbatim:1");
  AutocompleteResult result;
  result.AppendMatches(matches);
  // If the verbatim first match is followed by another verbatim match, don't
  // hide the top verbatim match.
  EXPECT_FALSE(result.ShouldHideTopMatch());
}

TEST_F(AutocompleteResultTest, DoNotHideTopMatch_TopMatchIsNotVerbatim) {
  ACMatches matches;
  AddMatch("http://search-history/", AutocompleteMatchType::SEARCH_HISTORY,
           &matches);
  AddMatch("http://search-what-you-typed/",
           AutocompleteMatchType::SEARCH_WHAT_YOU_TYPED, &matches);
  AddMatch("http://history-title/", AutocompleteMatchType::HISTORY_TITLE,
           &matches);

  base::FieldTrialList::CreateFieldTrial("InstantExtended",
                                         "Group1 hide_verbatim:1");
  AutocompleteResult result;
  result.AppendMatches(matches);
  // Top match is not a verbatim type match. Do not hide the top match.
  EXPECT_FALSE(result.ShouldHideTopMatch());
}

TEST_F(AutocompleteResultTest, DoNotHideTopMatch_FieldTrialFlagDisabled) {
  // Add some matches. This test config is identical to ShouldHideTopMatch test
  // except that the "hide_verbatim" flag is disabled in the field trials.
  ACMatches matches;
  AddMatch("http://search-what-you-typed/",
           AutocompleteMatchType::SEARCH_WHAT_YOU_TYPED, &matches);
  AddMatch("http://history-title/", AutocompleteMatchType::HISTORY_TITLE,
           &matches);
  AddMatch("http://search-history/", AutocompleteMatchType::SEARCH_HISTORY,
           &matches);

  base::FieldTrialList::CreateFieldTrial("InstantExtended",
                                         "Group1 hide_verbatim:0");
  AutocompleteResult result;
  result.AppendMatches(matches);
  // Field trial flag "hide_verbatim" is disabled. Do not hide top match.
  EXPECT_FALSE(result.ShouldHideTopMatch());
}

TEST_F(AutocompleteResultTest,
       TopMatchIsVerbatimAndHasNoConsecutiveVerbatimMatches) {
  ACMatches matches;
  AddMatch("http://url-what-you-typed/",
           AutocompleteMatchType::URL_WHAT_YOU_TYPED, &matches);
  AddMatch("http://history-title/", AutocompleteMatchType::HISTORY_TITLE,
           &matches);

  AutocompleteResult result;
  result.AppendMatches(matches);
  EXPECT_TRUE(result.TopMatchIsVerbatimAndHasNoConsecutiveVerbatimMatches());
}

TEST_F(AutocompleteResultTest,
       TopMatchIsVerbatimAndHasConsecutiveVerbatimMatches) {
  ACMatches matches;
  AddMatch("http://search-what-you-typed/",
           AutocompleteMatchType::SEARCH_WHAT_YOU_TYPED, &matches);
  AddMatch("http://url-what-you-typed/",
           AutocompleteMatchType::URL_WHAT_YOU_TYPED, &matches);
  AddMatch("http://history-title/", AutocompleteMatchType::HISTORY_TITLE,
           &matches);

  AutocompleteResult result;
  result.AppendMatches(matches);
  EXPECT_FALSE(result.TopMatchIsVerbatimAndHasNoConsecutiveVerbatimMatches());
}

TEST_F(AutocompleteResultTest, TopMatchIsNotVerbatim) {
  ACMatches matches;
  AutocompleteResult result;
  result.AppendMatches(matches);

  // Result set is empty.
  EXPECT_FALSE(result.TopMatchIsVerbatimAndHasNoConsecutiveVerbatimMatches());

  // Add a non-verbatim match to the result.
  AddMatch("http://history-title/", AutocompleteMatchType::HISTORY_TITLE,
           &matches);

  result.AppendMatches(matches);
  EXPECT_FALSE(result.TopMatchIsVerbatimAndHasNoConsecutiveVerbatimMatches());
}

TEST_F(AutocompleteResultTest,
       TopMatchIsVerbatimAndHasNoConsecutiveVerbatimMatches_SingleMatchFound) {
  ACMatches matches;
  AddMatch("http://url-what-you-typed/",
           AutocompleteMatchType::URL_WHAT_YOU_TYPED, &matches);

  AutocompleteResult result;
  result.AppendMatches(matches);
  EXPECT_TRUE(result.TopMatchIsVerbatimAndHasNoConsecutiveVerbatimMatches());
}
