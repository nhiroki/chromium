// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <fstream>

#include "base/auto_reset.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/history/history_backend.h"
#include "chrome/browser/history/history_database.h"
#include "chrome/browser/history/history_notifications.h"
#include "chrome/browser/history/history_service.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/history/in_memory_url_index.h"
#include "chrome/browser/history/in_memory_url_index_types.h"
#include "chrome/browser/history/url_index_private_data.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/history_index_restore_observer.h"
#include "chrome/test/base/testing_profile.h"
#include "components/bookmarks/test/bookmark_test_helpers.h"
#include "components/history/core/browser/history_client.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_source.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "sql/transaction.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::ASCIIToUTF16;

namespace {
const size_t kMaxMatches = 3;
}  // namespace

// The test version of the history url database table ('url') is contained in
// a database file created from a text file('url_history_provider_test.db.txt').
// The only difference between this table and a live 'urls' table from a
// profile is that the last_visit_time column in the test table contains a
// number specifying the number of days relative to 'today' to which the
// absolute time should be set during the test setup stage.
//
// The format of the test database text file is of a SQLite .dump file.
// Note that only lines whose first character is an upper-case letter are
// processed when creating the test database.

namespace history {

// -----------------------------------------------------------------------------

// Observer class so the unit tests can wait while the cache is being saved.
class CacheFileSaverObserver : public InMemoryURLIndex::SaveCacheObserver {
 public:
  explicit CacheFileSaverObserver(const base::Closure& task);

  bool succeeded() { return succeeded_; }

 private:
  // SaveCacheObserver implementation.
  virtual void OnCacheSaveFinished(bool succeeded) OVERRIDE;

  base::Closure task_;
  bool succeeded_;

  DISALLOW_COPY_AND_ASSIGN(CacheFileSaverObserver);
};

CacheFileSaverObserver::CacheFileSaverObserver(const base::Closure& task)
    : task_(task),
      succeeded_(false) {
}

void CacheFileSaverObserver::OnCacheSaveFinished(bool succeeded) {
  succeeded_ = succeeded;
  task_.Run();
}

// -----------------------------------------------------------------------------

class InMemoryURLIndexTest : public testing::Test {
 public:
  InMemoryURLIndexTest();

 protected:
  // Test setup.
  virtual void SetUp();

  // Allows the database containing the test data to be customized by
  // subclasses.
  virtual base::FilePath::StringType TestDBName() const;

  // Validates that the given |term| is contained in |cache| and that it is
  // marked as in-use.
  void CheckTerm(const URLIndexPrivateData::SearchTermCacheMap& cache,
                 base::string16 term) const;

  // Pass-through function to simplify our friendship with HistoryService.
  sql::Connection& GetDB();

  // Pass-through functions to simplify our friendship with InMemoryURLIndex.
  URLIndexPrivateData* GetPrivateData() const;
  base::CancelableTaskTracker* GetPrivateDataTracker() const;
  void ClearPrivateData();
  void set_history_dir(const base::FilePath& dir_path);
  bool GetCacheFilePath(base::FilePath* file_path) const;
  void PostRestoreFromCacheFileTask();
  void PostSaveToCacheFileTask();
  void Observe(int notification_type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details);
  const std::set<std::string>& scheme_whitelist();


  // Pass-through functions to simplify our friendship with URLIndexPrivateData.
  bool UpdateURL(const URLRow& row);
  bool DeleteURL(const GURL& url);

  // Data verification helper functions.
  void ExpectPrivateDataNotEmpty(const URLIndexPrivateData& data);
  void ExpectPrivateDataEmpty(const URLIndexPrivateData& data);
  void ExpectPrivateDataEqual(const URLIndexPrivateData& expected,
                              const URLIndexPrivateData& actual);

  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfile profile_;
  HistoryService* history_service_;

