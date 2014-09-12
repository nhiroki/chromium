// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_FILE_SYSTEM_PROVIDER_OPERATIONS_OPERATION_H_
#define CHROME_BROWSER_CHROMEOS_FILE_SYSTEM_PROVIDER_OPERATIONS_OPERATION_H_

#include <string>

#include "base/files/file.h"
#include "base/memory/scoped_ptr.h"
#include "chrome/browser/chromeos/file_system_provider/provided_file_system_info.h"
#include "chrome/browser/chromeos/file_system_provider/request_manager.h"
#include "storage/browser/fileapi/async_file_util.h"

namespace base {
class ListValue;
}  // namespace base

namespace extensions {
struct Event;
class EventRouter;
}  // namespace extensions

namespace chromeos {
namespace file_system_provider {
namespace operations {

// Base class for operation bridges between fileapi and providing extensions.
class Operation : public RequestManager::HandlerInterface {
 public:
  typedef base::Callback<bool(scoped_ptr<extensions::Event> event)>
      DispatchEventImplCallback;

  Operation(extensions::EventRouter* event_router,
            const ProvidedFileSystemInfo& file_system_info);
  virtual ~Operation();

  // RequestManager::HandlerInterface overrides.
  virtual bool Execute(int request_id) OVERRIDE = 0;
  virtual void OnSuccess(int request_id,
                         scoped_ptr<RequestValue> result,
                         bool has_more) OVERRIDE = 0;
  virtual void OnError(int request_id,
                       scoped_ptr<RequestValue> result,
                       base::File::Error error) OVERRIDE = 0;

  // Sets custom dispatchign event implementation for tests.
  void SetDispatchEventImplForTesting(
      const DispatchEventImplCallback& callback);

 protected:
  // Sends an event to the providing extension. Returns false, if the providing
  // extension does not handle the |event_name| event.
  bool SendEvent(int request_id,
                 const std::string& event_name,
                 scoped_ptr<base::ListValue> event_args);

  ProvidedFileSystemInfo file_system_info_;

 private:
  DispatchEventImplCallback dispatch_event_impl_;
  DISALLOW_COPY_AND_ASSIGN(Operation);
};

}  // namespace operations
}  // namespace file_system_provider
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_FILE_SYSTEM_PROVIDER_OPERATIONS_OPERATION_H_
