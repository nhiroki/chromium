// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/logging.h"
#include "content/browser/indexed_db/leveldb/leveldb_iterator_impl.h"
#include "content/browser/indexed_db/leveldb/leveldb_transaction.h"
#include "content/browser/indexed_db/mock_browsertest_indexed_db_class_factory.h"
#include "third_party/leveldatabase/src/include/leveldb/status.h"

namespace {

class FunctionTracer {
 public:
  FunctionTracer(const std::string& class_name,
                 const std::string& method_name,
                 int instance_num)
      : class_name_(class_name),
        method_name_(method_name),
        instance_count_(instance_num),
        current_call_num_(0) {}

  void log_call() {
    current_call_num_++;
    VLOG(0) << class_name_ << '[' << instance_count_ << "]::" << method_name_
            << "()[" << current_call_num_ << ']';
  }

 private:
  std::string class_name_;
  std::string method_name_;
  int instance_count_;
  int current_call_num_;
};

}  // namespace

namespace content {

class LevelDBTestTansaction : public LevelDBTransaction {
 public:
  LevelDBTestTansaction(LevelDBDatabase* db,
                        FailMethod fail_method,
                        int fail_on_call_num)
      : LevelDBTransaction(db),
        fail_method_(fail_method),
        fail_on_call_num_(fail_on_call_num),
        current_call_num_(0) {
    DCHECK(fail_method != FAIL_METHOD_NOTHING);
    DCHECK_GT(fail_on_call_num, 0);
  }

  virtual leveldb::Status Get(const base::StringPiece& key,
                              std::string* value,
                              bool* found) OVERRIDE {
    if (fail_method_ != FAIL_METHOD_GET ||
        ++current_call_num_ != fail_on_call_num_)
      return LevelDBTransaction::Get(key, value, found);

    *found = false;
    return leveldb::Status::Corruption("Corrupted for the test");
  }

  virtual leveldb::Status Commit() OVERRIDE {
    if (fail_method_ != FAIL_METHOD_COMMIT ||
        ++current_call_num_ != fail_on_call_num_)
      return LevelDBTransaction::Commit();

    return leveldb::Status::Corruption("Corrupted for the test");
  }

 private:
  virtual ~LevelDBTestTansaction() {}

  FailMethod fail_method_;
  int fail_on_call_num_;
  int current_call_num_;
};

class LevelDBTraceTansaction : public LevelDBTransaction {
 public:
  LevelDBTraceTansaction(LevelDBDatabase* db, int tx_num)
      : LevelDBTransaction(db),
        commit_tracer_(s_class_name, "Commit", tx_num),
        get_tracer_(s_class_name, "Get", tx_num) {}

  virtual leveldb::Status Get(const base::StringPiece& key,
                              std::string* value,
                              bool* found) OVERRIDE {
    get_tracer_.log_call();
    return LevelDBTransaction::Get(key, value, found);
  }

  virtual leveldb::Status Commit() OVERRIDE {
    commit_tracer_.log_call();
    return LevelDBTransaction::Commit();
  }

 private:
  static const std::string s_class_name;

  virtual ~LevelDBTraceTansaction() {}

  FunctionTracer commit_tracer_;
  FunctionTracer get_tracer_;
};

const std::string LevelDBTraceTansaction::s_class_name = "LevelDBTransaction";

class LevelDBTraceIteratorImpl : public LevelDBIteratorImpl {
 public:
  LevelDBTraceIteratorImpl(scoped_ptr<leveldb::Iterator> iterator, int inst_num)
      : LevelDBIteratorImpl(iterator.Pass()),
        is_valid_tracer_(s_class_name, "IsValid", inst_num),
        seek_to_last_tracer_(s_class_name, "SeekToLast", inst_num),
        seek_tracer_(s_class_name, "Seek", inst_num),
        next_tracer_(s_class_name, "Next", inst_num),
        prev_tracer_(s_class_name, "Prev", inst_num),
        key_tracer_(s_class_name, "Key", inst_num),
        value_tracer_(s_class_name, "Value", inst_num) {}
  virtual ~LevelDBTraceIteratorImpl() {}

 private:
  static const std::string s_class_name;

  virtual bool IsValid() const OVERRIDE {
    is_valid_tracer_.log_call();
    return LevelDBIteratorImpl::IsValid();
  }
  virtual leveldb::Status SeekToLast() OVERRIDE {
    seek_to_last_tracer_.log_call();
    return LevelDBIteratorImpl::SeekToLast();
  }
  virtual leveldb::Status Seek(const base::StringPiece& target) OVERRIDE {
    seek_tracer_.log_call();
    return LevelDBIteratorImpl::Seek(target);
  }
  virtual leveldb::Status Next() OVERRIDE {
    next_tracer_.log_call();
    return LevelDBIteratorImpl::Next();
  }
  virtual leveldb::Status Prev() OVERRIDE {
    prev_tracer_.log_call();
    return LevelDBIteratorImpl::Prev();
  }
  virtual base::StringPiece Key() const OVERRIDE {
    key_tracer_.log_call();
    return LevelDBIteratorImpl::Key();
  }
  virtual base::StringPiece Value() const OVERRIDE {
    value_tracer_.log_call();
    return LevelDBIteratorImpl::Value();
  }