  scoped_ptr<InMemoryURLIndex> url_index_;
  HistoryDatabase* history_database_;
};

InMemoryURLIndexTest::InMemoryURLIndexTest() {
}

sql::Connection& InMemoryURLIndexTest::GetDB() {
  return history_database_->GetDB();
}

URLIndexPrivateData* InMemoryURLIndexTest::GetPrivateData() const {
  DCHECK(url_index_->private_data());
  return url_index_->private_data();
}

base::CancelableTaskTracker* InMemoryURLIndexTest::GetPrivateDataTracker()
    const {
  DCHECK(url_index_->private_data_tracker());
  return url_index_->private_data_tracker();
}

void InMemoryURLIndexTest::ClearPrivateData() {
  return url_index_->ClearPrivateData();
}

void InMemoryURLIndexTest::set_history_dir(const base::FilePath& dir_path) {
  return url_index_->set_history_dir(dir_path);
}

bool InMemoryURLIndexTest::GetCacheFilePath(base::FilePath* file_path) const {
  DCHECK(file_path);
  return url_index_->GetCacheFilePath(file_path);
}

void InMemoryURLIndexTest::PostRestoreFromCacheFileTask() {
  url_index_->PostRestoreFromCacheFileTask();
}

void InMemoryURLIndexTest::PostSaveToCacheFileTask() {
  url_index_->PostSaveToCacheFileTask();
}

void InMemoryURLIndexTest::Observe(
    int notification_type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  url_index_->Observe(notification_type, source, details);
}

const std::set<std::string>& InMemoryURLIndexTest::scheme_whitelist() {
  return url_index_->scheme_whitelist();
}

bool InMemoryURLIndexTest::UpdateURL(const URLRow& row) {
  return GetPrivateData()->UpdateURL(history_service_,
                                     row,
                                     url_index_->languages_,
                                     url_index_->scheme_whitelist_,
                                     GetPrivateDataTracker());
}

bool InMemoryURLIndexTest::DeleteURL(const GURL& url) {
  return GetPrivateData()->DeleteURL(url);
}

void InMemoryURLIndexTest::SetUp() {
  // We cannot access the database until the backend has been loaded.
  ASSERT_TRUE(profile_.CreateHistoryService(true, false));
  profile_.CreateBookmarkModel(true);
  test::WaitForBookmarkModelToLoad(
      BookmarkModelFactory::GetForProfile(&profile_));
  profile_.BlockUntilHistoryProcessesPendingRequests();
  profile_.BlockUntilHistoryIndexIsRefreshed();
  history_service_ = HistoryServiceFactory::GetForProfile(
      &profile_, Profile::EXPLICIT_ACCESS);
  ASSERT_TRUE(history_service_);
  HistoryBackend* backend = history_service_->history_backend_.get();
  history_database_ = backend->db();

  // Create and populate a working copy of the URL history database.
  base::FilePath history_proto_path;
  PathService::Get(chrome::DIR_TEST_DATA, &history_proto_path);
  history_proto_path = history_proto_path.Append(
      FILE_PATH_LITERAL("History"));
  history_proto_path = history_proto_path.Append(TestDBName());
  EXPECT_TRUE(base::PathExists(history_proto_path));

  std::ifstream proto_file(history_proto_path.value().c_str());
  static const size_t kCommandBufferMaxSize = 2048;
  char sql_cmd_line[kCommandBufferMaxSize];

  sql::Connection& db(GetDB());
  ASSERT_TRUE(db.is_open());
  {
    sql::Transaction transaction(&db);
    transaction.Begin();
    while (!proto_file.eof()) {
      proto_file.getline(sql_cmd_line, kCommandBufferMaxSize);
      if (!proto_file.eof()) {
        // We only process lines which begin with a upper-case letter.
        // TODO(mrossetti): Can iswupper() be used here?
        if (sql_cmd_line[0] >= 'A' && sql_cmd_line[0] <= 'Z') {
          std::string sql_cmd(sql_cmd_line);
          sql::Statement sql_stmt(db.GetUniqueStatement(sql_cmd_line));
          EXPECT_TRUE(sql_stmt.Run());
        }
      }
    }
    transaction.Commit();
  }

  // Update the last_visit_time table column in the "urls" table
  // such that it represents a time relative to 'now'.
  sql::Statement statement(db.GetUniqueStatement(
      "SELECT" HISTORY_URL_ROW_FIELDS "FROM urls;"));
  ASSERT_TRUE(statement.is_valid());
  base::Time time_right_now = base::Time::NowFromSystemTime();
  base::TimeDelta day_delta = base::TimeDelta::FromDays(1);
  {
    sql::Transaction transaction(&db);
    transaction.Begin();
    while (statement.Step()) {
      URLRow row;
      history_database_->FillURLRow(statement, &row);
      base::Time last_visit = time_right_now;
      for (int64 i = row.last_visit().ToInternalValue(); i > 0; --i)
        last_visit -= day_delta;
      row.set_last_visit(last_visit);
      history_database_->UpdateURLRow(row.id(), row);
    }
    transaction.Commit();
  }

  // Update the visit_time table column in the "visits" table
  // such that it represents a time relative to 'now'.
  statement.Assign(db.GetUniqueStatement(
      "SELECT" HISTORY_VISIT_ROW_FIELDS "FROM visits;"));
  ASSERT_TRUE(statement.is_valid());
  {
    sql::Transaction transaction(&db);
    transaction.Begin();
    while (statement.Step()) {
      VisitRow row;
      history_database_->FillVisitRow(statement, &row);
      base::Time last_visit = time_right_now;
      for (int64 i = row.visit_time.ToInternalValue(); i > 0; --i)
        last_visit -= day_delta;
      row.visit_time = last_visit;
      history_database_->UpdateVisitRow(row);
    }
    transaction.Commit();
  }

  url_index_.reset(new InMemoryURLIndex(
      &profile_, base::FilePath(), "en,ja,hi,zh",
      history_service_->history_client()));
  url_index_->Init();
  url_index_->RebuildFromHistory(history_database_);
}

base::FilePath::StringType InMemoryURLIndexTest::TestDBName() const {
    return FILE_PATH_LITERAL("url_history_provider_test.db.txt");
}

void InMemoryURLIndexTest::CheckTerm(
    const URLIndexPrivateData::SearchTermCacheMap& cache,
    base::string16 term) const {
  URLIndexPrivateData::SearchTermCacheMap::const_iterator cache_iter(
      cache.find(term));
  ASSERT_TRUE(cache.end() != cache_iter)
      << "Cache does not contain '" << term << "' but should.";
  URLIndexPrivateData::SearchTermCacheItem cache_item = cache_iter->second;
  EXPECT_TRUE(cache_item.used_)
      << "Cache item '" << term << "' should be marked as being in use.";
}

void InMemoryURLIndexTest::ExpectPrivateDataNotEmpty(
    const URLIndexPrivateData& data) {
  EXPECT_FALSE(data.word_list_.empty());
  // available_words_ will be empty since we have freshly built the
  // data set for these tests.
  EXPECT_TRUE(data.available_words_.empty());
  EXPECT_FALSE(data.word_map_.empty());
  EXPECT_FALSE(data.char_word_map_.empty());
  EXPECT_FALSE(data.word_id_history_map_.empty());
  EXPECT_FALSE(data.history_id_word_map_.empty());
  EXPECT_FALSE(data.history_info_map_.empty());
}

void InMemoryURLIndexTest::ExpectPrivateDataEmpty(
    const URLIndexPrivateData& data) {
  EXPECT_TRUE(data.word_list_.empty());
  EXPECT_TRUE(data.available_words_.empty());
  EXPECT_TRUE(data.word_map_.empty());
  EXPECT_TRUE(data.char_word_map_.empty());
  EXPECT_TRUE(data.word_id_history_map_.empty());
  EXPECT_TRUE(data.history_id_word_map_.empty());
  EXPECT_TRUE(data.history_info_map_.empty());
}

// Helper function which compares two maps for equivalence. The maps' values
// are associative containers and their contents are compared as well.
template<typename T>
void ExpectMapOfContainersIdentical(const T& expected, const T& actual) {
  ASSERT_EQ(expected.size(), actual.size());
  for (typename T::const_iterator expected_iter = expected.begin();
       expected_iter != expected.end(); ++expected_iter) {
    typename T::const_iterator actual_iter = actual.find(expected_iter->first);
    ASSERT_TRUE(actual.end() != actual_iter);
    typename T::mapped_type const& expected_values(expected_iter->second);
    typename T::mapped_type const& actual_values(actual_iter->second);
    ASSERT_EQ(expected_values.size(), actual_values.size());
    for (typename T::mapped_type::const_iterator set_iter =
         expected_values.begin(); set_iter != expected_values.end(); ++set_iter)
      EXPECT_EQ(actual_values.count(*set_iter),
                expected_values.count(*set_iter));
  }
}

void InMemoryURLIndexTest::ExpectPrivateDataEqual(
    const URLIndexPrivateData& expected,
    const URLIndexPrivateData& actual) {
  EXPECT_EQ(expected.word_list_.size(), actual.word_list_.size());
  EXPECT_EQ(expected.word_map_.size(), actual.word_map_.size());
  EXPECT_EQ(expected.char_word_map_.size(), actual.char_word_map_.size());
  EXPECT_EQ(expected.word_id_history_map_.size(),
            actual.word_id_history_map_.size());
  EXPECT_EQ(expected.history_id_word_map_.size(),
            actual.history_id_word_map_.size());
  EXPECT_EQ(expected.history_info_map_.size(), actual.history_info_map_.size());
  EXPECT_EQ(expected.word_starts_map_.size(), actual.word_starts_map_.size());
  // WordList must be index-by-index equal.
  size_t count = expected.word_list_.size();
  for (size_t i = 0; i < count; ++i)
    EXPECT_EQ(expected.word_list_[i], actual.word_list_[i]);

  ExpectMapOfContainersIdentical(expected.char_word_map_,
                                 actual.char_word_map_);
  ExpectMapOfContainersIdentical(expected.word_id_history_map_,
                                 actual.word_id_history_map_);
  ExpectMapOfContainersIdentical(expected.history_id_word_map_,
                                 actual.history_id_word_map_);

  for (HistoryInfoMap::const_iterator expected_info =
      expected.history_info_map_.begin();
      expected_info != expected.history_info_map_.end(); ++expected_info) {
    HistoryInfoMap::const_iterator actual_info =
        actual.history_info_map_.find(expected_info->first);
    // NOTE(yfriedman): ASSERT_NE can't be used due to incompatibility between
    // gtest and STLPort in the Android build. See
    // http://code.google.com/p/googletest/issues/detail?id=359
    ASSERT_TRUE(actual_info != actual.history_info_map_.end());
    const URLRow& expected_row(expected_info->second.url_row);
    const URLRow& actual_row(actual_info->second.url_row);
    EXPECT_EQ(expected_row.visit_count(), actual_row.visit_count());
    EXPECT_EQ(expected_row.typed_count(), actual_row.typed_count());
    EXPECT_EQ(expected_row.last_visit(), actual_row.last_visit());
    EXPECT_EQ(expected_row.url(), actual_row.url());
    const VisitInfoVector& expected_visits(expected_info->second.visits);
    const VisitInfoVector& actual_visits(actual_info->second.visits);
    EXPECT_EQ(expected_visits.size(), actual_visits.size());
    for (size_t i = 0;
         i < std::min(expected_visits.size(), actual_visits.size()); ++i) {
      EXPECT_EQ(expected_visits[i].first, actual_visits[i].first);
      EXPECT_EQ(expected_visits[i].second, actual_visits[i].second);
    }
  }

  for (WordStartsMap::const_iterator expected_starts =
      expected.word_starts_map_.begin();
      expected_starts != expected.word_starts_map_.end();
      ++expected_starts) {
    WordStartsMap::const_iterator actual_starts =
        actual.word_starts_map_.find(expected_starts->first);
    // NOTE(yfriedman): ASSERT_NE can't be used due to incompatibility between
    // gtest and STLPort in the Android build. See
    // http://code.google.com/p/googletest/issues/detail?id=359
    ASSERT_TRUE(actual_starts != actual.word_starts_map_.end());
    const RowWordStarts& expected_word_starts(expected_starts->second);
    const RowWordStarts& actual_word_starts(actual_starts->second);
    EXPECT_EQ(expected_word_starts.url_word_starts_.size(),
              actual_word_starts.url_word_starts_.size());
    EXPECT_TRUE(std::equal(expected_word_starts.url_word_starts_.begin(),
                           expected_word_starts.url_word_starts_.end(),
                           actual_word_starts.url_word_starts_.begin()));
    EXPECT_EQ(expected_word_starts.title_word_starts_.size(),
              actual_word_starts.title_word_starts_.size());
    EXPECT_TRUE(std::equal(expected_word_starts.title_word_starts_.begin(),
                           expected_word_starts.title_word_starts_.end(),
                           actual_word_starts.title_word_starts_.begin()));
  }
}

//------------------------------------------------------------------------------

class LimitedInMemoryURLIndexTest : public InMemoryURLIndexTest {
 protected:
  virtual base::FilePath::StringType TestDBName() const OVERRIDE;
};

base::FilePath::StringType LimitedInMemoryURLIndexTest::TestDBName() const {
  return FILE_PATH_LITERAL("url_history_provider_test_limited.db.txt");
}

TEST_F(LimitedInMemoryURLIndexTest, Initialization) {
  // Verify that the database contains the expected number of items, which
  // is the pre-filtered count, i.e. all of the items.
  sql::Statement statement(GetDB().GetUniqueStatement("SELECT * FROM urls;"));
  ASSERT_TRUE(statement.is_valid());
  uint64 row_count = 0;
  while (statement.Step()) ++row_count;
  EXPECT_EQ(1U, row_count);
  url_index_.reset(new InMemoryURLIndex(
      &profile_, base::FilePath(), "en,ja,hi,zh",
      history_service_->history_client()));
  url_index_->Init();
  url_index_->RebuildFromHistory(history_database_);
  URLIndexPrivateData& private_data(*GetPrivateData());

  // history_info_map_ should have the same number of items as were filtered.
  EXPECT_EQ(1U, private_data.history_info_map_.size());
  EXPECT_EQ(35U, private_data.char_word_map_.size());
  EXPECT_EQ(17U, private_data.word_map_.size());
}

#if defined(OS_WIN)
// Flaky on windows trybots: http://crbug.com/351500
#define MAYBE_Retrieval DISABLED_Retrieval
#else
#define MAYBE_Retrieval Retrieval
#endif
TEST_F(InMemoryURLIndexTest, MAYBE_Retrieval) {
  // See if a very specific term gives a single result.
  ScoredHistoryMatches matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("DrudgeReport"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(1U, matches.size());

  // Verify that we got back the result we expected.
  EXPECT_EQ(5, matches[0].url_info.id());
  EXPECT_EQ("http://drudgereport.com/", matches[0].url_info.url().spec());
  EXPECT_EQ(ASCIIToUTF16("DRUDGE REPORT 2010"), matches[0].url_info.title());
  EXPECT_TRUE(matches[0].can_inline());

  // Make sure a trailing space prevents inline-ability but still results
  // in the expected result.
  matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("DrudgeReport "), base::string16::npos, kMaxMatches);
  ASSERT_EQ(1U, matches.size());
  EXPECT_EQ(5, matches[0].url_info.id());
  EXPECT_EQ("http://drudgereport.com/", matches[0].url_info.url().spec());
  EXPECT_EQ(ASCIIToUTF16("DRUDGE REPORT 2010"), matches[0].url_info.title());
  EXPECT_FALSE(matches[0].can_inline());

  // Search which should result in multiple results.
  matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("drudge"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(2U, matches.size());
  // The results should be in descending score order.
  EXPECT_GE(matches[0].raw_score(), matches[1].raw_score());

  // Search which should result in nearly perfect result.
  matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("Nearly Perfect Result"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(1U, matches.size());
  // The results should have a very high score.
  EXPECT_GT(matches[0].raw_score(), 900);
  EXPECT_EQ(32, matches[0].url_info.id());
  EXPECT_EQ("https://nearlyperfectresult.com/",
            matches[0].url_info.url().spec());  // Note: URL gets lowercased.
  EXPECT_EQ(ASCIIToUTF16("Practically Perfect Search Result"),
            matches[0].url_info.title());
  EXPECT_FALSE(matches[0].can_inline());

  // Search which should result in very poor result.
  matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("qui c"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(1U, matches.size());
  // The results should have a poor score.
  EXPECT_LT(matches[0].raw_score(), 500);
  EXPECT_EQ(33, matches[0].url_info.id());
  EXPECT_EQ("http://quiteuselesssearchresultxyz.com/",
            matches[0].url_info.url().spec());  // Note: URL gets lowercased.
  EXPECT_EQ(ASCIIToUTF16("Practically Useless Search Result"),
            matches[0].url_info.title());
  EXPECT_FALSE(matches[0].can_inline());

  // Search which will match at the end of an URL with encoded characters.
  matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("Mice"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(1U, matches.size());
  EXPECT_EQ(30, matches[0].url_info.id());
  EXPECT_FALSE(matches[0].can_inline());

  // Check that URLs are not escaped an escape time.
  matches = url_index_->HistoryItemsForTerms(
       ASCIIToUTF16("1% wikipedia"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(1U, matches.size());
  EXPECT_EQ(35, matches[0].url_info.id());
  EXPECT_EQ("http://en.wikipedia.org/wiki/1%25_rule_(Internet_culture)",
            matches[0].url_info.url().spec());

  // Verify that a single term can appear multiple times in the URL and as long
  // as one starts the URL it is still inlined.
  matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("fubar"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(1U, matches.size());
  EXPECT_EQ(34, matches[0].url_info.id());
  EXPECT_EQ("http://fubarfubarandfubar.com/", matches[0].url_info.url().spec());
  EXPECT_EQ(ASCIIToUTF16("Situation Normal -- FUBARED"),
            matches[0].url_info.title());
  EXPECT_TRUE(matches[0].can_inline());
}

TEST_F(InMemoryURLIndexTest, CursorPositionRetrieval) {
  // See if a very specific term with no cursor gives an empty result.
  ScoredHistoryMatches matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("DrudReport"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(0U, matches.size());

  // The same test with the cursor at the end should give an empty result.
  matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("DrudReport"), 10u, kMaxMatches);
  ASSERT_EQ(0U, matches.size());

  // If the cursor is between Drud and Report, we should find the desired
  // result.
  matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("DrudReport"), 4u, kMaxMatches);
  ASSERT_EQ(1U, matches.size());
  EXPECT_EQ("http://drudgereport.com/", matches[0].url_info.url().spec());
  EXPECT_EQ(ASCIIToUTF16("DRUDGE REPORT 2010"), matches[0].url_info.title());

  // Now check multi-word inputs.  No cursor should fail to find a
  // result on this input.
  matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("MORTGAGERATE DROPS"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(0U, matches.size());

  // Ditto with cursor at end.
  matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("MORTGAGERATE DROPS"), 18u, kMaxMatches);
  ASSERT_EQ(0U, matches.size());

  // If the cursor is between MORTAGE And RATE, we should find the
  // desired result.
  matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("MORTGAGERATE DROPS"), 8u, kMaxMatches);
  ASSERT_EQ(1U, matches.size());
  EXPECT_EQ("http://www.reuters.com/article/idUSN0839880620100708",
            matches[0].url_info.url().spec());
  EXPECT_EQ(ASCIIToUTF16(
      "UPDATE 1-US 30-yr mortgage rate drops to new record low | Reuters"),
            matches[0].url_info.title());
}

TEST_F(InMemoryURLIndexTest, URLPrefixMatching) {
  // "drudgere" - found, can inline
  ScoredHistoryMatches matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("drudgere"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(1U, matches.size());
  EXPECT_TRUE(matches[0].can_inline());

  // "drudgere" - found, can inline
  matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("drudgere"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(1U, matches.size());
  EXPECT_TRUE(matches[0].can_inline());

  // "www.atdmt" - not found
  matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("www.atdmt"), base::string16::npos, kMaxMatches);
  EXPECT_EQ(0U, matches.size());

  // "atdmt" - found, cannot inline
  matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("atdmt"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(1U, matches.size());
  EXPECT_FALSE(matches[0].can_inline());

  // "view.atdmt" - found, can inline
  matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("view.atdmt"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(1U, matches.size());
  EXPECT_TRUE(matches[0].can_inline());

  // "view.atdmt" - found, can inline
  matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("view.atdmt"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(1U, matches.size());
  EXPECT_TRUE(matches[0].can_inline());

  // "cnn.com" - found, can inline
  matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("cnn.com"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(2U, matches.size());
  // One match should be inline-able, the other not.
  EXPECT_TRUE(matches[0].can_inline() != matches[1].can_inline());

  // "www.cnn.com" - found, can inline
  matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("www.cnn.com"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(1U, matches.size());
  EXPECT_TRUE(matches[0].can_inline());

  // "ww.cnn.com" - found because we allow mid-term matches in hostnames
  matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("ww.cnn.com"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(1U, matches.size());

  // "www.cnn.com" - found, can inline
  matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("www.cnn.com"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(1U, matches.size());
  EXPECT_TRUE(matches[0].can_inline());

  // "tp://www.cnn.com" - not found because we don't allow tp as a mid-term
  // match
  matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("tp://www.cnn.com"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(0U, matches.size());
}

TEST_F(InMemoryURLIndexTest, ProperStringMatching) {
  // Search for the following with the expected results:
  // "atdmt view" - found
  // "atdmt.view" - not found
  // "view.atdmt" - found
  ScoredHistoryMatches matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("atdmt view"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(1U, matches.size());
  matches = url_index_->HistoryItemsForTerms(
       ASCIIToUTF16("atdmt.view"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(0U, matches.size());
  matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("view.atdmt"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(1U, matches.size());
}

TEST_F(InMemoryURLIndexTest, HugeResultSet) {
  // Create a huge set of qualifying history items.
  for (URLID row_id = 5000; row_id < 6000; ++row_id) {
    URLRow new_row(GURL("http://www.brokeandaloneinmanitoba.com/"), row_id);
    new_row.set_last_visit(base::Time::Now());
    EXPECT_TRUE(UpdateURL(new_row));
  }

  ScoredHistoryMatches matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("b"), base::string16::npos, kMaxMatches);
  URLIndexPrivateData& private_data(*GetPrivateData());
  ASSERT_EQ(kMaxMatches, matches.size());
  // There are 7 matches already in the database.
  ASSERT_EQ(1008U, private_data.pre_filter_item_count_);
  ASSERT_EQ(500U, private_data.post_filter_item_count_);
  ASSERT_EQ(kMaxMatches, private_data.post_scoring_item_count_);
}

#if defined(OS_WIN)
// Flaky on windows trybots: http://crbug.com/351500
#define MAYBE_TitleSearch DISABLED_TitleSearch
#else
#define MAYBE_TitleSearch TitleSearch
#endif
TEST_F(InMemoryURLIndexTest, MAYBE_TitleSearch) {
  // Signal if someone has changed the test DB.
  EXPECT_EQ(29U, GetPrivateData()->history_info_map_.size());

  // Ensure title is being searched.
  ScoredHistoryMatches matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("MORTGAGE RATE DROPS"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(1U, matches.size());

  // Verify that we got back the result we expected.
  EXPECT_EQ(1, matches[0].url_info.id());
  EXPECT_EQ("http://www.reuters.com/article/idUSN0839880620100708",
            matches[0].url_info.url().spec());
  EXPECT_EQ(ASCIIToUTF16(
      "UPDATE 1-US 30-yr mortgage rate drops to new record low | Reuters"),
      matches[0].url_info.title());
}

TEST_F(InMemoryURLIndexTest, TitleChange) {
  // Verify current title terms retrieves desired item.
  base::string16 original_terms =
      ASCIIToUTF16("lebronomics could high taxes influence");
  ScoredHistoryMatches matches = url_index_->HistoryItemsForTerms(
      original_terms, base::string16::npos, kMaxMatches);
  ASSERT_EQ(1U, matches.size());

  // Verify that we got back the result we expected.
  const URLID expected_id = 3;
  EXPECT_EQ(expected_id, matches[0].url_info.id());
  EXPECT_EQ("http://www.businessandmedia.org/articles/2010/20100708120415.aspx",
            matches[0].url_info.url().spec());
  EXPECT_EQ(ASCIIToUTF16(
      "LeBronomics: Could High Taxes Influence James' Team Decision?"),
      matches[0].url_info.title());
  URLRow old_row(matches[0].url_info);

  // Verify new title terms retrieves nothing.
  base::string16 new_terms = ASCIIToUTF16("does eat oats little lambs ivy");
  matches = url_index_->HistoryItemsForTerms(
      new_terms, base::string16::npos, kMaxMatches);
  ASSERT_EQ(0U, matches.size());

  // Update the row.
  old_row.set_title(ASCIIToUTF16("Does eat oats and little lambs eat ivy"));
  EXPECT_TRUE(UpdateURL(old_row));

  // Verify we get the row using the new terms but not the original terms.
  matches = url_index_->HistoryItemsForTerms(
      new_terms, base::string16::npos, kMaxMatches);
  ASSERT_EQ(1U, matches.size());
  EXPECT_EQ(expected_id, matches[0].url_info.id());
  matches = url_index_->HistoryItemsForTerms(
      original_terms, base::string16::npos, kMaxMatches);
  ASSERT_EQ(0U, matches.size());
}

TEST_F(InMemoryURLIndexTest, NonUniqueTermCharacterSets) {
  // The presence of duplicate characters should succeed. Exercise by cycling
  // through a string with several duplicate characters.
  ScoredHistoryMatches matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("ABRA"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(1U, matches.size());
  EXPECT_EQ(28, matches[0].url_info.id());
  EXPECT_EQ("http://www.ddj.com/windows/184416623",
            matches[0].url_info.url().spec());

  matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("ABRACAD"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(1U, matches.size());
  EXPECT_EQ(28, matches[0].url_info.id());

  matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("ABRACADABRA"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(1U, matches.size());
  EXPECT_EQ(28, matches[0].url_info.id());

  matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("ABRACADABR"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(1U, matches.size());
  EXPECT_EQ(28, matches[0].url_info.id());

  matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("ABRACA"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(1U, matches.size());
  EXPECT_EQ(28, matches[0].url_info.id());
}

TEST_F(InMemoryURLIndexTest, TypedCharacterCaching) {
  // Verify that match results for previously typed characters are retained
  // (in the term_char_word_set_cache_) and reused, if possible, in future
  // autocompletes.

  URLIndexPrivateData::SearchTermCacheMap& cache(
      GetPrivateData()->search_term_cache_);

  // The cache should be empty at this point.
  EXPECT_EQ(0U, cache.size());

  // Now simulate typing search terms into the omnibox and check the state of
  // the cache as each item is 'typed'.

  // Simulate typing "r" giving "r" in the simulated omnibox. The results for
  // 'r' will be not cached because it is only 1 character long.
  url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("r"), base::string16::npos, kMaxMatches);
  EXPECT_EQ(0U, cache.size());

  // Simulate typing "re" giving "r re" in the simulated omnibox.
  // 're' should be cached at this point but not 'r' as it is a single
  // character.
  url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("r re"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(1U, cache.size());
  CheckTerm(cache, ASCIIToUTF16("re"));

  // Simulate typing "reco" giving "r re reco" in the simulated omnibox.
  // 're' and 'reco' should be cached at this point but not 'r' as it is a
  // single character.
  url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("r re reco"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(2U, cache.size());
  CheckTerm(cache, ASCIIToUTF16("re"));
  CheckTerm(cache, ASCIIToUTF16("reco"));

  // Simulate typing "mort".
  // Since we now have only one search term, the cached results for 're' and
  // 'reco' should be purged, giving us only 1 item in the cache (for 'mort').
  url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("mort"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(1U, cache.size());
  CheckTerm(cache, ASCIIToUTF16("mort"));

  // Simulate typing "reco" giving "mort reco" in the simulated omnibox.
  url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("mort reco"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(2U, cache.size());
  CheckTerm(cache, ASCIIToUTF16("mort"));
  CheckTerm(cache, ASCIIToUTF16("reco"));

  // Simulate a <DELETE> by removing the 'reco' and adding back the 'rec'.
  url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("mort rec"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(2U, cache.size());
  CheckTerm(cache, ASCIIToUTF16("mort"));
  CheckTerm(cache, ASCIIToUTF16("rec"));
}

TEST_F(InMemoryURLIndexTest, AddNewRows) {
  // Verify that the row we're going to add does not already exist.
  URLID new_row_id = 87654321;
  // Newly created URLRows get a last_visit time of 'right now' so it should
  // qualify as a quick result candidate.
  EXPECT_TRUE(url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("brokeandalone"), base::string16::npos, kMaxMatches)
          .empty());

  // Add a new row.
  URLRow new_row(GURL("http://www.brokeandaloneinmanitoba.com/"), new_row_id++);
  new_row.set_last_visit(base::Time::Now());
  EXPECT_TRUE(UpdateURL(new_row));

  // Verify that we can retrieve it.
  EXPECT_EQ(1U, url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("brokeandalone"), base::string16::npos, kMaxMatches).size());

  // Add it again just to be sure that is harmless and that it does not update
  // the index.
  EXPECT_FALSE(UpdateURL(new_row));
  EXPECT_EQ(1U, url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("brokeandalone"), base::string16::npos, kMaxMatches).size());

  // Make up an URL that does not qualify and try to add it.
  URLRow unqualified_row(GURL("http://www.brokeandaloneinmanitoba.com/"),
                         new_row_id++);
  EXPECT_FALSE(UpdateURL(new_row));
}

TEST_F(InMemoryURLIndexTest, DeleteRows) {
  ScoredHistoryMatches matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("DrudgeReport"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(1U, matches.size());

  // Delete the URL then search again.
  EXPECT_TRUE(DeleteURL(matches[0].url_info.url()));
  EXPECT_TRUE(url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("DrudgeReport"), base::string16::npos, kMaxMatches).empty());

  // Make up an URL that does not exist in the database and delete it.
  GURL url("http://www.hokeypokey.com/putyourrightfootin.html");
  EXPECT_FALSE(DeleteURL(url));
}

TEST_F(InMemoryURLIndexTest, ExpireRow) {
  ScoredHistoryMatches matches = url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("DrudgeReport"), base::string16::npos, kMaxMatches);
  ASSERT_EQ(1U, matches.size());

  // Determine the row id for the result, remember that id, broadcast a
  // delete notification, then ensure that the row has been deleted.
  URLsDeletedDetails deleted_details;
  deleted_details.all_history = false;
  deleted_details.rows.push_back(matches[0].url_info);
  Observe(chrome::NOTIFICATION_HISTORY_URLS_DELETED,
          content::Source<InMemoryURLIndexTest>(this),
          content::Details<history::HistoryDetails>(&deleted_details));
  EXPECT_TRUE(url_index_->HistoryItemsForTerms(
      ASCIIToUTF16("DrudgeReport"), base::string16::npos, kMaxMatches).empty());
}

TEST_F(InMemoryURLIndexTest, WhitelistedURLs) {
  struct TestData {
    const std::string url_spec;
    const bool expected_is_whitelisted;
  } data[] = {
    // URLs with whitelisted schemes.
    { "about:histograms", true },
    { "chrome://settings", true },
    { "file://localhost/Users/joeschmoe/sekrets", true },
    { "ftp://public.mycompany.com/myfile.txt", true },
    { "http://www.google.com/translate", true },
    { "https://www.gmail.com/", true },
    { "mailto:support@google.com", true },
    // URLs with unacceptable schemes.
    { "aaa://www.dummyhost.com;frammy", false },
    { "aaas://www.dummyhost.com;frammy", false },
    { "acap://suzie@somebody.com", false },
    { "cap://cal.example.com/Company/Holidays", false },
    { "cid:foo4*foo1@bar.net", false },
    { "crid://example.com/foobar", false },
    { "data:image/png;base64,iVBORw0KGgoAAAANSUhE=", false },
    { "dict://dict.org/d:shortcake:", false },
    { "dns://192.168.1.1/ftp.example.org?type=A", false },
    { "fax:+358.555.1234567", false },
    { "geo:13.4125,103.8667", false },
    { "go:Mercedes%20Benz", false },
    { "gopher://farnsworth.ca:666/gopher", false },
    { "h323:farmer-john;sixpence", false },
    { "iax:johnQ@example.com/12022561414", false },
    { "icap://icap.net/service?mode=translate&lang=french", false },
    { "im:fred@example.com", false },
    { "imap://michael@minbari.org/users.*", false },
    { "info:ddc/22/eng//004.678", false },
    { "ipp://example.com/printer/fox", false },
    { "iris:dreg1//example.com/local/myhosts", false },
    { "iris.beep:dreg1//example.com/local/myhosts", false },
    { "iris.lws:dreg1//example.com/local/myhosts", false },
    { "iris.xpc:dreg1//example.com/local/myhosts", false },
    { "iris.xpcs:dreg1//example.com/local/myhosts", false },
    { "ldap://ldap.itd.umich.edu/o=University%20of%20Michigan,c=US", false },
    { "mid:foo4%25foo1@bar.net", false },
    { "modem:+3585551234567;type=v32b?7e1;type=v110", false },
    { "msrp://atlanta.example.com:7654/jshA7weztas;tcp", false },
    { "msrps://atlanta.example.com:7654/jshA7weztas;tcp", false },
    { "news:colorectal.info.banned", false },
    { "nfs://server/d/e/f", false },
    { "nntp://www.example.com:6543/info.comp.lies/1234", false },
    { "pop://rg;AUTH=+APOP@mail.mycompany.com:8110", false },
    { "pres:fred@example.com", false },
    { "prospero://host.dom//pros/name", false },
    { "rsync://syler@lost.com/Source", false },
    { "rtsp://media.example.com:554/twister/audiotrack", false },
    { "service:acap://some.where.net;authentication=KERBEROSV4", false },
    { "shttp://www.terces.com/secret", false },
    { "sieve://example.com//script", false },
    { "sip:+1-212-555-1212:1234@gateway.com;user=phone", false },
    { "sips:+1-212-555-1212:1234@gateway.com;user=phone", false },
    { "sms:+15105551212?body=hello%20there", false },
    { "snmp://tester5@example.com:8161/bridge1;800002b804616263", false },
    { "soap.beep://stockquoteserver.example.com/StockQuote", false },
    { "soap.beeps://stockquoteserver.example.com/StockQuote", false },
    { "tag:blogger.com,1999:blog-555", false },
    { "tel:+358-555-1234567;postd=pp22", false },
    { "telnet://mayor_margie:one2rule4All@www.mycity.com:6789/", false },
    { "tftp://example.com/mystartupfile", false },
    { "tip://123.123.123.123/?urn:xopen:xid", false },
    { "tv:nbc.com", false },
    { "urn:foo:A123,456", false },
    { "vemmi://zeus.mctel.fr/demo", false },
    { "wais://www.mydomain.net:8765/mydatabase", false },
    { "xmpp:node@example.com", false },
    { "xmpp://guest@example.com", false },
  };

  URLIndexPrivateData& private_data(*GetPrivateData());
  const std::set<std::string>& whitelist(scheme_whitelist());
  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(data); ++i) {
    GURL url(data[i].url_spec);
    EXPECT_EQ(data[i].expected_is_whitelisted,
              private_data.URLSchemeIsWhitelisted(url, whitelist));
  }
}

TEST_F(InMemoryURLIndexTest, ReadVisitsFromHistory) {
  const HistoryInfoMap& history_info_map = GetPrivateData()->history_info_map_;

  // Check (for URL with id 1) that the number of visits and their
  // transition types are what we expect.  We don't bother checking
  // the timestamps because it's too much trouble.  (The timestamps go
  // through a transformation in InMemoryURLIndexTest::SetUp().  We
  // assume that if the count and transitions show up with the right
  // information, we're getting the right information from the history
  // database file.)
  HistoryInfoMap::const_iterator entry = history_info_map.find(1);
  ASSERT_TRUE(entry != history_info_map.end());
  {
    const VisitInfoVector& visits = entry->second.visits;
    EXPECT_EQ(3u, visits.size());
    EXPECT_EQ(0u, visits[0].second);
    EXPECT_EQ(1u, visits[1].second);
    EXPECT_EQ(0u, visits[2].second);
  }

  // Ditto but for URL with id 35.
  entry = history_info_map.find(35);
  ASSERT_TRUE(entry != history_info_map.end());
  {
    const VisitInfoVector& visits = entry->second.visits;
    EXPECT_EQ(2u, visits.size());
    EXPECT_EQ(1u, visits[0].second);
    EXPECT_EQ(1u, visits[1].second);
  }

  // The URL with id 32 has many visits listed in the database, but we
  // should only read the most recent 10 (which are all transition type 0).
  entry = history_info_map.find(32);
  ASSERT_TRUE(entry != history_info_map.end());
  {
    const VisitInfoVector& visits = entry->second.visits;
    EXPECT_EQ(10u, visits.size());
    for (size_t i = 0; i < visits.size(); ++i)
      EXPECT_EQ(0u, visits[i].second);
  }
}

TEST_F(InMemoryURLIndexTest, CacheSaveRestore) {
  base::ScopedTempDir temp_directory;
  ASSERT_TRUE(temp_directory.CreateUniqueTempDir());
  set_history_dir(temp_directory.path());

  URLIndexPrivateData& private_data(*GetPrivateData());

  // Ensure that there is really something there to be saved.
  EXPECT_FALSE(private_data.word_list_.empty());
  // available_words_ will already be empty since we have freshly built the
  // data set for this test.
  EXPECT_TRUE(private_data.available_words_.empty());
  EXPECT_FALSE(private_data.word_map_.empty());
  EXPECT_FALSE(private_data.char_word_map_.empty());
  EXPECT_FALSE(private_data.word_id_history_map_.empty());
  EXPECT_FALSE(private_data.history_id_word_map_.empty());
  EXPECT_FALSE(private_data.history_info_map_.empty());
  EXPECT_FALSE(private_data.word_starts_map_.empty());

  // Make sure the data we have was built from history.  (Version 0
  // means rebuilt from history.)
  EXPECT_EQ(0, private_data.restored_cache_version_);

  // Capture the current private data for later comparison to restored data.
  scoped_refptr<URLIndexPrivateData> old_data(private_data.Duplicate());
  const base::Time rebuild_time = private_data.last_time_rebuilt_from_history_;

  {
    // Save then restore our private data.
    base::RunLoop run_loop;
    CacheFileSaverObserver save_observer(run_loop.QuitClosure());
    url_index_->set_save_cache_observer(&save_observer);
    PostSaveToCacheFileTask();
    run_loop.Run();
    EXPECT_TRUE(save_observer.succeeded());
  }

  // Clear and then prove it's clear before restoring.
  ClearPrivateData();
  EXPECT_TRUE(private_data.word_list_.empty());
  EXPECT_TRUE(private_data.available_words_.empty());
  EXPECT_TRUE(private_data.word_map_.empty());
  EXPECT_TRUE(private_data.char_word_map_.empty());
  EXPECT_TRUE(private_data.word_id_history_map_.empty());
  EXPECT_TRUE(private_data.history_id_word_map_.empty());
  EXPECT_TRUE(private_data.history_info_map_.empty());
  EXPECT_TRUE(private_data.word_starts_map_.empty());

  {
    base::RunLoop run_loop;
    HistoryIndexRestoreObserver restore_observer(run_loop.QuitClosure());
    url_index_->set_restore_cache_observer(&restore_observer);
    PostRestoreFromCacheFileTask();
    run_loop.Run();
    EXPECT_TRUE(restore_observer.succeeded());
  }

  URLIndexPrivateData& new_data(*GetPrivateData());

  // Make sure the data we have was reloaded from cache.  (Version 0
  // means rebuilt from history; anything else means restored from
  // a cache version.)  Also, the rebuild time should not have changed.
  EXPECT_GT(new_data.restored_cache_version_, 0);
  EXPECT_EQ(rebuild_time, new_data.last_time_rebuilt_from_history_);

  // Compare the captured and restored for equality.
  ExpectPrivateDataEqual(*old_data.get(), new_data);
}

#if defined(OS_WIN)
// http://crbug.com/351500
#define MAYBE_RebuildFromHistoryIfCacheOld DISABLED_RebuildFromHistoryIfCacheOld
#else
#define MAYBE_RebuildFromHistoryIfCacheOld RebuildFromHistoryIfCacheOld
#endif
TEST_F(InMemoryURLIndexTest, MAYBE_RebuildFromHistoryIfCacheOld) {
  base::ScopedTempDir temp_directory;
  ASSERT_TRUE(temp_directory.CreateUniqueTempDir());
  set_history_dir(temp_directory.path());

  URLIndexPrivateData& private_data(*GetPrivateData());

  // Ensure that there is really something there to be saved.
  EXPECT_FALSE(private_data.word_list_.empty());
  // available_words_ will already be empty since we have freshly built the
  // data set for this test.
  EXPECT_TRUE(private_data.available_words_.empty());
  EXPECT_FALSE(private_data.word_map_.empty());
  EXPECT_FALSE(private_data.char_word_map_.empty());
  EXPECT_FALSE(private_data.word_id_history_map_.empty());
  EXPECT_FALSE(private_data.history_id_word_map_.empty());
  EXPECT_FALSE(private_data.history_info_map_.empty());
  EXPECT_FALSE(private_data.word_starts_map_.empty());

  // Make sure the data we have was built from history.  (Version 0
  // means rebuilt from history.)
  EXPECT_EQ(0, private_data.restored_cache_version_);

  // Overwrite the build time so that we'll think the data is too old
  // and rebuild the cache from history.
  const base::Time fake_rebuild_time =
      private_data.last_time_rebuilt_from_history_ -
      base::TimeDelta::FromDays(30);
  private_data.last_time_rebuilt_from_history_ = fake_rebuild_time;

  // Capture the current private data for later comparison to restored data.
  scoped_refptr<URLIndexPrivateData> old_data(private_data.Duplicate());

  {
    // Save then restore our private data.
    base::RunLoop run_loop;
    CacheFileSaverObserver save_observer(run_loop.QuitClosure());
    url_index_->set_save_cache_observer(&save_observer);
    PostSaveToCacheFileTask();
    run_loop.Run();
    EXPECT_TRUE(save_observer.succeeded());
  }

  // Clear and then prove it's clear before restoring.
  ClearPrivateData();
  EXPECT_TRUE(private_data.word_list_.empty());
  EXPECT_TRUE(private_data.available_words_.empty());
  EXPECT_TRUE(private_data.word_map_.empty());
  EXPECT_TRUE(private_data.char_word_map_.empty());
  EXPECT_TRUE(private_data.word_id_history_map_.empty());
  EXPECT_TRUE(private_data.history_id_word_map_.empty());
  EXPECT_TRUE(private_data.history_info_map_.empty());
  EXPECT_TRUE(private_data.word_starts_map_.empty());

  {
    base::RunLoop run_loop;
    HistoryIndexRestoreObserver restore_observer(run_loop.QuitClosure());
    url_index_->set_restore_cache_observer(&restore_observer);
    PostRestoreFromCacheFileTask();
    run_loop.Run();
    EXPECT_TRUE(restore_observer.succeeded());
  }

  URLIndexPrivateData& new_data(*GetPrivateData());

  // Make sure the data we have was rebuilt from history.  (Version 0
  // means rebuilt from history; anything else means restored from
  // a cache version.)
  EXPECT_EQ(0, new_data.restored_cache_version_);
  EXPECT_NE(fake_rebuild_time, new_data.last_time_rebuilt_from_history_);

  // Compare the captured and restored for equality.
  ExpectPrivateDataEqual(*old_data.get(), new_data);
}

class InMemoryURLIndexCacheTest : public testing::Test {
 public:
  InMemoryURLIndexCacheTest() {}

 protected:
  virtual void SetUp() OVERRIDE;

  // Pass-through functions to simplify our friendship with InMemoryURLIndex.
  void set_history_dir(const base::FilePath& dir_path);
  bool GetCacheFilePath(base::FilePath* file_path) const;

  base::ScopedTempDir temp_dir_;
  scoped_ptr<InMemoryURLIndex> url_index_;
};

void InMemoryURLIndexCacheTest::SetUp() {
  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  HistoryClient history_client;
  base::FilePath path(temp_dir_.path());
  url_index_.reset(new InMemoryURLIndex(
      NULL, path, "en,ja,hi,zh", &history_client));
}

void InMemoryURLIndexCacheTest::set_history_dir(
    const base::FilePath& dir_path) {
  return url_index_->set_history_dir(dir_path);
}

bool InMemoryURLIndexCacheTest::GetCacheFilePath(
    base::FilePath* file_path) const {
  DCHECK(file_path);
  return url_index_->GetCacheFilePath(file_path);
}

TEST_F(InMemoryURLIndexCacheTest, CacheFilePath) {
  base::FilePath expectedPath =
      temp_dir_.path().Append(FILE_PATH_LITERAL("History Provider Cache"));
  std::vector<base::FilePath::StringType> expected_parts;
  expectedPath.GetComponents(&expected_parts);
  base::FilePath full_file_path;
  ASSERT_TRUE(GetCacheFilePath(&full_file_path));
  std::vector<base::FilePath::StringType> actual_parts;
  full_file_path.GetComponents(&actual_parts);
  ASSERT_EQ(expected_parts.size(), actual_parts.size());
  size_t count = expected_parts.size();
  for (size_t i = 0; i < count; ++i)
    EXPECT_EQ(expected_parts[i], actual_parts[i]);
  // Must clear the history_dir_ to satisfy the dtor's DCHECK.
  set_history_dir(base::FilePath());
}

}  // namespace history
