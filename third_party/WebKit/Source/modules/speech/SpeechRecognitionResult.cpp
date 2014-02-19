/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "modules/speech/SpeechRecognitionResult.h"

namespace WebCore {

DEFINE_GC_INFO(SpeechRecognitionResult);

SpeechRecognitionResult::~SpeechRecognitionResult()
{
}

PassRefPtrWillBeRawPtr<SpeechRecognitionResult> SpeechRecognitionResult::create(const WillBeHeapVector<RefPtrWillBeMember<SpeechRecognitionAlternative> >& alternatives, bool final)
{
    return adoptRefWillBeNoop(new SpeechRecognitionResult(alternatives, final));
}

SpeechRecognitionAlternative* SpeechRecognitionResult::item(unsigned long index)
{
    if (index >= m_alternatives.size())
        return 0;

    return m_alternatives[index].get();
}

SpeechRecognitionResult::SpeechRecognitionResult(const WillBeHeapVector<RefPtrWillBeMember<SpeechRecognitionAlternative> >& alternatives, bool final)
    : m_final(final)
    , m_alternatives(alternatives)
{
    ScriptWrappable::init(this);
}

void SpeechRecognitionResult::trace(Visitor* visitor)
{
    visitor->trace(m_alternatives);
}

} // namespace WebCore
