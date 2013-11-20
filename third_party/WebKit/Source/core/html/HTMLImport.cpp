/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "core/html/HTMLImport.h"

#include "core/dom/Document.h"

namespace WebCore {

Frame* HTMLImport::frame()
{
    return master()->frame();
}

Document* HTMLImport::master()
{
    return root()->document();
}

HTMLImportsController* HTMLImport::controller()
{
    return root()->toController();
}

void HTMLImport::appendChild(HTMLImport* child)
{
    if (isBlocked())
        child->block();
    TreeNode<HTMLImport>::appendChild(child);
    blockAfter(child);
}

bool HTMLImport::areChilrenLoaded() const
{
    for (HTMLImport* child = firstChild(); child; child = child->next()) {
        if (!child->isLoaded())
            return false;
    }

    return true;
}

bool HTMLImport::arePredecessorsLoaded() const
{
    HTMLImport* parent = this->parent();
    if (!parent)
        return true;

    for (HTMLImport* sibling = parent->firstChild(); sibling; sibling = sibling->next()) {
        if (sibling == this)
            break;
        if (!sibling->isLoaded())
            return false;
    }

    return true;
}

void HTMLImport::block()
{
    m_blocked = true;
}

void HTMLImport::unblock()
{
    bool wasBlocked = m_blocked;
    m_blocked = false;
    if (wasBlocked)
        didUnblock();
}

void HTMLImport::didUnblock()
{
    ASSERT(!isBlocked());
    if (!isProcessing())
        return;

    if (Document* document = this->document())
        document->didLoadAllImports();
}

bool HTMLImport::unblock(HTMLImport* import)
{
    ASSERT(import->arePredecessorsLoaded());
    ASSERT(import->isBlocked() || import->areChilrenLoaded());

    if (import->isBlocked()) {
        for (HTMLImport* child = import->firstChild(); child; child = child->next()) {
            if (!unblock(child))
                return false;
        }
    }

    import->unblock();
    return import->isLoaded();
}

void HTMLImport::block(HTMLImport* import)
{
    for (HTMLImport* child = import; child; child = traverseNext(child, import))
        child->block();
}

void HTMLImport::blockAfter(HTMLImport* child)
{
    ASSERT(child->parent() == this);

    for (HTMLImport* sibling = lastChild(); sibling; sibling = sibling->previous()) {
        if (sibling == child)
            break;
        HTMLImport::block(sibling);
    }

    this->block();

    if (HTMLImport* parent = this->parent())
        parent->blockAfter(this);
}


} // namespace WebCore
