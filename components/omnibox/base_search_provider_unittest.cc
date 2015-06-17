// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/base_search_provider.h"

#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "components/omnibox/autocomplete_match.h"
#include "components/omnibox/autocomplete_match_type.h"
#include "components/omnibox/autocomplete_provider_client.h"
#include "components/omnibox/autocomplete_scheme_classifier.h"
#include "components/omnibox/search_suggestion_parser.h"
#include "components/omnibox/suggestion_answer.h"
#include "components/search_engines/search_terms_data.h"
#include "components/search_engines/template_url_service.h"
#include "components/search_engines/template_url_service_client.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::NiceMock;
using testing::Return;
using testing::_;

class MockAutocompleteProviderClient : public AutocompleteProviderClient {
 public:
  MockAutocompleteProviderClient() {}
  MOCK_METHOD0(GetRequestContext, net::URLRequestContextGetter*());
  MOCK_METHOD0(GetSchemeClassifier, const AutocompleteSchemeClassifier&());
  MOCK_METHOD0(GetHistoryService, history::HistoryService*());
  MOCK_METHOD0(GetBookmarkModel, bookmarks::BookmarkModel*());
  MOCK_METHOD0(GetInMemoryDatabase, history::URLDatabase*());
  MOCK_METHOD0(GetTemplateURLService, TemplateURLService*());
  MOCK_METHOD0(GetSearchTermsData, const SearchTermsData&());
  MOCK_METHOD0(GetAcceptLanguages, std::string());
  MOCK_METHOD0(IsOffTheRecord, bool());
  MOCK_METHOD0(SearchSuggestEnabled, bool());
  MOCK_METHOD0(ShowBookmarkBar, bool());
  MOCK_METHOD0(TabSyncEnabledAndUnencrypted, bool());
  MOCK_METHOD6(
      Classify,
      void(const base::string16& text,
           bool prefer_keyword,
           bool allow_exact_keyword_match,
           metrics::OmniboxEventProto::PageClassification page_classification,
           AutocompleteMatch* match,
           GURL* alternate_nav_url));
  MOCK_METHOD2(DeleteMatchingURLsForKeywordFromHistory,
               void(history::KeywordID keyword_id, const base::string16& term));
  MOCK_METHOD1(PrefetchImage, void(const GURL& url));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockAutocompleteProviderClient);
};

class TestBaseSearchProvider : public BaseSearchProvider {
 public:
  typedef BaseSearchProvider::MatchMap MatchMap;

  TestBaseSearchProvider(AutocompleteProvider::Type type,
                         AutocompleteProviderClient* client,
                         TemplateURLService* template_url_service)
      : BaseSearchProvider(type, client, template_url_service) {}
  MOCK_METHOD1(DeleteMatch, void(const AutocompleteMatch& match));
  MOCK_CONST_METHOD1(AddProviderInfo, void(ProvidersInfo* provider_info));
  MOCK_CONST_METHOD1(GetTemplateURL, const TemplateURL*(bool is_keyword));
  MOCK_CONST_METHOD1(GetInput, const AutocompleteInput(bool is_keyword));
  MOCK_CONST_METHOD1(ShouldAppendExtraParams,
                     bool(const SearchSuggestionParser::SuggestResult& result));
  MOCK_METHOD1(RecordDeletionResult, void(bool success));

  MOCK_METHOD3(Start,
               void(const AutocompleteInput& input, bool minimal_changes,
                    bool called_due_to_focus));
  void AddMatchToMap(const SearchSuggestionParser::SuggestResult& result,
                     const std::string& metadata,
                     int accepted_suggestion,
                     bool mark_as_deletable,
                     bool in_keyword_mode,
                     MatchMap* map) {
    BaseSearchProvider::AddMatchToMap(result,
                                      metadata,
                                      accepted_suggestion,
                                      mark_as_deletable,
                                      in_keyword_mode,
                                      map);
  }

 protected:
  virtual ~TestBaseSearchProvider() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(TestBaseSearchProvider);
};

class BaseSearchProviderTest : public testing::Test {
 public:
  ~BaseSearchProviderTest() override {}

 protected:
  void SetUp() override {
    service_.reset(
        new TemplateURLService(NULL,
                               scoped_ptr<SearchTermsData>(new SearchTermsData),
                               NULL,
                               scoped_ptr<TemplateURLServiceClient>(),
                               NULL,
                               NULL,
                               base::Closure()));
    client_.reset(new NiceMock<MockAutocompleteProviderClient>());
    provider_ = new NiceMock<TestBaseSearchProvider>(
        AutocompleteProvider::TYPE_SEARCH, client_.get(), service_.get());
  }

  scoped_refptr<NiceMock<TestBaseSearchProvider> > provider_;
  scoped_ptr<TemplateURLService> service_;
  scoped_ptr<NiceMock<MockAutocompleteProviderClient>> client_;
};

