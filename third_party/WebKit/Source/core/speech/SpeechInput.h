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

#ifndef SpeechInput_h
#define SpeechInput_h

#if ENABLE(INPUT_SPEECH)

#include "core/page/Page.h"
#include "core/speech/SpeechInputListener.h"
#include "wtf/Forward.h"
#include "wtf/HashMap.h"

namespace WebCore {

class IntRect;
class SecurityOrigin;
class SpeechInputClient;

// This class connects the input elements requiring speech input with the platform specific
// speech recognition engine. It provides methods for the input elements to activate speech
// recognition and methods for the speech recognition engine to return back the results.
class SpeechInput FINAL : public NoBaseWillBeGarbageCollectedFinalized<SpeechInput>, public SpeechInputListener, public WillBeHeapSupplement<Page> {
    WILL_BE_USING_GARBAGE_COLLECTED_MIXIN(SpeechInput);
    WTF_MAKE_NONCOPYABLE(SpeechInput);
public:
    virtual ~SpeechInput();

    static PassOwnPtrWillBeRawPtr<SpeechInput> create(PassOwnPtr<SpeechInputClient>);
    static const char* supplementName();
    static SpeechInput* from(Page* page) { return static_cast<SpeechInput*>(WillBeHeapSupplement<Page>::from(page, supplementName())); }

    // Generates a unique ID for the given listener to be used for speech requests.
    // This should be the first call made by listeners before anything else.
    int registerListener(SpeechInputListener*);

    // Invoked when the listener is done with recording or getting destroyed.
    // Failure to unregister may result in crashes if there were any pending speech events.
    void unregisterListener(int);

    // Methods invoked by the input elements.
    bool startRecognition(int listenerId, const IntRect& elementRect, const AtomicString& language, const String& grammar, SecurityOrigin*);
    void stopRecording(int);
    void cancelRecognition(int);

    // SpeechInputListener methods.
    virtual void didCompleteRecording(int) OVERRIDE;
    virtual void didCompleteRecognition(int) OVERRIDE;
    virtual void setRecognitionResult(int, const SpeechInputResultArray&) OVERRIDE;

    virtual void trace(Visitor*) OVERRIDE;
    void clearWeakMembers(Visitor*);

private:
    explicit SpeechInput(PassOwnPtr<SpeechInputClient>);

    OwnPtr<SpeechInputClient> m_client;
    // FIXME: Oilpan: SpeechInputListener is a weak pointer, but it needs manual
    // processing. We keep it raw here to avoid the default weak processing.
    WillBeHeapHashMap<int, SpeechInputListener*> m_listeners;
    int m_nextListenerId;
};

} // namespace WebCore

#endif // ENABLE(INPUT_SPEECH)

#endif // SpeechInput_h
