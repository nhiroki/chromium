#!/usr/bin/env python
# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

""" Parser for PPAPI IDL """

#
# IDL Parser
#
# The parser is uses the PLY yacc library to build a set of parsing rules based
# on WebIDL.
#
# WebIDL, and WebIDL grammar can be found at:
#   http://dev.w3.org/2006/webapi/WebIDL/
# PLY can be found at:
#   http://www.dabeaz.com/ply/
#
# The parser generates a tree by recursively matching sets of items against
# defined patterns.  When a match is made, that set of items is reduced
# to a new item.   The new item can provide a match for parent patterns.
# In this way an AST is built (reduced) depth first.
#

#
# Disable check for line length and Member as Function due to how grammar rules
# are defined with PLY
#
# pylint: disable=R0201
# pylint: disable=C0301

import os.path
import sys
import time

from idl_lexer import IDLLexer
from idl_node import IDLAttribute, IDLNode

from ply import lex
from ply import yacc

#
# ERROR_REMAP
#
# Maps the standard error formula into a more friendly error message.
#
ERROR_REMAP = {
  'Unexpected ")" after "(".' : 'Empty argument list.',
  'Unexpected ")" after ",".' : 'Missing argument.',
  'Unexpected "}" after ",".' : 'Trailing comma in block.',
  'Unexpected "}" after "{".' : 'Unexpected empty block.',
  'Unexpected comment after "}".' : 'Unexpected trailing comment.',
  'Unexpected "{" after keyword "enum".' : 'Enum missing name.',
  'Unexpected "{" after keyword "struct".' : 'Struct missing name.',
  'Unexpected "{" after keyword "interface".' : 'Interface missing name.',
}


def Boolean(val):
  """Convert to strict boolean type."""
  if val:
    return True
  return False


def ListFromConcat(*items):
  """Generate list by concatenating inputs"""
  itemsout = []
  for item in items:
    if item is None:
      continue
    if type(item) is not type([]):
      itemsout.append(item)
    else:
      itemsout.extend(item)

  return itemsout

def ExpandProduction(p):
  if type(p) == list:
    return '[' + ', '.join([ExpandProduction(x) for x in p]) + ']'
  if type(p) == IDLNode:
    return 'Node:' + str(p)
  if type(p) == IDLAttribute:
    return 'Attr:' + str(p)
  if type(p) == str:
    return 'str:' + p
  return '%s:%s' % (p.__class__.__name__, str(p))

# TokenTypeName
#
# Generate a string which has the type and value of the token.
#
def TokenTypeName(t):
  if t.type == 'SYMBOL':
    return 'symbol %s' % t.value
  if t.type in ['HEX', 'INT', 'OCT', 'FLOAT']:
    return 'value %s' % t.value
  if t.type == 'string' :
    return 'string "%s"' % t.value
  if t.type == 'COMMENT' :
    return 'comment'
  if t.type == t.value:
    return '"%s"' % t.value
  if t.type == ',':
    return 'Comma'
  if t.type == 'identifier':
    return 'identifier "%s"' % t.value
  return 'keyword "%s"' % t.value


#
# IDL Parser
#
# The Parser inherits the from the Lexer to provide PLY with the tokenizing
# definitions.  Parsing patterns are encoded as functions where p_<name> is
# is called any time a patern matching the function documentation is found.
# Paterns are expressed in the form of:
# """ <new item> : <item> ....
#                | <item> ...."""
#
# Where new item is the result of a match against one or more sets of items
# separated by the "|".
#
# The function is called with an object 'p' where p[0] is the output object
# and p[n] is the set of inputs for positive values of 'n'.  Len(p) can be
# used to distinguish between multiple item sets in the pattern.
#
# For more details on parsing refer to the PLY documentation at
#    http://www.dabeaz.com/ply/
#
# The parser is based on the WebIDL standard.  See:
#    http://www.w3.org/TR/WebIDL/#idl-grammar
#
# The various productions are annotated so that the WHOLE number greater than
# zero in the comment denotes the matching WebIDL grammar definition.
#
# Productions with a fractional component in the comment denote additions to
# the WebIDL spec, such as comments.
#


