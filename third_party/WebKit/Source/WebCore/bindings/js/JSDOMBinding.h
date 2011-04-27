/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Samuel Weinig <sam@webkit.org>
 *  Copyright (C) 2009 Google, Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef JSDOMBinding_h
#define JSDOMBinding_h

#include "JSDOMGlobalObject.h"
#include "JSDOMWrapper.h"
#include "DOMWrapperWorld.h"
#include "Document.h"
#include "Element.h"
#include "StyleBase.h"
#include <heap/Weak.h>
#include <runtime/Completion.h>
#include <runtime/Lookup.h>
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>

namespace JSC {
    class JSGlobalData;
    class DebuggerCallFrame;
}

namespace WebCore {

    class CSSValue;
    class Document;
    class Frame;
    class JSNode;
    class KURL;
    class Node;
    class ScriptController;
    class ScriptCachedFrameData;

    typedef int ExceptionCode;

    // Base class for all constructor objects in the JSC bindings.
    class DOMConstructorObject : public JSDOMWrapper {
    public:
        static JSC::Structure* createStructure(JSC::JSGlobalData& globalData, JSC::JSValue prototype)
        {
            return JSC::Structure::create(globalData, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), AnonymousSlotCount, &s_info);
        }

    protected:
        static const unsigned StructureFlags = JSC::ImplementsHasInstance | JSC::OverridesVisitChildren | JSDOMWrapper::StructureFlags;
        DOMConstructorObject(JSC::Structure* structure, JSDOMGlobalObject* globalObject)
            : JSDOMWrapper(structure, globalObject)
        {
        }
    };

    // Constructors using this base class depend on being in a Document and
    // can never be used from a WorkerContext.
    class DOMConstructorWithDocument : public DOMConstructorObject {
    public:
        Document* document() const
        {
            return static_cast<Document*>(scriptExecutionContext());
        }

    protected:
        DOMConstructorWithDocument(JSC::Structure* structure, JSDOMGlobalObject* globalObject)
            : DOMConstructorObject(structure, globalObject)
        {
            ASSERT(globalObject->scriptExecutionContext()->isDocument());
        }
    };
    
    JSC::Structure* getCachedDOMStructure(JSDOMGlobalObject*, const JSC::ClassInfo*);
    JSC::Structure* cacheDOMStructure(JSDOMGlobalObject*, JSC::Structure*, const JSC::ClassInfo*);

    inline JSDOMGlobalObject* deprecatedGlobalObjectForPrototype(JSC::ExecState* exec)
    {
        // FIXME: Callers to this function should be using the global object
        // from which the object is being created, instead of assuming the lexical one.
        // e.g. subframe.document.body should use the subframe's global object, not the lexical one.
        return static_cast<JSDOMGlobalObject*>(exec->lexicalGlobalObject());
    }

    template<class WrapperClass> inline JSC::Structure* getDOMStructure(JSC::ExecState* exec, JSDOMGlobalObject* globalObject)
    {
        if (JSC::Structure* structure = getCachedDOMStructure(globalObject, &WrapperClass::s_info))
            return structure;
        return cacheDOMStructure(globalObject, WrapperClass::createStructure(exec->globalData(), WrapperClass::createPrototype(exec, globalObject)), &WrapperClass::s_info);
    }

    template<class WrapperClass> inline JSC::Structure* deprecatedGetDOMStructure(JSC::ExecState* exec)
    {
        // FIXME: This function is wrong.  It uses the wrong global object for creating the prototype structure.
        return getDOMStructure<WrapperClass>(exec, deprecatedGlobalObjectForPrototype(exec));
    }

    template<class WrapperClass> inline JSC::JSObject* getDOMPrototype(JSC::ExecState* exec, JSC::JSGlobalObject* globalObject)
    {
        return static_cast<JSC::JSObject*>(asObject(getDOMStructure<WrapperClass>(exec, static_cast<JSDOMGlobalObject*>(globalObject))->storedPrototype()));
    }

    // Overload these functions to provide a fast path for wrapper access.
    inline JSDOMWrapper* getInlineCachedWrapper(DOMWrapperWorld*, void*) { return 0; }
    inline bool setInlineCachedWrapper(DOMWrapperWorld*, void*, JSDOMWrapper*) { return false; }
    inline bool clearInlineCachedWrapper(DOMWrapperWorld*, void*, JSDOMWrapper*) { return false; }

    // Overload these functions to provide a custom WeakHandleOwner.
    inline JSC::WeakHandleOwner* wrapperOwner(DOMWrapperWorld* world, void*) { return world->defaultWrapperOwner(); }
    inline void* wrapperContext(DOMWrapperWorld*, void* domObject) { return domObject; }

    template <typename DOMClass> inline JSDOMWrapper* getCachedWrapper(DOMWrapperWorld* world, DOMClass* domObject)
    {
        if (JSDOMWrapper* wrapper = getInlineCachedWrapper(world, domObject))
            return wrapper;
        return world->m_wrappers.get(domObject).get();
    }

    template <typename DOMClass> inline void cacheWrapper(DOMWrapperWorld* world, DOMClass* domObject, JSDOMWrapper* wrapper)
    {
        if (setInlineCachedWrapper(world, domObject, wrapper))
            return;
        ASSERT(!world->m_wrappers.contains(domObject));
        world->m_wrappers.set(domObject, JSC::Weak<JSDOMWrapper>(*world->globalData(), wrapper, wrapperOwner(world, domObject), wrapperContext(world, domObject)));
    }

    template <typename DOMClass> inline void uncacheWrapper(DOMWrapperWorld* world, DOMClass* domObject, JSDOMWrapper* wrapper)
    {
        if (clearInlineCachedWrapper(world, domObject, wrapper))
            return;
        ASSERT(world->m_wrappers.find(domObject)->second.get() == wrapper);
        world->m_wrappers.remove(domObject);
    }
    
    #define CREATE_DOM_WRAPPER(exec, globalObject, className, object) createWrapper<JS##className>(exec, globalObject, static_cast<className*>(object))
    template<class WrapperClass, class DOMClass> inline JSDOMWrapper* createWrapper(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, DOMClass* node)
    {
        ASSERT(node);
        ASSERT(!getCachedWrapper(currentWorld(exec), node));
        WrapperClass* wrapper = new (exec) WrapperClass(getDOMStructure<WrapperClass>(exec, globalObject), globalObject, node);
        // FIXME: The entire function can be removed, once we fix caching.
        // This function is a one-off hack to make Nodes cache in the right global object.
        cacheWrapper(currentWorld(exec), node, wrapper);
        return wrapper;
    }

    template<class WrapperClass, class DOMClass> inline JSC::JSValue wrap(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, DOMClass* domObject)
    {
        if (!domObject)
            return JSC::jsNull();
        if (JSDOMWrapper* wrapper = getCachedWrapper(currentWorld(exec), domObject))
            return wrapper;
        return createWrapper<WrapperClass>(exec, globalObject, domObject);
    }

    inline void* root(Node* node)
    {
        if (node->inDocument())
            return node->document();

        while (node->parentNode())
            node = node->parentNode();
        return node;
    }

    inline void* root(StyleBase* styleBase)
    {
        while (styleBase->parent())
            styleBase = styleBase->parent();

        if (Node* node = styleBase->node())
            return root(node);
        return styleBase;
    }

    HashMap<CSSValue*, void*>& cssValueRoots();

    const JSC::HashTable* getHashTableForGlobalData(JSC::JSGlobalData&, const JSC::HashTable* staticTable);

    void reportException(JSC::ExecState*, JSC::JSValue exception);
    void reportCurrentException(JSC::ExecState*);

    // Convert a DOM implementation exception code into a JavaScript exception in the execution state.
    void setDOMException(JSC::ExecState*, ExceptionCode);

    JSC::JSValue jsString(JSC::ExecState*, const String&); // empty if the string is null
    JSC::JSValue jsStringSlowCase(JSC::ExecState*, JSStringCache&, StringImpl*);
    JSC::JSValue jsString(JSC::ExecState*, const KURL&); // empty if the URL is null
    inline JSC::JSValue jsString(JSC::ExecState* exec, const AtomicString& s)
    { 
        return jsString(exec, s.string());
    }
        
    JSC::JSValue jsStringOrNull(JSC::ExecState*, const String&); // null if the string is null
    JSC::JSValue jsStringOrNull(JSC::ExecState*, const KURL&); // null if the URL is null

    JSC::JSValue jsStringOrUndefined(JSC::ExecState*, const String&); // undefined if the string is null
    JSC::JSValue jsStringOrUndefined(JSC::ExecState*, const KURL&); // undefined if the URL is null

    JSC::JSValue jsStringOrFalse(JSC::ExecState*, const String&); // boolean false if the string is null
    JSC::JSValue jsStringOrFalse(JSC::ExecState*, const KURL&); // boolean false if the URL is null

    // See JavaScriptCore for explanation: Should be used for any string that is already owned by another
    // object, to let the engine know that collecting the JSString wrapper is unlikely to save memory.
    JSC::JSValue jsOwnedStringOrNull(JSC::ExecState*, const String&); 

    String identifierToString(const JSC::Identifier&);
    String ustringToString(const JSC::UString&);
    JSC::UString stringToUString(const String&);

    AtomicString identifierToAtomicString(const JSC::Identifier&);
    AtomicString ustringToAtomicString(const JSC::UString&);
    AtomicStringImpl* findAtomicString(const JSC::Identifier&);

    String valueToStringWithNullCheck(JSC::ExecState*, JSC::JSValue); // null if the value is null
    String valueToStringWithUndefinedOrNullCheck(JSC::ExecState*, JSC::JSValue); // null if the value is null or undefined

    inline int32_t finiteInt32Value(JSC::JSValue value, JSC::ExecState* exec, bool& okay)
    {
        double number = value.toNumber(exec);
        okay = isfinite(number);
        return JSC::toInt32(number);
    }

    // Returns a Date instance for the specified value, or null if the value is NaN or infinity.
    JSC::JSValue jsDateOrNull(JSC::ExecState*, double);
    // NaN if the value can't be converted to a date.
    double valueToDate(JSC::ExecState*, JSC::JSValue);

    template <typename T>
    inline JSC::JSValue toJS(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, PassRefPtr<T> ptr)
    {
        return toJS(exec, globalObject, ptr.get());
    }

    // Validates that the passed object is a sequence type per section 4.1.13 of the WebIDL spec.
    JSC::JSObject* toJSSequence(JSC::ExecState*, JSC::JSValue, unsigned&);

    bool checkNodeSecurity(JSC::ExecState*, Node*);

    // Helpers for Window, History, and Location classes to implement cross-domain policy.
    // Besides the cross-domain check, they need non-caching versions of staticFunctionGetter for
    // because we do not want current property values involved at all.
    // FIXME: These functions should be named frameAllowsAccessFrom, because the access is *to* the frame.
    bool allowsAccessFromFrame(JSC::ExecState*, Frame*);
    bool allowsAccessFromFrame(JSC::ExecState*, Frame*, String& message);
    DOMWindow* activeDOMWindow(JSC::ExecState*);
    DOMWindow* firstDOMWindow(JSC::ExecState*);

    void printErrorMessageForFrame(Frame*, const String& message);
    JSC::JSValue objectToStringFunctionGetter(JSC::ExecState*, JSC::JSValue, const JSC::Identifier& propertyName);

    Frame* toDynamicFrame(JSC::ExecState*);
    bool processingUserGesture();
    
    inline JSC::JSValue jsString(JSC::ExecState* exec, const String& s)
    {
        StringImpl* stringImpl = s.impl();
        if (!stringImpl || !stringImpl->length())
            return jsEmptyString(exec);

        if (stringImpl->length() == 1 && stringImpl->characters()[0] <= 0xFF)
            return jsString(exec, stringToUString(s));

        JSStringCache& stringCache = currentWorld(exec)->m_stringCache;
        JSStringCache::iterator it = stringCache.find(stringImpl);
        if (it != stringCache.end())
            return it->second.get();

        return jsStringSlowCase(exec, stringCache, stringImpl);
    }

    inline DOMObjectWrapperMap& domObjectWrapperMapFor(JSC::ExecState* exec)
    {
        return currentWorld(exec)->m_wrappers;
    }

    inline String ustringToString(const JSC::UString& u)
    {
        return u.impl();
    }

    inline JSC::UString stringToUString(const String& s)
    {
        return JSC::UString(s.impl());
    }

    inline String identifierToString(const JSC::Identifier& i)
    {
        return i.impl();
    }

    inline AtomicString ustringToAtomicString(const JSC::UString& u)
    {
        return AtomicString(u.impl());
    }

    inline AtomicString identifierToAtomicString(const JSC::Identifier& identifier)
    {
        return AtomicString(identifier.impl());
    }

} // namespace WebCore

#endif // JSDOMBinding_h
