// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/views/menu_test_base.h"
#include "ui/views/controls/menu/menu_item_view.h"
#include "ui/views/controls/menu/submenu_view.h"

template<ui::KeyboardCode KEYCODE, int EXPECTED_COMMAND>
class MenuControllerMnemonicTest : public MenuTestBase {
 public:
  MenuControllerMnemonicTest() {
  }

  ~MenuControllerMnemonicTest() override {
  }

  // MenuTestBase overrides:
  void BuildMenu(views::MenuItemView* menu) override {
    ASSERT_NE(ui::VKEY_DIVIDE, '/');
    menu->AppendMenuItemWithLabel(1, base::ASCIIToUTF16("One&/"));
    menu->AppendMenuItemWithLabel(2, base::ASCIIToUTF16("Two"));
  }

  void DoTestWithMenuOpen() override {
    ASSERT_TRUE(menu()->GetSubmenu()->IsShowing());
    KeyPress(KEYCODE,
             CreateEventTask(this, &MenuControllerMnemonicTest::Step2));
  }

  void Step2() {
    ASSERT_EQ(EXPECTED_COMMAND, last_command());
    if (EXPECTED_COMMAND == 0) {
      KeyPress(ui::VKEY_ESCAPE,
               CreateEventTask(this, &MenuControllerMnemonicTest::Step3));
    } else {
      ASSERT_FALSE(menu()->GetSubmenu()->IsShowing());
      Done();
    }
  }

  void Step3() {
    ASSERT_FALSE(menu()->GetSubmenu()->IsShowing());
    Done();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MenuControllerMnemonicTest);
};

// Pressing the mnemonic for a menu item should execute the command for that
// menu item.
typedef MenuControllerMnemonicTest<ui::VKEY_DIVIDE,1>
    MenuControllerMnemonicTestMnemonicMatch;

#if defined(USE_OZONE) || defined(OS_WIN)
// ozone bringup - http://crbug.com/401304
// flaky on windows - http://crbug.com/523255
#define MAYBE_MnemonicMatch DISABLED_MnemonicMatch
#else
#define MAYBE_MnemonicMatch MnemonicMatch
#endif

VIEW_TEST(MenuControllerMnemonicTestMnemonicMatch, MAYBE_MnemonicMatch);

// Pressing a key which matches the first letter of the menu item's title
// should execute the command for that menu item.
typedef MenuControllerMnemonicTest<ui::VKEY_T,2>
    MenuControllerMnemonicTestTitleMatch;

#if defined(USE_OZONE) || defined(OS_WIN)
// ozone bringup - http://crbug.com/401304
// flaky on windows - http://crbug.com/523255
#define MAYBE_TitleMatch DISABLED_TitleMatch
#else
#define MAYBE_TitleMatch TitleMatch
#endif

VIEW_TEST(MenuControllerMnemonicTestTitleMatch, MAYBE_TitleMatch);

// Pressing an arbitrary key should not execute any commands.
typedef MenuControllerMnemonicTest<ui::VKEY_A,0>
    MenuControllerMnemonicTestNoMatch;

#if defined(USE_OZONE)
// ozone bringup - http://crbug.com/401304
#define MAYBE_NoMatch DISABLED_NoMatch
#else
#define MAYBE_NoMatch NoMatch
#endif

VIEW_TEST(MenuControllerMnemonicTestNoMatch, MAYBE_NoMatch);
