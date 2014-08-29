// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_PEPPER_PLUGIN_OBJECT_H_
#define CONTENT_RENDERER_PEPPER_PLUGIN_OBJECT_H_

#include <string>

#include "base/basictypes.h"
#include "base/memory/weak_ptr.h"
#include "gin/interceptor.h"
#include "gin/wrappable.h"
#include "ppapi/c/pp_var.h"

struct PPP_Class_Deprecated;

namespace gin {
  class Arguments;
}  // namespace gin

namespace content {

class PepperPluginInstanceImpl;

// A PluginObject is a JS-accessible object implemented by the plugin.
//
// In contrast, a var of type PP_VARTYPE_OBJECT is a reference to a JS object,
// which might be implemented by the plugin (here) or by the JS engine.
class PluginObject : public gin::Wrappable<PluginObject>,
                     public gin::NamedPropertyInterceptor {
 public:
  static gin::WrapperInfo kWrapperInfo;

  virtual ~PluginObject();

  // Returns the PluginObject which is contained in the given v8 object, or NULL
  // if the object isn't backed by a PluginObject.
  static PluginObject* FromV8Object(v8::Isolate* isolate,
                                    v8::Handle<v8::Object> v8_object);

  // Allocates a new PluginObject and returns it as a PP_Var with a
  // refcount of 1.
  static PP_Var Create(PepperPluginInstanceImpl* instance,
                       const PPP_Class_Deprecated* ppp_class,
                       void* ppp_class_data);

  // gin::NamedPropertyInterceptor
  virtual v8::Local<v8::Value> GetNamedProperty(
      v8::Isolate* isolate,
      const std::string& property) OVERRIDE;
  virtual std::vector<std::string> EnumerateNamedProperties(
      v8::Isolate* isolate) OVERRIDE;

  const PPP_Class_Deprecated* ppp_class() { return ppp_class_; }
  void* ppp_class_data() { return ppp_class_data_; }

  // Called when the instance is destroyed.
  void InstanceDeleted();

 private:
  PluginObject(PepperPluginInstanceImpl* instance,
               const PPP_Class_Deprecated* ppp_class,
               void* ppp_class_data);

  // gin::Wrappable
  virtual gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) OVERRIDE;

  // Helper method to get named properties.
  v8::Local<v8::Value> GetPropertyOrMethod(v8::Isolate* isolate,
                                           PP_Var identifier_var);

  void Call(const std::string& identifier, gin::Arguments* args);

  PepperPluginInstanceImpl* instance_;

  const PPP_Class_Deprecated* ppp_class_;
  void* ppp_class_data_;

  base::WeakPtrFactory<PluginObject> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PluginObject);
};

}  // namespace content

#endif  // CONTENT_RENDERER_PEPPER_PLUGIN_OBJECT_H_
