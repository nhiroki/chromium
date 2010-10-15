/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "Element.h"
#include "FormatBlockCommand.h"
#include "Document.h"
#include "htmlediting.h"
#include "HTMLElement.h"
#include "HTMLNames.h"
#include "visible_units.h"

namespace WebCore {

using namespace HTMLNames;

static Node* enclosingBlockToSplitTreeTo(Node* startNode);

FormatBlockCommand::FormatBlockCommand(Document* document, const QualifiedName& tagName) 
    : ApplyBlockElementCommand(document, tagName)
{
}

void FormatBlockCommand::formatRange(const Position& start, const Position& end, RefPtr<Element>& blockNode)
{
    Node* nodeToSplitTo = enclosingBlockToSplitTreeTo(start.node());
    RefPtr<Node> outerBlock = (start.node() == nodeToSplitTo) ? start.node() : splitTreeToNode(start.node(), nodeToSplitTo);
    RefPtr<Node> nodeAfterInsertionPosition = outerBlock;

    Element* refNode = enclosingBlockFlowElement(end);
    Element* root = editableRootForPosition(start);
    if (isElementToApplyInFormatBlockCommand(refNode->tagQName()) && start == startOfBlock(start) && end == endOfBlock(end)
        && refNode != root && !root->isDescendantOf(refNode)) {
        // Already in a block element that only contains the current paragraph
        if (refNode->hasTagName(tagName()))
            return;
        nodeAfterInsertionPosition = refNode;
    }

    if (!blockNode) {
        // Create a new blockquote and insert it as a child of the root editable element. We accomplish
        // this by splitting all parents of the current paragraph up to that point.
        blockNode = createBlockElement();
        insertNodeBefore(blockNode, nodeAfterInsertionPosition);
    }

    Position lastParagraphInBlockNode = lastPositionInNode(blockNode.get());
    bool wasEndOfParagraph = isEndOfParagraph(lastParagraphInBlockNode);

    moveParagraphWithClones(start, end, blockNode.get(), outerBlock.get());

    if (wasEndOfParagraph && !isEndOfParagraph(lastParagraphInBlockNode) && !isStartOfParagraph(lastParagraphInBlockNode))
        insertBlockPlaceholder(lastParagraphInBlockNode);
}

// FIXME: We should consider merging this function with isElementForFormatBlockCommand in Editor.cpp
// Checks if a tag name is valid for execCommand('FormatBlock').
bool FormatBlockCommand::isElementToApplyInFormatBlockCommand(const QualifiedName& tagName)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, blockTags, ());
    if (blockTags.isEmpty()) {
        blockTags.add(addressTag);
        blockTags.add(articleTag);
        blockTags.add(asideTag);
        blockTags.add(blockquoteTag);
        blockTags.add(ddTag);
        blockTags.add(divTag);
        blockTags.add(dlTag);
        blockTags.add(dtTag);
        blockTags.add(footerTag);
        blockTags.add(h1Tag);
        blockTags.add(h2Tag);
        blockTags.add(h3Tag);
        blockTags.add(h4Tag);
        blockTags.add(h5Tag);
        blockTags.add(h6Tag);
        blockTags.add(headerTag);
        blockTags.add(hgroupTag);
        blockTags.add(navTag);
        blockTags.add(pTag);
        blockTags.add(preTag);
        blockTags.add(sectionTag);
    }
    return blockTags.contains(tagName);
}

Node* enclosingBlockToSplitTreeTo(Node* startNode)
{
    Node* lastBlock = startNode;
    for (Node* n = startNode; n; n = n->parentNode()) {
        if (!n->isContentEditable())
            return lastBlock;
        if (isTableCell(n) || n->hasTagName(bodyTag) || !n->parentNode() || !n->parentNode()->isContentEditable()
            || (n->isElementNode() && FormatBlockCommand::isElementToApplyInFormatBlockCommand(static_cast<Element*>(n)->tagQName())))
            return n;
        if (isBlock(n))
            lastBlock = n;
        if (isListElement(n))
            return n->parentNode()->isContentEditable() ? n->parentNode() : n;
    }
    return lastBlock;
}

}
