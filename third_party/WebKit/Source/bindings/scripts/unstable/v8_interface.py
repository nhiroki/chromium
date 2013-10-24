# Copyright (C) 2013 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Generate template values for an interface.

FIXME: Not currently used in build.
This is a rewrite of the Perl IDL compiler in Python, but is not complete.
Once it is complete, we will switch all IDL files over to Python at once.
Until then, please work on the Perl IDL compiler.
For details, see bug http://crbug.com/239771
"""

import v8_attributes
from v8_globals import includes
from v8_utilities import cpp_name, runtime_enabled_function_name, v8_class_name


INTERFACE_H_INCLUDES = set([
    'bindings/v8/V8Binding.h',
    'bindings/v8/V8DOMWrapper.h',  # FIXME: necessary?
    'bindings/v8/WrapperTypeInfo.h',  # FIXME: necessary?
])
INTERFACE_CPP_INCLUDES = set([
    'RuntimeEnabledFeatures.h',
    'bindings/v8/ScriptController.h',
    'bindings/v8/V8Binding.h',
    'bindings/v8/V8DOMConfiguration.h',  # FIXME: necessary?
    'bindings/v8/V8DOMWrapper.h',  # FIXME: necessary?
    'core/dom/ContextFeatures.h',
    'core/dom/Document.h',
    'platform/TraceEvent.h',
    'wtf/UnusedParam.h',
])


def generate_interface(interface):
    includes.clear()
    includes.update(INTERFACE_CPP_INCLUDES)

    template_contents = {
        'interface_name': interface.name,
        'cpp_class_name': cpp_name(interface),
        'v8_class_name': v8_class_name(interface),
        'constants': [generate_constant(constant) for constant in interface.constants],
        'do_not_check_constants': 'DoNotCheckConstants' in interface.extended_attributes,
        'header_includes': INTERFACE_H_INCLUDES,
    }
    template_contents.update(v8_attributes.generate_attributes(interface))
    return template_contents


def generate_constant(constant):
    # Extended Attributes: DeprecateAs, RuntimeEnabled, Reflect
    # (Blink-only) string literals are unquoted in tokenizer, must be re-quoted
    # in C++.
    if constant.data_type == 'DOMString':
        value = '"%s"' % constant.value
    else:
        value = constant.value
    reflected_name = constant.extended_attributes.get('Reflect', constant.name)

    constant_parameter = {
        'name': constant.name,
        # FIXME: use 'reflected_name' as correct 'name'
        'reflected_name': reflected_name,
        'value': value,
        'runtime_enabled_function_name': runtime_enabled_function_name(constant),
    }
    return constant_parameter
