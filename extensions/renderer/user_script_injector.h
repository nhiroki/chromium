// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_USER_SCRIPT_INJECTOR_H_
#define EXTENSIONS_RENDERER_USER_SCRIPT_INJECTOR_H_

#include <string>

#include "base/memory/scoped_ptr.h"
#include "base/scoped_observer.h"
#include "extensions/common/user_script.h"
#include "extensions/renderer/script_injection.h"
#include "extensions/renderer/user_script_set.h"

namespace blink {
class WebFrame;
}

namespace extensions {
class Extension;

// A ScriptInjector for UserScripts.
class UserScriptInjector : public ScriptInjector,
                           public UserScriptSet::Observer {
 public:
  UserScriptInjector(const UserScript* user_script,
                     UserScriptSet* user_script_set);
  virtual ~UserScriptInjector();

 private:
  // UserScriptSet::Observer implementation.
  virtual void OnUserScriptsUpdated(
      const std::set<std::string>& changed_extensions,
      const std::vector<UserScript*>& scripts) OVERRIDE;

  // ScriptInjector implementation.
  virtual UserScript::InjectionType script_type() const OVERRIDE;
  virtual bool ShouldExecuteInChildFrames() const OVERRIDE;
  virtual bool ShouldExecuteInMainWorld() const OVERRIDE;
  virtual bool IsUserGesture() const OVERRIDE;
  virtual bool ExpectsResults() const OVERRIDE;
  virtual bool ShouldInjectJs(
      UserScript::RunLocation run_location) const OVERRIDE;
  virtual bool ShouldInjectCss(
      UserScript::RunLocation run_location) const OVERRIDE;
  virtual PermissionsData::AccessType CanExecuteOnFrame(
      const Extension* extension,
      blink::WebFrame* web_frame,
      int tab_id,
      const GURL& top_url) const OVERRIDE;
  virtual std::vector<blink::WebScriptSource> GetJsSources(
      UserScript::RunLocation run_location) const OVERRIDE;
  virtual std::vector<std::string> GetCssSources(
      UserScript::RunLocation run_location) const OVERRIDE;
  virtual void OnInjectionComplete(
      scoped_ptr<base::ListValue> execution_results,
      ScriptsRunInfo* scripts_run_info,
      UserScript::RunLocation run_location) OVERRIDE;
  virtual void OnWillNotInject(InjectFailureReason reason) OVERRIDE;

  // The associated user script. Owned by the UserScriptInjector that created
  // this object.
  const UserScript* script_;

  // The id of the associated user script. We cache this because when we update
  // the |script_| associated with this injection, the old referance may be
  // deleted.
  int script_id_;

  // The associated extension id, preserved for the same reason as |script_id|.
  std::string extension_id_;

  ScopedObserver<UserScriptSet, UserScriptSet::Observer>
      user_script_set_observer_;

  DISALLOW_COPY_AND_ASSIGN(UserScriptInjector);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_USER_SCRIPT_INJECTOR_H_
