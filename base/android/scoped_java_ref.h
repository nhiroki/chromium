// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_ANDROID_SCOPED_JAVA_REF_H_
#define BASE_ANDROID_SCOPED_JAVA_REF_H_

#include <jni.h>
#include <stddef.h>

#include "base/base_export.h"
#include "base/basictypes.h"
#include "base/logging.h"
#include "base/template_util.h"

namespace base {
namespace android {

// Creates a new local reference frame, in which at least a given number of
// local references can be created. Note that local references already created
// in previous local frames are still valid in the current local frame.
class BASE_EXPORT ScopedJavaLocalFrame {
 public:
  explicit ScopedJavaLocalFrame(JNIEnv* env);
  ScopedJavaLocalFrame(JNIEnv* env, int capacity);
  ~ScopedJavaLocalFrame();

 private:
  // This class is only good for use on the thread it was created on so
  // it's safe to cache the non-threadsafe JNIEnv* inside this object.
  JNIEnv* env_;

  DISALLOW_COPY_AND_ASSIGN(ScopedJavaLocalFrame);
};

// Forward declare the generic java reference template class.
template<typename T> class JavaRef;

// Template specialization of JavaRef, which acts as the base class for all
// other JavaRef<> template types. This allows you to e.g. pass
// ScopedJavaLocalRef<jstring> into a function taking const JavaRef<jobject>&
template<>
class BASE_EXPORT JavaRef<jobject> {
 public:
  jobject obj() const { return obj_; }

  bool is_null() const { return obj_ == NULL; }

 protected:
  // Initializes a NULL reference. Don't add anything else here; it's inlined.
  JavaRef() : obj_(NULL) {}

  // Takes ownership of the |obj| reference passed; requires it to be a local
  // reference type.
#if DCHECK_IS_ON()
  // Implementation contains a DCHECK; implement out-of-line when DCHECK_IS_ON.
  JavaRef(JNIEnv* env, jobject obj);
#else
  // Don't add anything else here; it's inlined.
  JavaRef(JNIEnv* env, jobject obj) : obj_(obj) {}
#endif

  // Don't add anything else here; it's inlined.
  ~JavaRef() {}

  // The following are implementation detail convenience methods, for
  // use by the sub-classes.
  JNIEnv* SetNewLocalRef(JNIEnv* env, jobject obj);
  void SetNewGlobalRef(JNIEnv* env, jobject obj);
  void ResetLocalRef(JNIEnv* env);
  void ResetGlobalRef();
  jobject ReleaseInternal();

 private:
  jobject obj_;

  DISALLOW_COPY_AND_ASSIGN(JavaRef);
};

// Generic base class for ScopedJavaLocalRef and ScopedJavaGlobalRef. Useful
// for allowing functions to accept a reference without having to mandate
// whether it is a local or global type.
template<typename T>
class JavaRef : public JavaRef<jobject> {
 public:
  T obj() const { return static_cast<T>(JavaRef<jobject>::obj()); }

 protected:
  JavaRef() {}
  ~JavaRef() {}

  JavaRef(JNIEnv* env, T obj) : JavaRef<jobject>(env, obj) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(JavaRef);
};

// Holds a local reference to a JNI method parameter.
// Method parameters should not be deleted, and so this class exists purely to
// wrap them as a JavaRef<T> in the JNI binding generator. Do not create
// instances manually.
template<typename T>
class JavaParamRef : public JavaRef<T> {
 public:
  // Assumes that |obj| is a parameter passed to a JNI method from Java.
  // Does not assume ownership as parameters should not be deleted.
  JavaParamRef(JNIEnv* env, T obj) : JavaRef<T>(env, obj) {}

  ~JavaParamRef() {}

  // TODO(torne): remove this cast once we're using JavaRef consistently.
  // http://crbug.com/506850
  operator T() const { return JavaRef<T>::obj(); }

 private:
  DISALLOW_COPY_AND_ASSIGN(JavaParamRef);
};

// Holds a local reference to a Java object. The local reference is scoped
// to the lifetime of this object.
// Instances of this class may hold onto any JNIEnv passed into it until
// destroyed. Therefore, since a JNIEnv is only suitable for use on a single
// thread, objects of this class must be created, used, and destroyed, on a
// single thread.
// Therefore, this class should only be used as a stack-based object and from a
// single thread. If you wish to have the reference outlive the current
// callstack (e.g. as a class member) or you wish to pass it across threads,
// use a ScopedJavaGlobalRef instead.
template<typename T>
class ScopedJavaLocalRef : public JavaRef<T> {
 public:
  ScopedJavaLocalRef() : env_(NULL) {}

