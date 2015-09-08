// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/history/android/sqlite_cursor.h"

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/bind.h"
#include "base/logging.h"
#include "components/history/core/browser/android/android_history_types.h"
#include "content/public/browser/browser_thread.h"
#include "jni/SQLiteCursor_jni.h"
#include "sql/statement.h"

using base::android::ConvertUTF8ToJavaString;
using base::android::ScopedJavaLocalRef;
using content::BrowserThread;

namespace {

SQLiteCursor::JavaColumnType ToJavaColumnType(sql::ColType type) {
  switch (type) {
    case sql::COLUMN_TYPE_INTEGER:
      return SQLiteCursor::NUMERIC;
    case sql::COLUMN_TYPE_FLOAT:
      return SQLiteCursor::DOUBLE;
    case sql::COLUMN_TYPE_TEXT:
      return SQLiteCursor::LONG_VAR_CHAR;
    case sql::COLUMN_TYPE_BLOB:
      return SQLiteCursor::BLOB;
    case sql::COLUMN_TYPE_NULL:
      return SQLiteCursor::NULL_TYPE;
    default:
      NOTREACHED();
  }
  return SQLiteCursor::NULL_TYPE;
}

}  // namespace.


SQLiteCursor::TestObserver::TestObserver() {
}

SQLiteCursor::TestObserver::~TestObserver() {
}

ScopedJavaLocalRef<jobject> SQLiteCursor::NewJavaSqliteCursor(
    JNIEnv* env,
    const std::vector<std::string>& column_names,
    history::AndroidStatement* statement,
    AndroidHistoryProviderService* service) {
  SQLiteCursor* cursor = new SQLiteCursor(column_names, statement, service);
  return Java_SQLiteCursor_create(env, reinterpret_cast<intptr_t>(cursor));
}

bool SQLiteCursor::RegisterSqliteCursor(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

jint SQLiteCursor::GetCount(JNIEnv* env, jobject obj) {
  // Moves to maxium possible position so we will reach the last row, then finds
  // out the total number of rows.
  int current_position = position_;
  int count = MoveTo(env, obj, std::numeric_limits<int>::max() - 1) + 1;
  // Moves back to the previous position.
  MoveTo(env, obj, current_position);
  return count;
}

ScopedJavaLocalRef<jobjectArray> SQLiteCursor::GetColumnNames(JNIEnv* env,
                                                              jobject obj) {
  return base::android::ToJavaArrayOfStrings(env, column_names_);
}

ScopedJavaLocalRef<jstring> SQLiteCursor::GetString(JNIEnv* env,
                                                    jobject obj,
                                                    jint column) {
  base::string16 value = statement_->statement()->ColumnString16(column);
  return ScopedJavaLocalRef<jstring>(env,
      env->NewString(value.data(), value.size()));
}

jlong SQLiteCursor::GetLong(JNIEnv* env, jobject obj, jint column) {
  return statement_->statement()->ColumnInt64(column);
}

jint SQLiteCursor::GetInt(JNIEnv* env, jobject obj, jint column) {
  return statement_->statement()->ColumnInt(column);
}

jdouble SQLiteCursor::GetDouble(JNIEnv* env, jobject obj, jint column) {
  return statement_->statement()->ColumnDouble(column);
}

ScopedJavaLocalRef<jbyteArray> SQLiteCursor::GetBlob(JNIEnv* env,
                                                     jobject obj,
                                                     jint column) {
  std::vector<unsigned char> blob;

  // Assume the client will only get favicon using GetBlob.
  if (statement_->favicon_index() == column) {
    if (!GetFavicon(statement_->statement()->ColumnInt(column), &blob))
      return ScopedJavaLocalRef<jbyteArray>();
  } else {
    statement_->statement()->ColumnBlobAsVector(column, &blob);
  }
  return base::android::ToJavaByteArray(env, &blob[0], blob.size());
}

jboolean SQLiteCursor::IsNull(JNIEnv* env, jobject obj, jint column) {
  return NULL_TYPE == GetColumnTypeInternal(column) ? JNI_TRUE : JNI_FALSE;
}

jint SQLiteCursor::MoveTo(JNIEnv* env, jobject obj, jint pos) {
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
      base::Bind(&SQLiteCursor::RunMoveStatementOnUIThread,
      base::Unretained(this), pos));
  if (test_observer_)
    test_observer_->OnPostMoveToTask();

  event_.Wait();
  return position_;
}