class IDLParser(object):
#
# We force all input files to start with two comments.  The first comment is a
# Copyright notice followed by a file comment and finally by file level
# productions.
#
  # [0] Insert a TOP definition for Copyright and Comments
  def p_Top(self, p):
    """Top : COMMENT COMMENT Definitions"""
    Copyright = self.BuildComment('Copyright', p, 1)
    Filedoc = self.BuildComment('Comment', p, 2)
    p[0] = ListFromConcat(Copyright, Filedoc, p[3])

  # [0.1] Add support for Multiple COMMENTS
  def p_Comments(self, p):
    """Comments : CommentsRest"""
    if len(p) > 1:
      p[0] = p[1]

  # [0.2] Produce a COMMENT and aggregate sibling comments
  def p_CommentsRest(self, p):
    """CommentsRest : COMMENT CommentsRest
                    | """
    if len(p) > 1:
      p[0] = ListFromConcat(self.BuildComment('Comment', p, 1), p[2])


#
#The parser is based on the WebIDL standard.  See:
# http://www.w3.org/TR/WebIDL/#idl-grammar
#
  # [1]
  def p_Definitions(self, p):
    """Definitions : ExtendedAttributeList Definition Definitions
           | """
    if len(p) > 1:
      p[2].AddChildren(p[1])
      p[0] = ListFromConcat(p[2], p[3])

      # [2] Add INLINE definition
  def p_Definition(self, p):
    """Definition : CallbackOrInterface
                  | Partial
                  | Dictionary
                  | Exception
                  | Enum
                  | Typedef
                  | ImplementsStatement"""
    p[0] = p[1]

  # [2.1] Error recovery for definition
  def p_DefinitionError(self, p):
    """Definition : error ';'"""
    p[0] = self.BuildError(p, 'Definition')

  # [3]
  def p_CallbackOrInterface(self, p):
    """CallbackOrInterface : CALLBACK CallbackRestOrInterface
                           | Interface"""
    if len(p) > 2:
      p[0] = p[2]
    else:
      p[0] = p[1]

  # [4]
  def p_CallbackRestOrInterface(self, p):
    """CallbackRestOrInterface : CallbackRest
                               | Interface"""
    p[0] = p[1]

  # [5]
  def p_Interface(self, p):
    """Interface : INTERFACE identifier Inheritance '{' InterfaceMembers '}' ';'"""
    p[0] = self.BuildNamed('Interface', p, 2, ListFromConcat(p[3], p[5]))

  # [6] Error recovery for PARTIAL
  def p_Partial(self, p):
    """Partial : PARTIAL PartialDefinition"""
    p[2].AddChildren(self.BuildTrue('Partial'))
    p[0] = p[2]

  # [6.1] Error recovery for Enums
  def p_PartialError(self, p):
    """Partial : PARTIAL error"""
    p[0] = self.BuildError(p, 'Partial')

  # [7]
  def p_PartialDefinition(self, p):
    """PartialDefinition : PartialDictionary
                         | PartialInterface"""
    p[0] = p[1]

  # [8]
  def p_PartialInterface(self, p):
    """PartialInterface : INTERFACE identifier '{' InterfaceMembers '}' ';'"""
    p[0] = self.BuildNamed('Interface', p, 2, p[4])

  # [9]
  def p_InterfaceMembers(self, p):
    """InterfaceMembers : ExtendedAttributeList InterfaceMember InterfaceMembers
                        |"""
    if len(p) > 1:
      p[2].AddChildren(p[1])
      p[0] = ListFromConcat(p[2], p[3])

  # [10]
  def p_InterfaceMember(self, p):
    """InterfaceMember : Const
                       | AttributeOrOperation"""
    p[0] = p[1]

  # [11]
  def p_Dictionary(self, p):
    """Dictionary : DICTIONARY identifier Inheritance '{' DictionaryMembers '}' ';'"""
    p[0] = self.BuildNamed('Dictionary', p, 2, ListFromConcat(p[3], p[5]))

  # [11.1] Error recovery for regular Dictionary
  def p_DictionaryError(self, p):
    """Dictionary : DICTIONARY error ';'"""
    p[0] = self.BuildError(p, 'Dictionary')

  # [12]
  def p_DictionaryMembers(self, p):
    """DictionaryMembers : ExtendedAttributeList DictionaryMember DictionaryMembers
                         |"""
    if len(p) > 1:
      p[2].AddChildren(p[1])
      p[0] = ListFromConcat(p[2], p[3])

  # [13]
  def p_DictionaryMember(self, p):
    """DictionaryMember : Type identifier Default ';'"""
    p[0] = self.BuildNamed('Key', p, 2, ListFromConcat(p[1], p[3]))

  # [14]
  def p_PartialDictionary(self, p):
    """PartialDictionary : DICTIONARY identifier '{' DictionaryMembers '}' ';'"""
    partial = self.BuildTrue('Partial')
    p[0] = self.BuildNamed('Dictionary', p, 2, ListFromConcat(p[4], partial))

  # [14.1] Error recovery for Partial Dictionary
  def p_PartialDictionaryError(self, p):
    """PartialDictionary : DICTIONARY error ';'"""
    p[0] = self.BuildError(p, 'PartialDictionary')

  # [15]
  def p_Default(self, p):
    """Default : '=' DefaultValue
               |"""
    if len(p) > 1:
      p[0] = self.BuildProduction('Default', p, 2, p[2])

  # [16]
  def p_DefaultValue(self, p):
    """DefaultValue : ConstValue
                    | string"""
    if type(p[1]) == str:
      p[0] = ListFromConcat(self.BuildAttribute('TYPE', 'DOMString'),
                            self.BuildAttribute('NAME', p[1]))
    else:
      p[0] = p[1]

  # [17]
  def p_Exception(self, p):
    """Exception : EXCEPTION identifier Inheritance '{' ExceptionMembers '}' ';'"""
    p[0] = self.BuildNamed('Exception', p, 2, ListFromConcat(p[3], p[5]))

  # [18]
  def p_ExceptionMembers(self, p):
    """ExceptionMembers : ExtendedAttributeList ExceptionMember ExceptionMembers
                        |"""
    if len(p) > 1:
      p[2].AddChildren(p[1])
      p[0] = ListFromConcat(p[2], p[3])

  # [18.1] Error recovery for ExceptionMembers
  def p_ExceptionMembersError(self, p):
    """ExceptionMembers : error"""
    p[0] = self.BuildError(p, 'ExceptionMembers')

  # [19]
  def p_Inheritance(self, p):
    """Inheritance : ':' identifier
                   |"""
    if len(p) > 1:
      p[0] = self.BuildNamed('Inherit', p, 2)

  # [20]
  def p_Enum(self, p):
    """Enum : ENUM identifier '{' EnumValueList '}' ';'"""
    p[0] = self.BuildNamed('Enum', p, 2, p[4])

  # [20.1] Error recovery for Enums
  def p_EnumError(self, p):
    """Enum : ENUM error ';'"""
    p[0] = self.BuildError(p, 'Enum')

  # [21]
  def p_EnumValueList(self, p):
    """EnumValueList : ExtendedAttributeList string EnumValues"""
    enum = self.BuildNamed('EnumItem', p, 2, p[1])
    p[0] = ListFromConcat(enum, p[3])

  # [22]
  def p_EnumValues(self, p):
    """EnumValues : ',' ExtendedAttributeList string EnumValues
                  |"""
    if len(p) > 1:
      enum = self.BuildNamed('EnumItem', p, 3, p[2])
      p[0] = ListFromConcat(enum, p[4])

  # [23]
  def p_CallbackRest(self, p):
    """CallbackRest : identifier '=' ReturnType '(' ArgumentList ')' ';'"""
    arguments = self.BuildProduction('Arguments', p, 4, p[5])
    p[0] = self.BuildNamed('Callback', p, 1, ListFromConcat(p[3], arguments))

  # [24]
  def p_Typedef(self, p):
    """Typedef : TYPEDEF ExtendedAttributeList Type identifier ';'"""
    p[0] = self.BuildNamed('Typedef', p, 4, ListFromConcat(p[2], p[3]))

  # [24.1] Error recovery for Typedefs
  def p_TypedefError(self, p):
    """Typedef : TYPEDEF error ';'"""
    p[0] = self.BuildError(p, 'Typedef')

  # [25]
  def p_ImplementsStatement(self, p):
    """ImplementsStatement : identifier IMPLEMENTS identifier ';'"""
    name = self.BuildAttribute('REFERENCE', p[3])
    p[0] = self.BuildNamed('Implements', p, 1, name)

  # [26]
  def p_Const(self,  p):
    """Const : CONST ConstType identifier '=' ConstValue ';'"""
    value = self.BuildProduction('Value', p, 5, p[5])
    p[0] = self.BuildNamed('Const', p, 3, ListFromConcat(p[2], value))

  # [27]
  def p_ConstValue(self, p):
    """ConstValue : BooleanLiteral
                  | FloatLiteral
                  | integer
                  | null"""
    if type(p[1]) == str:
      p[0] = ListFromConcat(self.BuildAttribute('TYPE', 'integer'),
                            self.BuildAttribute('NAME', p[1]))
    else:
      p[0] = p[1]

  # [27.1] Add definition for NULL
  def p_null(self, p):
    """null : NULL"""
    p[0] = ListFromConcat(self.BuildAttribute('TYPE', 'NULL'),
                          self.BuildAttribute('NAME', 'NULL'))

  # [28]
  def p_BooleanLiteral(self, p):
    """BooleanLiteral : TRUE
                      | FALSE"""
    value = self.BuildAttribute('VALUE', Boolean(p[1] == 'true'))
    p[0] = ListFromConcat(self.BuildAttribute('TYPE', 'boolean'), value)

  # [29]
  def p_FloatLiteral(self, p):
    """FloatLiteral : float
                    | '-' INFINITY
                    | INFINITY
                    | NAN """
    if len(p) > 2:
      val = '-Infinity'
    else:
      val = p[1]
    p[0] = ListFromConcat(self.BuildAttribute('TYPE', 'float'),
                          self.BuildAttribute('VALUE', val))

  # [30]
  def p_AttributeOrOperation(self, p):
    """AttributeOrOperation : STRINGIFIER StringifierAttributeOrOperation
                            | Attribute
                            | Operation"""
    if len(p) > 2:
      p[0] = p[2]
    else:
      p[0] = p[1]

  # [31]
  def p_StringifierAttributeOrOperation(self, p):
    """StringifierAttributeOrOperation : Attribute
                                       | OperationRest
                                       | ';'"""
    if p[1] == ';':
      p[0] = self.BuildAttribute('STRINGIFIER', Boolean(True))
    else:
      p[0] = ListFromConcat(self.BuildAttribute('STRINGIFIER', p[1]), p[1])

  # [32]
  def p_Attribute(self, p):
    """Attribute : Inherit ReadOnly ATTRIBUTE Type identifier ';'"""
    p[0] = self.BuildNamed('Attribute', p, 5,
                           ListFromConcat(p[1], p[2], p[4]))

  # [33]
  def p_Inherit(self, p):
    """Inherit : INHERIT
               |"""
    if len(p) > 1:
      p[0] = self.BuildTrue('INHERIT')

  # [34]
  def p_ReadOnly(self, p):
    """ReadOnly : READONLY
               |"""
    if len(p) > 1:
      p[0] = self.BuildTrue('READONLY')

  # [35]
  def p_Operation(self, p):
    """Operation : Qualifiers OperationRest"""
    p[2].AddChildren(p[1])
    p[0] = p[2]

  # [36]
  def p_Qualifiers(self, p):
    """Qualifiers : STATIC
                  | Specials"""
    if p[1] == 'static':
      p[0] = self.BuildTrue('STATIC')
    else:
      p[0] = p[1]

  # [37]
  def p_Specials(self, p):
    """Specials : Special Specials
                | """
    if len(p) > 1:
      p[0] = ListFromConcat(p[1], p[2])

  # [38]
  def p_Special(self, p):
    """Special : GETTER
               | SETTER
               | CREATOR
               | DELETER
               | LEGACYCALLER"""
    p[0] = self.BuildTrue(p[1].upper())


  # [39]
  def p_OperationRest(self, p):
    """OperationRest : ReturnType OptionalIdentifier '(' ArgumentList ')' ';'"""
    arguments = self.BuildProduction('Arguments', p, 3, p[4])
    p[0] = self.BuildNamed('Operation', p, 2, ListFromConcat(p[1], arguments))

  # [40]
  def p_OptionalIdentifier(self, p):
    """OptionalIdentifier : identifier
                          |"""
    if len(p) > 1:
      p[0] = p[1]
    else:
      p[0] = '_unnamed_'

  # [41]
  def p_ArgumentList(self, p):
    """ArgumentList : Argument Arguments
                    |"""
    if len(p) > 1:
      p[0] = ListFromConcat(p[1], p[2])

  # [41.1] ArgumentList error recovery
  def p_ArgumentListError(self, p):
    """ArgumentList : error """
    p[0] = self.BuildError(p, 'ArgumentList')

  # [42]
  def p_Arguments(self, p):
    """Arguments : ',' Argument Arguments
                 |"""
    if len(p) > 1:
      p[0] = ListFromConcat(p[2], p[3])

  # [43]
  def p_Argument(self, p):
    """Argument : ExtendedAttributeList OptionalOrRequiredArgument"""
    p[2].AddChildren(p[1])
    p[0] = p[2]


  # [44]
  def p_OptionalOrRequiredArgument(self, p):
    """OptionalOrRequiredArgument : OPTIONAL Type ArgumentName Default
                                  | Type Ellipsis ArgumentName"""
    if len(p) > 4:
      arg = self.BuildNamed('Argument', p, 3, ListFromConcat(p[2], p[4]))
      arg.AddChildren(self.BuildTrue('OPTIONAL'))
    else:
      arg = self.BuildNamed('Argument', p, 3, ListFromConcat(p[1], p[2]))
    p[0] = arg

  # [45]
  def p_ArgumentName(self, p):
    """ArgumentName : ArgumentNameKeyword
                    | identifier"""
    p[0] = p[1]

  # [46]
  def p_Ellipsis(self, p):
    """Ellipsis : ELLIPSIS
                |"""
    if len(p) > 1:
      p[0] = self.BuildNamed('Argument', p, 1)
      p[0].AddChildren(self.BuildTrue('ELLIPSIS'))

  # [47]
  def p_ExceptionMember(self, p):
    """ExceptionMember : Const
                       | ExceptionField"""
    p[0] = p[1]

  # [48]
  def p_ExceptionField(self, p):
    """ExceptionField : Type identifier ';'"""
    p[0] = self.BuildNamed('ExceptionField', p, 2, p[1])

  # [48.1] Error recovery for ExceptionMembers
  def p_ExceptionFieldError(self, p):
    """ExceptionField : error"""
    p[0] = self.BuildError(p, 'ExceptionField')

  # [49] Add optional comment field
  def p_ExtendedAttributeList(self, p):
    """ExtendedAttributeList : Comments '[' ExtendedAttribute ExtendedAttributes ']'
                             | Comments """
    if len(p) > 2:
      items = ListFromConcat(p[3], p[4])
      attribs = self.BuildProduction('ExtAttributes', p, 2, items)
      p[0] = ListFromConcat(p[1], attribs)
    else:
      p[0] = p[1]

  # [50]
  def p_ExtendedAttributes(self, p):
    """ExtendedAttributes : ',' ExtendedAttribute ExtendedAttributes
                          |"""
    if len(p) > 1:
      p[0] = ListFromConcat(p[2], p[3])

  # We only support:
  #    [ identifier ]
  #    [ identifier = identifier ]
  #    [ identifier ( ArgumentList )]
  #    [ identifier = identifier ( ArgumentList )]
  # [51] map directly to 74-77
  # [52-54, 56] are unsupported
  def p_ExtendedAttribute(self, p):
    """ExtendedAttribute : ExtendedAttributeNoArgs
                         | ExtendedAttributeArgList
                         | ExtendedAttributeIdent
                         | ExtendedAttributeNamedArgList"""
    p[0] = p[1]

  # [55]
  def p_ArgumentNameKeyword(self, p):
    """ArgumentNameKeyword : ATTRIBUTE
                           | CALLBACK
                           | CONST
                           | CREATOR
                           | DELETER
                           | DICTIONARY
                           | ENUM
                           | EXCEPTION
                           | GETTER
                           | IMPLEMENTS
                           | INHERIT
                           | LEGACYCALLER
                           | PARTIAL
                           | SETTER
                           | STATIC
                           | STRINGIFIER
                           | TYPEDEF
                           | UNRESTRICTED"""
    p[0] = p[1]

  # [57]
  def p_Type(self, p):
    """Type : SingleType
            | UnionType TypeSuffix"""
    if len(p) == 2:
      p[0] = self.BuildProduction('Type', p, 1, p[1])
    else:
      p[0] = self.BuildProduction('Type', p, 1, ListFromConcat(p[1], p[2]))

  # [58]
  def p_SingleType(self, p):
    """SingleType : NonAnyType
                  | ANY TypeSuffixStartingWithArray"""
    if len(p) == 2:
      p[0] = p[1]
    else:
      p[0] = ListFromConcat(self.BuildProduction('Any', p, 1), p[2])

  # [59]
  def p_UnionType(self, p):
    """UnionType : '(' UnionMemberType OR UnionMemberType UnionMemberTypes ')'"""

  # [60]
  def p_UnionMemberType(self, p):
    """UnionMemberType : NonAnyType
                       | UnionType TypeSuffix
                       | ANY '[' ']' TypeSuffix"""
  # [61]
  def p_UnionMemberTypes(self, p):
    """UnionMemberTypes : OR UnionMemberType UnionMemberTypes
                        |"""

  # [62] Moved DATE, DOMSTRING, OBJECT to PrimitiveType
  def p_NonAnyType(self, p):
    """NonAnyType : PrimitiveType TypeSuffix
                  | identifier TypeSuffix
                  | SEQUENCE '<' Type '>' Null"""
    if len(p) == 3:
      if type(p[1]) == str:
        typeref = self.BuildNamed('Typeref', p, 1)
      else:
        typeref = p[1]
      p[0] = ListFromConcat(typeref, p[2])

    if len(p) == 6:
      p[0] = self.BuildProduction('Sequence', p, 1, ListFromConcat(p[3], p[5]))


  # [63]
  def p_ConstType(self,  p):
    """ConstType : PrimitiveType Null
                 | identifier Null"""
    if type(p[1]) == str:
      p[0] = self.BuildNamed('Typeref', p, 1, p[2])
    else:
      p[1].AddChildren(p[2])
      p[0] = p[1]


  # [64]
  def p_PrimitiveType(self, p):
    """PrimitiveType : UnsignedIntegerType
                     | UnrestrictedFloatType
                     | BOOLEAN
                     | BYTE
                     | OCTET
                     | DOMSTRING
                     | DATE
                     | OBJECT"""
    if type(p[1]) == str:
      p[0] = self.BuildNamed('PrimitiveType', p, 1)
    else:
      p[0] = p[1]


  # [65]
  def p_UnrestrictedFloatType(self, p):
    """UnrestrictedFloatType : UNRESTRICTED FloatType
                             | FloatType"""
    if len(p) == 2:
      typeref = self.BuildNamed('PrimitiveType', p, 1)
    else:
      typeref = self.BuildNamed('PrimitiveType', p, 2)
      typeref.AddChildren(self.BuildTrue('UNRESTRICTED'))
    p[0] = typeref


  # [66]
  def p_FloatType(self, p):
    """FloatType : FLOAT
                 | DOUBLE"""
    p[0] = p[1]

  # [67]
  def p_UnsignedIntegerType(self, p):
    """UnsignedIntegerType : UNSIGNED IntegerType
                           | IntegerType"""
    if len(p) == 2:
      p[0] = p[1]
    else:
      p[0] = 'unsigned ' + p[2]

  # [68]
  def p_IntegerType(self, p):
    """IntegerType : SHORT
                   | LONG OptionalLong"""
    if len(p) == 2:
      p[0] = p[1]
    else:
      p[0] = p[1] + p[2]

  # [69]
  def p_OptionalLong(self, p):
    """OptionalLong : LONG
                    | """
    if len(p) > 1:
      p[0] = ' ' + p[1]
    else:
      p[0] = ''


  # [70] Add support for sized array
  def p_TypeSuffix(self, p):
    """TypeSuffix : '[' integer ']' TypeSuffix
                  | '[' ']' TypeSuffix
                  | '?' TypeSuffixStartingWithArray
                  | """
    if len(p) == 5:
      p[0] = self.BuildNamed('Array', p, 2, p[4])

    if len(p) == 4:
      p[0] = self.BuildProduction('Array', p, 1, p[3])

    if len(p) == 3:
      p[0] = ListFromConcat(self.BuildTrue('NULLABLE'), p[2])


  # [71]
  def p_TypeSuffixStartingWithArray(self, p):
    """TypeSuffixStartingWithArray : '[' ']' TypeSuffix
                                   | """
    if len(p) > 1:
      p[0] = self.BuildProduction('Array', p, 0, p[3])

  # [72]
  def p_Null(self, p):
    """Null : '?'
            |"""
    if len(p) > 1:
      p[0] = self.BuildTrue('NULLABLE')

  # [73]
  def p_ReturnType(self, p):
    """ReturnType : Type
                  | VOID"""
    if p[1] == 'void':
      p[0] = self.BuildProduction('Type', p, 1)
      p[0].AddChildren(self.BuildNamed('PrimitiveType', p, 1))
    else:
      p[0] = p[1]

  # [74]
  def p_ExtendedAttributeNoArgs(self, p):
    """ExtendedAttributeNoArgs : identifier"""
    p[0] = self.BuildNamed('ExtAttribute', p, 1)

  # [75]
  def p_ExtendedAttributeArgList(self, p):
    """ExtendedAttributeArgList : identifier '(' ArgumentList ')'"""
    arguments = self.BuildProduction('Arguments', p, 2, p[3])
    p[0] = self.BuildNamed('ExtAttribute', p, 1, arguments)

  # [76]
  def p_ExtendedAttributeIdent(self, p):
    """ExtendedAttributeIdent : identifier '=' identifier"""
    value = self.BuildAttribute('VALUE', p[3])
    p[0] = self.BuildNamed('ExtAttribute', p, 1, value)

  # [77]
  def p_ExtendedAttributeNamedArgList(self, p):
    """ExtendedAttributeNamedArgList : identifier '=' identifier '(' ArgumentList ')'"""
    args = self.BuildProduction('Arguments', p, 4, p[5])
    value = self.BuildNamed('Call', p, 3, args)
    p[0] = self.BuildNamed('ExtAttribute', p, 1, value)

