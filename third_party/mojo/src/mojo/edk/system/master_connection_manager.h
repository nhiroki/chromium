// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_SYSTEM_MASTER_CONNECTION_MANAGER_H_
#define MOJO_EDK_SYSTEM_MASTER_CONNECTION_MANAGER_H_

#include <stdint.h>

#include "base/containers/hash_tables.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/edk/system/connection_manager.h"
#include "mojo/edk/system/system_impl_export.h"

namespace base {
class TaskRunner;
class WaitableEvent;
}

namespace mojo {

namespace embedder {
class MasterProcessDelegate;
class SlaveInfo;
}

namespace system {

// The master process will always have this "process identifier".
const ProcessIdentifier kMasterProcessIdentifier = 1;

// The |ConnectionManager| implementation for the master process.
//
// Objects of this class must be created, initialized (via |Init()|), shut down
// (via |Shutdown()|), and destroyed on the same thread (the "creation thread").
// Otherwise, its public methods are thread-safe (except that they may not be
// called from its internal, private thread).
class MOJO_SYSTEM_IMPL_EXPORT MasterConnectionManager
    : public ConnectionManager {
 public:
  // Note: None of the public methods may be called from |private_thread_|.

  MasterConnectionManager();
  ~MasterConnectionManager() override;

  // No other methods may be called until after this has been called.
  // |master_process_delegate| must stay alive at least until after |Shutdown()|
  // has been called; its methods will be called on this object's creation
  // thread.
  void Init(embedder::MasterProcessDelegate* master_process_delegate);

  // No other methods may be called after this is (or while it is being) called.
  void Shutdown();

  // Adds a slave process and sets up/tracks a connection to that slave (using
  // |platform_handle|). (|slave_info| is used by the caller/implementation of
  // |embedder::MasterProcessDelegate| to track this process; ownership of
  // |slave_info| will be returned to the delegate via |OnSlaveDisconnect()|,
  // which will always be called for each slave, assuming proper shutdown.)
  void AddSlave(scoped_ptr<embedder::SlaveInfo> slave_info,
                embedder::ScopedPlatformHandle platform_handle);

  // |ConnectionManager| methods:
  bool AllowConnect(const ConnectionIdentifier& connection_id) override;
  bool CancelConnect(const ConnectionIdentifier& connection_id) override;
  bool Connect(const ConnectionIdentifier& connection_id,
               ProcessIdentifier* peer_process_identifier,
               embedder::ScopedPlatformHandle* platform_handle) override;

 private:
  class Helper;

  // These should be thread-safe and may be called on any thread, including
  // |private_thread_|:
  bool AllowConnectImpl(ProcessIdentifier process_identifier,
                        const ConnectionIdentifier& connection_id);
  bool CancelConnectImpl(ProcessIdentifier process_identifier,
                         const ConnectionIdentifier& connection_id);
  bool ConnectImpl(ProcessIdentifier process_identifier,
                   const ConnectionIdentifier& connection_id,
                   ProcessIdentifier* peer_process_identifier,
                   embedder::ScopedPlatformHandle* platform_handle);

  // These should only be called on |private_thread_|:
  void ShutdownOnPrivateThread();
  // Signals |*event| on completion.
  void AddSlaveOnPrivateThread(scoped_ptr<embedder::SlaveInfo> slave_info,
                               embedder::ScopedPlatformHandle platform_handle,
                               base::WaitableEvent* event);
  // Called by |Helper::OnError()|.
  void OnError(ProcessIdentifier process_identifier);
  // Posts a call to |master_process_delegate_->OnSlaveDisconnect()|.
  void CallOnSlaveDisconnect(scoped_ptr<embedder::SlaveInfo> slave_info);

  // Asserts that the current thread is the creation thread. (This actually
  // checks the current message loop, which is what we depend on, not the thread
  // per se.)
  void AssertOnCreationThread() const;

  // Asserts that the current thread is *not* |private_thread_| (no-op if
  // DCHECKs are not enabled). This should only be called while
  // |private_thread_| is alive (i.e., after |Init()| but before |Shutdown()|).
  void AssertNotOnPrivateThread() const;

  // Asserts that the current thread is |private_thread_| (no-op if DCHECKs are
  // not enabled). This should only be called while |private_thread_| is alive
  // (i.e., after |Init()| but before |Shutdown()|).
  void AssertOnPrivateThread() const;

  const scoped_refptr<base::TaskRunner> creation_thread_task_runner_;

  // This is set in |Init()| before |private_thread_| exists and only cleared in
  // |Shutdown()| after |private_thread_| is dead. Thus it's safe to "use" on
  // |private_thread_|. (Note that |master_process_delegate_| may only be called
  // from the creation thread.)
  embedder::MasterProcessDelegate* master_process_delegate_;

  // This is a private I/O thread on which this class does the bulk of its work.
  // It is started in |Init()| and terminated in |Shutdown()|.
  base::Thread private_thread_;

  // The following members are only accessed on |private_thread_|:
  ProcessIdentifier next_process_identifier_;
  base::hash_map<ProcessIdentifier, Helper*> helpers_;  // Owns its values.

  // Protects the members below (except in the constructor, |Init()|,
  // |Shutdown()|/|ShutdownOnPrivateThread()|, and the destructor).
  base::Lock lock_;

  struct PendingConnectionInfo;
  base::hash_map<ConnectionIdentifier, PendingConnectionInfo*>
      pending_connections_;  // Owns its values.

  DISALLOW_COPY_AND_ASSIGN(MasterConnectionManager);
};

}  // namespace system
}  // namespace mojo

#endif  // MOJO_EDK_SYSTEM_MASTER_CONNECTION_MANAGER_H_
