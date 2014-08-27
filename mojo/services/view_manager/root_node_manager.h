// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_SERVICES_VIEW_MANAGER_ROOT_NODE_MANAGER_H_
#define MOJO_SERVICES_VIEW_MANAGER_ROOT_NODE_MANAGER_H_

#include <map>
#include <set>

#include "base/basictypes.h"
#include "mojo/public/cpp/bindings/array.h"
#include "mojo/services/view_manager/display_manager.h"
#include "mojo/services/view_manager/ids.h"
#include "mojo/services/view_manager/node.h"
#include "mojo/services/view_manager/node_delegate.h"
#include "mojo/services/view_manager/view_manager_export.h"

namespace ui {
class Event;
}

namespace mojo {

class ApplicationConnection;

namespace service {

class DisplayManagerDelegate;
class ViewManagerServiceImpl;

// RootNodeManager is responsible for managing the set of
// ViewManagerServiceImpls as well as providing the root of the node hierarchy.
class MOJO_VIEW_MANAGER_EXPORT RootNodeManager : public NodeDelegate {
 public:
  // Create when a ViewManagerServiceImpl is about to make a change. Ensures
  // clients are notified of the correct change id.
  class ScopedChange {
   public:
    ScopedChange(ViewManagerServiceImpl* connection,
                 RootNodeManager* root,
                 bool is_delete_node);
    ~ScopedChange();

    ConnectionSpecificId connection_id() const { return connection_id_; }
    bool is_delete_node() const { return is_delete_node_; }

    // Marks the connection with the specified id as having seen a message.
    void MarkConnectionAsMessaged(ConnectionSpecificId connection_id) {
      message_ids_.insert(connection_id);
    }

    // Returns true if MarkConnectionAsMessaged(connection_id) was invoked.
    bool DidMessageConnection(ConnectionSpecificId connection_id) const {
      return message_ids_.count(connection_id) > 0;
    }

   private:
    RootNodeManager* root_;
    const ConnectionSpecificId connection_id_;
    const bool is_delete_node_;

    // See description of MarkConnectionAsMessaged/DidMessageConnection.
    std::set<ConnectionSpecificId> message_ids_;

    DISALLOW_COPY_AND_ASSIGN(ScopedChange);
  };

  RootNodeManager(ApplicationConnection* app_connection,
                  DisplayManagerDelegate* display_manager_delegate,
                  const Callback<void()>& native_viewport_closed_callback);
  virtual ~RootNodeManager();

  // Returns the id for the next ViewManagerServiceImpl.
  ConnectionSpecificId GetAndAdvanceNextConnectionId();

  void AddConnection(ViewManagerServiceImpl* connection);
  void RemoveConnection(ViewManagerServiceImpl* connection);

  // Establishes the initial client. Similar to Connect(), but the resulting
  // client is allowed to do anything.
  void EmbedRoot(const std::string& url,
                 InterfaceRequest<ServiceProvider> service_provider);

  // See description of ViewManagerService::Embed() for details. This assumes
  // |transport_node_id| is valid.
  void Embed(ConnectionSpecificId creator_id,
             const String& url,
             Id transport_node_id,
             InterfaceRequest<ServiceProvider> service_provider);

  // Returns the connection by id.
  ViewManagerServiceImpl* GetConnection(ConnectionSpecificId connection_id);

  // Returns the Node identified by |id|.
  Node* GetNode(const NodeId& id);

  Node* root() { return root_.get(); }

  bool IsProcessingChange() const { return current_change_ != NULL; }

  bool is_processing_delete_node() const {
    return current_change_ && current_change_->is_delete_node(); }

  // Invoked when a connection messages a client about the change. This is used
  // to avoid sending ServerChangeIdAdvanced() unnecessarily.
  void OnConnectionMessagedClient(ConnectionSpecificId id);

  // Returns true if OnConnectionMessagedClient() was invoked for id.
  bool DidConnectionMessageClient(ConnectionSpecificId id) const;

  ViewManagerServiceImpl* GetConnectionByCreator(
      ConnectionSpecificId creator_id,
      const std::string& url) const;

  // Returns the ViewManagerServiceImpl that has |id| as a root.
  ViewManagerServiceImpl* GetConnectionWithRoot(const NodeId& id) {
    return const_cast<ViewManagerServiceImpl*>(
        const_cast<const RootNodeManager*>(this)->GetConnectionWithRoot(id));
  }
  const ViewManagerServiceImpl* GetConnectionWithRoot(const NodeId& id) const;

  void DispatchNodeInputEventToWindowManager(EventPtr event);

  // These functions trivially delegate to all ViewManagerServiceImpls, which in
  // term notify their clients.
  void ProcessNodeDestroyed(Node* node);
  void ProcessNodeBoundsChanged(const Node* node,
                                const gfx::Rect& old_bounds,
                                const gfx::Rect& new_bounds);
  void ProcessNodeHierarchyChanged(const Node* node,
                                   const Node* new_parent,
                                   const Node* old_parent);
  void ProcessNodeReorder(const Node* node,
                          const Node* relative_node,
                          const OrderDirection direction);
  void ProcessNodeDeleted(const NodeId& node);

 private:
  // Used to setup any static state needed by RootNodeManager.
  struct Context {
    Context();
    ~Context();
  };

  typedef std::map<ConnectionSpecificId, ViewManagerServiceImpl*> ConnectionMap;

  // Invoked when a connection is about to make a change.  Subsequently followed
  // by FinishChange() once the change is done.
  //
  // Changes should never nest, meaning each PrepareForChange() must be
  // balanced with a call to FinishChange() with no PrepareForChange()
  // in between.
  void PrepareForChange(ScopedChange* change);

  // Balances a call to PrepareForChange().
  void FinishChange();

  // Returns true if the specified connection originated the current change.
  bool IsChangeSource(ConnectionSpecificId connection_id) const {
    return current_change_ && current_change_->connection_id() == connection_id;
  }

  // Implementation of the two embed variants.
  ViewManagerServiceImpl* EmbedImpl(
      ConnectionSpecificId creator_id,
      const String& url,
      const NodeId& root_id,
      InterfaceRequest<ServiceProvider> service_provider);

  // Overridden from NodeDelegate:
  virtual void OnNodeDestroyed(const Node* node) OVERRIDE;
  virtual void OnNodeHierarchyChanged(const Node* node,
                                      const Node* new_parent,
                                      const Node* old_parent) OVERRIDE;
  virtual void OnNodeBoundsChanged(const Node* node,
                                   const gfx::Rect& old_bounds,
                                   const gfx::Rect& new_bounds) OVERRIDE;
  virtual void OnNodeBitmapChanged(const Node* node) OVERRIDE;

  Context context_;

  ApplicationConnection* app_connection_;

  // ID to use for next ViewManagerServiceImpl.
  ConnectionSpecificId next_connection_id_;

  // Set of ViewManagerServiceImpls.
  ConnectionMap connection_map_;

  DisplayManager display_manager_;

  // Root node.
  scoped_ptr<Node> root_;

  // Set of ViewManagerServiceImpls created by way of Connect(). These have to
  // be explicitly destroyed.
  std::set<ViewManagerServiceImpl*> connections_created_by_connect_;

  // If non-null we're processing a change. The ScopedChange is not owned by us
  // (it's created on the stack by ViewManagerServiceImpl).
  ScopedChange* current_change_;

  DISALLOW_COPY_AND_ASSIGN(RootNodeManager);
};

}  // namespace service
}  // namespace mojo

#endif  // MOJO_SERVICES_VIEW_MANAGER_ROOT_NODE_MANAGER_H_
