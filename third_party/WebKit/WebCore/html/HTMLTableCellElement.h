/*
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 *           (C) 1997 Torben Weis (weis@kde.org)
 *           (C) 1998 Waldo Bastian (bastian@kde.org)
 *           (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2010 Apple Inc. All rights reserved.
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
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef HTMLTableCellElement_h
#define HTMLTableCellElement_h

#include "HTMLTablePartElement.h"

namespace WebCore {

class HTMLTableCellElement : public HTMLTablePartElement {
public:
    static PassRefPtr<HTMLTableCellElement> create(const QualifiedName&, Document*);

    int cellIndex() const;

    int col() const { return m_col; }
    void setCol(int col) { m_col = col; }
    int row() const { return m_row; }
    void setRow(int row) { m_row = row; }

    int colSpan() const { return m_colSpan; }
    int rowSpan() const { return m_rowSpan; }

    void setCellIndex(int);

    String abbr() const;
    void setAbbr(const String&);

    String align() const;
    void setAlign(const String&);

    String axis() const;
    void setAxis(const String&);

    String bgColor() const;
    void setBgColor(const String&);

    String ch() const;
    void setCh(const String&);

    String chOff() const;
    void setChOff(const String&);

    void setColSpan(int);

    String headers() const;
    void setHeaders(const String&);

    String height() const;
    void setHeight(const String&);

    bool noWrap() const;
    void setNoWrap(bool);

    void setRowSpan(int);

    String scope() const;
    void setScope(const String&);

    String vAlign() const;
    void setVAlign(const String&);

    String width() const;
    void setWidth(const String&);

private:
    HTMLTableCellElement(const QualifiedName&, Document*);

    virtual HTMLTagStatus endTagRequirement() const { return TagStatusOptional; }
    virtual int tagPriority() const { return 6; }

    virtual bool mapToEntry(const QualifiedName&, MappedAttributeEntry&) const;
    virtual void parseMappedAttribute(Attribute*);

    // used by table cells to share style decls created by the enclosing table.
    virtual bool canHaveAdditionalAttributeStyleDecls() const { return true; }
    virtual void additionalAttributeStyleDecls(Vector<CSSMutableStyleDeclaration*>&);
    
    virtual bool isURLAttribute(Attribute*) const;

    virtual void addSubresourceAttributeURLs(ListHashSet<KURL>&) const;

    int m_row;
    int m_col;
    int m_rowSpan;
    int m_colSpan;
};

} // namespace

#endif