TEST_F(BaseSearchProviderTest, PreserveAnswersWhenDeduplicating) {
  TemplateURLData data;
  data.SetURL("http://foo.com/url?bar={searchTerms}");
  scoped_ptr<TemplateURL> template_url(new TemplateURL(data));

  TestBaseSearchProvider::MatchMap map;
  base::string16 query = base::ASCIIToUTF16("weather los angeles");
  base::string16 answer_contents = base::ASCIIToUTF16("some answer content");
  base::string16 answer_type = base::ASCIIToUTF16("2334");
  scoped_ptr<SuggestionAnswer> answer(new SuggestionAnswer());
  answer->set_type(2334);

  EXPECT_CALL(*provider_, GetInput(_))
      .WillRepeatedly(Return(AutocompleteInput()));
  EXPECT_CALL(*provider_, GetTemplateURL(_))
      .WillRepeatedly(Return(template_url.get()));

  SearchSuggestionParser::SuggestResult more_relevant(
      query, AutocompleteMatchType::SEARCH_HISTORY, query, base::string16(),
      base::string16(), base::string16(), base::string16(), nullptr,
      std::string(), std::string(), false, 1300, true, false, query);
  provider_->AddMatchToMap(
      more_relevant, std::string(), TemplateURLRef::NO_SUGGESTION_CHOSEN,
      false, false, &map);

  SearchSuggestionParser::SuggestResult less_relevant(
      query, AutocompleteMatchType::SEARCH_SUGGEST, query, base::string16(),
      base::string16(), answer_contents, answer_type,
      SuggestionAnswer::copy(answer.get()), std::string(), std::string(),
      false, 850, true, false, query);
  provider_->AddMatchToMap(
      less_relevant, std::string(), TemplateURLRef::NO_SUGGESTION_CHOSEN,
      false, false, &map);

  ASSERT_EQ(1U, map.size());
  AutocompleteMatch match = map.begin()->second;
  ASSERT_EQ(1U, match.duplicate_matches.size());
  AutocompleteMatch duplicate = match.duplicate_matches[0];

  EXPECT_EQ(answer_contents, match.answer_contents);
  EXPECT_EQ(answer_type, match.answer_type);
  EXPECT_TRUE(answer->Equals(*match.answer));
  EXPECT_EQ(AutocompleteMatchType::SEARCH_HISTORY, match.type);
  EXPECT_EQ(1300, match.relevance);

  EXPECT_EQ(answer_contents, duplicate.answer_contents);
  EXPECT_EQ(answer_type, duplicate.answer_type);
  EXPECT_TRUE(answer->Equals(*duplicate.answer));
  EXPECT_EQ(AutocompleteMatchType::SEARCH_SUGGEST, duplicate.type);
  EXPECT_EQ(850, duplicate.relevance);

  // Ensure answers are not copied over existing answers.
  map.clear();
  base::string16 answer_contents2 = base::ASCIIToUTF16("different answer");
  base::string16 answer_type2 = base::ASCIIToUTF16("8242");
  scoped_ptr<SuggestionAnswer> answer2(new SuggestionAnswer());
  answer2->set_type(8242);
  more_relevant = SearchSuggestionParser::SuggestResult(
      query, AutocompleteMatchType::SEARCH_HISTORY, query, base::string16(),
      base::string16(), answer_contents2, answer_type2,
      SuggestionAnswer::copy(answer2.get()), std::string(), std::string(),
      false, 1300, true, false, query);
  provider_->AddMatchToMap(
      more_relevant, std::string(), TemplateURLRef::NO_SUGGESTION_CHOSEN,
      false, false, &map);
  provider_->AddMatchToMap(
      less_relevant, std::string(), TemplateURLRef::NO_SUGGESTION_CHOSEN,
      false, false, &map);
  ASSERT_EQ(1U, map.size());
  match = map.begin()->second;
  ASSERT_EQ(1U, match.duplicate_matches.size());
  duplicate = match.duplicate_matches[0];

  EXPECT_EQ(answer_contents2, match.answer_contents);
  EXPECT_EQ(answer_type2, match.answer_type);
  EXPECT_TRUE(answer2->Equals(*match.answer));
  EXPECT_EQ(AutocompleteMatchType::SEARCH_HISTORY, match.type);
  EXPECT_EQ(1300, match.relevance);

  EXPECT_EQ(answer_contents, duplicate.answer_contents);
  EXPECT_EQ(answer_type, duplicate.answer_type);
  EXPECT_TRUE(answer->Equals(*duplicate.answer));
  EXPECT_EQ(AutocompleteMatchType::SEARCH_SUGGEST, duplicate.type);
  EXPECT_EQ(850, duplicate.relevance);
}
