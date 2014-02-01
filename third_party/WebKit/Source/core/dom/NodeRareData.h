/*
 * Copyright (C) 2008, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2008 David Smith <catfish.man@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef NodeRareData_h
#define NodeRareData_h

#include "core/dom/ChildNodeList.h"
#include "core/dom/EmptyNodeList.h"
#include "core/dom/LiveNodeList.h"
#include "core/dom/MutationObserverRegistration.h"
#include "core/dom/QualifiedName.h"
#include "core/dom/TagCollection.h"
#include "core/page/Page.h"
#include "wtf/HashSet.h"
#include "wtf/OwnPtr.h"
#include "wtf/PassOwnPtr.h"
#include "wtf/text/AtomicString.h"
#include "wtf/text/StringHash.h"

namespace WebCore {

class LabelsNodeList;
class RadioNodeList;
class TreeScope;

class NodeListsNodeData {
    WTF_MAKE_NONCOPYABLE(NodeListsNodeData); WTF_MAKE_FAST_ALLOCATED;
public:
    void clearChildNodeListCache()
    {
        if (m_childNodeList && m_childNodeList->isChildNodeList())
            toChildNodeList(m_childNodeList)->invalidateCache();
    }

    PassRefPtr<ChildNodeList> ensureChildNodeList(ContainerNode* node)
    {
        if (m_childNodeList)
            return toChildNodeList(m_childNodeList);
        RefPtr<ChildNodeList> list = ChildNodeList::create(node);
        m_childNodeList = list.get();
        return list.release();
    }

    PassRefPtr<EmptyNodeList> ensureEmptyChildNodeList(Node* node)
    {
        if (m_childNodeList)
            return toEmptyNodeList(m_childNodeList);
        RefPtr<EmptyNodeList> list = EmptyNodeList::create(node);
        m_childNodeList = list.get();
        return list.release();
    }

    void removeChildNodeList(ChildNodeList* list)
    {
        ASSERT(m_childNodeList == list);
        if (deleteThisAndUpdateNodeRareDataIfAboutToRemoveLastList(list->ownerNode()))
            return;
        m_childNodeList = 0;
    }

    void removeEmptyChildNodeList(EmptyNodeList* list)
    {
        ASSERT(m_childNodeList == list);
        if (deleteThisAndUpdateNodeRareDataIfAboutToRemoveLastList(list->ownerNode()))
            return;
        m_childNodeList = 0;
    }

    template <typename StringType>
    struct NodeListCacheMapEntryHash {
        static unsigned hash(const std::pair<unsigned char, StringType>& entry)
        {
            return DefaultHash<StringType>::Hash::hash(entry.second) + entry.first;
        }
        static bool equal(const std::pair<unsigned char, StringType>& a, const std::pair<unsigned char, StringType>& b) { return a == b; }
        static const bool safeToCompareToEmptyOrDeleted = DefaultHash<StringType>::Hash::safeToCompareToEmptyOrDeleted;
    };

    typedef HashMap<std::pair<unsigned char, AtomicString>, LiveNodeListBase*, NodeListCacheMapEntryHash<AtomicString> > NodeListAtomicNameCacheMap;
    typedef HashMap<std::pair<unsigned char, String>, LiveNodeListBase*, NodeListCacheMapEntryHash<String> > NodeListNameCacheMap;
    typedef HashMap<QualifiedName, TagCollection*> TagCollectionCacheNS;

    template<typename T>
    PassRefPtr<T> addCacheWithAtomicName(ContainerNode* node, CollectionType collectionType, const AtomicString& name)
    {
        NodeListAtomicNameCacheMap::AddResult result = m_atomicNameCaches.add(namedNodeListKey(collectionType, name), 0);
        if (!result.isNewEntry)
            return static_cast<T*>(result.iterator->value);

        RefPtr<T> list = T::create(node, collectionType, name);
        result.iterator->value = list.get();
        return list.release();
    }

    // FIXME: This function should be renamed since it doesn't have an atomic name.
    template<typename T>
    PassRefPtr<T> addCacheWithAtomicName(ContainerNode* node, CollectionType collectionType)
    {
        NodeListAtomicNameCacheMap::AddResult result = m_atomicNameCaches.add(namedNodeListKey(collectionType, starAtom), 0);
        if (!result.isNewEntry)
            return static_cast<T*>(result.iterator->value);

        RefPtr<T> list = T::create(node, collectionType);
        result.iterator->value = list.get();
        return list.release();
    }

    template<typename T>
    T* cacheWithAtomicName(CollectionType collectionType)
    {
        return static_cast<T*>(m_atomicNameCaches.get(namedNodeListKey(collectionType, starAtom)));
    }

    template<typename T>
    PassRefPtr<T> addCacheWithName(Node* node, CollectionType collectionType, const String& name)
    {
        NodeListNameCacheMap::AddResult result = m_nameCaches.add(namedNodeListKey(collectionType, name), 0);
        if (!result.isNewEntry)
            return static_cast<T*>(result.iterator->value);

        RefPtr<T> list = T::create(node, name);
        result.iterator->value = list.get();
        return list.release();
    }

    PassRefPtr<TagCollection> addCacheWithQualifiedName(ContainerNode* node, const AtomicString& namespaceURI, const AtomicString& localName)
    {
        QualifiedName name(nullAtom, localName, namespaceURI);
        TagCollectionCacheNS::AddResult result = m_tagCollectionCacheNS.add(name, 0);
        if (!result.isNewEntry)
            return result.iterator->value;

        RefPtr<TagCollection> list = TagCollection::create(node, namespaceURI, localName);
        result.iterator->value = list.get();
        return list.release();
    }

    void removeCacheWithAtomicName(LiveNodeListBase* list, CollectionType collectionType, const AtomicString& name = starAtom)
    {
        ASSERT(list == m_atomicNameCaches.get(namedNodeListKey(collectionType, name)));
        if (deleteThisAndUpdateNodeRareDataIfAboutToRemoveLastList(list->ownerNode()))
            return;
        m_atomicNameCaches.remove(namedNodeListKey(collectionType, name));
    }

    void removeCacheWithName(LiveNodeListBase* list, CollectionType collectionType, const String& name)
    {
        ASSERT(list == m_nameCaches.get(namedNodeListKey(collectionType, name)));
        if (deleteThisAndUpdateNodeRareDataIfAboutToRemoveLastList(list->ownerNode()))
            return;
        m_nameCaches.remove(namedNodeListKey(collectionType, name));
    }

    void removeCacheWithQualifiedName(LiveNodeListBase* list, const AtomicString& namespaceURI, const AtomicString& localName)
    {
        QualifiedName name(nullAtom, localName, namespaceURI);
        ASSERT(list == m_tagCollectionCacheNS.get(name));
        if (deleteThisAndUpdateNodeRareDataIfAboutToRemoveLastList(list->ownerNode()))
            return;
        m_tagCollectionCacheNS.remove(name);
    }

    static PassOwnPtr<NodeListsNodeData> create()
    {
        return adoptPtr(new NodeListsNodeData);
    }

    void invalidateCaches(const QualifiedName* attrName = 0);
    bool isEmpty() const
    {
        return m_atomicNameCaches.isEmpty() && m_nameCaches.isEmpty() && m_tagCollectionCacheNS.isEmpty();
    }

    void adoptTreeScope()
    {
        invalidateCaches();
    }

    void adoptDocument(Document& oldDocument, Document& newDocument)
    {
        invalidateCaches();

        if (oldDocument != newDocument) {
            NodeListAtomicNameCacheMap::const_iterator atomicNameCacheEnd = m_atomicNameCaches.end();
            for (NodeListAtomicNameCacheMap::const_iterator it = m_atomicNameCaches.begin(); it != atomicNameCacheEnd; ++it) {
                LiveNodeListBase* list = it->value;
                oldDocument.unregisterNodeList(list);
                newDocument.registerNodeList(list);
            }

            NodeListNameCacheMap::const_iterator nameCacheEnd = m_nameCaches.end();
            for (NodeListNameCacheMap::const_iterator it = m_nameCaches.begin(); it != nameCacheEnd; ++it) {
                LiveNodeListBase* list = it->value;
                oldDocument.unregisterNodeList(list);
                newDocument.registerNodeList(list);
            }

            TagCollectionCacheNS::const_iterator tagEnd = m_tagCollectionCacheNS.end();
            for (TagCollectionCacheNS::const_iterator it = m_tagCollectionCacheNS.begin(); it != tagEnd; ++it) {
                LiveNodeListBase* list = it->value;
                ASSERT(!list->isRootedAtDocument());
                oldDocument.unregisterNodeList(list);
                newDocument.registerNodeList(list);
            }
        }
    }

private:
    NodeListsNodeData()
        : m_childNodeList(0)
    { }

    std::pair<unsigned char, AtomicString> namedNodeListKey(CollectionType type, const AtomicString& name)
    {
        return std::pair<unsigned char, AtomicString>(type, name);
    }

    std::pair<unsigned char, String> namedNodeListKey(CollectionType type, const String& name)
    {
        return std::pair<unsigned char, String>(type, name);
    }

    bool deleteThisAndUpdateNodeRareDataIfAboutToRemoveLastList(Node*);

    // Can be a ChildNodeList or an EmptyNodeList.
    NodeList* m_childNodeList;
    NodeListAtomicNameCacheMap m_atomicNameCaches;
    NodeListNameCacheMap m_nameCaches;
    TagCollectionCacheNS m_tagCollectionCacheNS;
};

class NodeMutationObserverData {
    WTF_MAKE_NONCOPYABLE(NodeMutationObserverData); WTF_MAKE_FAST_ALLOCATED;
public:
    Vector<OwnPtr<MutationObserverRegistration> > registry;
    HashSet<MutationObserverRegistration*> transientRegistry;

    static PassOwnPtr<NodeMutationObserverData> create() { return adoptPtr(new NodeMutationObserverData); }

private:
    NodeMutationObserverData() { }
};

class NodeRareData : public NodeRareDataBase {
    WTF_MAKE_NONCOPYABLE(NodeRareData); WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<NodeRareData> create(RenderObject* renderer) { return adoptPtr(new NodeRareData(renderer)); }

    void clearNodeLists() { m_nodeLists.clear(); }
    NodeListsNodeData* nodeLists() const { return m_nodeLists.get(); }
    NodeListsNodeData& ensureNodeLists()
    {
        if (!m_nodeLists)
            m_nodeLists = NodeListsNodeData::create();
        return *m_nodeLists;
    }

    NodeMutationObserverData* mutationObserverData() { return m_mutationObserverData.get(); }
    NodeMutationObserverData& ensureMutationObserverData()
    {
        if (!m_mutationObserverData)
            m_mutationObserverData = NodeMutationObserverData::create();
        return *m_mutationObserverData;
    }

    unsigned connectedSubframeCount() const { return m_connectedFrameCount; }
    void incrementConnectedSubframeCount(unsigned amount)
    {
        m_connectedFrameCount += amount;
    }
    void decrementConnectedSubframeCount(unsigned amount)
    {
        ASSERT(m_connectedFrameCount);
        ASSERT(amount <= m_connectedFrameCount);
        m_connectedFrameCount -= amount;
    }

protected:
    NodeRareData(RenderObject* renderer)
        : NodeRareDataBase(renderer)
        , m_connectedFrameCount(0)
    { }

private:
    unsigned m_connectedFrameCount : 10; // Must fit Page::maxNumberOfFrames.

    OwnPtr<NodeListsNodeData> m_nodeLists;
    OwnPtr<NodeMutationObserverData> m_mutationObserverData;
};

inline bool NodeListsNodeData::deleteThisAndUpdateNodeRareDataIfAboutToRemoveLastList(Node* ownerNode)
{
    ASSERT(ownerNode);
    ASSERT(ownerNode->nodeLists() == this);
    if ((m_childNodeList ? 1 : 0) + m_atomicNameCaches.size() + m_nameCaches.size() + m_tagCollectionCacheNS.size() != 1)
        return false;
    ownerNode->clearNodeLists();
    return true;
}

// Ensure the 10 bits reserved for the m_connectedFrameCount cannot overflow
COMPILE_ASSERT(Page::maxNumberOfFrames < 1024, Frame_limit_should_fit_in_rare_data_count);

} // namespace WebCore

#endif // NodeRareData_h
