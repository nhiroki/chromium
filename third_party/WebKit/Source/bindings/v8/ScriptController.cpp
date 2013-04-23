/*
 * Copyright (C) 2008, 2009 Google Inc. All rights reserved.
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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
#include "bindings/v8/ScriptController.h"

#include "ScriptRunner.h"
#include "V8DOMWindow.h"
#include "V8Event.h"
#include "V8HTMLEmbedElement.h"
#include "bindings/v8/BindingState.h"
#include "bindings/v8/NPObjectWrapper.h"
#include "bindings/v8/NPV8Object.h"
#include "bindings/v8/ScriptCallStackFactory.h"
#include "bindings/v8/ScriptSourceCode.h"
#include "bindings/v8/ScriptValue.h"
#include "bindings/v8/V8Binding.h"
#include "bindings/v8/V8GCController.h"
#include "bindings/v8/V8HiddenPropertyName.h"
#include "bindings/v8/V8NPObject.h"
#include "bindings/v8/V8PerContextData.h"
#include "bindings/v8/V8RecursionScope.h"
#include "bindings/v8/npruntime_impl.h"
#include "bindings/v8/npruntime_priv.h"
#include "core/dom/Document.h"
#include "core/dom/Event.h"
#include "core/dom/EventListener.h"
#include "core/dom/EventNames.h"
#include "core/dom/Node.h"
#include "core/dom/ScriptableDocumentParser.h"
#include "core/dom/UserGestureIndicator.h"
#include "core/inspector/InspectorInstrumentation.h"
#include "core/inspector/ScriptCallStack.h"
#include "core/loader/DocumentLoader.h"
#include "core/loader/FrameLoader.h"
#include "core/loader/FrameLoaderClient.h"
#include "core/page/ContentSecurityPolicy.h"
#include "core/page/DOMWindow.h"
#include "core/page/Frame.h"
#include "core/page/Page.h"
#include "core/page/SecurityOrigin.h"
#include "core/page/Settings.h"
#include "core/platform/HistogramSupport.h"
#include "core/platform/NotImplemented.h"
#include "core/platform/Widget.h"
#include "core/platform/chromium/TraceEvent.h"
#include "core/plugins/PluginViewBase.h"
#include "wtf/CurrentTime.h"
#include "wtf/StdLibExtras.h"
#include "wtf/StringExtras.h"
#include "wtf/text/CString.h"
#include "wtf/text/StringBuilder.h"
#include "wtf/text/TextPosition.h"


namespace WebCore {

void ScriptController::initializeThreading()
{
    static bool initializedThreading = false;
    if (!initializedThreading) {
        WTF::initializeThreading();
        WTF::initializeMainThread();
        initializedThreading = true;
    }
}

bool ScriptController::canAccessFromCurrentOrigin(Frame *frame)
{
    return !v8::Context::InContext() || BindingSecurity::shouldAllowAccessToFrame(BindingState::instance(), frame);
}

ScriptController::ScriptController(Frame* frame)
    : m_frame(frame)
    , m_sourceURL(0)
    , m_isolate(v8::Isolate::GetCurrent())
    , m_windowShell(V8DOMWindowShell::create(frame, mainThreadNormalWorld(), m_isolate))
    , m_paused(false)
    , m_wrappedWindowScriptNPObject(0)
{
}

ScriptController::~ScriptController()
{
    clearForClose(true);
}

void ScriptController::clearScriptObjects()
{
    PluginObjectMap::iterator it = m_pluginObjects.begin();
    for (; it != m_pluginObjects.end(); ++it) {
        _NPN_UnregisterObject(it->value);
        _NPN_ReleaseObject(it->value);
    }
    m_pluginObjects.clear();

    if (m_wrappedWindowScriptNPObject) {
        NPObjectWrapper* windowScriptObjectWrapper = NPObjectWrapper::getWrapper(m_wrappedWindowScriptNPObject);
        ASSERT(windowScriptObjectWrapper);

        NPObject* windowScriptNPObject = NPObjectWrapper::getUnderlyingNPObject(m_wrappedWindowScriptNPObject);
        ASSERT(windowScriptNPObject);
        // Call _NPN_DeallocateObject() instead of _NPN_ReleaseObject() so that we don't leak if a plugin fails to release the window
        // script object properly.
        // This shouldn't cause any problems for plugins since they should have already been stopped and destroyed at this point.
        _NPN_DeallocateObject(windowScriptNPObject);

        // Clear out the wrapped window script object pointer held by the wrapper.
        windowScriptObjectWrapper->clear();
        _NPN_ReleaseObject(m_wrappedWindowScriptNPObject);
        m_wrappedWindowScriptNPObject = 0;
    }
}

void ScriptController::clearForOutOfMemory()
{
    clearForClose(true);
}

void ScriptController::clearForClose(bool destroyGlobal)
{
    m_windowShell->clearForClose(destroyGlobal);
    for (IsolatedWorldMap::iterator iter = m_isolatedWorlds.begin(); iter != m_isolatedWorlds.end(); ++iter)
        iter->value->clearForClose(destroyGlobal);
    V8GCController::hintForCollectGarbage();
}

void ScriptController::clearForClose()
{
    double start = currentTime();
    clearForClose(false);
    HistogramSupport::histogramCustomCounts("WebCore.ScriptController.clearForClose", (currentTime() - start) * 1000, 0, 10000, 50);
}

void ScriptController::updateSecurityOrigin()
{
    m_windowShell->updateSecurityOrigin();
}

void ScriptController::updatePlatformScriptObjects()
{
    notImplemented();
}

bool ScriptController::processingUserGesture()
{
    return UserGestureIndicator::processingUserGesture();
}

v8::Local<v8::Value> ScriptController::callFunction(v8::Handle<v8::Function> function, v8::Handle<v8::Object> receiver, int argc, v8::Handle<v8::Value> args[])
{
    // Keep Frame (and therefore ScriptController) alive.
    RefPtr<Frame> protect(m_frame);
    return ScriptController::callFunctionWithInstrumentation(m_frame ? m_frame->document() : 0, function, receiver, argc, args);
}

static inline void resourceInfo(const v8::Handle<v8::Function> function, String& resourceName, int& lineNumber)
{
    v8::ScriptOrigin origin = function->GetScriptOrigin();
    if (origin.ResourceName().IsEmpty()) {
        resourceName = "undefined";
        lineNumber = 1;
    } else {
        resourceName = toWebCoreString(origin.ResourceName());
        lineNumber = function->GetScriptLineNumber() + 1;
    }
}

static inline String resourceString(const v8::Handle<v8::Function> function)
{
    String resourceName;
    int lineNumber;
    resourceInfo(function, resourceName, lineNumber);

    StringBuilder builder;
    builder.append(resourceName);
    builder.append(':');
    builder.appendNumber(lineNumber);
    return builder.toString();
}

v8::Local<v8::Value> ScriptController::callFunctionWithInstrumentation(ScriptExecutionContext* context, v8::Handle<v8::Function> function, v8::Handle<v8::Object> receiver, int argc, v8::Handle<v8::Value> args[])
{
    V8GCController::checkMemoryUsage();

    if (V8RecursionScope::recursionLevel() >= kMaxRecursionDepth)
        return handleMaxRecursionDepthExceeded();

    InspectorInstrumentationCookie cookie;
    if (InspectorInstrumentation::timelineAgentEnabled(context)) {
        String resourceName;
        int lineNumber;
        resourceInfo(function, resourceName, lineNumber);
        cookie = InspectorInstrumentation::willCallFunction(context, resourceName, lineNumber);
    }

    v8::Local<v8::Value> result;
    {
        TRACE_EVENT1("v8", "v8.callFunction", "callsite", resourceString(function).utf8());
        V8RecursionScope recursionScope(context);
        result = function->Call(receiver, argc, args);
    }

    InspectorInstrumentation::didCallFunction(cookie);
    crashIfV8IsDead();
    return result;
}

ScriptValue ScriptController::callFunctionEvenIfScriptDisabled(v8::Handle<v8::Function> function, v8::Handle<v8::Object> receiver, int argc, v8::Handle<v8::Value> argv[])
{
    // FIXME: This should probably perform the same isPaused check that happens in ScriptController::executeScript.
    return ScriptValue(callFunction(function, receiver, argc, argv));
}

v8::Local<v8::Value> ScriptController::compileAndRunScript(const ScriptSourceCode& source)
{
    ASSERT(v8::Context::InContext());

    V8GCController::checkMemoryUsage();

    InspectorInstrumentationCookie cookie = InspectorInstrumentation::willEvaluateScript(m_frame, source.url().isNull() ? String() : source.url().string(), source.startLine());

    v8::Local<v8::Value> result;
    {
        // Isolate exceptions that occur when compiling and executing
        // the code. These exceptions should not interfere with
        // javascript code we might evaluate from C++ when returning
        // from here.
        v8::TryCatch tryCatch;
        tryCatch.SetVerbose(true);

        // Compile the script.
        v8::Handle<v8::String> code = v8String(source.source(), m_isolate);
        TRACE_EVENT_BEGIN0("v8", "v8.compile");
        OwnPtr<v8::ScriptData> scriptData = ScriptSourceCode::precompileScript(code, source.cachedScript());

        // NOTE: For compatibility with WebCore, ScriptSourceCode's line starts at
        // 1, whereas v8 starts at 0.
        v8::Handle<v8::Script> script = ScriptSourceCode::compileScript(code, source.url(), source.startPosition(), scriptData.get(), m_isolate);
        TRACE_EVENT_END0("v8", "v8.compile");
        TRACE_EVENT0("v8", "v8.run");

        // Keep Frame (and therefore ScriptController) alive.
        RefPtr<Frame> protect(m_frame);
        result = ScriptRunner::runCompiledScript(script, m_frame->document());
        ASSERT(!tryCatch.HasCaught() || result.IsEmpty());
    }

    InspectorInstrumentation::didEvaluateScript(cookie);

    return result;
}

ScriptValue ScriptController::evaluate(const ScriptSourceCode& sourceCode)
{
    String sourceURL = sourceCode.url();
    const String* savedSourceURL = m_sourceURL;
    m_sourceURL = &sourceURL;

    v8::HandleScope handleScope;
    v8::Handle<v8::Context> v8Context = ScriptController::mainWorldContext(m_frame);
    if (v8Context.IsEmpty())
        return ScriptValue();

    v8::Context::Scope scope(v8Context);

    RefPtr<Frame> protect(m_frame);

    v8::Local<v8::Value> object = compileAndRunScript(sourceCode);

    m_sourceURL = savedSourceURL;

    if (object.IsEmpty())
        return ScriptValue();

    return ScriptValue(object);
}

bool ScriptController::initializeMainWorld()
{
    if (m_windowShell->isContextInitialized())
        return false;
    return windowShell(mainThreadNormalWorld())->isContextInitialized();
}

V8DOMWindowShell* ScriptController::existingWindowShell(DOMWrapperWorld* world)
{
    ASSERT(world);

    if (world->isMainWorld())
        return m_windowShell->isContextInitialized() ? m_windowShell.get() : 0;

    // FIXME: Remove this block. See comment with existingWindowShellWorkaroundWorld().
    if (world == existingWindowShellWorkaroundWorld())
        return m_windowShell.get();

    IsolatedWorldMap::iterator iter = m_isolatedWorlds.find(world->worldId());
    if (iter == m_isolatedWorlds.end())
        return 0;
    return iter->value->isContextInitialized() ? iter->value.get() : 0;
}

V8DOMWindowShell* ScriptController::windowShell(DOMWrapperWorld* world)
{
    ASSERT(world);

    V8DOMWindowShell* shell = 0;
    if (world->isMainWorld())
        shell = m_windowShell.get();
    else {
        IsolatedWorldMap::iterator iter = m_isolatedWorlds.find(world->worldId());
        if (iter != m_isolatedWorlds.end())
            shell = iter->value.get();
        else {
            OwnPtr<V8DOMWindowShell> isolatedWorldShell = V8DOMWindowShell::create(m_frame, world, m_isolate);
            shell = isolatedWorldShell.get();
            m_isolatedWorlds.set(world->worldId(), isolatedWorldShell.release());
        }
    }
    if (!shell->isContextInitialized() && shell->initializeIfNeeded()) {
        if (world->isMainWorld()) {
            // FIXME: Remove this if clause. See comment with existingWindowShellWorkaroundWorld().
            m_frame->loader()->dispatchDidClearWindowObjectInWorld(existingWindowShellWorkaroundWorld());
        } else
            m_frame->loader()->dispatchDidClearWindowObjectInWorld(world);
    }
    return shell;
}

void ScriptController::evaluateInIsolatedWorld(int worldID, const Vector<ScriptSourceCode>& sources, int extensionGroup, Vector<ScriptValue>* results)
{
    ASSERT(worldID > 0);

    v8::HandleScope handleScope;
    v8::Local<v8::Array> v8Results;
    {
        v8::HandleScope evaluateHandleScope;
        RefPtr<DOMWrapperWorld> world = DOMWrapperWorld::ensureIsolatedWorld(worldID, extensionGroup);
        V8DOMWindowShell* isolatedWorldShell = windowShell(world.get());

        if (!isolatedWorldShell->isContextInitialized())
            return;

        v8::Local<v8::Context> context = isolatedWorldShell->context();
        v8::Context::Scope contextScope(context);
        v8::Local<v8::Array> resultArray = v8::Array::New(sources.size());

        for (size_t i = 0; i < sources.size(); ++i) {
            v8::Local<v8::Value> evaluationResult = compileAndRunScript(sources[i]);
            if (evaluationResult.IsEmpty())
                evaluationResult = v8::Local<v8::Value>::New(v8::Undefined());
            resultArray->Set(i, evaluationResult);
        }

        v8Results = evaluateHandleScope.Close(resultArray);
    }

    if (results && !v8Results.IsEmpty()) {
        for (size_t i = 0; i < v8Results->Length(); ++i)
            results->append(ScriptValue(v8Results->Get(i)));
    }
}

bool ScriptController::shouldBypassMainWorldContentSecurityPolicy()
{
    if (DOMWrapperWorld* world = isolatedWorldForEnteredContext())
        return world->isolatedWorldHasContentSecurityPolicy();
    return false;
}

TextPosition ScriptController::eventHandlerPosition() const
{
    ScriptableDocumentParser* parser = m_frame->document()->scriptableDocumentParser();
    if (parser)
        return parser->textPosition();
    return TextPosition::minimumPosition();
}

static inline v8::Local<v8::Context> contextForWorld(ScriptController* scriptController, DOMWrapperWorld* world)
{
    return scriptController->windowShell(world)->context();
}

v8::Local<v8::Context> ScriptController::currentWorldContext()
{
    if (!v8::Context::InContext())
        return contextForWorld(this, mainThreadNormalWorld());

    v8::Handle<v8::Context> context = v8::Context::GetEntered();
    DOMWrapperWorld* isolatedWorld = DOMWrapperWorld::isolatedWorld(context);
    if (!isolatedWorld)
        return contextForWorld(this, mainThreadNormalWorld());

    Frame* frame = toFrameIfNotDetached(context);
    if (!m_frame)
        return v8::Local<v8::Context>();

    if (m_frame == frame)
        return v8::Local<v8::Context>::New(context);

    return contextForWorld(this, isolatedWorld);
}

v8::Local<v8::Context> ScriptController::mainWorldContext()
{
    return contextForWorld(this, mainThreadNormalWorld());
}

v8::Local<v8::Context> ScriptController::mainWorldContext(Frame* frame)
{
    if (!frame)
        return v8::Local<v8::Context>();

    return contextForWorld(frame->script(), mainThreadNormalWorld());
}

// Create a V8 object with an interceptor of NPObjectPropertyGetter.
void ScriptController::bindToWindowObject(Frame* frame, const String& key, NPObject* object)
{
    v8::HandleScope handleScope;

    v8::Handle<v8::Context> v8Context = ScriptController::mainWorldContext(frame);
    if (v8Context.IsEmpty())
        return;

    v8::Context::Scope scope(v8Context);

    v8::Handle<v8::Object> value = createV8ObjectForNPObject(object, 0);

    // Attach to the global object.
    v8::Handle<v8::Object> global = v8Context->Global();
    global->Set(v8String(key, v8Context->GetIsolate()), value);
}

void ScriptController::enableEval()
{
    if (!m_windowShell->isContextInitialized())
        return;
    v8::HandleScope handleScope;
    m_windowShell->context()->AllowCodeGenerationFromStrings(true);
}

void ScriptController::disableEval(const String& errorMessage)
{
    if (!m_windowShell->isContextInitialized())
        return;
    v8::HandleScope handleScope;
    v8::Local<v8::Context> v8Context = m_windowShell->context();
    v8Context->AllowCodeGenerationFromStrings(false);
    v8Context->SetErrorMessageForCodeGenerationFromStrings(v8String(errorMessage, v8Context->GetIsolate()));
}

PassScriptInstance ScriptController::createScriptInstanceForWidget(Widget* widget)
{
    ASSERT(widget);

    if (!widget->isPluginViewBase())
        return 0;

    NPObject* npObject = toPluginViewBase(widget)->scriptableObject();

    if (!npObject)
        return 0;

    // Frame Memory Management for NPObjects
    // -------------------------------------
    // NPObjects are treated differently than other objects wrapped by JS.
    // NPObjects can be created either by the browser (e.g. the main
    // window object) or by the plugin (the main plugin object
    // for a HTMLEmbedElement). Further, unlike most DOM Objects, the frame
    // is especially careful to ensure NPObjects terminate at frame teardown because
    // if a plugin leaks a reference, it could leak its objects (or the browser's objects).
    //
    // The Frame maintains a list of plugin objects (m_pluginObjects)
    // which it can use to quickly find the wrapped embed object.
    //
    // Inside the NPRuntime, we've added a few methods for registering
    // wrapped NPObjects. The purpose of the registration is because
    // javascript garbage collection is non-deterministic, yet we need to
    // be able to tear down the plugin objects immediately. When an object
    // is registered, javascript can use it. When the object is destroyed,
    // or when the object's "owning" object is destroyed, the object will
    // be un-registered, and the javascript engine must not use it.
    //
    // Inside the javascript engine, the engine can keep a reference to the
    // NPObject as part of its wrapper. However, before accessing the object
    // it must consult the _NPN_Registry.

    v8::Local<v8::Object> wrapper = createV8ObjectForNPObject(npObject, 0);

    // Track the plugin object. We've been given a reference to the object.
    m_pluginObjects.set(widget, npObject);

    return V8ScriptInstance::create(wrapper);
}

void ScriptController::cleanupScriptObjectsForPlugin(Widget* nativeHandle)
{
    PluginObjectMap::iterator it = m_pluginObjects.find(nativeHandle);
    if (it == m_pluginObjects.end())
        return;
    _NPN_UnregisterObject(it->value);
    _NPN_ReleaseObject(it->value);
    m_pluginObjects.remove(it);
}

V8Extensions& ScriptController::registeredExtensions()
{
    DEFINE_STATIC_LOCAL(V8Extensions, extensions, ());
    return extensions;
}

void ScriptController::registerExtensionIfNeeded(v8::Extension* extension)
{
    const V8Extensions& extensions = registeredExtensions();
    for (size_t i = 0; i < extensions.size(); ++i) {
        if (extensions[i] == extension)
            return;
    }
    v8::RegisterExtension(extension);
    registeredExtensions().append(extension);
}

static NPObject* createNoScriptObject()
{
    notImplemented();
    return 0;
}

static NPObject* createScriptObject(Frame* frame)
{
    v8::HandleScope handleScope;
    v8::Handle<v8::Context> v8Context = ScriptController::mainWorldContext(frame);
    if (v8Context.IsEmpty())
        return createNoScriptObject();

    v8::Context::Scope scope(v8Context);
    DOMWindow* window = frame->document()->domWindow();
    v8::Handle<v8::Value> global = toV8(window, v8::Handle<v8::Object>(), v8Context->GetIsolate());
    ASSERT(global->IsObject());
    return npCreateV8ScriptObject(0, v8::Handle<v8::Object>::Cast(global), window);
}

NPObject* ScriptController::windowScriptNPObject()
{
    if (m_wrappedWindowScriptNPObject)
        return m_wrappedWindowScriptNPObject;

    NPObject* windowScriptNPObject = 0;
    if (canExecuteScripts(NotAboutToExecuteScript)) {
        // JavaScript is enabled, so there is a JavaScript window object.
        // Return an NPObject bound to the window object.
        windowScriptNPObject = createScriptObject(m_frame);
        _NPN_RegisterObject(windowScriptNPObject, 0);
    } else {
        // JavaScript is not enabled, so we cannot bind the NPObject to the
        // JavaScript window object. Instead, we create an NPObject of a
        // different class, one which is not bound to a JavaScript object.
        windowScriptNPObject = createNoScriptObject();
    }

    m_wrappedWindowScriptNPObject = NPObjectWrapper::create(windowScriptNPObject);
    return m_wrappedWindowScriptNPObject;
}

NPObject* ScriptController::createScriptObjectForPluginElement(HTMLPlugInElement* plugin)
{
    // Can't create NPObjects when JavaScript is disabled.
    if (!canExecuteScripts(NotAboutToExecuteScript))
        return createNoScriptObject();

    v8::HandleScope handleScope;
    v8::Handle<v8::Context> v8Context = ScriptController::mainWorldContext(m_frame);
    if (v8Context.IsEmpty())
        return createNoScriptObject();
    v8::Context::Scope scope(v8Context);

    DOMWindow* window = m_frame->document()->domWindow();
    v8::Handle<v8::Value> v8plugin = toV8(static_cast<HTMLEmbedElement*>(plugin), v8::Handle<v8::Object>(), v8Context->GetIsolate());
    if (!v8plugin->IsObject())
        return createNoScriptObject();

    return npCreateV8ScriptObject(0, v8::Handle<v8::Object>::Cast(v8plugin), window);
}


void ScriptController::clearWindowShell()
{
    double start = currentTime();
    // V8 binding expects ScriptController::clearWindowShell only be called
    // when a frame is loading a new page. This creates a new context for the new page.
    m_windowShell->clearForNavigation();
    for (IsolatedWorldMap::iterator iter = m_isolatedWorlds.begin(); iter != m_isolatedWorlds.end(); ++iter)
        iter->value->clearForNavigation();
    V8GCController::hintForCollectGarbage();
    HistogramSupport::histogramCustomCounts("WebCore.ScriptController.clearWindowShell", (currentTime() - start) * 1000, 0, 10000, 50);
}

void ScriptController::setCaptureCallStackForUncaughtExceptions(bool value)
{
    v8::V8::SetCaptureStackTraceForUncaughtExceptions(value, ScriptCallStack::maxCallStackSizeToCapture, stackTraceOptions);
}

void ScriptController::collectIsolatedContexts(Vector<std::pair<ScriptState*, SecurityOrigin*> >& result)
{
    v8::HandleScope handleScope;
    for (IsolatedWorldMap::iterator it = m_isolatedWorlds.begin(); it != m_isolatedWorlds.end(); ++it) {
        V8DOMWindowShell* isolatedWorldShell = it->value.get();
        SecurityOrigin* origin = isolatedWorldShell->world()->isolatedWorldSecurityOrigin();
        if (!origin)
            continue;
        v8::Local<v8::Context> v8Context = isolatedWorldShell->context();
        if (v8Context.IsEmpty())
            continue;
        ScriptState* scriptState = ScriptState::forContext(v8Context);
        result.append(std::pair<ScriptState*, SecurityOrigin*>(scriptState, origin));
    }
}

bool ScriptController::setContextDebugId(int debugId)
{
    ASSERT(debugId > 0);
    if (!m_windowShell->isContextInitialized())
        return false;
    v8::HandleScope scope;
    v8::Local<v8::Context> context = m_windowShell->context();
    return V8PerContextDebugData::setContextDebugData(context, "page", debugId);
}

int ScriptController::contextDebugId(v8::Handle<v8::Context> context)
{
    return V8PerContextDebugData::contextDebugId(context);
}

void ScriptController::updateDocument()
{
    // For an uninitialized main window shell, do not incur the cost of context initialization during FrameLoader::init().
    if ((!m_windowShell->isContextInitialized() || !m_windowShell->isGlobalInitialized()) && m_frame->loader()->stateMachine()->creatingInitialEmptyDocument())
        return;

    if (!initializeMainWorld())
        windowShell(mainThreadNormalWorld())->updateDocument();
}

void ScriptController::namedItemAdded(HTMLDocument* doc, const AtomicString& name)
{
    windowShell(mainThreadNormalWorld())->namedItemAdded(doc, name);
}

void ScriptController::namedItemRemoved(HTMLDocument* doc, const AtomicString& name)
{
    windowShell(mainThreadNormalWorld())->namedItemRemoved(doc, name);
}

bool ScriptController::canExecuteScripts(ReasonForCallingCanExecuteScripts reason)
{
    if (m_frame->document() && m_frame->document()->isSandboxed(SandboxScripts)) {
        // FIXME: This message should be moved off the console once a solution to https://bugs.webkit.org/show_bug.cgi?id=103274 exists.
        if (reason == AboutToExecuteScript)
            m_frame->document()->addConsoleMessage(SecurityMessageSource, ErrorMessageLevel, "Blocked script execution in '" + m_frame->document()->url().elidedString() + "' because the document's frame is sandboxed and the 'allow-scripts' permission is not set.");
        return false;
    }

    if (m_frame->document() && m_frame->document()->isViewSource()) {
        ASSERT(m_frame->document()->securityOrigin()->isUnique());
        return true;
    }

    Settings* settings = m_frame->settings();
    const bool allowed = m_frame->loader()->client()->allowScript(settings && settings->isScriptEnabled());
    if (!allowed && reason == AboutToExecuteScript)
        m_frame->loader()->client()->didNotAllowScript();
    return allowed;
}

ScriptValue ScriptController::executeScript(const String& script, bool forceUserGesture)
{
    UserGestureIndicator gestureIndicator(forceUserGesture ? DefinitelyProcessingNewUserGesture : PossiblyProcessingUserGesture);
    return executeScript(ScriptSourceCode(script, m_frame->document()->url()));
}

ScriptValue ScriptController::executeScript(const ScriptSourceCode& sourceCode)
{
    if (!canExecuteScripts(AboutToExecuteScript) || isPaused())
        return ScriptValue();

    RefPtr<Frame> protect(m_frame); // Script execution can destroy the frame, and thus the ScriptController.

    return evaluate(sourceCode);
}

bool ScriptController::executeIfJavaScriptURL(const KURL& url, ShouldReplaceDocumentIfJavaScriptURL shouldReplaceDocumentIfJavaScriptURL)
{
    if (!protocolIsJavaScript(url))
        return false;

    if (!m_frame->page()
        || !m_frame->document()->contentSecurityPolicy()->allowJavaScriptURLs(m_frame->document()->url(), eventHandlerPosition().m_line))
        return true;

    // We need to hold onto the Frame here because executing script can
    // destroy the frame.
    RefPtr<Frame> protector(m_frame);
    RefPtr<Document> ownerDocument(m_frame->document());

    const int javascriptSchemeLength = sizeof("javascript:") - 1;

    String decodedURL = decodeURLEscapeSequences(url.string());
    ScriptValue result = executeScript(decodedURL.substring(javascriptSchemeLength));

    // If executing script caused this frame to be removed from the page, we
    // don't want to try to replace its document!
    if (!m_frame->page())
        return true;

    String scriptResult;
    if (!result.getString(scriptResult))
        return true;

    // FIXME: We should always replace the document, but doing so
    //        synchronously can cause crashes:
    //        http://bugs.webkit.org/show_bug.cgi?id=16782
    if (shouldReplaceDocumentIfJavaScriptURL == ReplaceDocumentIfJavaScriptURL) {
        // We're still in a frame, so there should be a DocumentLoader.
        ASSERT(m_frame->document()->loader());
        
        // DocumentWriter::replaceDocument can cause the DocumentLoader to get deref'ed and possible destroyed,
        // so protect it with a RefPtr.
        if (RefPtr<DocumentLoader> loader = m_frame->document()->loader())
            loader->writer()->replaceDocument(scriptResult, ownerDocument.get());
    }
    return true;
}

} // namespace WebCore
