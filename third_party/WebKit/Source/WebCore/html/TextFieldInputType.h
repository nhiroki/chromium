/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#ifndef TextFieldInputType_h
#define TextFieldInputType_h

#include "InputType.h"

namespace WebCore {

// The class represents types of which UI contain text fields.
// It supports not only the types for BaseTextInputType but also type=number.
class TextFieldInputType : public InputType {
protected:
    TextFieldInputType(HTMLInputElement*);
    virtual bool canSetSuggestedValue();
    virtual void handleKeydownEvent(KeyboardEvent*);
    void handleKeydownEventForSpinButton(KeyboardEvent*);
    void handleWheelEventForSpinButton(WheelEvent*);

    virtual HTMLElement* innerTextElement() const { return m_innerText; }
    virtual HTMLElement* innerSpinButtonElement() const { return m_innerSpinButton; }
    virtual HTMLElement* outerSpinButtonElement() const { return m_outerSpinButton; }
#if ENABLE(INPUT_SPEECH)
    virtual HTMLElement* speechButtonElement() const { return m_speechButton; }
#endif

protected:
    virtual void createShadowSubtree();
    virtual void destroyShadowSubtree();
    void setInnerTextElement(HTMLElement* element) { m_innerText = element; }
#if ENABLE(INPUT_SPEECH)
    void setSpeechButtonElement(HTMLElement* element) { m_speechButton = element; }
#endif

private:
    virtual bool isTextField() const;
    virtual bool valueMissing(const String&) const;
    virtual void handleBeforeTextInsertedEvent(BeforeTextInsertedEvent*);
    virtual void forwardEvent(Event*);
    virtual bool shouldSubmitImplicitly(Event*);
    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*) const;
    virtual bool shouldUseInputMethod() const;
    virtual String sanitizeValue(const String&);
    virtual bool shouldRespectListAttribute();

    HTMLElement* m_innerText;
    HTMLElement* m_innerSpinButton;
    HTMLElement* m_outerSpinButton;
#if ENABLE(INPUT_SPEECH)
    HTMLElement* m_speechButton;
#endif
};

} // namespace WebCore

#endif // TextFieldInputType_h
