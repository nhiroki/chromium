/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"
#include "core/dom/NamedFlowCollection.h"

#include "RuntimeEnabledFeatures.h"
#include "core/dom/DOMNamedFlowCollection.h"
#include "core/dom/Document.h"
#include "core/dom/NamedFlow.h"
#include "core/inspector/InspectorInstrumentation.h"
#include "wtf/text/StringHash.h"
#include "wtf/text/WTFString.h"

namespace WebCore {

NamedFlowCollection::NamedFlowCollection(Document* document)
    : DocumentLifecycleObserver(document)
{
    ASSERT(RuntimeEnabledFeatures::cssRegionsEnabled());
}

Vector<RefPtr<NamedFlow> > NamedFlowCollection::namedFlows()
{
    Vector<RefPtr<NamedFlow> > namedFlows;

    for (NamedFlowSet::iterator it = m_namedFlows.begin(); it != m_namedFlows.end(); ++it) {
        if ((*it)->flowState() == NamedFlow::FlowStateNull)
            continue;

        namedFlows.append(RefPtr<NamedFlow>(*it));
    }

    return namedFlows;
}

NamedFlow* NamedFlowCollection::flowByName(const String& flowName)
{
    NamedFlowSet::iterator it = m_namedFlows.find<String, NamedFlowHashTranslator>(flowName);
    if (it == m_namedFlows.end() || (*it)->flowState() == NamedFlow::FlowStateNull)
        return 0;

    return *it;
}

PassRefPtr<NamedFlow> NamedFlowCollection::ensureFlowWithName(const String& flowName)
{
    NamedFlowSet::iterator it = m_namedFlows.find<String, NamedFlowHashTranslator>(flowName);
    if (it != m_namedFlows.end()) {
        NamedFlow* namedFlow = *it;
        ASSERT(namedFlow->flowState() == NamedFlow::FlowStateNull);

        return namedFlow;
    }

    RefPtr<NamedFlow> newFlow = NamedFlow::create(this, flowName);
    m_namedFlows.add(newFlow.get());

    InspectorInstrumentation::didCreateNamedFlow(document(), newFlow.get());

    return newFlow.release();
}

void NamedFlowCollection::discardNamedFlow(NamedFlow* namedFlow)
{
    // The document is not valid anymore so the collection will be destroyed anyway.
    if (!document())
        return;

    ASSERT(namedFlow->flowState() == NamedFlow::FlowStateNull);
    ASSERT(m_namedFlows.contains(namedFlow));

    InspectorInstrumentation::willRemoveNamedFlow(document(), namedFlow);

    m_namedFlows.remove(namedFlow);
}

Document* NamedFlowCollection::document() const
{
    return lifecycleContext();
}

PassRefPtr<DOMNamedFlowCollection> NamedFlowCollection::createCSSOMSnapshot()
{
    Vector<NamedFlow*> createdFlows;
    for (NamedFlowSet::iterator it = m_namedFlows.begin(); it != m_namedFlows.end(); ++it)
        if ((*it)->flowState() == NamedFlow::FlowStateCreated)
            createdFlows.append(*it);
    return DOMNamedFlowCollection::create(createdFlows);
}

// The HashFunctions object used by the HashSet to compare between NamedFlows.
// It is safe to set safeToCompareToEmptyOrDeleted because the HashSet will never contain null pointers or deleted values.
struct NamedFlowCollection::NamedFlowHashFunctions {
    static unsigned hash(NamedFlow* key) { return DefaultHash<String>::Hash::hash(key->name()); }
    static bool equal(NamedFlow* a, NamedFlow* b) { return a->name() == b->name(); }
    static const bool safeToCompareToEmptyOrDeleted = true;
};

// The HashTranslator is used to lookup a NamedFlow in the set using a name.
struct NamedFlowCollection::NamedFlowHashTranslator {
    static unsigned hash(const String& key) { return DefaultHash<String>::Hash::hash(key); }
    static bool equal(NamedFlow* a, const String& b) { return a->name() == b; }
};

} // namespace WebCore
