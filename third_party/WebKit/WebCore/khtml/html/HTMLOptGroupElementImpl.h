/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef HTML_HTMLOptGroupElementImpl_H
#define HTML_HTMLOptGroupElementImpl_H

#include "HTMLGenericFormElementImpl.h"

namespace DOM {

class HTMLOptGroupElementImpl : public HTMLGenericFormElementImpl
{
public:
    HTMLOptGroupElementImpl(DocumentImpl*, HTMLFormElementImpl* = 0);
    virtual ~HTMLOptGroupElementImpl();

    virtual bool checkDTD(const NodeImpl*);
    virtual DOMString type() const;
    virtual bool isFocusable() const;
    virtual PassRefPtr<NodeImpl> insertBefore(PassRefPtr<NodeImpl> newChild, NodeImpl* refChild, ExceptionCode&);
    virtual PassRefPtr<NodeImpl> replaceChild(PassRefPtr<NodeImpl> newChild, NodeImpl* oldChild, ExceptionCode&);
    virtual PassRefPtr<NodeImpl> removeChild(NodeImpl* child, ExceptionCode&);
    virtual PassRefPtr<NodeImpl> appendChild(PassRefPtr<NodeImpl> newChild, ExceptionCode&);
    virtual ContainerNodeImpl* addChild(PassRefPtr<NodeImpl>);
    virtual void parseMappedAttribute(MappedAttributeImpl*);

    void recalcSelectOptions();

    DOMString label() const;
    void setLabel(const DOMString&);
};

} //namespace

#endif