  mutable FunctionTracer is_valid_tracer_;
  mutable FunctionTracer seek_to_last_tracer_;
  mutable FunctionTracer seek_tracer_;
  mutable FunctionTracer next_tracer_;
  mutable FunctionTracer prev_tracer_;
  mutable FunctionTracer key_tracer_;
  mutable FunctionTracer value_tracer_;
};

const std::string LevelDBTraceIteratorImpl::s_class_name = "LevelDBIterator";

class LevelDBTestIteratorImpl : public content::LevelDBIteratorImpl {
 public:
  LevelDBTestIteratorImpl(scoped_ptr<leveldb::Iterator> iterator,
                          FailMethod fail_method,
                          int fail_on_call_num)
      : LevelDBIteratorImpl(iterator.Pass()),
        fail_method_(fail_method),
        fail_on_call_num_(fail_on_call_num),
        current_call_num_(0) {}
  virtual ~LevelDBTestIteratorImpl() {}

 private:
  virtual leveldb::Status Seek(const base::StringPiece& target) OVERRIDE {
    if (fail_method_ != FAIL_METHOD_SEEK ||
        ++current_call_num_ != fail_on_call_num_)
      return LevelDBIteratorImpl::Seek(target);
    return leveldb::Status::Corruption("Corrupted for test");
  }

  FailMethod fail_method_;
  int fail_on_call_num_;
  int current_call_num_;
};

MockBrowserTestIndexedDBClassFactory::MockBrowserTestIndexedDBClassFactory()
    : failure_class_(FAIL_CLASS_NOTHING),
      failure_method_(FAIL_METHOD_NOTHING),
      only_trace_calls_(false) {
}

MockBrowserTestIndexedDBClassFactory::~MockBrowserTestIndexedDBClassFactory() {
}

LevelDBTransaction*
MockBrowserTestIndexedDBClassFactory::CreateLevelDBTransaction(
    LevelDBDatabase* db) {
  instance_count_[FAIL_CLASS_LEVELDB_TRANSACTION] =
      instance_count_[FAIL_CLASS_LEVELDB_TRANSACTION] + 1;
  if (only_trace_calls_) {
    return new LevelDBTraceTansaction(
        db, instance_count_[FAIL_CLASS_LEVELDB_TRANSACTION]);
  } else {
    if (failure_class_ == FAIL_CLASS_LEVELDB_TRANSACTION &&
        instance_count_[FAIL_CLASS_LEVELDB_TRANSACTION] ==
            fail_on_instance_num_[FAIL_CLASS_LEVELDB_TRANSACTION]) {
      return new LevelDBTestTansaction(
          db,
          failure_method_,
          fail_on_call_num_[FAIL_CLASS_LEVELDB_TRANSACTION]);
    } else {
      return IndexedDBClassFactory::CreateLevelDBTransaction(db);
    }
  }
}

LevelDBIteratorImpl* MockBrowserTestIndexedDBClassFactory::CreateIteratorImpl(
    scoped_ptr<leveldb::Iterator> iterator) {
  instance_count_[FAIL_CLASS_LEVELDB_ITERATOR] =
      instance_count_[FAIL_CLASS_LEVELDB_ITERATOR] + 1;
  if (only_trace_calls_) {
    return new LevelDBTraceIteratorImpl(
        iterator.Pass(), instance_count_[FAIL_CLASS_LEVELDB_ITERATOR]);
  } else {
    if (failure_class_ == FAIL_CLASS_LEVELDB_ITERATOR &&
        instance_count_[FAIL_CLASS_LEVELDB_ITERATOR] ==
            fail_on_instance_num_[FAIL_CLASS_LEVELDB_ITERATOR]) {
      return new LevelDBTestIteratorImpl(
          iterator.Pass(),
          failure_method_,
          fail_on_call_num_[FAIL_CLASS_LEVELDB_ITERATOR]);
    } else {
      return new LevelDBIteratorImpl(iterator.Pass());
    }
  }
}

void MockBrowserTestIndexedDBClassFactory::FailOperation(
    FailClass failure_class,
    FailMethod failure_method,
    int fail_on_instance_num,
    int fail_on_call_num) {
  VLOG(0) << "FailOperation: class=" << failure_class
          << ", method=" << failure_method
          << ", instanceNum=" << fail_on_instance_num
          << ", callNum=" << fail_on_call_num;
  DCHECK(failure_class != FAIL_CLASS_NOTHING);
  DCHECK(failure_method != FAIL_METHOD_NOTHING);
  failure_class_ = failure_class;
  failure_method_ = failure_method;
  fail_on_instance_num_[failure_class_] = fail_on_instance_num;
  fail_on_call_num_[failure_class_] = fail_on_call_num;
  instance_count_.clear();
}

void MockBrowserTestIndexedDBClassFactory::Reset() {
  failure_class_ = FAIL_CLASS_NOTHING;
  failure_method_ = FAIL_METHOD_NOTHING;
  instance_count_.clear();
  fail_on_instance_num_.clear();
  fail_on_call_num_.clear();
}

}  // namespace content
