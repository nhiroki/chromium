/*
 * Copyright (C) 2009, 2010, 2011, 2012, 2013 Apple Inc. All rights reserved.
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

#ifndef MediaPlayerPrivate_h
#define MediaPlayerPrivate_h

#include "MediaPlayer.h"
#include "TimeRanges.h"
#include <wtf/Forward.h>

namespace WebCore {

class IntRect;
class IntSize;
class PlatformTextTrack;

class MediaPlayerPrivateInterface {
    WTF_MAKE_NONCOPYABLE(MediaPlayerPrivateInterface); WTF_MAKE_FAST_ALLOCATED;
public:
    MediaPlayerPrivateInterface() { }
    virtual ~MediaPlayerPrivateInterface() { }

    virtual void load(const String& url) = 0;
    virtual void load(const String& url, PassRefPtr<MediaSource>) = 0;
    virtual void cancelLoad() = 0;

    virtual void prepareToPlay() { }
    virtual PlatformMedia platformMedia() const { return NoPlatformMedia; }
    virtual PlatformLayer* platformLayer() const { return 0; }

    virtual void play() = 0;
    virtual void pause() = 0;

    virtual bool supportsFullscreen() const { return false; }
    virtual bool supportsSave() const { return false; }
    virtual bool supportsScanning() const { return false; }

    virtual IntSize naturalSize() const = 0;

    virtual bool hasVideo() const = 0;
    virtual bool hasAudio() const = 0;

    virtual void setVisible(bool) = 0;

    virtual double duration() const { return 0; }

    virtual double currentTime() const { return 0; }

    virtual void seek(double) { }

    virtual bool seeking() const = 0;

    virtual double startTime() const { return 0; }
    virtual double initialTime() const { return 0; }

    virtual void setRate(double) { }

    virtual void setPreservesPitch(bool) { }

    virtual bool paused() const = 0;

    virtual void setVolume(double) { }

    virtual bool supportsMuting() const { return false; }
    virtual void setMuted(bool) { }

    virtual bool hasClosedCaptions() const { return false; }    
    virtual void setClosedCaptionsVisible(bool) { }

    virtual MediaPlayer::NetworkState networkState() const = 0;
    virtual MediaPlayer::ReadyState readyState() const = 0;

    virtual PassRefPtr<TimeRanges> seekable() const { return maxTimeSeekable() ? TimeRanges::create(0, maxTimeSeekable()) : TimeRanges::create(); }
    virtual double maxTimeSeekable() const { return 0; }
    virtual PassRefPtr<TimeRanges> buffered() const = 0;

    virtual bool didLoadingProgress() const = 0;

    virtual void setSize(const IntSize&) = 0;

    virtual void paint(GraphicsContext*, const IntRect&) = 0;

    virtual void paintCurrentFrameInContext(GraphicsContext* c, const IntRect& r) { paint(c, r); }
    virtual bool copyVideoTextureToPlatformTexture(GraphicsContext3D*, Platform3DObject, GC3Dint, GC3Denum, GC3Denum, bool, bool) { return false; }

    virtual void setPreload(MediaPlayer::Preload) { }

    virtual bool hasAvailableVideoFrame() const { return readyState() >= MediaPlayer::HaveCurrentData; }

    virtual bool canLoadPoster() const { return false; }
    virtual void setPoster(const String&) { }

#if USE(NATIVE_FULLSCREEN_VIDEO)
    virtual void enterFullscreen() { }
    virtual void exitFullscreen() { }
#endif

#if USE(NATIVE_FULLSCREEN_VIDEO)
    virtual bool canEnterFullscreen() const { return false; }
#endif

    // whether accelerated rendering is supported by the media engine for the current media.
    virtual bool supportsAcceleratedRendering() const { return false; }
    // called when the rendering system flips the into or out of accelerated rendering mode.
    virtual void acceleratedRenderingStateChanged() { }

    virtual bool hasSingleSecurityOrigin() const { return false; }

    virtual bool didPassCORSAccessCheck() const { return false; }

    virtual MediaPlayer::MovieLoadType movieLoadType() const { return MediaPlayer::Unknown; }

    virtual void prepareForRendering() { }

    // Time value in the movie's time scale. It is only necessary to override this if the media
    // engine uses rational numbers to represent media time.
    virtual double mediaTimeForTimeValue(double timeValue) const { return timeValue; }

    // Overide this if it is safe for HTMLMediaElement to cache movie time and report
    // 'currentTime' as [cached time + elapsed wall time]. Returns the maximum wall time
    // it is OK to calculate movie time before refreshing the cached time.
    virtual double maximumDurationToCacheMediaTime() const { return 0; }

    virtual unsigned decodedFrameCount() const { return 0; }
    virtual unsigned droppedFrameCount() const { return 0; }
    virtual unsigned audioDecodedByteCount() const { return 0; }
    virtual unsigned videoDecodedByteCount() const { return 0; }

    void getSitesInMediaCache(Vector<String>&) { }
    void clearMediaCache() { }
    void clearMediaCacheForSite(const String&) { }

    virtual String engineDescription() const { return emptyString(); }

#if ENABLE(WEB_AUDIO)
    virtual AudioSourceProvider* audioSourceProvider() { return 0; }
#endif

#if ENABLE(ENCRYPTED_MEDIA)
    virtual MediaPlayer::MediaKeyException addKey(const String&, const unsigned char*, unsigned, const unsigned char*, unsigned, const String&) { return MediaPlayer::KeySystemNotSupported; }
    virtual MediaPlayer::MediaKeyException generateKeyRequest(const String&, const unsigned char*, unsigned) { return MediaPlayer::KeySystemNotSupported; }
    virtual MediaPlayer::MediaKeyException cancelKeyRequest(const String&, const String&) { return MediaPlayer::KeySystemNotSupported; }
#endif

    virtual bool requiresTextTrackRepresentation() const { return false; }
    virtual void setTextTrackRepresentation(TextTrackRepresentation*) { }

#if USE(PLATFORM_TEXT_TRACK_MENU)
    virtual bool implementsTextTrackControls() const { return false; }
    virtual PassRefPtr<PlatformTextTrackMenuInterface> textTrackMenu() { return 0; }
#endif

};

}

#endif
