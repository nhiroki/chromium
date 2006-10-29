/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com 
 * All rights reserved.
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
#include "FrameGdk.h"
#include "Element.h"
#include "RenderObject.h"
#include "RenderWidget.h"
#include "RenderLayer.h"
#include "Page.h"
#include "Document.h"
#include "DOMWindow.h"
#include "DOMImplementation.h"
#include "BrowserExtensionGdk.h"
#include "Document.h"
#include "Settings.h"
#include "Plugin.h"
#include "FramePrivate.h"
#include "GraphicsContext.h"
#include "HTMLDocument.h"
#include "ResourceHandle.h"
#include "ResourceHandleInternal.h"
#include "PlatformMouseEvent.h"
#include "PlatformKeyboardEvent.h"
#include "PlatformWheelEvent.h"
#include "MouseEventWithHitTestResults.h"
#include "SelectionController.h"
#include "TypingCommand.h"
#include "SSLKeyGenerator.h"
#include "KeyboardCodes.h"
#include "FrameLoadRequest.h"
#include <gdk/gdk.h>


// This function loads resources from WebKit
// This does not belong here and I'm not sure where
// it should go
// I don't know what the plans or design is
// for none code resources
Vector<char> loadResourceIntoArray(const char* resourceName)
{
    Vector<char> resource;
    //if (strcmp(resourceName,"missingImage") == 0) {
    //}
    return resource;
}

