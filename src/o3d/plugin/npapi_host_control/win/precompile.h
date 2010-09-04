/*
 * Copyright 2009, Google Inc.
 * All rights reserved.
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


// Precompiled header generated by Visual Studio 2008.

#ifndef O3D_PLUGIN_NPAPI_HOST_CONTROL_WIN_PRECOMPILE_H_
#define O3D_PLUGIN_NPAPI_HOST_CONTROL_WIN_PRECOMPILE_H_

#ifndef STRICT
#define STRICT
#endif

// Modify the following defines if you have to target a platform prior to the
// ones specified below.
// Refer to MSDN for the latest info on corresponding values for
// different platforms.
#ifndef WINVER  // Allow use of features specific to Windows XP or later.
#define WINVER 0x0501  // Change this to the appropriate value to target
                       // other versions of Windows.
#endif

#ifndef _WIN32_WINNT  // Allow use of features specific to Windows XP or later.
#define _WIN32_WINNT 0x0501  // Change this to the appropriate value to target
                             // other versions of Windows.
#endif

#ifndef _WIN32_WINDOWS  // Allow use of features specific to Windows 98 or
                        //  later.
#define _WIN32_WINDOWS 0x0410  // Change this to the appropriate value to
                               //  target Windows Me or later.
#endif

#ifndef _WIN32_IE  // Allow use of features specific to IE 6.0 or later.
#define _WIN32_IE 0x0600  // Change this to the appropriate value to target
                          //  other versions of IE.
#endif

#define _ATL_APARTMENT_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE

// Some CString constructors will be explicit.
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS


#include <atlbase.h>
#include <atlcom.h>
#include <atlstr.h>

#include "base/basictypes.h"
#include "plugin/npapi_host_control/win/resource.h"

using namespace ATL;

#endif  // O3D_PLUGIN_NPAPI_HOST_CONTROL_WIN_PRECOMPILE_H_
