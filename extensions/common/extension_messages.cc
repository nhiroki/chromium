// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/extension_messages.h"

#include "content/public/common/common_param_traits.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest.h"
#include "extensions/common/manifest_handler.h"
#include "extensions/common/permissions/permissions_data.h"
#include "extensions/common/permissions/permissions_info.h"

using extensions::APIPermission;
using extensions::APIPermissionInfo;
using extensions::APIPermissionSet;
using extensions::Extension;
using extensions::Manifest;
using extensions::ManifestHandler;
using extensions::ManifestPermission;
using extensions::ManifestPermissionSet;
using extensions::PermissionSet;
using extensions::URLPatternSet;

ExtensionMsg_PermissionSetStruct::ExtensionMsg_PermissionSetStruct() {
}

ExtensionMsg_PermissionSetStruct::ExtensionMsg_PermissionSetStruct(
    const PermissionSet& permissions)
    : apis(permissions.apis()),
      manifest_permissions(permissions.manifest_permissions()),
      explicit_hosts(permissions.explicit_hosts()),
      scriptable_hosts(permissions.scriptable_hosts()) {
}

ExtensionMsg_PermissionSetStruct::~ExtensionMsg_PermissionSetStruct() {
}

scoped_refptr<const PermissionSet>
ExtensionMsg_PermissionSetStruct::ToPermissionSet() const {
  return new PermissionSet(
      apis, manifest_permissions, explicit_hosts, scriptable_hosts);
}

ExtensionMsg_Loaded_Params::ExtensionMsg_Loaded_Params()
    : location(Manifest::INVALID_LOCATION),
      creation_flags(Extension::NO_FLAGS) {}

ExtensionMsg_Loaded_Params::~ExtensionMsg_Loaded_Params() {}

ExtensionMsg_Loaded_Params::ExtensionMsg_Loaded_Params(
    const Extension* extension)
    : manifest(extension->manifest()->value()->DeepCopy()),
      location(extension->location()),
      path(extension->path()),
      active_permissions(*extension->permissions_data()->active_permissions()),
      withheld_permissions(
          *extension->permissions_data()->withheld_permissions()),
      id(extension->id()),
      creation_flags(extension->creation_flags()) {
}

scoped_refptr<Extension> ExtensionMsg_Loaded_Params::ConvertToExtension(
    std::string* error) const {
  scoped_refptr<Extension> extension =
      Extension::Create(path, location, *manifest, creation_flags, error);
  if (extension.get()) {
    extension->permissions_data()->SetPermissions(
        active_permissions.ToPermissionSet(),
        withheld_permissions.ToPermissionSet());
  }
  return extension;
}