namespace WebCore {

FrameGdkClientDefault::FrameGdkClientDefault()
    : ResourceHandleClient()
    , m_frame(0)
    , m_beginCalled(false)
{
}

FrameGdkClientDefault::~FrameGdkClientDefault()
{
}

void FrameGdkClientDefault::setFrame(const FrameGdk* frame)
{
    m_frame = const_cast<FrameGdk*>(frame);
}

void FrameGdkClientDefault::openURL(const KURL& url)
{
    m_frame->didOpenURL(url);
    m_beginCalled = false;

    ResourceRequest request(url);
    RefPtr<ResourceHandle> loader = ResourceHandle::create(request, this, 0);
}

void FrameGdkClientDefault::submitForm(const String& method, const KURL& url, const FormData* postData)
{
    m_beginCalled = false;

    ResourceRequest request(url);
    request.setHTTPMethod(method);
    request.setHTTPBody(*postData);

    RefPtr<ResourceHandle> loader = ResourceHandle::create(request, this, 0);
}

void FrameGdkClientDefault::receivedResponse(ResourceHandle*, PlatformResponse)
{
    // no-op
}

void FrameGdkClientDefault::didReceiveData(ResourceHandle* job, const char* data, int length)
{
    if (!m_beginCalled) {
        m_beginCalled = true;

#if 0  // FIXME: This is from Qt version, need to be removed or Gdk equivalent written
        // Assign correct mimetype _before_ calling begin()!
        ResourceHandleInternal* d = job->getInternal();
        if (d) {
            ResourceRequest request(m_frame->resourceRequest());
            request.m_responseMIMEType = d->m_mimetype;
            m_frame->setResourceRequest(request);
        }
#endif
        m_frame->begin(job->url());
    }

    m_frame->write(data, length);
}

void FrameGdkClientDefault::receivedAllData(ResourceHandle* job, PlatformData data)
{
    m_frame->end();
    m_beginCalled = false;
}

static void doScroll(const RenderObject* r, float deltaX, float deltaY)
{
    // FIXME: The scrolling done here should be done in the default handlers
    // of the elements rather than here in the part.
    if (!r)
        return;

    //broken since it calls scroll on scrollbars
    //and we have none now
    //r->scroll(direction, ScrollByWheel, multiplier);
    if (!r->layer())
        return;

    int x = r->layer()->scrollXOffset() + deltaX;
    int y = r->layer()->scrollYOffset() + deltaY;
    r->layer()->scrollToOffset(x, y, true, true);
}

FrameGdk::FrameGdk(GdkDrawable* gdkdrawable)
    : Frame(new Page, 0), m_drawable(gdkdrawable)
{
    d->m_extension = new BrowserExtensionGdk(this);
    Settings* settings = new Settings;
    settings->setAutoLoadImages(true);
    settings->setMinFontSize(5);
    settings->setMinLogicalFontSize(5);
    settings->setShouldPrintBackgrounds(true);

    settings->setMediumFixedFontSize(14);
    settings->setMediumFontSize(14);
    settings->setSerifFontName("Times New Roman");
    settings->setSansSerifFontName("Arial");
    settings->setFixedFontName("Courier");
    settings->setStdFontName("Arial");
    setSettings(settings);
    page()->setMainFrame(this);
    FrameView* view = new FrameView(this);
    setView(view);
    IntRect geom = frameGeometry();
    view->resize(geom.width(), geom.height());
    view->ScrollView::setDrawable(gdkdrawable);

    m_client = new FrameGdkClientDefault();
    m_client->setFrame(this);
}

FrameGdk::FrameGdk(Page* page, Element* element)
    : Frame(page,element)
{
    d->m_extension = new BrowserExtensionGdk(this);
    Settings* settings = new Settings;
    settings->setAutoLoadImages(true);
    setSettings(settings);
    m_client = new FrameGdkClientDefault();
    m_client->setFrame(this);
}

FrameGdk::~FrameGdk()
{
    cancelAndClear();
}

void FrameGdk::submitForm(const FrameLoadRequest& frameLoadRequest)
{
    ResourceRequest request = frameLoadRequest.m_request;

    // FIXME: this is a hack inherited from FrameMac, and should be pushed into Frame
    if (d->m_submittedFormURL == request.url())
        return;

    d->m_submittedFormURL = request.url();

    if (m_client)
        m_client->submitForm(request.doPost() ? "POST" : "GET", request.url(), &request.postData);
    clearRecordedFormValues();
}

void FrameGdk::urlSelected(const FrameLoadRequest& frameLoadRequest)
{
    ResourceRequest request = frameLoadRequest.m_request;

    if (!m_client)
        return;

    m_client->openURL(request.url());
}

String FrameGdk::userAgent() const
{
    return "Mozilla/5.0 (PC; U; Intel; Linux; en) AppleWebKit/420+ (KHTML, like Gecko)";
}

void FrameGdk::runJavaScriptAlert(String const& message)
{
}

bool FrameGdk::runJavaScriptConfirm(String const& message)
{
    return true;
}

void FrameGdk::setTitle(const String &title)
{
}

void FrameGdk::handleGdkEvent(GdkEvent* event)
{
    switch (event->type) {
        case GDK_EXPOSE: {
            GdkRectangle clip;
            gdk_region_get_clipbox(event->expose.region, &clip);
            gdk_window_begin_paint_region (event->any.window, event->expose.region);
            cairo_t* cr = gdk_cairo_create (event->any.window);
            GraphicsContext* ctx = new GraphicsContext(cr);
            paint(ctx, IntRect(clip.x, clip.y, clip.width, clip.height));
            delete ctx;
            cairo_destroy(cr);
            gdk_window_end_paint (event->any.window);
            break;
        }
        case GDK_SCROLL: {
            PlatformWheelEvent wheelEvent(event);
            view()->handleWheelEvent(wheelEvent);
            if (wheelEvent.isAccepted()) {
                return;
            }
            RenderObject::NodeInfo nodeInfo(true, true);
            renderer()->layer()->hitTest(nodeInfo, wheelEvent.pos());
            Node* node = nodeInfo.innerNode();
            if (!node)
                return;
            //Default to scrolling the page
            //not sure why its null
            //broke anyway when its not null
            doScroll(renderer(), wheelEvent.deltaX(), wheelEvent.deltaY());
            break;
        }
        case GDK_DRAG_ENTER:
        case GDK_DRAG_LEAVE:
        case GDK_DRAG_MOTION:
        case GDK_DRAG_STATUS:
        case GDK_DROP_START:
        case GDK_DROP_FINISHED: {
            //bool updateDragAndDrop(const PlatformMouseEvent&, Clipboard*);
            //void cancelDragAndDrop(const PlatformMouseEvent&, Clipboard*);
            //bool performDragAndDrop(const PlatformMouseEvent&, Clipboard*);
            break;
        }
        case GDK_MOTION_NOTIFY:
            view()->handleMouseMoveEvent(event);
            break;
        case GDK_BUTTON_PRESS:
        case GDK_2BUTTON_PRESS:
        case GDK_3BUTTON_PRESS:
            view()->handleMousePressEvent(event);
            break;
        case GDK_BUTTON_RELEASE:
            view()->handleMouseReleaseEvent(event);
            break;
        case GDK_KEY_PRESS:
        case GDK_KEY_RELEASE: {
            PlatformKeyboardEvent kevent(event);
            bool handled = false;
            if (!kevent.isKeyUp()) {
                Node* start = selectionController()->start().node();
                if (start && start->isContentEditable()) {
                    switch(kevent.WindowsKeyCode()) {
                        case VK_BACK:
                            TypingCommand::deleteKeyPressed(document());
                            break;
                        case VK_DELETE:
                            TypingCommand::forwardDeleteKeyPressed(document());
                            break;
                        case VK_LEFT:
                            selectionController()->modify(SelectionController::MOVE, SelectionController::LEFT, CharacterGranularity);
                            break;
                        case VK_RIGHT:
                            selectionController()->modify(SelectionController::MOVE, SelectionController::RIGHT, CharacterGranularity);
                            break;
                        case VK_UP:
                            selectionController()->modify(SelectionController::MOVE, SelectionController::BACKWARD, ParagraphGranularity);
                            break;
                        case VK_DOWN:
                            selectionController()->modify(SelectionController::MOVE, SelectionController::FORWARD, ParagraphGranularity);
                            break;
                        default:
                            TypingCommand::insertText(document(), kevent.text(), false);

                    }
                    handled = true;
                }
                if (!handled) {
                    switch (kevent.WindowsKeyCode()) {
                        case VK_LEFT:
                            doScroll(renderer(), true, -120);
                            break;
                        case VK_RIGHT:
                            doScroll(renderer(), true, 120);
                            break;
                        case VK_UP:
                            doScroll(renderer(), false, -120);
                            break;
                        case VK_PRIOR:
                            //return SB_PAGEUP;
                            break;
                        case VK_NEXT:
                            //return SB_PAGEDOWN;
                            break;
                        case VK_DOWN:
                            doScroll(renderer(), false, 120);
                            break;
                        case VK_HOME:
                            renderer()->layer()->scrollToOffset(0, 0, true, true);
                            doScroll(renderer(), false, 120);
                            break;
                        case VK_END:
                            renderer()->layer()->scrollToOffset(0,
                                                                renderer()->height(), true, true);
                            break;
                        case VK_SPACE:
                            if (kevent.shiftKey())
                                doScroll(renderer(), false, -120);
                            else
                                doScroll(renderer(), false, 120);
                            break;
                    }

                }
            }
        }
        default:
            break;
    }
}

void FrameGdk::setFrameGeometry(const IntRect &r)
{
    if (!m_drawable || !GDK_IS_WINDOW(m_drawable))
        return;
    GdkWindow* window = GDK_WINDOW(m_drawable);
    gdk_window_move_resize(window, r.x(), r.y(), r.width(), r.height());
}

IntRect FrameGdk::frameGeometry() const
{
    gint x, y, width, height, depth;
    if (!m_drawable)
        return IntRect();

    if (!GDK_IS_WINDOW(m_drawable)) {
        gdk_drawable_get_size(m_drawable, &width, &height);
        return IntRect(0, 0, width, height);
    }

    GdkWindow* window = GDK_WINDOW(m_drawable);
    gdk_window_get_geometry(window, &x, &y, &width, &height, &depth);
    return IntRect(x, y, width, height);
}

bool FrameGdk::passWheelEventToChildWidget(Node* node)
{
    if (!node)
        return false;
    RenderObject* renderer = node->renderer();
    if (!renderer || !renderer->isWidget())
        return false;
    Widget* widget = static_cast<RenderWidget*>(renderer)->widget();
    if (!widget)
        return false;
    return true;
}

bool FrameGdk::passSubframeEventToSubframe(MouseEventWithHitTestResults& mev, Frame*)
{
    if (mev.targetNode() == 0)
        return true;
    return false;
}

}
