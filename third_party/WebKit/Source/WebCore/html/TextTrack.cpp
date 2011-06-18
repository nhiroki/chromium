/*
 * Copyright (C) 2011 Google Inc.  All rights reserved.
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

#if ENABLE(VIDEO_TRACK)

#include "TextTrack.h"

#include "TextTrackCueList.h"
#include "TextTrackPrivate.h"

namespace WebCore {

class NullTextTrackPrivate : public TextTrackPrivateInterface {
public:
    NullTextTrackPrivate();
    virtual ~NullTextTrackPrivate();

    // TextTrackPrivateInterface methods
    virtual String kind() const { return emptyString(); }
    virtual String label() const { return emptyString(); }
    virtual String language() const { return emptyString(); }
    virtual TextTrack::ReadyState readyState() const { return TextTrack::NONE; }
    virtual TextTrack::Mode mode() const { return TextTrack::OFF; }
    virtual void setMode(TextTrack::Mode) { }
    virtual PassRefPtr<TextTrackCueList> cues() const { return 0; }
    virtual PassRefPtr<TextTrackCueList> activeCues() const { return 0; }
    virtual void load(const String&) { }
};

static PassOwnPtr<NullTextTrackPrivate> createNullTextTrackPrivate()
{
    return adoptPtr(new NullTextTrackPrivate());
}

TextTrack::TextTrack()
    : m_private(createNullTextTrackPrivate())
{
}

TextTrack::~TextTrack()
{
}

String TextTrack::kind() const
{
    return m_private->kind();
}

String TextTrack::label() const
{
    return m_private->label();
}

String TextTrack::language() const
{
    return m_private->language();
}

TextTrack::ReadyState TextTrack::readyState() const
{
    return m_private->readyState();
}

TextTrack::Mode TextTrack::mode() const
{
    return m_private->mode();
}

void TextTrack::setMode(unsigned short mode, ExceptionCode& ec)
{
    // 4.8.10.12.5 On setting the mode, if the new value is not either 0, 1, or 2,
    // the user agent must throw an INVALID_ACCESS_ERR exception.
    if (mode == TextTrack::OFF || mode == TextTrack::HIDDEN || mode == TextTrack::SHOWING)
        m_private->setMode(static_cast<Mode>(mode));
    else
        ec = INVALID_ACCESS_ERR;
}

PassRefPtr<TextTrackCueList> TextTrack::cues() const
{
    return m_private->cues();
}

PassRefPtr<TextTrackCueList> TextTrack::activeCues() const
{
    return m_private->activeCues();
}

} // namespace WebCore

#endif