  // Non-explicit copy constructor, to allow ScopedJavaLocalRef to be returned
  // by value as this is the normal usage pattern.
  ScopedJavaLocalRef(const ScopedJavaLocalRef<T>& other)
      : env_(other.env_) {
    this->SetNewLocalRef(env_, other.obj());
  }

  template<typename U>
  explicit ScopedJavaLocalRef(const U& other)
      : env_(NULL) {
    this->Reset(other);
  }

  // Assumes that |obj| is a local reference to a Java object and takes
  // ownership  of this local reference.
  ScopedJavaLocalRef(JNIEnv* env, T obj) : JavaRef<T>(env, obj), env_(env) {}

  ~ScopedJavaLocalRef() {
    this->Reset();
  }

  // Overloaded assignment operator defined for consistency with the implicit
  // copy constructor.
  void operator=(const ScopedJavaLocalRef<T>& other) {
    this->Reset(other);
  }

  void Reset() {
    this->ResetLocalRef(env_);
  }

  template<typename U>
  void Reset(const ScopedJavaLocalRef<U>& other) {
    // We can copy over env_ here as |other| instance must be from the same
    // thread as |this| local ref. (See class comment for multi-threading
    // limitations, and alternatives).
    this->Reset(other.env_, other.obj());
  }

  template<typename U>
  void Reset(const U& other) {
    // If |env_| was not yet set (is still NULL) it will be attached to the
    // current thread in SetNewLocalRef().
    this->Reset(env_, other.obj());
  }

  template<typename U>
  void Reset(JNIEnv* env, U obj) {
    static_assert(base::is_convertible<U, T>::value,
                  "U must be convertible to T");
    env_ = this->SetNewLocalRef(env, obj);
  }

  // Releases the local reference to the caller. The caller *must* delete the
  // local reference when it is done with it. Note that calling a Java method
  // is *not* a transfer of ownership and Release() should not be used.
  T Release() {
    return static_cast<T>(this->ReleaseInternal());
  }

 private:
  // This class is only good for use on the thread it was created on so
  // it's safe to cache the non-threadsafe JNIEnv* inside this object.
  JNIEnv* env_;

  // Prevent ScopedJavaLocalRef(JNIEnv*, T obj) from being used to take
  // ownership of a JavaParamRef's underlying object - parameters are not
  // allowed to be deleted and so should not be owned by ScopedJavaLocalRef.
  // TODO(torne): this can be removed once JavaParamRef no longer has an
  // implicit conversion back to T.
  ScopedJavaLocalRef(JNIEnv* env, const JavaParamRef<T>& other);
};

// Holds a global reference to a Java object. The global reference is scoped
// to the lifetime of this object. This class does not hold onto any JNIEnv*
// passed to it, hence it is safe to use across threads (within the constraints
// imposed by the underlying Java object that it references).
template<typename T>
class ScopedJavaGlobalRef : public JavaRef<T> {
 public:
  ScopedJavaGlobalRef() {}

  explicit ScopedJavaGlobalRef(const ScopedJavaGlobalRef<T>& other) {
    this->Reset(other);
  }

  ScopedJavaGlobalRef(JNIEnv* env, T obj) { this->Reset(env, obj); }

  template<typename U>
  explicit ScopedJavaGlobalRef(const U& other) {
    this->Reset(other);
  }

  ~ScopedJavaGlobalRef() {
    this->Reset();
  }

  void Reset() {
    this->ResetGlobalRef();
  }

  template<typename U>
  void Reset(const U& other) {
    this->Reset(NULL, other.obj());
  }

  template<typename U>
  void Reset(JNIEnv* env, const JavaParamRef<U>& other) {
    this->Reset(env, other.obj());
  }

  template<typename U>
  void Reset(JNIEnv* env, U obj) {
    static_assert(base::is_convertible<U, T>::value,
                  "U must be convertible to T");
    this->SetNewGlobalRef(env, obj);
  }

  // Releases the global reference to the caller. The caller *must* delete the
  // global reference when it is done with it. Note that calling a Java method
  // is *not* a transfer of ownership and Release() should not be used.
  T Release() {
    return static_cast<T>(this->ReleaseInternal());
  }
};

}  // namespace android
}  // namespace base

#endif  // BASE_ANDROID_SCOPED_JAVA_REF_H_
