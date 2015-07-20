// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/setup/update_active_setup_version_work_item.h"

#include <windows.h>

#include <ostream>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/test/test_reg_util_win.h"
#include "base/win/registry.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::ValuesIn;

namespace {

const HKEY kActiveSetupRoot = HKEY_LOCAL_MACHINE;
const base::char16 kActiveSetupPath[] = L"Active Setup\\test";

struct UpdateActiveSetupVersionWorkItemTestCase {
  // The initial value to be set in the registry prior to executing the
  // UpdateActiveSetupVersionWorkItem. No value will be set if this null.
  const wchar_t* initial_value;

  // Whether to ask the UpdateActiveSetupVersionWorkItem to bump the OS_UPGRADES
  // component of the Active Setup version.
  bool bump_os_upgrades_component;

  // The expected value after executing the UpdateActiveSetupVersionWorkItem.
  const wchar_t* expected_result;
} const kUpdateActiveSetupVersionWorkItemTestCases[] = {
    // Initial install.
    {nullptr, false, L"43,0,0,0"},
    // No-op update.
    {L"43.0.0.0", false, L"43,0,0,0"},
    // Update only major component.
    {L"24,1,2,3", false, L"43,1,2,3"},
    // Reset from bogus value.
    {L"zzz", false, L"43,0,0,0"},
    // Reset from invalid version (too few components).
    {L"1,2", false, L"43,0,0,0"},
    // Reset from invalid version (too many components).
    {L"43,1,2,3,10", false, L"43,0,0,0"},
    // Reset from empty string.
    {L"", false, L"43,0,0,0"},

    // Same tests with an OS_UPGRADES component bump.
    {nullptr, true, L"43,0,1,0"},
    {L"43.0.0.0", true, L"43,0,1,0"},
    {L"24,1,2,3", true, L"43,1,3,3"},
    {L"zzz", true, L"43,0,1,0"},
    {L"1,2", true, L"43,0,1,0"},
    {L"43,1,2,3,10", true, L"43,0,1,0"},
    {L"", true, L"43,0,1,0"},
    // Bumping a negative OS upgrade component should result in it being
    // reset and subsequently bumped to 1 as usual.
    {L"43,11,-123,33", true, L"43,11,1,33"},
};

// Implements PrintTo in order for gtest to be able to print the problematic
// UpdateActiveSetupVersionWorkItemTestCase on failure.
void PrintTo(const UpdateActiveSetupVersionWorkItemTestCase& test_case,
             ::std::ostream* os) {
  *os << "Initial value: "
      << (test_case.initial_value ? test_case.initial_value : L"(empty)")
      << ", bump_os_upgrades_component: "
      << test_case.bump_os_upgrades_component
      << ", expected result: " << test_case.expected_result;
}

}  // namespace

class UpdateActiveSetupVersionWorkItemTest
    : public testing::TestWithParam<UpdateActiveSetupVersionWorkItemTestCase> {
 public:
  UpdateActiveSetupVersionWorkItemTest() {}

  void SetUp() override {
    registry_override_manager_.OverrideRegistry(kActiveSetupRoot);
  }

 private:
  registry_util::RegistryOverrideManager registry_override_manager_;

  DISALLOW_COPY_AND_ASSIGN(UpdateActiveSetupVersionWorkItemTest);
};

TEST_P(UpdateActiveSetupVersionWorkItemTest, Execute) {
  // Get the parametrized |test_case| which defines 5 steps:
  //   1) Maybe set an initial Active Setup version in the registry according to
  //      |test_case.initial_value|.
  //   2) Declare the work to be done by the UpdateActiveSetupVersionWorkItem
  //      based on |test_case.bump_os_upgrades_component|.
  //   3) Unconditionally execute the Active Setup work items.
  //   4) Verify that the updated Active Setup version is as expected by
  //      |test_case.expected_result|.
  //   5) Rollback and verify that |test_case.initial_value| is back.
  const UpdateActiveSetupVersionWorkItemTestCase& test_case = GetParam();

  base::win::RegKey test_key;

  ASSERT_EQ(ERROR_FILE_NOT_FOUND,
            test_key.Open(kActiveSetupRoot, kActiveSetupPath, KEY_READ));

  UpdateActiveSetupVersionWorkItem active_setup_work_item(
      kActiveSetupPath, test_case.bump_os_upgrades_component
                            ? UpdateActiveSetupVersionWorkItem::
                                  UPDATE_AND_BUMP_OS_UPGRADES_COMPONENT
                            : UpdateActiveSetupVersionWorkItem::UPDATE);

  // Create the key and set the |initial_value| *after* the WorkItem to confirm
  // that all of the work is done when executing the item, not when creating it.
  ASSERT_EQ(ERROR_SUCCESS,
            test_key.Create(kActiveSetupRoot, kActiveSetupPath, KEY_SET_VALUE));
  if (test_case.initial_value) {
    ASSERT_EQ(ERROR_SUCCESS,
              test_key.WriteValue(L"Version", test_case.initial_value));
  }

  EXPECT_TRUE(active_setup_work_item.Do());

  {
    base::string16 version_out;
    EXPECT_EQ(ERROR_SUCCESS, test_key.Open(kActiveSetupRoot, kActiveSetupPath,
                                           KEY_QUERY_VALUE));
    EXPECT_EQ(ERROR_SUCCESS, test_key.ReadValue(L"Version", &version_out));
    EXPECT_EQ(test_case.expected_result, version_out);
  }

  active_setup_work_item.Rollback();

  {
    EXPECT_EQ(ERROR_SUCCESS, test_key.Open(kActiveSetupRoot, kActiveSetupPath,
                                           KEY_QUERY_VALUE));

    base::string16 version_out;
    LONG read_result = test_key.ReadValue(L"Version", &version_out);
    if (test_case.initial_value) {
      EXPECT_EQ(ERROR_SUCCESS, read_result);
      EXPECT_EQ(test_case.initial_value, version_out);
    } else {
      EXPECT_EQ(ERROR_FILE_NOT_FOUND, read_result);
    }
  }
}

INSTANTIATE_TEST_CASE_P(UpdateActiveSetupVersionWorkItemTestInstance,
                        UpdateActiveSetupVersionWorkItemTest,
                        ValuesIn(kUpdateActiveSetupVersionWorkItemTestCases));
