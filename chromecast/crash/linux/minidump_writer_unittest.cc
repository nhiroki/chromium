// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fstream>

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/memory/scoped_ptr.h"
#include "base/test/scoped_path_override.h"
#include "chromecast/crash/linux/crash_testing_utils.h"
#include "chromecast/crash/linux/dump_info.h"
#include "chromecast/crash/linux/minidump_generator.h"
#include "chromecast/crash/linux/minidump_writer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromecast {
namespace {

const char kDumplogFile[] = "dumplog";
const char kLockfileName[] = "lockfile";
const char kMetadataName[] = "metadata";
const char kMinidumpSubdir[] = "minidumps";

class FakeMinidumpGenerator : public MinidumpGenerator {
 public:
  FakeMinidumpGenerator() {}
  ~FakeMinidumpGenerator() override {}

  // MinidumpGenerator implementation:
  bool Generate(const std::string& minidump_path) override { return true; }
};

int FakeDumpState(const std::string& minidump_path) {
  return 0;
}

}  // namespace

class MinidumpWriterTest : public testing::Test {
 protected:
  MinidumpWriterTest() {}
  ~MinidumpWriterTest() override {}

  void SetUp() override {
    // Set up a temporary directory which will be used as our fake home dir.
    base::FilePath fake_home_dir;
    ASSERT_TRUE(base::CreateNewTempDirectory("", &fake_home_dir));
    home_.reset(new base::ScopedPathOverride(base::DIR_HOME, fake_home_dir));
    minidump_dir_ = fake_home_dir.Append(kMinidumpSubdir);
    dumplog_file_ = minidump_dir_.Append(kDumplogFile);
    lockfile_path_ = minidump_dir_.Append(kLockfileName);
    metadata_path_ = minidump_dir_.Append(kMetadataName);

    // Create the minidump directory
    ASSERT_TRUE(base::CreateDirectory(minidump_dir_));

    // Lockfile will be automatically created by AppendLockFile
  }

  bool AppendLockFile(const DumpInfo& dump) {
    return chromecast::AppendLockFile(
        lockfile_path_.value(), metadata_path_.value(), dump);
  }

  FakeMinidumpGenerator fake_generator_;
  base::FilePath minidump_dir_;
  base::FilePath dumplog_file_;
  base::FilePath lockfile_path_;
  base::FilePath metadata_path_;

 private:
  scoped_ptr<base::ScopedPathOverride> home_;

  DISALLOW_COPY_AND_ASSIGN(MinidumpWriterTest);
};

TEST_F(MinidumpWriterTest, Write_FailsWithIncorrectMinidumpPath) {
  MinidumpWriter writer(&fake_generator_,
                        "/path/to/wrong/dir",
                        MinidumpParams(),
                        base::Bind(&FakeDumpState));

  ASSERT_EQ(-1, writer.Write());
}

TEST_F(MinidumpWriterTest, Write_FailsWithMultiLevelRelativeMinidumpPath) {
  MinidumpWriter writer(&fake_generator_,
                        "subdir/dumplog",
                        MinidumpParams(),
                        base::Bind(&FakeDumpState));

  ASSERT_EQ(-1, writer.Write());
}

TEST_F(MinidumpWriterTest, Write_SucceedsWithSimpleFilename) {
  MinidumpWriter writer(&fake_generator_,
                        "dumplog",
                        MinidumpParams(),
                        base::Bind(&FakeDumpState));

  ASSERT_EQ(0, writer.Write());
}

TEST_F(MinidumpWriterTest, Write_SucceedsWithCorrectMinidumpPath) {
  MinidumpWriter writer(&fake_generator_,
                        dumplog_file_.value(),
                        MinidumpParams(),
                        base::Bind(&FakeDumpState));

  ASSERT_EQ(0, writer.Write());
}

TEST_F(MinidumpWriterTest, Write_FailsWithSubdirInCorrectPath) {
  MinidumpWriter writer(&fake_generator_,
                        dumplog_file_.Append("subdir/logfile").value(),
                        MinidumpParams(),
                        base::Bind(&FakeDumpState));
  ASSERT_EQ(-1, writer.Write());
}

}  // namespace chromecast
