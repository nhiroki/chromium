// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package com.google.protobuf;

import com.google.protobuf.Internal.ProtobufList;

import java.util.AbstractList;
import java.util.Collection;

/**
 * An abstract implementation of {@link ProtobufList} which manages mutability semantics. All mutate
 * methods are check if the list is mutable before proceeding. Subclasses must invoke
 * {@link #ensureIsMutable()} manually when overriding those methods.
 */
abstract class AbstractProtobufList<E> extends AbstractList<E> implements ProtobufList<E> {

  /**
   * Whether or not this list is modifiable.
   */
  private boolean isMutable;
  
  /**
   * Constructs a mutable list by default.
   */
  AbstractProtobufList() {
    isMutable = true;
  }

  @Override
  public boolean add(E e) {
    ensureIsMutable();
    return super.add(e);
  }

  @Override
  public void add(int index, E element) {
    ensureIsMutable();
    super.add(index, element);
  }

  @Override
  public boolean addAll(Collection<? extends E> c) {
    ensureIsMutable();
    return super.addAll(c);
  }
  
  @Override
  public boolean addAll(int index, Collection<? extends E> c) {
    ensureIsMutable();
    return super.addAll(index, c);
  }

  @Override
  public void clear() {
    ensureIsMutable();
    super.clear();
  }
  
  @Override
  public boolean isModifiable() {
    return isMutable;
  }
  
  @Override
  public final void makeImmutable() {
    isMutable = false;
  }
  
  @Override
  public E remove(int index) {
    ensureIsMutable();
    return super.remove(index);
  }
  
  @Override
  public boolean remove(Object o) {
    ensureIsMutable();
    return super.remove(o);
  }
  
  @Override
  public boolean removeAll(Collection<?> c) {
    ensureIsMutable();
    return super.removeAll(c);
  }
  
  @Override
  public boolean retainAll(Collection<?> c) {
    ensureIsMutable();
    return super.retainAll(c);
  }
  
  @Override
  public E set(int index, E element) {
    ensureIsMutable();
    return super.set(index, element);
  }
  
  /**
   * Throws an {@link UnsupportedOperationException} if the list is immutable. Subclasses are
   * responsible for invoking this method on mutate operations.
   */
  protected void ensureIsMutable() {
    if (!isMutable) {
      throw new UnsupportedOperationException();
    }
  }
}
