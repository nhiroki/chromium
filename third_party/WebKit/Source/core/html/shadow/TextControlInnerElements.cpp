/*
 * Copyright (C) 2006, 2008, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
#include "core/html/shadow/TextControlInnerElements.h"

#include "HTMLNames.h"
#include "core/dom/Document.h"
#include "core/dom/NodeRenderStyle.h"
#include "core/events/MouseEvent.h"
#include "core/events/TextEvent.h"
#include "core/events/TextEventInputType.h"
#include "core/frame/LocalFrame.h"
#include "core/html/HTMLInputElement.h"
#include "core/html/shadow/ShadowElementNames.h"
#include "core/page/EventHandler.h"
#include "core/rendering/RenderTextControlSingleLine.h"
#include "core/rendering/RenderView.h"
#include "platform/UserGestureIndicator.h"

namespace WebCore {

using namespace HTMLNames;

TextControlInnerContainer::TextControlInnerContainer(Document& document)
    : HTMLDivElement(document)
{
}

PassRefPtr<TextControlInnerContainer> TextControlInnerContainer::create(Document& document)
{
    RefPtr<TextControlInnerContainer> element = adoptRef(new TextControlInnerContainer(document));
    element->setAttribute(idAttr, ShadowElementNames::textFieldContainer());
    return element.release();
}

RenderObject* TextControlInnerContainer::createRenderer(RenderStyle*)
{
    return new RenderTextControlInnerContainer(this);
}

// ---------------------------

EditingViewPortElement::EditingViewPortElement(Document& document)
    : HTMLDivElement(document)
{
    setHasCustomStyleCallbacks();
}

PassRefPtr<EditingViewPortElement> EditingViewPortElement::create(Document& document)
{
    RefPtr<EditingViewPortElement> element = adoptRef(new EditingViewPortElement(document));
    element->setAttribute(idAttr, ShadowElementNames::editingViewPort());
    return element.release();
}

PassRefPtr<RenderStyle> EditingViewPortElement::customStyleForRenderer()
{
    // FXIME: Move these styles to html.css.

    RefPtr<RenderStyle> style = RenderStyle::create();
    style->inheritFrom(shadowHost()->renderStyle());

    style->setFlexGrow(1);
    style->setDisplay(BLOCK);
    style->setDirection(LTR);

    // We don't want the shadow dom to be editable, so we set this block to
    // read-only in case the input itself is editable.
    style->setUserModify(READ_ONLY);
    style->setUnique();

    return style.release();
}

// ---------------------------

inline TextControlInnerTextElement::TextControlInnerTextElement(Document& document)
    : HTMLDivElement(document)
{
    setHasCustomStyleCallbacks();
}

PassRefPtr<TextControlInnerTextElement> TextControlInnerTextElement::create(Document& document)
{
    RefPtr<TextControlInnerTextElement> element = adoptRef(new TextControlInnerTextElement(document));
    element->setAttribute(idAttr, ShadowElementNames::innerEditor());
    return element.release();
}

void TextControlInnerTextElement::defaultEventHandler(Event* event)
{
    // FIXME: In the future, we should add a way to have default event listeners.
    // Then we would add one to the text field's inner div, and we wouldn't need this subclass.
    // Or possibly we could just use a normal event listener.
    if (event->isBeforeTextInsertedEvent() || event->type() == EventTypeNames::webkitEditableContentChanged) {
        Element* shadowAncestor = shadowHost();
        // A TextControlInnerTextElement can have no host if its been detached,
        // but kept alive by an EditCommand. In this case, an undo/redo can
        // cause events to be sent to the TextControlInnerTextElement. To
        // prevent an infinite loop, we must check for this case before sending
        // the event up the chain.
        if (shadowAncestor)
            shadowAncestor->defaultEventHandler(event);
    }
    if (!event->defaultHandled())
        HTMLDivElement::defaultEventHandler(event);
}

RenderObject* TextControlInnerTextElement::createRenderer(RenderStyle*)
{
    return new RenderTextControlInnerBlock(this);
}

PassRefPtr<RenderStyle> TextControlInnerTextElement::customStyleForRenderer()
{
    RenderObject* parentRenderer = shadowHost()->renderer();
    if (!parentRenderer || !parentRenderer->isTextControl())
        return originalStyleForRenderer();
    RenderTextControl* textControlRenderer = toRenderTextControl(parentRenderer);
    return textControlRenderer->createInnerTextStyle(textControlRenderer->style());
}

// ----------------------------

inline SearchFieldDecorationElement::SearchFieldDecorationElement(Document& document)
    : HTMLDivElement(document)
{
}

PassRefPtr<SearchFieldDecorationElement> SearchFieldDecorationElement::create(Document& document)
{
    RefPtr<SearchFieldDecorationElement> element = adoptRef(new SearchFieldDecorationElement(document));
    element->setAttribute(idAttr, ShadowElementNames::searchDecoration());
    return element.release();
}

const AtomicString& SearchFieldDecorationElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, resultsDecorationId, ("-webkit-search-results-decoration", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, decorationId, ("-webkit-search-decoration", AtomicString::ConstructFromLiteral));
    Element* host = shadowHost();
    if (!host)
        return resultsDecorationId;
    if (isHTMLInputElement(*host)) {
        if (toHTMLInputElement(host)->maxResults() < 0)
            return decorationId;
        return resultsDecorationId;
    }
    return resultsDecorationId;
}

void SearchFieldDecorationElement::defaultEventHandler(Event* event)
{
    // On mousedown, focus the search field
    HTMLInputElement* input = toHTMLInputElement(shadowHost());
    if (input && event->type() == EventTypeNames::mousedown && event->isMouseEvent() && toMouseEvent(event)->button() == LeftButton) {
        input->focus();
        input->select();
        event->setDefaultHandled();
    }

    if (!event->defaultHandled())
        HTMLDivElement::defaultEventHandler(event);
}

bool SearchFieldDecorationElement::willRespondToMouseClickEvents()
{
    return true;
}

// ----------------------------

inline SearchFieldCancelButtonElement::SearchFieldCancelButtonElement(Document& document)
    : HTMLDivElement(document)
    , m_capturing(false)
{
}

PassRefPtr<SearchFieldCancelButtonElement> SearchFieldCancelButtonElement::create(Document& document)
{
    RefPtr<SearchFieldCancelButtonElement> element = adoptRef(new SearchFieldCancelButtonElement(document));
    element->setShadowPseudoId(AtomicString("-webkit-search-cancel-button", AtomicString::ConstructFromLiteral));
    element->setAttribute(idAttr, ShadowElementNames::clearButton());
    return element.release();
}

void SearchFieldCancelButtonElement::detach(const AttachContext& context)
{
    if (m_capturing) {
        if (LocalFrame* frame = document().frame())
            frame->eventHandler().setCapturingMouseEventsNode(nullptr);
    }
    HTMLDivElement::detach(context);
}


void SearchFieldCancelButtonElement::defaultEventHandler(Event* event)
{
    // If the element is visible, on mouseup, clear the value, and set selection
    RefPtr<HTMLInputElement> input(toHTMLInputElement(shadowHost()));
    if (!input || input->isDisabledOrReadOnly()) {
        if (!event->defaultHandled())
            HTMLDivElement::defaultEventHandler(event);
        return;
    }


    if (event->type() == EventTypeNames::click && event->isMouseEvent() && toMouseEvent(event)->button() == LeftButton) {
        input->setValueForUser("");
        input->onSearch();
        event->setDefaultHandled();
    }

    if (!event->defaultHandled())
        HTMLDivElement::defaultEventHandler(event);
}

bool SearchFieldCancelButtonElement::willRespondToMouseClickEvents()
{
    const HTMLInputElement* input = toHTMLInputElement(shadowHost());
    if (input && !input->isDisabledOrReadOnly())
        return true;

    return HTMLDivElement::willRespondToMouseClickEvents();
}

}
