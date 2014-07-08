// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/services/public/cpp/view_manager/node.h"

#include "mojo/services/public/cpp/view_manager/lib/node_private.h"
#include "mojo/services/public/cpp/view_manager/lib/view_manager_client_impl.h"
#include "mojo/services/public/cpp/view_manager/lib/view_private.h"
#include "mojo/services/public/cpp/view_manager/node_observer.h"
#include "mojo/services/public/cpp/view_manager/view.h"

namespace mojo {
namespace view_manager {

namespace {

void NotifyViewTreeChangeAtReceiver(
    Node* receiver,
    const NodeObserver::TreeChangeParams& params,
    bool change_applied) {
  NodeObserver::TreeChangeParams local_params = params;
  local_params.receiver = receiver;
  if (change_applied) {
    FOR_EACH_OBSERVER(NodeObserver,
                      *NodePrivate(receiver).observers(),
                      OnTreeChanged(local_params));
  } else {
    FOR_EACH_OBSERVER(NodeObserver,
                      *NodePrivate(receiver).observers(),
                      OnTreeChanging(local_params));
  }
}

void NotifyViewTreeChangeUp(
    Node* start_at,
    const NodeObserver::TreeChangeParams& params,
    bool change_applied) {
  for (Node* current = start_at; current; current = current->parent())
    NotifyViewTreeChangeAtReceiver(current, params, change_applied);
}

void NotifyViewTreeChangeDown(
    Node* start_at,
    const NodeObserver::TreeChangeParams& params,
    bool change_applied) {
  NotifyViewTreeChangeAtReceiver(start_at, params, change_applied);
  Node::Children::const_iterator it = start_at->children().begin();
  for (; it != start_at->children().end(); ++it)
    NotifyViewTreeChangeDown(*it, params, change_applied);
}

void NotifyViewTreeChange(
    const NodeObserver::TreeChangeParams& params,
    bool change_applied) {
  NotifyViewTreeChangeDown(params.target, params, change_applied);
  if (params.old_parent)
    NotifyViewTreeChangeUp(params.old_parent, params, change_applied);
  if (params.new_parent)
    NotifyViewTreeChangeUp(params.new_parent, params, change_applied);
}

class ScopedTreeNotifier {
 public:
  ScopedTreeNotifier(Node* target, Node* old_parent, Node* new_parent) {
    params_.target = target;
    params_.old_parent = old_parent;
    params_.new_parent = new_parent;
    NotifyViewTreeChange(params_, false);
  }
  ~ScopedTreeNotifier() {
    NotifyViewTreeChange(params_, true);
  }

 private:
  NodeObserver::TreeChangeParams params_;

  DISALLOW_COPY_AND_ASSIGN(ScopedTreeNotifier);
};

void RemoveChildImpl(Node* child, Node::Children* children) {
  Node::Children::iterator it =
      std::find(children->begin(), children->end(), child);
  if (it != children->end()) {
    children->erase(it);
    NodePrivate(child).ClearParent();
  }
}

class ScopedOrderChangedNotifier {
 public:
  ScopedOrderChangedNotifier(Node* node,
                             Node* relative_node,
                             OrderDirection direction)
      : node_(node),
        relative_node_(relative_node),
        direction_(direction) {
    FOR_EACH_OBSERVER(NodeObserver,
                      *NodePrivate(node_).observers(),
                      OnNodeReordering(node_, relative_node_, direction_));
  }
  ~ScopedOrderChangedNotifier() {
    FOR_EACH_OBSERVER(NodeObserver,
                      *NodePrivate(node_).observers(),
                      OnNodeReordered(node_, relative_node_, direction_));
  }

 private:
  Node* node_;
  Node* relative_node_;
  OrderDirection direction_;