#
# Parser Errors
#
# p_error is called whenever the parser can not find a pattern match for
# a set of items from the current state.  The p_error function defined here
# is triggered logging an error, and parsing recovery happens as the
# p_<type>_error functions defined above are called.  This allows the parser
# to continue so as to capture more than one error per file.
#
  def p_error(self, t):
    if t:
      lineno = t.lineno
      pos = t.lexpos
      prev = self.yaccobj.symstack[-1]
      if type(prev) == lex.LexToken:
        msg = "Unexpected %s after %s." % (
            TokenTypeName(t), TokenTypeName(prev))
      else:
        msg = "Unexpected %s." % (t.value)
    else:
      last = self.LastToken()
      lineno = last.lineno
      pos = last.lexpos
      msg = "Unexpected end of file after %s." % TokenTypeName(last)
      self.yaccobj.restart()

    # Attempt to remap the error to a friendlier form
    if msg in ERROR_REMAP:
      msg = ERROR_REMAP[msg]

    self._last_error_msg = msg
    self._last_error_lineno = lineno
    self._last_error_pos = pos

  def Warn(self, node, msg):
    sys.stdout.write(node.GetLogLine(msg))
    self.parse_warnings += 1

  def LastToken(self):
    return self.lexer.last

  def __init__(self, lexer, verbose=False, debug=False, mute_error=False):
    self.lexer = lexer
    self.tokens = lexer.KnownTokens()
    self.yaccobj = yacc.yacc(module=self, tabmodule=None, debug=False,
                             optimize=0, write_tables=0)
    self.parse_debug = debug
    self.verbose = verbose
    self.mute_error = mute_error
    self._parse_errors = 0
    self._parse_warnings = 0
    self._last_error_msg = None
    self._last_error_lineno = 0
    self._last_error_pos = 0


