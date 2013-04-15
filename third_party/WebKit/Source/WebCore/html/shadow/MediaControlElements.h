/*
 * Copyright (C) 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MediaControlElements_h
#define MediaControlElements_h

#if ENABLE(VIDEO)
#include "MediaControlElementTypes.h"
#include "TextTrackRepresentation.h"

namespace WebCore {

// ----------------------------

class MediaControlPanelElement : public MediaControlDivElement {
public:
    static PassRefPtr<MediaControlPanelElement> create(Document*);

    void setCanBeDragged(bool);
    void setIsDisplayed(bool);

    void resetPosition();
    void makeOpaque();
    void makeTransparent();

    virtual bool willRespondToMouseMoveEvents() OVERRIDE { return true; }
    virtual bool willRespondToMouseClickEvents() OVERRIDE { return true; }

private:
    explicit MediaControlPanelElement(Document*);

    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
    virtual void defaultEventHandler(Event*) OVERRIDE;

    void startDrag(const LayoutPoint& eventLocation);
    void continueDrag(const LayoutPoint& eventLocation);
    void endDrag();

    void startTimer();
    void stopTimer();
    void transitionTimerFired(Timer<MediaControlPanelElement>*);

    void setPosition(const LayoutPoint&);

    bool m_canBeDragged;
    bool m_isBeingDragged;
    bool m_isDisplayed;
    bool m_opaque;
    LayoutPoint m_lastDragEventLocation;
    LayoutPoint m_cumulativeDragOffset;

    Timer<MediaControlPanelElement> m_transitionTimer;
};

// ----------------------------

class MediaControlPanelEnclosureElement : public MediaControlDivElement {
public:
    static PassRefPtr<MediaControlPanelEnclosureElement> create(Document*);

private:
    explicit MediaControlPanelEnclosureElement(Document*);
    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
};

// ----------------------------

class MediaControlOverlayEnclosureElement : public MediaControlDivElement {
public:
    static PassRefPtr<MediaControlOverlayEnclosureElement> create(Document*);

private:
    explicit MediaControlOverlayEnclosureElement(Document*);
    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
};

// ----------------------------

class MediaControlPanelMuteButtonElement : public MediaControlMuteButtonElement {
public:
    static PassRefPtr<MediaControlPanelMuteButtonElement> create(Document*, MediaControls*);

    virtual bool willRespondToMouseMoveEvents() OVERRIDE { return true; }

private:
    explicit MediaControlPanelMuteButtonElement(Document*, MediaControls*);

    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
    virtual void defaultEventHandler(Event*) OVERRIDE;

    MediaControls* m_controls;
};

// ----------------------------

class MediaControlVolumeSliderMuteButtonElement : public MediaControlMuteButtonElement {
public:
    static PassRefPtr<MediaControlVolumeSliderMuteButtonElement> create(Document*);

private:
    explicit MediaControlVolumeSliderMuteButtonElement(Document*);
    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
};


// ----------------------------

class MediaControlPlayButtonElement : public MediaControlInputElement {
public:
    static PassRefPtr<MediaControlPlayButtonElement> create(Document*);

    virtual bool willRespondToMouseClickEvents() OVERRIDE { return true; }
    virtual void updateDisplayType() OVERRIDE;

private:
    explicit MediaControlPlayButtonElement(Document*);

    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
    virtual void defaultEventHandler(Event*) OVERRIDE;
};

// ----------------------------

class MediaControlOverlayPlayButtonElement : public MediaControlInputElement {
public:
    static PassRefPtr<MediaControlOverlayPlayButtonElement> create(Document*);

    virtual void updateDisplayType() OVERRIDE;

private:
    explicit MediaControlOverlayPlayButtonElement(Document*);

    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
    virtual void defaultEventHandler(Event*) OVERRIDE;
};

// ----------------------------

class MediaControlToggleClosedCaptionsButtonElement : public MediaControlInputElement {
public:
    static PassRefPtr<MediaControlToggleClosedCaptionsButtonElement> create(Document*, MediaControls*);

    virtual bool willRespondToMouseClickEvents() OVERRIDE { return true; }

    virtual void updateDisplayType() OVERRIDE;

private:
    explicit MediaControlToggleClosedCaptionsButtonElement(Document*, MediaControls*);

    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
    virtual void defaultEventHandler(Event*) OVERRIDE;
};

// ----------------------------

class MediaControlClosedCaptionsContainerElement : public MediaControlDivElement {
public:
    static PassRefPtr<MediaControlClosedCaptionsContainerElement> create(Document*);

    virtual bool willRespondToMouseClickEvents() OVERRIDE { return true; }

private:
    MediaControlClosedCaptionsContainerElement(Document*);
    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
};

// ----------------------------

class MediaControlClosedCaptionsTrackListElement : public MediaControlDivElement {
public:
    static PassRefPtr<MediaControlClosedCaptionsTrackListElement> create(Document*, MediaControls*);

    virtual bool willRespondToMouseClickEvents() OVERRIDE { return true; }

    void updateDisplay();
    void resetTrackListMenu() { m_trackListHasChanged = true; }

private:
    MediaControlClosedCaptionsTrackListElement(Document*, MediaControls*);

    void rebuildTrackListMenu();

    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
    virtual void defaultEventHandler(Event*) OVERRIDE;

    typedef Vector<RefPtr<Element> > TrackMenuItems;
    TrackMenuItems m_menuItems;
    typedef HashMap<RefPtr<Element>, RefPtr<TextTrack> > MenuItemToTrackMap;
    MenuItemToTrackMap m_menuToTrackMap;
    MediaControls* m_controls;
    bool m_trackListHasChanged;
};

// ----------------------------

class MediaControlTimelineElement : public MediaControlInputElement {
public:
    static PassRefPtr<MediaControlTimelineElement> create(Document*, MediaControls*);

    virtual bool willRespondToMouseClickEvents() OVERRIDE;

    void setPosition(double);
    void setDuration(double);

private:
    explicit MediaControlTimelineElement(Document*, MediaControls*);

    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
    virtual void defaultEventHandler(Event*) OVERRIDE;

    MediaControls* m_controls;
};

// ----------------------------

class MediaControlFullscreenButtonElement : public MediaControlInputElement {
public:
    static PassRefPtr<MediaControlFullscreenButtonElement> create(Document*);

    virtual bool willRespondToMouseClickEvents() OVERRIDE { return true; }

    virtual void setIsFullscreen(bool);

private:
    explicit MediaControlFullscreenButtonElement(Document*);

    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
    virtual void defaultEventHandler(Event*) OVERRIDE;
};

// ----------------------------

class MediaControlPanelVolumeSliderElement : public MediaControlVolumeSliderElement {
public:
    static PassRefPtr<MediaControlPanelVolumeSliderElement> create(Document*);

private:
    explicit MediaControlPanelVolumeSliderElement(Document*);
    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
};

// ----------------------------

class MediaControlTimeRemainingDisplayElement : public MediaControlTimeDisplayElement {
public:
    static PassRefPtr<MediaControlTimeRemainingDisplayElement> create(Document*);

private:
    explicit MediaControlTimeRemainingDisplayElement(Document*);
    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
};

// ----------------------------

class MediaControlCurrentTimeDisplayElement : public MediaControlTimeDisplayElement {
public:
    static PassRefPtr<MediaControlCurrentTimeDisplayElement> create(Document*);

private:
    explicit MediaControlCurrentTimeDisplayElement(Document*);
    virtual const AtomicString& shadowPseudoId() const OVERRIDE;
};

// ----------------------------

class MediaControlTextTrackContainerElement : public MediaControlDivElement, public TextTrackRepresentationClient {
public:
    static PassRefPtr<MediaControlTextTrackContainerElement> create(Document*);

    void updateDisplay();
    void updateSizes(bool forceUpdate = false);
    static const AtomicString& textTrackContainerElementShadowPseudoId();

private:
    explicit MediaControlTextTrackContainerElement(Document*);
    virtual const AtomicString& shadowPseudoId() const OVERRIDE;

    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);

    virtual void paintTextTrackRepresentation(GraphicsContext*, const IntRect&) OVERRIDE;
    virtual void textTrackRepresentationBoundsChanged(const IntRect&) OVERRIDE;
    OwnPtr<TextTrackRepresentation> m_textTrackRepresentation;

    IntRect m_videoDisplaySize;
    float m_fontSize;
};


} // namespace WebCore

#endif // ENABLE(VIDEO)

#endif // MediaControlElements_h