jint SQLiteCursor::GetColumnType(JNIEnv* env, jobject obj, jint column) {
  return GetColumnTypeInternal(column);
}

void SQLiteCursor::Destroy(JNIEnv* env, jobject obj) {
  // We do our best to cleanup when Destroy() is called from Java's finalize()
  // where the UI message loop might stop running or in the process of shutting
  // down, as the whole process will be destroyed soon, it's fine to leave some
  // objects out there.
  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    DestroyOnUIThread();
  } else if (!BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                 base::Bind(&SQLiteCursor::DestroyOnUIThread,
                     base::Unretained(this)))) {
    delete this;
  }
}

SQLiteCursor::SQLiteCursor(const std::vector<std::string>& column_names,
                           history::AndroidStatement* statement,
                           AndroidHistoryProviderService* service)
    : position_(-1),
      event_(false, false),
      statement_(statement),
      column_names_(column_names),
      service_(service),
      count_(-1),
      test_observer_(NULL) {
}

SQLiteCursor::~SQLiteCursor() {
}

void SQLiteCursor::DestroyOnUIThread() {
  // Consumer requests were set in the UI thread. They must be cancelled
  // using the same thread.
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  tracker_.reset();
  service_->CloseStatement(statement_);
  delete this;
}

bool SQLiteCursor::GetFavicon(favicon_base::FaviconID id,
                              std::vector<unsigned char>* image_data) {
  if (id) {
    BrowserThread::PostTask(
        BrowserThread::UI,
        FROM_HERE,
        base::Bind(&SQLiteCursor::GetFaviconForIDInUIThread,
                   base::Unretained(this), id,
                   base::Bind(&SQLiteCursor::OnFaviconData,
                              base::Unretained(this))));

    if (test_observer_)
      test_observer_->OnPostGetFaviconTask();

    event_.Wait();
    if (!favicon_bitmap_result_.is_valid())
      return false;

    scoped_refptr<base::RefCountedMemory> bitmap_data =
        favicon_bitmap_result_.bitmap_data;
    image_data->assign(bitmap_data->front(),
                       bitmap_data->front() + bitmap_data->size());
    return true;
  }

  return false;
}

void SQLiteCursor::GetFaviconForIDInUIThread(
    favicon_base::FaviconID id,
    const favicon_base::FaviconRawBitmapCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!tracker_.get())
    tracker_.reset(new base::CancelableTaskTracker());
  service_->GetLargestRawFaviconForID(id, callback, tracker_.get());
}

void SQLiteCursor::OnFaviconData(
    const favicon_base::FaviconRawBitmapResult& bitmap_result) {
  favicon_bitmap_result_ = bitmap_result;
  event_.Signal();
  if (test_observer_)
    test_observer_->OnGetFaviconResult();
}

void SQLiteCursor::OnMoved(int pos) {
  position_ = pos;
  event_.Signal();
  if (test_observer_)
    // Notified test_observer on UI thread instead of the one it will wait.
    test_observer_->OnGetMoveToResult();
}

SQLiteCursor::JavaColumnType SQLiteCursor::GetColumnTypeInternal(int column) {
  if (column == statement_->favicon_index())
    return SQLiteCursor::BLOB;

  return ToJavaColumnType(statement_->statement()->ColumnType(column));
}

void SQLiteCursor::RunMoveStatementOnUIThread(int pos) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!tracker_.get())
    tracker_.reset(new base::CancelableTaskTracker());
  service_->MoveStatement(
      statement_,
      position_,
      pos,
      base::Bind(&SQLiteCursor::OnMoved, base::Unretained(this)),
      tracker_.get());
}