namespace IPC {

template <>
struct ParamTraits<Manifest::Location> {
  typedef Manifest::Location param_type;
  static void Write(Message* m, const param_type& p) {
    int val = static_cast<int>(p);
    WriteParam(m, val);
  }
  static bool Read(const Message* m, PickleIterator* iter, param_type* p) {
    int val = 0;
    if (!ReadParam(m, iter, &val) ||
        val < Manifest::INVALID_LOCATION ||
        val >= Manifest::NUM_LOCATIONS)
      return false;
    *p = static_cast<param_type>(val);
    return true;
  }
  static void Log(const param_type& p, std::string* l) {
    ParamTraits<int>::Log(static_cast<int>(p), l);
  }
};

void ParamTraits<URLPattern>::Write(Message* m, const param_type& p) {
  WriteParam(m, p.valid_schemes());
  WriteParam(m, p.GetAsString());
}

bool ParamTraits<URLPattern>::Read(const Message* m, PickleIterator* iter,
                                   param_type* p) {
  int valid_schemes;
  std::string spec;
  if (!ReadParam(m, iter, &valid_schemes) ||
      !ReadParam(m, iter, &spec))
    return false;

  // TODO(jstritar): We don't want the URLPattern to fail parsing when the
  // scheme is invalid. Instead, the pattern should parse but it should not
  // match the invalid patterns. We get around this by setting the valid
  // schemes after parsing the pattern. Update these method calls once we can
  // ignore scheme validation with URLPattern parse options. crbug.com/90544
  p->SetValidSchemes(URLPattern::SCHEME_ALL);
  URLPattern::ParseResult result = p->Parse(spec);
  p->SetValidSchemes(valid_schemes);
  return URLPattern::PARSE_SUCCESS == result;
}

void ParamTraits<URLPattern>::Log(const param_type& p, std::string* l) {
  LogParam(p.GetAsString(), l);
}

void ParamTraits<URLPatternSet>::Write(Message* m, const param_type& p) {
  WriteParam(m, p.patterns());
}

bool ParamTraits<URLPatternSet>::Read(const Message* m, PickleIterator* iter,
                                        param_type* p) {
  std::set<URLPattern> patterns;
  if (!ReadParam(m, iter, &patterns))
    return false;

  for (std::set<URLPattern>::iterator i = patterns.begin();
       i != patterns.end(); ++i)
    p->AddPattern(*i);
  return true;
}

void ParamTraits<URLPatternSet>::Log(const param_type& p, std::string* l) {
  LogParam(p.patterns(), l);
}

void ParamTraits<APIPermission::ID>::Write(
    Message* m, const param_type& p) {
  WriteParam(m, static_cast<int>(p));
}

bool ParamTraits<APIPermission::ID>::Read(
    const Message* m, PickleIterator* iter, param_type* p) {
  int api_id = -2;
  if (!ReadParam(m, iter, &api_id))
    return false;

  *p = static_cast<APIPermission::ID>(api_id);
  return true;
}

void ParamTraits<APIPermission::ID>::Log(
    const param_type& p, std::string* l) {
  LogParam(static_cast<int>(p), l);
}

void ParamTraits<APIPermissionSet>::Write(
    Message* m, const param_type& p) {
  APIPermissionSet::const_iterator it = p.begin();
  const APIPermissionSet::const_iterator end = p.end();
  WriteParam(m, p.size());
  for (; it != end; ++it) {
    WriteParam(m, it->id());
    it->Write(m);
  }
}

bool ParamTraits<APIPermissionSet>::Read(
    const Message* m, PickleIterator* iter, param_type* r) {
  size_t size;
  if (!ReadParam(m, iter, &size))
    return false;
  for (size_t i = 0; i < size; ++i) {
    APIPermission::ID id;
    if (!ReadParam(m, iter, &id))
      return false;
    const APIPermissionInfo* permission_info =
      extensions::PermissionsInfo::GetInstance()->GetByID(id);
    if (!permission_info)
      return false;
    scoped_ptr<APIPermission> p(permission_info->CreateAPIPermission());
    if (!p->Read(m, iter))
      return false;
    r->insert(p.release());
  }
  return true;
}

void ParamTraits<APIPermissionSet>::Log(
    const param_type& p, std::string* l) {
  LogParam(p.map(), l);
}

void ParamTraits<ManifestPermissionSet>::Write(
    Message* m, const param_type& p) {
  ManifestPermissionSet::const_iterator it = p.begin();
  const ManifestPermissionSet::const_iterator end = p.end();
  WriteParam(m, p.size());
  for (; it != end; ++it) {
    WriteParam(m, it->name());
    it->Write(m);
  }
}

bool ParamTraits<ManifestPermissionSet>::Read(
    const Message* m, PickleIterator* iter, param_type* r) {
  size_t size;
  if (!ReadParam(m, iter, &size))
    return false;
  for (size_t i = 0; i < size; ++i) {
    std::string name;
    if (!ReadParam(m, iter, &name))
      return false;
    scoped_ptr<ManifestPermission> p(ManifestHandler::CreatePermission(name));
    if (!p)
      return false;
    if (!p->Read(m, iter))
      return false;
    r->insert(p.release());
  }
  return true;
}

void ParamTraits<ManifestPermissionSet>::Log(
    const param_type& p, std::string* l) {
  LogParam(p.map(), l);
}

void ParamTraits<ExtensionMsg_PermissionSetStruct>::Write(Message* m,
                                                          const param_type& p) {
  WriteParam(m, p.apis);
  WriteParam(m, p.manifest_permissions);
  WriteParam(m, p.explicit_hosts);
  WriteParam(m, p.scriptable_hosts);
}

bool ParamTraits<ExtensionMsg_PermissionSetStruct>::Read(const Message* m,
                                                         PickleIterator* iter,
                                                         param_type* p) {
  return ReadParam(m, iter, &p->apis) &&
         ReadParam(m, iter, &p->manifest_permissions) &&
         ReadParam(m, iter, &p->explicit_hosts) &&
         ReadParam(m, iter, &p->scriptable_hosts);
}

void ParamTraits<ExtensionMsg_PermissionSetStruct>::Log(const param_type& p,
                                                        std::string* l) {
  LogParam(p.apis, l);
  LogParam(p.manifest_permissions, l);
  LogParam(p.explicit_hosts, l);
  LogParam(p.scriptable_hosts, l);
}

void ParamTraits<ExtensionMsg_Loaded_Params>::Write(Message* m,
                                                    const param_type& p) {
  WriteParam(m, p.location);
  WriteParam(m, p.path);
  WriteParam(m, *(p.manifest));
  WriteParam(m, p.creation_flags);
  WriteParam(m, p.active_permissions);
  WriteParam(m, p.withheld_permissions);
}

bool ParamTraits<ExtensionMsg_Loaded_Params>::Read(const Message* m,
                                                   PickleIterator* iter,
                                                   param_type* p) {
  p->manifest.reset(new base::DictionaryValue());
  return ReadParam(m, iter, &p->location) && ReadParam(m, iter, &p->path) &&
         ReadParam(m, iter, p->manifest.get()) &&
         ReadParam(m, iter, &p->creation_flags) &&
         ReadParam(m, iter, &p->active_permissions) &&
         ReadParam(m, iter, &p->withheld_permissions);
}

void ParamTraits<ExtensionMsg_Loaded_Params>::Log(const param_type& p,
                                                  std::string* l) {
  l->append(p.id);
}

}  // namespace IPC
