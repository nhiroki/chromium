// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/memory/ref_counted.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/thread_task_runner_handle.h"
#include "base/values.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "extensions/browser/extensions_test.h"
#include "extensions/browser/sandboxed_unpacker.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_paths.h"
#include "extensions/common/switches.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace extensions {

class MockSandboxedUnpackerClient : public SandboxedUnpackerClient {
 public:
  void WaitForUnpack() {
    scoped_refptr<content::MessageLoopRunner> runner =
        new content::MessageLoopRunner;
    quit_closure_ = runner->QuitClosure();
    runner->Run();
  }

  base::FilePath temp_dir() const { return temp_dir_; }
  base::string16 unpack_err() const { return error_; }

 private:
  ~MockSandboxedUnpackerClient() override {}

  void OnUnpackSuccess(const base::FilePath& temp_dir,
                       const base::FilePath& extension_root,
                       const base::DictionaryValue* original_manifest,
                       const Extension* extension,
                       const SkBitmap& install_icon) override {
    temp_dir_ = temp_dir;
    quit_closure_.Run();
  }

  void OnUnpackFailure(const CrxInstallError& error) override {
    error_ = error.message();
    quit_closure_.Run();
  }

  base::string16 error_;
  base::Closure quit_closure_;
  base::FilePath temp_dir_;
};

class SandboxedUnpackerTest : public ExtensionsTest {
 public:
  void SetUp() override {
    ExtensionsTest::SetUp();
    ASSERT_TRUE(extensions_dir_.CreateUniqueTempDir());
    browser_threads_.reset(new content::TestBrowserThreadBundle(
        content::TestBrowserThreadBundle::IO_MAINLOOP));
    in_process_utility_thread_helper_.reset(
        new content::InProcessUtilityThreadHelper);
    // It will delete itself.
    client_ = new MockSandboxedUnpackerClient;
  }

  void TearDown() override {
    // Need to destruct SandboxedUnpacker before the message loop since
    // it posts a task to it.
    sandboxed_unpacker_ = NULL;
    base::RunLoop().RunUntilIdle();
    ExtensionsTest::TearDown();
  }

  void SetupUnpacker(const std::string& crx_name,
                     const std::string& package_hash) {
    base::FilePath original_path;
    ASSERT_TRUE(PathService::Get(extensions::DIR_TEST_DATA, &original_path));
    original_path = original_path.AppendASCII("unpacker").AppendASCII(crx_name);
    ASSERT_TRUE(base::PathExists(original_path)) << original_path.value();

    sandboxed_unpacker_ = new SandboxedUnpacker(
        extensions::CRXFileInfo(std::string(), original_path, package_hash),
        Manifest::INTERNAL, Extension::NO_FLAGS, extensions_dir_.path(),
        base::ThreadTaskRunnerHandle::Get(), client_);

    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::Bind(&SandboxedUnpacker::Start, sandboxed_unpacker_.get()));
    client_->WaitForUnpack();
  }

  base::FilePath GetInstallPath() {
    return client_->temp_dir().AppendASCII(kTempExtensionName);
  }

  base::string16 GetInstallError() { return client_->unpack_err(); }

 protected:
  base::ScopedTempDir extensions_dir_;
  MockSandboxedUnpackerClient* client_;
  scoped_refptr<SandboxedUnpacker> sandboxed_unpacker_;
  scoped_ptr<content::TestBrowserThreadBundle> browser_threads_;
  scoped_ptr<content::InProcessUtilityThreadHelper>
      in_process_utility_thread_helper_;
};

TEST_F(SandboxedUnpackerTest, NoCatalogsSuccess) {
  SetupUnpacker("no_l10n.crx", "");
  // Check that there is no _locales folder.
  base::FilePath install_path = GetInstallPath().Append(kLocaleFolder);
  EXPECT_FALSE(base::PathExists(install_path));
}

TEST_F(SandboxedUnpackerTest, WithCatalogsSuccess) {
  SetupUnpacker("good_l10n.crx", "");
  // Check that there is _locales folder.
  base::FilePath install_path = GetInstallPath().Append(kLocaleFolder);
  EXPECT_TRUE(base::PathExists(install_path));
}

TEST_F(SandboxedUnpackerTest, FailHashCheck) {
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      extensions::switches::kEnableCrxHashCheck);
  SetupUnpacker("good_l10n.crx", "badhash");
  // Check that there is an error message.
  EXPECT_NE(base::string16(), GetInstallError());
}

TEST_F(SandboxedUnpackerTest, PassHashCheck) {
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      extensions::switches::kEnableCrxHashCheck);
  SetupUnpacker(
      "good_l10n.crx",
      "6fa171c726373785aa4fcd2df448c3db0420a95d5044fbee831f089b979c4068");
  // Check that there is no error message.
  EXPECT_EQ(base::string16(), GetInstallError());
}

TEST_F(SandboxedUnpackerTest, SkipHashCheck) {
  SetupUnpacker("good_l10n.crx", "badhash");
  // Check that there is no error message.
  EXPECT_EQ(base::string16(), GetInstallError());
}

}  // namespace extensions
