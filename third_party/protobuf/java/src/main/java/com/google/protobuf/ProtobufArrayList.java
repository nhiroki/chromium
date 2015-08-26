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

import java.util.ArrayList;
import java.util.List;

/**
 * Implements {@link ProtobufList} for non-primitive and {@link String} types.
 */
class ProtobufArrayList<E> extends AbstractProtobufList<E> {

  private static final ProtobufArrayList<Object> EMPTY_LIST = new ProtobufArrayList<Object>();
  static {
    EMPTY_LIST.makeImmutable();
  }
  
  @SuppressWarnings("unchecked") // Guaranteed safe by runtime.
  public static <E> ProtobufArrayList<E> emptyList() {
    return (ProtobufArrayList<E>) EMPTY_LIST;
  }
  
  private final List<E> list;
  
  ProtobufArrayList() {
    list = new ArrayList<E>();
  }
  
  ProtobufArrayList(List<E> toCopy) {
    list = new ArrayList<E>(toCopy);
  }
  
  @Override
  public void add(int index, E element) {
    ensureIsMutable();
    list.add(index, element);
    modCount++;
  }

  @Override
  public E get(int index) {
    return list.get(index);
  }
  
  @Override
  public E remove(int index) {
    ensureIsMutable();
    E toReturn = list.remove(index);
    modCount++;
    return toReturn;
  }
  
  @Override
  public E set(int index, E element) {
    ensureIsMutable();
    E toReturn = list.set(index, element);
    modCount++;
    return toReturn;
  }

  @Override
  public int size() {
    return list.size();
  }
}