  DISALLOW_COPY_AND_ASSIGN(ScopedOrderChangedNotifier);
};

// Returns true if the order actually changed.
bool ReorderImpl(Node::Children* children,
                 Node* node,
                 Node* relative,
                 OrderDirection direction) {
  DCHECK(relative);
  DCHECK_NE(node, relative);
  DCHECK_EQ(node->parent(), relative->parent());

  const size_t child_i =
      std::find(children->begin(), children->end(), node) - children->begin();
  const size_t target_i =
      std::find(children->begin(), children->end(), relative) -
      children->begin();
  if ((direction == ORDER_ABOVE && child_i == target_i + 1) ||
      (direction == ORDER_BELOW && child_i + 1 == target_i)) {
    return false;
  }

  ScopedOrderChangedNotifier notifier(node, relative, direction);

  const size_t dest_i =
      direction == ORDER_ABOVE ?
      (child_i < target_i ? target_i : target_i + 1) :
      (child_i < target_i ? target_i - 1 : target_i);
  children->erase(children->begin() + child_i);
  children->insert(children->begin() + dest_i, node);

  return true;
}

class ScopedSetActiveViewNotifier {
 public:
  ScopedSetActiveViewNotifier(Node* node, View* old_view, View* new_view)
      : node_(node),
        old_view_(old_view),
        new_view_(new_view) {
    FOR_EACH_OBSERVER(NodeObserver,
                      *NodePrivate(node).observers(),
                      OnNodeActiveViewChanging(node_, old_view_, new_view_));
  }
  ~ScopedSetActiveViewNotifier() {
    FOR_EACH_OBSERVER(NodeObserver,
                      *NodePrivate(node_).observers(),
                      OnNodeActiveViewChanged(node_, old_view_, new_view_));
  }

 private:
  Node* node_;
  View* old_view_;
  View* new_view_;

  DISALLOW_COPY_AND_ASSIGN(ScopedSetActiveViewNotifier);
};

class ScopedSetBoundsNotifier {
 public:
  ScopedSetBoundsNotifier(Node* node,
                          const gfx::Rect& old_bounds,
                          const gfx::Rect& new_bounds)
      : node_(node),
        old_bounds_(old_bounds),
        new_bounds_(new_bounds) {
    FOR_EACH_OBSERVER(NodeObserver,
                      *NodePrivate(node_).observers(),
                      OnNodeBoundsChanging(node_, old_bounds_, new_bounds_));
  }
  ~ScopedSetBoundsNotifier() {
    FOR_EACH_OBSERVER(NodeObserver,
                      *NodePrivate(node_).observers(),
                      OnNodeBoundsChanged(node_, old_bounds_, new_bounds_));
  }

 private:
  Node* node_;
  const gfx::Rect old_bounds_;
  const gfx::Rect new_bounds_;

  DISALLOW_COPY_AND_ASSIGN(ScopedSetBoundsNotifier);
};

class ScopedDestructionNotifier {
 public:
  explicit ScopedDestructionNotifier(Node* node)
      : node_(node) {
    FOR_EACH_OBSERVER(NodeObserver,
                      *NodePrivate(node_).observers(),
                      OnNodeDestroying(node_));
  }
  ~ScopedDestructionNotifier() {
    FOR_EACH_OBSERVER(NodeObserver,
                      *NodePrivate(node_).observers(),
                      OnNodeDestroyed(node_));
  }

 private:
  Node* node_;

