/*
 * Copyright (C) 2014 Google Inc. All rights reserved.
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

#ifndef Handle_h
#define Handle_h

#include "heap/Heap.h"
#include "heap/ThreadState.h"
#include "heap/Visitor.h"

namespace WebCore {

template<typename T> class Member;

class PersistentNode {
public:
    explicit PersistentNode(TraceCallback trace) : m_trace(trace) { }

    virtual ~PersistentNode() { }

    // Ideally the trace method should be virtual and automatically dispatch
    // to the most specific implementation. However having a virtual method
    // on PersistentNode leads to too eager template instantiation with MSVC
    // which leads to include cycles.
    // Instead we call the constructor with a TraceCallback which knows the
    // type of the most specific child and calls trace directly. See
    // TraceMethodDelegate in Visitor.h for how this is done.
    void trace(Visitor* visitor)
    {
        m_trace(visitor, this);
    }

protected:
    TraceCallback m_trace;

private:
    PersistentNode* m_next;
    PersistentNode* m_prev;

    template<ThreadAffinity affinity, typename Owner> friend class PersistentBase;
    friend class PersistentAnchor;
    friend class ThreadState;
};

template<ThreadAffinity Affinity, typename Owner>
class PersistentBase : public PersistentNode {
public:
    ~PersistentBase()
    {
#ifndef NDEBUG
        m_threadState->checkThread();
#endif
        m_next->m_prev = m_prev;
        m_prev->m_next = m_next;
    }

protected:
    inline PersistentBase()
        : PersistentNode(TraceMethodDelegate<Owner, &Owner::trace>::trampoline)
#ifndef NDEBUG
        , m_threadState(state())
#endif
    {
#ifndef NDEBUG
        m_threadState->checkThread();
#endif
        ThreadState* threadState = state();
        m_prev = threadState->roots();
        m_next = threadState->roots()->m_next;
        threadState->roots()->m_next = this;
        m_next->m_prev = this;
    }

    inline explicit PersistentBase(const PersistentBase& otherref)
        : PersistentNode(otherref.m_trace)
#ifndef NDEBUG
        , m_threadState(state())
#endif
    {
#ifndef NDEBUG
        m_threadState->checkThread();
#endif
        ASSERT(otherref.m_threadState == m_threadState);
        PersistentBase* other = const_cast<PersistentBase*>(&otherref);
        m_prev = other;
        m_next = other->m_next;
        other->m_next = this;
        m_next->m_prev = this;
    }

    inline PersistentBase& operator=(const PersistentBase& otherref)
    {
        return *this;
    }

    static ThreadState* state() { return ThreadStateFor<Affinity>::state(); }

#ifndef NDEBUG
private:
    ThreadState* m_threadState;
#endif
};

// A dummy Persistent handle that ensures the list of persistents is never null.
// This removes a test from a hot path.
class PersistentAnchor : public PersistentNode {
public:
    void trace(Visitor*) { }

private:
    ~PersistentAnchor() { }
    PersistentAnchor() : PersistentNode(TraceMethodDelegate<PersistentAnchor, &PersistentAnchor::trace>::trampoline)
    {
        m_next = this;
        m_prev = this;
    }

    friend class ThreadState;
};

// Persistent handles are used to store pointers into the
// managed heap. As long as the Persistent handle is alive
// the GC will keep the object pointed to alive. Persistent
// handles can be stored in objects and they are not scoped.
// Persistent handles must not be used to contain pointers
// between objects that are in the managed heap. They are only
// meant to point to managed heap objects from variables/members
// outside the managed heap.
//
// A Persistent is always a GC root from the point of view of
// the garbage collector.
template<typename T>
class Persistent : public PersistentBase<ThreadingTrait<T>::Affinity, Persistent<T> > {
public:
    Persistent()
        : m_raw(0)
    {
    }

    Persistent(T* raw)
        : m_raw(raw)
    {
    }

    Persistent(std::nullptr_t)
        : m_raw(0)
    {
    }

    Persistent(const Persistent& other)
        : m_raw(other)
    {
    }

    template<typename U>
    Persistent(const Persistent<U>& other)
        : m_raw(other)
    {
    }

    template<typename U>
    Persistent(const Member<U>& other)
        : m_raw(other)
    {
    }

    template<typename U>
    Persistent& operator=(U* other)
    {
        m_raw = other;
        return *this;
    }

    virtual ~Persistent()
    {
        m_raw = 0;
    }

    template<typename U>
    U* as() const
    {
        return static_cast<U*>(m_raw);
    }

    void trace(Visitor* visitor) { visitor->mark(m_raw); }

    T* clear()
    {
        T* result = m_raw;
        m_raw = 0;
        return result;
    }

    T& operator*() const { return *m_raw; }

    bool operator!() const { return !m_raw; }

    operator T*() const { return m_raw; }

    T* operator->() const { return *this; }

    Persistent& operator=(std::nullptr_t)
    {
        m_raw = 0;
        return *this;
    }

    Persistent& operator=(const Persistent& other)
    {
        m_raw = other;
        return *this;
    }

    template<typename U>
    Persistent& operator=(const Persistent<U>& other)
    {
        m_raw = other;
        return *this;
    }

    template<typename U>
    Persistent& operator=(const Member<U>& other)
    {
        m_raw = other;
        return *this;
    }

    T* raw() const { return m_raw; }

private:
    T* m_raw;
};

// Members are used in classes to contain strong pointers to other oilpan heap
// allocated objects.
// All Member fields of a class must be traced in the class' trace method.
// During the mark phase of the GC all live objects are marked as live and
// all Member fields of a live object will be traced marked as live as well.
template<typename T>
class Member {
public:
    Member()
        : m_raw(0)
    {
    }

    Member(T* raw)
        : m_raw(raw)
    {
    }

    Member(std::nullptr_t)
        : m_raw(0)
    {
    }

    Member(WTF::HashTableDeletedValueType)
        : m_raw(reinterpret_cast<T*>(-1))
    {
    }

    bool isHashTableDeletedValue() const
    {
        return m_raw == reinterpret_cast<T*>(-1);
    }

    template<typename U>
    Member(const Persistent<U>& other)
        : m_raw(other)
    {
    }

    Member(const Member& other)
        : m_raw(other)
    {
    }

    template<typename U>
    Member(const Member<U>& other)
        : m_raw(other)
    {
    }

    T* clear()
    {
        T* result = m_raw;
        m_raw = 0;
        return result;
    }

    template<typename U>
    U* as() const
    {
        return static_cast<U*>(m_raw);
    }

    bool operator!() const { return !m_raw; }

    operator T*() const { return m_raw; }

    T* operator->() const { return m_raw; }
    T& operator*() const { return *m_raw; }

    Member& operator=(std::nullptr_t)
    {
        m_raw = 0;
        return *this;
    }

    template<typename U>
    Member& operator=(const Persistent<U>& other)
    {
        m_raw = other;
        return *this;
    }

    Member& operator=(const Member& other)
    {
        m_raw = other;
        return *this;
    }

    template<typename U>
    Member& operator=(const Member<U>& other)
    {
        m_raw = other;
        return *this;
    }

    template<typename U>
    Member& operator=(U* other)
    {
        m_raw = other;
        return *this;
    }

    void swap(Member<T>& other) { std::swap(m_raw, other.m_raw); }

protected:
    T* raw() const { return m_raw; }

    T* m_raw;

    template<typename U> friend class Member;
    template<typename U> friend class Persistent;
    friend class Visitor;
    template<typename U> friend struct WTF::PtrHash;
    // FIXME: Uncomment when HeapObjectHash is moved.
    // friend struct HeapObjectHash<T>;
    // FIXME: Uncomment when ObjectAliveTrait (weak pointer support) is moved.
    // friend struct ObjectAliveTrait<Member<T> >;
    template<typename U, typename V> friend bool operator==(const Member<U>&, const Persistent<V>&);
    template<typename U, typename V> friend bool operator!=(const Member<U>&, const Persistent<V>&);
    template<typename U, typename V> friend bool operator==(const Persistent<U>&, const Member<V>&);
    template<typename U, typename V> friend bool operator!=(const Persistent<U>&, const Member<V>&);
    template<typename U, typename V> friend bool operator==(const Member<U>&, const Member<V>&);
    template<typename U, typename V> friend bool operator!=(const Member<U>&, const Member<V>&);
};

template<typename T>
class TraceTrait<Member<T> > {
public:
    static void trace(Visitor* visitor, void* self)
    {
        TraceTrait<T>::mark(visitor, *static_cast<Member<T>*>(self));
    }
};

// Comparison operators between Members and Persistents
template<typename T, typename U> inline bool operator==(const Member<T>& a, const Member<U>& b) { return a.m_raw == b.m_raw; }
template<typename T, typename U> inline bool operator!=(const Member<T>& a, const Member<U>& b) { return a.m_raw != b.m_raw; }
template<typename T, typename U> inline bool operator==(const Member<T>& a, const Persistent<U>& b) { return a.m_raw == b.m_raw; }
template<typename T, typename U> inline bool operator!=(const Member<T>& a, const Persistent<U>& b) { return a.m_raw != b.m_raw; }
template<typename T, typename U> inline bool operator==(const Persistent<T>& a, const Member<U>& b) { return a.m_raw == b.m_raw; }
template<typename T, typename U> inline bool operator!=(const Persistent<T>& a, const Member<U>& b) { return a.m_raw != b.m_raw; }
template<typename T, typename U> inline bool operator==(const Persistent<T>& a, const Persistent<U>& b) { return a.m_raw == b.m_raw; }
template<typename T, typename U> inline bool operator!=(const Persistent<T>& a, const Persistent<U>& b) { return a.m_raw != b.m_raw; }

}

#endif
