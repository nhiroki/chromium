/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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
#include "DOMWindow.h"

#include "V8Binding.h"
#include "V8CustomBinding.h"
#include "V8CustomEventListener.h"
#include "V8Proxy.h"

#include "DOMTimer.h"
#include "Frame.h"
#include "FrameLoadRequest.h"
#include "FrameView.h"
#include "Page.h"
#include "PlatformScreen.h"
#include "ScriptSourceCode.h"
#include "Settings.h"
#include "WindowFeatures.h"


// Horizontal and vertical offset, from the parent content area, around newly
// opened popups that don't specify a location.
static const int popupTilePixels = 10;

namespace WebCore {

ACCESSOR_SETTER(DOMWindowLocation)
{
    v8::Handle<v8::Object> holder = V8Proxy::LookupDOMWrapper(V8ClassIndex::DOMWINDOW, info.This());
    if (holder.IsEmpty())
        return;

    DOMWindow* imp = V8Proxy::ToNativeObject<DOMWindow>(V8ClassIndex::DOMWINDOW, holder);
    WindowSetLocation(imp, toWebCoreString(value));
}


ACCESSOR_SETTER(DOMWindowOpener)
{
    DOMWindow* imp = V8Proxy::ToNativeObject<DOMWindow>(V8ClassIndex::DOMWINDOW, info.Holder());

    if (!V8Proxy::CanAccessFrame(imp->frame(), true))
        return;
  
    // Opener can be shadowed if it is in the same domain.
    // Have a special handling of null value to behave
    // like Firefox. See bug http://b/1224887 & http://b/791706.
    if (value->IsNull()) {
        // imp->frame() cannot be null,
        // otherwise, SameOrigin check would have failed.
        ASSERT(imp->frame());
        imp->frame()->loader()->setOpener(0);
    }

    // Delete the accessor from this object.
    info.Holder()->Delete(name);

    // Put property on the front (this) object.
    info.This()->Set(name, value);
}

CALLBACK_FUNC_DECL(DOMWindowAddEventListener)
{
    INC_STATS("DOM.DOMWindow.addEventListener()");
    DOMWindow* imp = V8Proxy::ToNativeObject<DOMWindow>(V8ClassIndex::DOMWINDOW, args.Holder());

    if (!V8Proxy::CanAccessFrame(imp->frame(), true))
        return v8::Undefined();

    if (!imp->frame())
        return v8::Undefined();  // DOMWindow could be disconnected from the frame
  
    Document* doc = imp->frame()->document();
    if (!doc)
        return v8::Undefined();

    // TODO: Check if there is not enough arguments
    V8Proxy* proxy = V8Proxy::retrieve(imp->frame());
    if (!proxy)
        return v8::Undefined();

    RefPtr<EventListener> listener = proxy->FindOrCreateV8EventListener(args[1], false);

    if (listener) {
        String eventType = toWebCoreString(args[0]);
        bool useCapture = args[2]->BooleanValue();
        doc->addWindowEventListener(eventType, listener, useCapture);
    }

    return v8::Undefined();
}


CALLBACK_FUNC_DECL(DOMWindowRemoveEventListener)
{
    INC_STATS("DOM.DOMWindow.removeEventListener()");
    DOMWindow* imp = V8Proxy::ToNativeObject<DOMWindow>(V8ClassIndex::DOMWINDOW, args.Holder());

    if (!V8Proxy::CanAccessFrame(imp->frame(), true))
        return v8::Undefined();

    if (!imp->frame())
        return v8::Undefined();

    Document* doc = imp->frame()->document();
    if (!doc)
        return v8::Undefined();

    V8Proxy* proxy = V8Proxy::retrieve(imp->frame());
    if (!proxy)
        return v8::Undefined();

    RefPtr<EventListener> listener = proxy->FindV8EventListener(args[1], false);

    if (listener) {
        String eventType = toWebCoreString(args[0]);
        bool useCapture = args[2]->BooleanValue();
        doc->removeWindowEventListener(eventType, listener.get(), useCapture);
    }

    return v8::Undefined();
}

CALLBACK_FUNC_DECL(DOMWindowPostMessage)
{
    INC_STATS("DOM.DOMWindow.postMessage()");
    DOMWindow* window = V8Proxy::ToNativeObject<DOMWindow>(V8ClassIndex::DOMWINDOW, args.Holder());

    DOMWindow* source = V8Proxy::retrieveActiveFrame()->domWindow();
    ASSERT(source->frame());

    String uri = source->frame()->loader()->url().string();

    v8::TryCatch tryCatch;

    String message = toWebCoreString(args[0]);
    MessagePort* port = 0;
    String domain;

    // This function has variable arguments and can either be:
    //   postMessage(message, port, domain);
    // or
    //   postMessage(message, domain);
    if (args.Length() > 2) {
        if (V8Proxy::IsWrapperOfType(args[1], V8ClassIndex::MESSAGEPORT))
            port = V8Proxy::ToNativeObject<MessagePort>(V8ClassIndex::MESSAGEPORT, args[1]);
        domain = valueToStringWithNullOrUndefinedCheck(args[2]);
    } else
        domain = valueToStringWithNullOrUndefinedCheck(args[1]);

    if (tryCatch.HasCaught())
        return v8::Undefined();

    ExceptionCode ec = 0;
    window->postMessage(message, port, domain, source, ec);
    if (ec)
        V8Proxy::SetDOMException(ec);

    return v8::Undefined();
}


static bool canShowModalDialogNow(const Frame* frame)
{
    // A frame can out live its page. See bug 1219613.
    if (!frame || !frame->page())
        return false;
    return frame->page()->chrome()->canRunModalNow();
}

static bool allowPopUp()
{
    Frame* frame = V8Proxy::retrieveActiveFrame();

    ASSERT(frame);
    if (frame->script()->processingUserGesture())
        return true;
    Settings* settings = frame->settings();
    return settings && settings->JavaScriptCanOpenWindowsAutomatically();
}

static HashMap<String, String> parseModalDialogFeatures(const String& featuresArg)
{
    HashMap<String, String> map;

    Vector<String> features;
    featuresArg.split(';', features);
    Vector<String>::const_iterator end = features.end();
    for (Vector<String>::const_iterator it = features.begin(); it != end; ++it) {
        String featureString = *it;
        int pos = featureString.find('=');
        int colonPos = featureString.find(':');
        if (pos >= 0 && colonPos >= 0)
            continue;  // ignore any strings that have both = and :
        if (pos < 0)
            pos = colonPos;
        if (pos < 0) {
            // null string for value means key without value
            map.set(featureString.stripWhiteSpace().lower(), String());
        } else {
            String key = featureString.left(pos).stripWhiteSpace().lower();
            String val = featureString.substring(pos + 1).stripWhiteSpace().lower();
            int spacePos = val.find(' ');
            if (spacePos != -1)
                val = val.left(spacePos);
            map.set(key, val);
        }
    }

    return map;
}


static Frame* createWindow(Frame* openerFrame,
                           const String& url,
                           const String& frameName,
                           const WindowFeatures& windowFeatures,
                           v8::Local<v8::Value> dialogArgs)
{
    Frame* activeFrame = V8Proxy::retrieveActiveFrame();

    ResourceRequest request;
    if (activeFrame)
        request.setHTTPReferrer(activeFrame->loader()->outgoingReferrer());
    FrameLoadRequest frameRequest(request, frameName);

    // FIXME: It's much better for client API if a new window starts with a URL,
    // here where we know what URL we are going to open. Unfortunately, this
    // code passes the empty string for the URL, but there's a reason for that.
    // Before loading we have to set up the opener, openedByDOM,
    // and dialogArguments values. Also, to decide whether to use the URL
    // we currently do an allowsAccessFrom call using the window we create,
    // which can't be done before creating it. We'd have to resolve all those
    // issues to pass the URL instead of "".

    bool created;
    // We pass in the opener frame here so it can be used for looking up the
    // frame name, in case the active frame is different from the opener frame,
    // and the name references a frame relative to the opener frame, for example
    // "_self" or "_parent".
    Frame* newFrame = activeFrame->loader()->createWindow(openerFrame->loader(), frameRequest, windowFeatures, created);
    if (!newFrame)
        return 0;

    newFrame->loader()->setOpener(openerFrame);
    newFrame->loader()->setOpenedByDOM();

    // Set dialog arguments on the global object of the new frame.
    if (!dialogArgs.IsEmpty()) {
        v8::Local<v8::Context> context = V8Proxy::GetContext(newFrame);
        if (!context.IsEmpty()) {
            v8::Context::Scope scope(context);
            context->Global()->Set(v8::String::New("dialogArguments"), dialogArgs);
        }
    }

    if (!parseURL(url).startsWith("javascript:", false)
        || ScriptController::isSafeScript(newFrame)) {
        KURL completedUrl =
            url.isEmpty() ? KURL("") : activeFrame->document()->completeURL(url);
        bool userGesture = activeFrame->script()->processingUserGesture();

        if (created)
            newFrame->loader()->changeLocation(completedUrl, activeFrame->loader()->outgoingReferrer(), false, false, userGesture);
        else if (!url.isEmpty())
            newFrame->loader()->scheduleLocationChange(completedUrl.string(), activeFrame->loader()->outgoingReferrer(), false, userGesture);
    }

    return newFrame;
}



CALLBACK_FUNC_DECL(DOMWindowShowModalDialog)
{
    INC_STATS("DOM.DOMWindow.showModalDialog()");
    DOMWindow* window = V8Proxy::ToNativeObject<DOMWindow>(
        V8ClassIndex::DOMWINDOW, args.Holder());
    Frame* frame = window->frame();

    if (!frame || !V8Proxy::CanAccessFrame(frame, true)) 
        return v8::Undefined();

    if (!canShowModalDialogNow(frame) || !allowPopUp())
        return v8::Undefined();

    String url = valueToStringWithNullOrUndefinedCheck(args[0]);
    v8::Local<v8::Value> dialogArgs = args[1];
    String featureArgs = valueToStringWithNullOrUndefinedCheck(args[2]);

    const HashMap<String, String> features = parseModalDialogFeatures(featureArgs);

    const bool trusted = false;

    FloatRect screenRect = screenAvailableRect(frame->view());

    WindowFeatures features;
    // default here came from frame size of dialog in MacIE.
    features.width = WindowFeatures::floatFeature(features, "dialogwidth", 100, screenRect.width(), 620);
    features.widthSet = true;
    // default here came from frame size of dialog in MacIE.
    features.height = WindowFeatures::floatFeature(features, "dialogheight", 100, screenRect.height(), 450);
    features.heightSet = true;
  
    features.x = WindowFeatures::floatFeature(features, "dialogleft", screenRect.x(), screenRect.right() - features.width, -1);
    features.xSet = features.x > 0;
    features.y = WindowFeatures::floatFeature(features, "dialogtop", screenRect.y(), screenRect.bottom() - features.height, -1);
    features.ySet = features.y > 0;

    if (WindowFeatures::boolFeature(features, "center", true)) {
        if (!features.xSet) {
            features.x = screenRect.x() + (screenRect.width() - features.width) / 2;
            features.xSet = true;
        }
        if (!features.ySet) {
            features.y = screenRect.y() + (screenRect.height() - features.height) / 2;
            features.ySet = true;
        }
    }

    features.dialog = true;
    features.resizable = WindowFeatures::boolFeature(features, "resizable");
    features.scrollbarsVisible = WindowFeatures::boolFeature(features, "scroll", true);
    features.statusBarVisible = WindowFeatures::boolFeature(features, "status", !trusted);
    features.menuBarVisible = false;
    features.toolBarVisible = false;
    features.locationBarVisible = false;
    features.fullscreen = false;

    Frame* dialogFrame = createWindow(frame, url, "", features, dialogArgs);
    if (!dialogFrame)
        return v8::Undefined();

    // Hold on to the context of the dialog window long enough to retrieve the
    // value of the return value property.
    v8::Local<v8::Context> context = V8Proxy::GetContext(dialogFrame);

    // Run the dialog.
    dialogFrame->page()->chrome()->runModal();

    // Extract the return value property from the dialog window.
    v8::Local<v8::Value> returnValue;
    if (!context.IsEmpty()) {
        v8::Context::Scope scope(context);
        returnValue = context->Global()->Get(v8::String::New("returnValue"));
    }

    if (!returnValue.IsEmpty())
        return returnValue;

    return v8::Undefined();
}


CALLBACK_FUNC_DECL(DOMWindowOpen)
{
    INC_STATS("DOM.DOMWindow.open()");
    DOMWindow* parent = V8Proxy::ToNativeObject<DOMWindow>(V8ClassIndex::DOMWINDOW, args.Holder());
    Frame* frame = parent->frame();

    if (!V8Proxy::CanAccessFrame(frame, true))
      return v8::Undefined();

    Frame* activeFrame = V8Proxy::retrieveActiveFrame();
    if (!activeFrame)
      return v8::Undefined();

    Page* page = frame->page();
    if (!page)
      return v8::Undefined();

    String urlString = valueToStringWithNullOrUndefinedCheck(args[0]);
    AtomicString frameName = (args[1]->IsUndefined() || args[1]->IsNull()) ? "_blank" : AtomicString(toWebCoreString(args[1]));

    // Because FrameTree::find() returns true for empty strings, we must check
    // for empty framenames. Otherwise, illegitimate window.open() calls with
    // no name will pass right through the popup blocker.
    if (!allowPopUp() &&
        (frameName.isEmpty() || !frame->tree()->find(frameName))) {
        return v8::Undefined();
    }

    // Get the target frame for the special cases of _top and _parent.  In those
    // cases, we can schedule a location change right now and return early.
    bool topOrParent = false;
    if (frameName == "_top") {
        frame = frame->tree()->top();
        topOrParent = true;
    } else if (frameName == "_parent") {
        if (Frame* parent = frame->tree()->parent())
            frame = parent;
        topOrParent = true;
    }
    if (topOrParent) {
        if (!activeFrame->loader()->shouldAllowNavigation(frame))
            return v8::Undefined();
    
        String completedUrl;
        if (!urlString.isEmpty())
            completedUrl = activeFrame->document()->completeURL(urlString);
    
        if (!completedUrl.isEmpty() &&
            (!parseURL(urlString).startsWith("javascript:", false)
             || ScriptController::isSafeScript(frame))) {
            bool userGesture = activeFrame->script()->processingUserGesture();
            frame->loader()->scheduleLocationChange(completedUrl, activeFrame->loader()->outgoingReferrer(), false, userGesture);
        }
        return V8Proxy::ToV8Object(V8ClassIndex::DOMWINDOW, frame->domWindow());
    }

    // In the case of a named frame or a new window, we'll use the
    // createWindow() helper.

    // Parse the values, and then work with a copy of the parsed values
    // so we can restore the values we may not want to overwrite after
    // we do the multiple monitor fixes.
    WindowFeatures rawFeatures(valueToStringWithNullOrUndefinedCheck(args[2]));
    WindowFeatures windowFeatures(rawFeatures);
    FloatRect screenRect = screenAvailableRect(page->mainFrame()->view());

    // Set default size and location near parent window if none were specified.
    // These may be further modified by adjustWindowRect, below.
    if (!windowFeatures.xSet) {
        windowFeatures.x = parent->screenX() - screenRect.x() + popupTilePixels;
        windowFeatures.xSet = true;
    }
    if (!windowFeatures.ySet) {
        windowFeatures.y = parent->screenY() - screenRect.y() + popupTilePixels;
        windowFeatures.ySet = true;
    }
    if (!windowFeatures.widthSet) {
        windowFeatures.width = parent->innerWidth();
        windowFeatures.widthSet = true;
    }
    if (!windowFeatures.heightSet) {
        windowFeatures.height = parent->innerHeight();
        windowFeatures.heightSet = true;
    }

    FloatRect windowRect(windowFeatures.x, windowFeatures.y, windowFeatures.width, windowFeatures.height);

    // The new window's location is relative to its current screen, so shift
    // it in case it's on a secondary monitor. See http://b/viewIssue?id=967905.
    windowRect.move(screenRect.x(), screenRect.y());
    WebCore::DOMWindow::adjustWindowRect(screenRect, windowRect, windowRect);

    windowFeatures.x = windowRect.x();
    windowFeatures.y = windowRect.y();
    windowFeatures.height = windowRect.height();
    windowFeatures.width = windowRect.width();

    // If either of the origin coordinates weren't set in the original
    // string, make sure they aren't set now.
    if (!rawFeatures.xSet) {
        windowFeatures.x = 0;
        windowFeatures.xSet = false;
    }
    if (!rawFeatures.ySet) {
        windowFeatures.y = 0;
        windowFeatures.ySet = false;
    }

    frame = createWindow(frame, urlString, frameName, windowFeatures, v8::Local<v8::Value>());

    if (!frame)
        return v8::Undefined();

    return V8Proxy::ToV8Object(V8ClassIndex::DOMWINDOW, frame->domWindow());
}


INDEXED_PROPERTY_GETTER(DOMWindow)
{
    INC_STATS("DOM.DOMWindow.IndexedPropertyGetter");
    v8::Handle<v8::Object> holder = V8Proxy::LookupDOMWrapper(V8ClassIndex::DOMWINDOW, info.This());
    if (holder.IsEmpty())
        return notHandledByInterceptor();

    DOMWindow* window = V8Proxy::ToNativeObject<DOMWindow>(V8ClassIndex::DOMWINDOW, holder);
    if (!window)
        return notHandledByInterceptor();

    Frame* frame = window->frame();
    if (!frame)
        return notHandledByInterceptor();

    Frame* child = frame->tree()->child(index);
    if (child)
        return V8Proxy::ToV8Object(V8ClassIndex::DOMWINDOW, child->domWindow());

    return notHandledByInterceptor();
}


NAMED_PROPERTY_GETTER(DOMWindow)
{
    INC_STATS("DOM.DOMWindow.NamedPropertyGetter");
    // The key must be a string.
    if (!name->IsString())
        return notHandledByInterceptor();

    v8::Handle<v8::Object> holder = V8Proxy::LookupDOMWrapper(V8ClassIndex::DOMWINDOW, info.This());
    if (holder.IsEmpty())
        return notHandledByInterceptor();

    DOMWindow* window = V8Proxy::ToNativeObject<DOMWindow>(V8ClassIndex::DOMWINDOW, holder);
    if (!window)
        return notHandledByInterceptor();

    String propName = toWebCoreString(name);

    Frame* frame = window->frame();
    // window is detached from a frame.
    if (!frame)
        return notHandledByInterceptor();

    // Search sub-frames.
    Frame* child = frame->tree()->child(propName);
    if (child)
        return V8Proxy::ToV8Object(V8ClassIndex::DOMWINDOW, child->domWindow());

    // Search IDL functions defined in the prototype
    v8::Handle<v8::Value> result = holder->GetRealNamedPropertyInPrototypeChain(name);
    if (!result.IsEmpty())
        return result;

    // Lazy initialization map keeps global properties that can be lazily
    // initialized. The value is the code to instantiate the property.
    // It must return the value of property after initialization.
    static HashMap<String, String> lazyInitMap;
    if (lazyInitMap.isEmpty()) {
      // "new Image()" does not appear to be well-defined in a spec, but Safari,
      // Opera, and Firefox all consider it to always create an HTML image
      // element, regardless of the current doctype.
      lazyInitMap.set("Image",
                       "function Image() { \
                          return document.createElementNS( \
                            'http://www.w3.org/1999/xhtml', 'img'); \
                        }; \
                        Image");
      lazyInitMap.set("Option",
        "function Option(text, value, defaultSelected, selected) { \
           var option = document.createElement('option'); \
           if (text == null) return option; \
           option.text = text; \
           if (value == null) return option; \
           option.value = value; \
           if (defaultSelected == null) return option; \
           option.defaultSelected = defaultSelected; \
           if (selected == null) return option; \
           option.selected = selected; \
           return option; \
         }; \
         Option");
    }

    String code = lazyInitMap.get(propName);
    if (!code.isEmpty()) {
        v8::Local<v8::Context> context = V8Proxy::GetContext(window->frame());
        // Bail out if we cannot get the context for the frame.
        if (context.IsEmpty())
            return notHandledByInterceptor();
  
        // switch to the target object's environment.
        v8::Context::Scope scope(context);

        // Set the property name to undefined to make sure that the
        // property exists.  This is necessary because this getter
        // might be called when evaluating 'var RangeException = value'
        // to figure out if we have a property named 'RangeException' before
        // we set RangeException to the new value.  In that case, we will
        // evaluate 'var RangeException = {}' and enter an infinite loop.
        // Setting the property name to undefined on the global object
        // ensures that we do not have to ask this getter to figure out
        // that we have the property.
        //
        // TODO(ager): We probably should implement the Has method
        // for the interceptor instead of using the default Has method
        // that calls Get.
        context->Global()->Set(v8String(propName), v8::Undefined());
        V8Proxy* proxy = V8Proxy::retrieve(window->frame());
        ASSERT(proxy);

        return proxy->evaluate(WebCore::ScriptSourceCode(code), 0);
    }

    // Search named items in the document.
    Document* doc = frame->document();
    if (doc) {
        RefPtr<HTMLCollection> items = doc->windowNamedItems(propName);
        if (items->length() >= 1) {
            if (items->length() == 1)
                return V8Proxy::NodeToV8Object(items->firstItem());
            else
                return V8Proxy::ToV8Object(V8ClassIndex::HTMLCOLLECTION, items.get());
        }
    }

    return notHandledByInterceptor();
}


void V8Custom::WindowSetLocation(DOMWindow* window, const String& v)
{
    if (!window->frame())
        return;

    Frame* activeFrame = ScriptController::retrieveActiveFrame();
    if (!activeFrame)
        return;

    if (!activeFrame->loader()->shouldAllowNavigation(window->frame()))
        return;

    if (!parseURL(v).startsWith("javascript:", false)
        || ScriptController::isSafeScript(window->frame())) {
        String completedUrl = activeFrame->loader()->completeURL(v).string();
  
        // FIXME: The JSC bindings pass !anyPageIsProcessingUserGesture() for
        // the lockHistory parameter.  We should probably do something similar.
  
        window->frame()->loader()->scheduleLocationChange(completedUrl,
            activeFrame->loader()->outgoingReferrer(), false, false,
            activeFrame->script()->processingUserGesture());
    }
}


CALLBACK_FUNC_DECL(DOMWindowSetTimeout)
{
    INC_STATS("DOM.DOMWindow.setTimeout()");
    return WindowSetTimeoutImpl(args, true);
}


CALLBACK_FUNC_DECL(DOMWindowSetInterval)
{
    INC_STATS("DOM.DOMWindow.setInterval()");
    return WindowSetTimeoutImpl(args, false);
}


void V8Custom::ClearTimeoutImpl(const v8::Arguments& args)
{
    v8::Handle<v8::Value> holder = args.Holder();
    DOMWindow* imp = V8Proxy::ToNativeObject<DOMWindow>(V8ClassIndex::DOMWINDOW, holder);
    if (!V8Proxy::CanAccessFrame(imp->frame(), true))
        return;
    ScriptExecutionContext* context = static_cast<ScriptExecutionContext*>(imp->frame()->document());
    int handle = toInt32(args[0]);
    DOMTimer::removeById(context, handle);
}


CALLBACK_FUNC_DECL(DOMWindowClearTimeout)
{
    INC_STATS("DOM.DOMWindow.clearTimeout");
    ClearTimeoutImpl(args);
    return v8::Undefined();
}

CALLBACK_FUNC_DECL(DOMWindowClearInterval)
{
    INC_STATS("DOM.DOMWindow.clearInterval");
    ClearTimeoutImpl(args);
    return v8::Undefined();
}

} // namespace WebCore
