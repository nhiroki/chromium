/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "WebEditorClient.h"

#define DISABLE_NOT_IMPLEMENTED_WARNINGS 1
#include "NotImplemented.h"

#include "WebPage.h"

#include <WebCore/EditCommand.h>
#include <WebCore/KeyboardEvent.h>

using namespace WebCore;

namespace WebKit {

void WebEditorClient::pageDestroyed()
{
    delete this;
}

bool WebEditorClient::shouldDeleteRange(Range* range)
{
    bool result = m_page->injectedBundleEditorClient().shouldDeleteRange(m_page, range);
    notImplemented();
    return result;
}

bool WebEditorClient::shouldShowDeleteInterface(HTMLElement*)
{
    notImplemented();
    return false;
}

bool WebEditorClient::smartInsertDeleteEnabled()
{
    notImplemented();
    return true;
}
 
bool WebEditorClient::isSelectTrailingWhitespaceEnabled()
{
    notImplemented();
    return false;
}

bool WebEditorClient::isContinuousSpellCheckingEnabled()
{
    notImplemented();
    return false;
}

void WebEditorClient::toggleContinuousSpellChecking()
{
    notImplemented();
}

bool WebEditorClient::isGrammarCheckingEnabled()
{
    notImplemented();
    return false;
}

void WebEditorClient::toggleGrammarChecking()
{
    notImplemented();
}

int WebEditorClient::spellCheckerDocumentTag()
{
    notImplemented();
    return false;
}

    
bool WebEditorClient::isEditable()
{
    notImplemented();
    return false;
}


bool WebEditorClient::shouldBeginEditing(Range* range)
{
    bool result = m_page->injectedBundleEditorClient().shouldBeginEditing(m_page, range);
    notImplemented();
    return result;
}

bool WebEditorClient::shouldEndEditing(Range* range)
{
    bool result = m_page->injectedBundleEditorClient().shouldEndEditing(m_page, range);
    notImplemented();
    return result;
}

bool WebEditorClient::shouldInsertNode(Node* node, Range* rangeToReplace, EditorInsertAction action)
{
    bool result = m_page->injectedBundleEditorClient().shouldInsertNode(m_page, node, rangeToReplace, action);
    notImplemented();
    return result;
}

bool WebEditorClient::shouldInsertText(const String& text, Range* rangeToReplace, EditorInsertAction action)
{
    bool result = m_page->injectedBundleEditorClient().shouldInsertText(m_page, text.impl(), rangeToReplace, action);
    notImplemented();
    return result;
}

bool WebEditorClient::shouldChangeSelectedRange(Range* fromRange, Range* toRange, EAffinity affinity, bool stillSelecting)
{
    bool result = m_page->injectedBundleEditorClient().shouldChangeSelectedRange(m_page, fromRange, toRange, affinity, stillSelecting);
    notImplemented();
    return result;
}
    
bool WebEditorClient::shouldApplyStyle(CSSStyleDeclaration* style, Range* range)
{
    bool result = m_page->injectedBundleEditorClient().shouldApplyStyle(m_page, style, range);
    notImplemented();
    return result;
}

bool WebEditorClient::shouldMoveRangeAfterDelete(Range*, Range*)
{
    notImplemented();
    return true;
}

void WebEditorClient::didBeginEditing()
{
    // FIXME: What good is a notification name, if it's always the same?
    static const String WebViewDidBeginEditingNotification = "WebViewDidBeginEditingNotification";
    m_page->injectedBundleEditorClient().didBeginEditing(m_page, WebViewDidBeginEditingNotification.impl());
    notImplemented();
}

void WebEditorClient::respondToChangedContents()
{
    static const String WebViewDidChangeNotification = "WebViewDidChangeNotification";
    m_page->injectedBundleEditorClient().didChange(m_page, WebViewDidChangeNotification.impl());
    notImplemented();
}

void WebEditorClient::respondToChangedSelection()
{
    static const String WebViewDidChangeSelectionNotification = "WebViewDidChangeSelectionNotification";
    m_page->injectedBundleEditorClient().didChangeSelection(m_page, WebViewDidChangeSelectionNotification.impl());
    notImplemented();
}

void WebEditorClient::didEndEditing()
{
    static const String WebViewDidEndEditingNotification = "WebViewDidEndEditingNotification";
    m_page->injectedBundleEditorClient().didEndEditing(m_page, WebViewDidEndEditingNotification.impl());
    notImplemented();
}

void WebEditorClient::didWriteSelectionToPasteboard()
{
    notImplemented();
}

void WebEditorClient::didSetSelectionTypesForPasteboard()
{
    notImplemented();
}

void WebEditorClient::registerCommandForUndo(PassRefPtr<EditCommand>)
{
    notImplemented();
}

void WebEditorClient::registerCommandForRedo(PassRefPtr<EditCommand>)
{
    notImplemented();
}

void WebEditorClient::clearUndoRedoOperations()
{
    notImplemented();
}

bool WebEditorClient::canUndo() const
{
    notImplemented();
    return false;
}

bool WebEditorClient::canRedo() const
{
    notImplemented();
    return false;
}

void WebEditorClient::undo()
{
    notImplemented();
}

void WebEditorClient::redo()
{
    notImplemented();
}

void WebEditorClient::handleKeyboardEvent(KeyboardEvent* event)
{
    if (m_page->handleEditingKeyboardEvent(event))
        event->setDefaultHandled();
}

void WebEditorClient::handleInputMethodKeydown(KeyboardEvent*)
{
    notImplemented();
}

void WebEditorClient::textFieldDidBeginEditing(Element*)
{
    notImplemented();
}

void WebEditorClient::textFieldDidEndEditing(Element*)
{
    notImplemented();
}

void WebEditorClient::textDidChangeInTextField(Element*)
{
    notImplemented();
}

bool WebEditorClient::doTextFieldCommandFromEvent(Element*, KeyboardEvent*)
{
    notImplemented();
    return false;
}

void WebEditorClient::textWillBeDeletedInTextField(Element*)
{
    notImplemented();
}

void WebEditorClient::textDidChangeInTextArea(Element*)
{
    notImplemented();
}

#if PLATFORM(MAC)
NSString* WebEditorClient::userVisibleString(NSURL*)
{
    notImplemented();
    return nil;
}

#ifdef BUILDING_ON_TIGER
NSArray* WebEditorClient::pasteboardTypesForSelection(Frame*)
{
    notImplemented();
    return nil;
}
#endif
#endif

#if PLATFORM(MAC) && !defined(BUILDING_ON_TIGER) && !defined(BUILDING_ON_LEOPARD)
void WebEditorClient::uppercaseWord()
{
    notImplemented();
}

void WebEditorClient::lowercaseWord()
{
    notImplemented();
}

void WebEditorClient::capitalizeWord()
{
    notImplemented();
}

void WebEditorClient::showSubstitutionsPanel(bool)
{
    notImplemented();
}

bool WebEditorClient::substitutionsPanelIsShowing()
{
    notImplemented();
    return false;
}

void WebEditorClient::toggleSmartInsertDelete()
{
    notImplemented();
}

bool WebEditorClient::isAutomaticQuoteSubstitutionEnabled()
{
    notImplemented();
    return false;
}

void WebEditorClient::toggleAutomaticQuoteSubstitution()
{
    notImplemented();
}

bool WebEditorClient::isAutomaticLinkDetectionEnabled()
{
    notImplemented();
    return false;
}

void WebEditorClient::toggleAutomaticLinkDetection()
{
    notImplemented();
}

bool WebEditorClient::isAutomaticDashSubstitutionEnabled()
{
    notImplemented();
    return false;
}

void WebEditorClient::toggleAutomaticDashSubstitution()
{
    notImplemented();
}

bool WebEditorClient::isAutomaticTextReplacementEnabled()
{
    notImplemented();
    return false;
}

void WebEditorClient::toggleAutomaticTextReplacement()
{
    notImplemented();
}

bool WebEditorClient::isAutomaticSpellingCorrectionEnabled()
{
    notImplemented();
    return false;
}

void WebEditorClient::toggleAutomaticSpellingCorrection()
{
    notImplemented();
}

#endif

void WebEditorClient::ignoreWordInSpellDocument(const String&)
{
    notImplemented();
}

void WebEditorClient::learnWord(const String&)
{
    notImplemented();
}

void WebEditorClient::checkSpellingOfString(const UChar*, int, int*, int*)
{
    notImplemented();
}

String WebEditorClient::getAutoCorrectSuggestionForMisspelledWord(const String&)
{
    notImplemented();
    return String();
}

void WebEditorClient::checkGrammarOfString(const UChar*, int, Vector<GrammarDetail>&, int*, int*)
{
    notImplemented();
}

#if PLATFORM(MAC) && !defined(BUILDING_ON_TIGER) && !defined(BUILDING_ON_LEOPARD)
void WebEditorClient::checkTextOfParagraph(const UChar*, int length, uint64_t, Vector<TextCheckingResult>&)
{
    notImplemented();
}
#endif

void WebEditorClient::updateSpellingUIWithGrammarString(const String&, const GrammarDetail&)
{
    notImplemented();
}

void WebEditorClient::updateSpellingUIWithMisspelledWord(const String&)
{
    notImplemented();
}

void WebEditorClient::showSpellingUI(bool)
{
    notImplemented();
}

bool WebEditorClient::spellingUIIsShowing()
{
    notImplemented();
    return false;
}

void WebEditorClient::getGuessesForWord(const String&, Vector<String>&)
{
    notImplemented();
}

void WebEditorClient::willSetInputMethodState()
{
    notImplemented();
}

void WebEditorClient::setInputMethodState(bool)
{
    notImplemented();
}

} // namespace WebKit