  DISALLOW_COPY_AND_ASSIGN(ScopedDestructionNotifier);
};

// Some operations are only permitted in the connection that created the node.
bool OwnsNode(ViewManager* manager, Node* node) {
  return !manager ||
      static_cast<ViewManagerClientImpl*>(manager)->OwnsNode(node->id());
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// Node, public:

// static
Node* Node::Create(ViewManager* view_manager) {
  Node* node = new Node(view_manager);
  static_cast<ViewManagerClientImpl*>(view_manager)->AddNode(node);
  return node;
}

void Node::Destroy() {
  if (!OwnsNode(manager_, this))
    return;

  if (manager_)
    static_cast<ViewManagerClientImpl*>(manager_)->DestroyNode(id_);
  while (!children_.empty())
    children_.front()->Destroy();
  LocalDestroy();
}

void Node::SetBounds(const gfx::Rect& bounds) {
  if (!OwnsNode(manager_, this))
    return;

  if (manager_)
    static_cast<ViewManagerClientImpl*>(manager_)->SetBounds(id_, bounds);
  LocalSetBounds(bounds_, bounds);
}

void Node::SetVisible(bool value) {
  if (manager_)
    static_cast<ViewManagerClientImpl*>(manager_)->SetVisible(id_, value);
}

void Node::AddObserver(NodeObserver* observer) {
  observers_.AddObserver(observer);
}

void Node::RemoveObserver(NodeObserver* observer) {
  observers_.RemoveObserver(observer);
}

void Node::AddChild(Node* child) {
  // TODO(beng): not necessarily valid to all connections, but possibly to the
  //             embeddee in an embedder-embeddee relationship.
  if (manager_)
    CHECK_EQ(NodePrivate(child).view_manager(), manager_);
  LocalAddChild(child);
  if (manager_)
    static_cast<ViewManagerClientImpl*>(manager_)->AddChild(child->id(), id_);
}

void Node::RemoveChild(Node* child) {
  // TODO(beng): not necessarily valid to all connections, but possibly to the
  //             embeddee in an embedder-embeddee relationship.
  if (manager_)
    CHECK_EQ(NodePrivate(child).view_manager(), manager_);
  LocalRemoveChild(child);
  if (manager_) {
    static_cast<ViewManagerClientImpl*>(manager_)->RemoveChild(child->id(),
                                                               id_);
  }
}

void Node::MoveToFront() {
  Reorder(parent_->children_.back(), ORDER_ABOVE);
}

void Node::MoveToBack() {
  Reorder(parent_->children_.front(), ORDER_BELOW);
}

void Node::Reorder(Node* relative, OrderDirection direction) {
  if (!LocalReorder(relative, direction))
    return;
  if (manager_) {
    static_cast<ViewManagerClientImpl*>(manager_)->Reorder(id_,
                                                            relative->id(),
                                                            direction);
  }
}

bool Node::Contains(Node* child) const {
  if (manager_)
    CHECK_EQ(NodePrivate(child).view_manager(), manager_);
  for (Node* p = child->parent(); p; p = p->parent()) {
    if (p == this)
      return true;
  }
  return false;
}

Node* Node::GetChildById(Id id) {
  if (id == id_)
    return this;
  // TODO(beng): this could be improved depending on how we decide to own nodes.
  Children::const_iterator it = children_.begin();
  for (; it != children_.end(); ++it) {
    Node* node = (*it)->GetChildById(id);
    if (node)
      return node;
  }
  return NULL;
}

void Node::SetActiveView(View* view) {
  // TODO(beng): not necessarily valid to all connections, but possibly to the
  //             embeddee in an embedder-embeddee relationship.
  if (manager_)
    CHECK_EQ(ViewPrivate(view).view_manager(), manager_);
  LocalSetActiveView(view);
  if (manager_) {
    static_cast<ViewManagerClientImpl*>(manager_)->SetActiveView(
        id_, active_view_->id());
  }
}

void Node::SetFocus() {
  if (manager_)
    static_cast<ViewManagerClientImpl*>(manager_)->SetFocus(id_);
}

void Node::Embed(const String& url) {
  static_cast<ViewManagerClientImpl*>(manager_)->Embed(url, id_);
}

////////////////////////////////////////////////////////////////////////////////
// Node, protected:

Node::Node()
    : manager_(NULL),
      id_(-1),
      parent_(NULL),
      active_view_(NULL) {}

Node::~Node() {
  ScopedDestructionNotifier notifier(this);
  if (parent_)
    parent_->LocalRemoveChild(this);
  // TODO(beng): It'd be better to do this via a destruction observer in the
  //             ViewManagerClientImpl.
  if (manager_)
    static_cast<ViewManagerClientImpl*>(manager_)->RemoveNode(id_);
}

////////////////////////////////////////////////////////////////////////////////
// Node, private:

Node::Node(ViewManager* manager)
    : manager_(manager),
      id_(static_cast<ViewManagerClientImpl*>(manager_)->CreateNode()),
      parent_(NULL),
      active_view_(NULL) {}

void Node::LocalDestroy() {
  delete this;
}

void Node::LocalAddChild(Node* child) {
  ScopedTreeNotifier notifier(child, child->parent(), this);
  if (child->parent())
    RemoveChildImpl(child, &child->parent_->children_);
  children_.push_back(child);
  child->parent_ = this;
}

void Node::LocalRemoveChild(Node* child) {
  DCHECK_EQ(this, child->parent());
  ScopedTreeNotifier notifier(child, this, NULL);
  RemoveChildImpl(child, &children_);
}

bool Node::LocalReorder(Node* relative, OrderDirection direction) {
  return ReorderImpl(&parent_->children_, this, relative, direction);
}

void Node::LocalSetActiveView(View* view) {
  ScopedSetActiveViewNotifier notifier(this, active_view_, view);
  if (active_view_)
    ViewPrivate(active_view_).set_node(NULL);
  active_view_ = view;
  if (active_view_)
    ViewPrivate(active_view_).set_node(this);
}

void Node::LocalSetBounds(const gfx::Rect& old_bounds,
                          const gfx::Rect& new_bounds) {
  DCHECK(old_bounds == bounds_);
  ScopedSetBoundsNotifier notifier(this, old_bounds, new_bounds);
  bounds_ = new_bounds;
}

}  // namespace view_manager
}  // namespace mojo
