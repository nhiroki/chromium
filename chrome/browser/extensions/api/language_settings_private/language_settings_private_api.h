// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_LANGUAGE_SETTINGS_PRIVATE_LANGUAGE_SETTINGS_PRIVATE_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_LANGUAGE_SETTINGS_PRIVATE_LANGUAGE_SETTINGS_PRIVATE_API_H_

#include "base/macros.h"
#include "extensions/browser/extension_function.h"

namespace extensions {

// Implements the languageSettingsPrivate.getLanguageList method.
class LanguageSettingsPrivateGetLanguageListFunction
    : public UIThreadExtensionFunction {
 public:
  LanguageSettingsPrivateGetLanguageListFunction();
  DECLARE_EXTENSION_FUNCTION("languageSettingsPrivate.getLanguageList",
                             LANGUAGESETTINGSPRIVATE_GETLANGUAGELIST)

 protected:
  ~LanguageSettingsPrivateGetLanguageListFunction() override;

  // ExtensionFunction overrides.
  ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(LanguageSettingsPrivateGetLanguageListFunction);
};

// Implements the languageSettingsPrivate.setLanguageList method.
class LanguageSettingsPrivateSetLanguageListFunction
    : public UIThreadExtensionFunction {
 public:
  LanguageSettingsPrivateSetLanguageListFunction();
  DECLARE_EXTENSION_FUNCTION("languageSettingsPrivate.setLanguageList",
                             LANGUAGESETTINGSPRIVATE_SETLANGUAGELIST)

 protected:
  ~LanguageSettingsPrivateSetLanguageListFunction() override;

  // ExtensionFunction overrides.
  ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(LanguageSettingsPrivateSetLanguageListFunction);
};

// Implements the languageSettingsPrivate.getSpellcheckDictionaryStatuses
// method.
class LanguageSettingsPrivateGetSpellcheckDictionaryStatusesFunction
    : public UIThreadExtensionFunction {
 public:
  LanguageSettingsPrivateGetSpellcheckDictionaryStatusesFunction();
  DECLARE_EXTENSION_FUNCTION(
      "languageSettingsPrivate.getSpellcheckDictionaryStatuses",
      LANGUAGESETTINGSPRIVATE_GETSPELLCHECKDICTIONARYSTATUS)

 protected:
  ~LanguageSettingsPrivateGetSpellcheckDictionaryStatusesFunction() override;

  // ExtensionFunction overrides.
  ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(
      LanguageSettingsPrivateGetSpellcheckDictionaryStatusesFunction);
};

// Implements the languageSettingsPrivate.getSpellcheckWords method.
class LanguageSettingsPrivateGetSpellcheckWordsFunction
    : public UIThreadExtensionFunction {
 public:
  LanguageSettingsPrivateGetSpellcheckWordsFunction();
  DECLARE_EXTENSION_FUNCTION("languageSettingsPrivate.getSpellcheckWords",
                             LANGUAGESETTINGSPRIVATE_GETSPELLCHECKWORDS)

 protected:
  ~LanguageSettingsPrivateGetSpellcheckWordsFunction() override;

  // ExtensionFunction overrides.
  ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(LanguageSettingsPrivateGetSpellcheckWordsFunction);
};

// Implements the languageSettingsPrivate.getTranslateTargetLanguage method.
class LanguageSettingsPrivateGetTranslateTargetLanguageFunction
    : public UIThreadExtensionFunction {
 public:
  LanguageSettingsPrivateGetTranslateTargetLanguageFunction();
  DECLARE_EXTENSION_FUNCTION(
      "languageSettingsPrivate.getTranslateTargetLanguage",
      LANGUAGESETTINGSPRIVATE_GETTRANSLATETARGETLANGUAGE)

 protected:
  ~LanguageSettingsPrivateGetTranslateTargetLanguageFunction() override;

  // ExtensionFunction overrides.
  ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(
      LanguageSettingsPrivateGetTranslateTargetLanguageFunction);
};

// Implements the languageSettingsPrivate.getInputMethodLists method.
class LanguageSettingsPrivateGetInputMethodListsFunction
    : public UIThreadExtensionFunction {
 public:
  LanguageSettingsPrivateGetInputMethodListsFunction();
  DECLARE_EXTENSION_FUNCTION("languageSettingsPrivate.getInputMethodLists",
                             LANGUAGESETTINGSPRIVATE_GETINPUTMETHODLISTS)

 protected:
  ~LanguageSettingsPrivateGetInputMethodListsFunction() override;

  // ExtensionFunction overrides.
  ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(LanguageSettingsPrivateGetInputMethodListsFunction);
};

// Implements the languageSettingsPrivate.addInputMethod method.
class LanguageSettingsPrivateAddInputMethodFunction
    : public UIThreadExtensionFunction {
 public:
  LanguageSettingsPrivateAddInputMethodFunction();
  DECLARE_EXTENSION_FUNCTION("languageSettingsPrivate.addInputMethod",
                             LANGUAGESETTINGSPRIVATE_ADDINPUTMETHOD)

 protected:
  ~LanguageSettingsPrivateAddInputMethodFunction() override;

  // ExtensionFunction overrides.
  ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(LanguageSettingsPrivateAddInputMethodFunction);
};

// Implements the languageSettingsPrivate.removeInputMethod method.
class LanguageSettingsPrivateRemoveInputMethodFunction
    : public UIThreadExtensionFunction {
 public:
  LanguageSettingsPrivateRemoveInputMethodFunction();
  DECLARE_EXTENSION_FUNCTION("languageSettingsPrivate.removeInputMethod",
                             LANGUAGESETTINGSPRIVATE_REMOVEINPUTMETHOD)

 protected:
  ~LanguageSettingsPrivateRemoveInputMethodFunction() override;

  // ExtensionFunction overrides.
  ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(LanguageSettingsPrivateRemoveInputMethodFunction);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_LANGUAGE_SETTINGS_PRIVATE_LANGUAGE_SETTINGS_PRIVATE_API_H_