#
# BuildProduction
#
# Production is the set of items sent to a grammar rule resulting in a new
# item being returned.
#
# p - Is the Yacc production object containing the stack of items
# index - Index into the production of the name for the item being produced.
# cls - The type of item being producted
# childlist - The children of the new item
  def BuildProduction(self, cls, p, index, childlist=None):
    try:
      if not childlist:
        childlist = []

      filename = self.lexer.Lexer().filename
      lineno = p.lineno(index)
      pos = p.lexpos(index)
      out = IDLNode(cls, filename, lineno, pos, childlist)
      return out
    except:
      print 'Exception while parsing:'
      for num, item in enumerate(p):
        print '  [%d] %s' % (num, ExpandProduction(item))
      if self.LastToken():
        print 'Last token: %s' % str(self.LastToken())
      raise

  def BuildNamed(self, cls, p, index, childlist=None):
    childlist = ListFromConcat(childlist)
    childlist.append(self.BuildAttribute('NAME', p[index]))
    return self.BuildProduction(cls, p, index, childlist)

  def BuildComment(self, cls, p, index):
    name = p[index]

    # Remove comment markers
    lines = []
    if name[:2] == '//':
      # For C++ style, remove any leading whitespace and the '//' marker from
      # each line.
      form = 'cc'
      for line in name.split('\n'):
        start = line.find('//')
        lines.append(line[start+2:])
    else:
      # For C style, remove ending '*/''
      form = 'c'
      for line in name[:-2].split('\n'):
        # Remove characters until start marker for this line '*' if found
        # otherwise it should be blank.
        offs = line.find('*')
        if offs >= 0:
          line = line[offs + 1:].rstrip()
        else:
          line = ''
        lines.append(line)
    name = '\n'.join(lines)
    childlist = [self.BuildAttribute('NAME', name),
                 self.BuildAttribute('FORM', form)]
    return self.BuildProduction(cls, p, index, childlist)

#
# BuildError
#
# Build and Errror node as part of the recovery process.
#
#
  def BuildError(self, p, prod):
    self._parse_errors += 1
    name = self.BuildAttribute('NAME', self._last_error_msg)
    line = self.BuildAttribute('LINE', self._last_error_lineno)
    pos = self.BuildAttribute('POS', self._last_error_pos)
    prod = self.BuildAttribute('PROD', prod)

    node = self.BuildProduction('Error', p, 1,
                                ListFromConcat(name, line, pos, prod))
    if not self.mute_error:
      node.Error(self._last_error_msg)

    return node

#
# BuildAttribute
#
# An ExtendedAttribute is a special production that results in a property
# which is applied to the adjacent item.  Attributes have no children and
# instead represent key/value pairs.
#
  def BuildAttribute(self, key, val):
    return IDLAttribute(key, val)

  def BuildFalse(self, key):
    return IDLAttribute(key, Boolean(False))

  def BuildTrue(self, key):
    return IDLAttribute(key, Boolean(True))

  def GetErrors(self):
    return self._parse_errors + self.lexer._lex_errors

#
# ParseData
#
# Attempts to parse the current data loaded in the lexer.
#
  def ParseText(self, filename, data):
    self._parse_errors = 0
    self._parse_warnings = 0
    self._last_error_msg = None
    self._last_error_lineno = 0
    self._last_error_pos = 0

    try:
      self.lexer.Tokenize(data, filename)
      nodes = self.yaccobj.parse(lexer=self.lexer) or []
      name = self.BuildAttribute('NAME', filename)
      return IDLNode('File', filename, 0, 0, nodes + [name])

    except lex.LexError as lexError:
      sys.stderr.write('Error in token: %s\n' % str(lexError))
    return None



def ParseFile(parser, filename):
  """Parse a file and return a File type of node."""
  with open(filename) as fileobject:
    try:
      out = parser.ParseText(filename, fileobject.read())
      out.SetProperty('DATETIME', time.ctime(os.path.getmtime(filename)))
      out.SetProperty('ERRORS', parser.GetErrors())
      return out

    except Exception as e:
      last = parser.LastToken()
      sys.stderr.write('%s(%d) : Internal parsing error\n\t%s.\n' % (
                       filename, last.lineno, str(e)))


def main(argv):
  nodes = []
  parser = IDLParser(IDLLexer())
  errors = 0
  for filename in argv:
    filenode = ParseFile(parser, filename)
    if (filenode):
      errors += filenode.GetProperty('ERRORS')
      nodes.append(filenode)

  ast = IDLNode('AST', '__AST__', 0, 0, nodes)

  print '\n'.join(ast.Tree(accept_props=['PROD']))
  if errors:
    print '\nFound %d errors.\n' % errors

  return errors


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
