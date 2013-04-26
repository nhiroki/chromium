/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef InspectorCanvasInstrumentation_h
#define InspectorCanvasInstrumentation_h

#include "ScriptObject.h"
#include "core/inspector/InspectorCanvasAgent.h"
#include "core/inspector/InspectorInstrumentation.h"
#include "core/inspector/InstrumentingAgents.h"
#include <wtf/UnusedParam.h>

namespace WebCore {

namespace InspectorInstrumentation {

inline ScriptObject wrapCanvas2DRenderingContextForInstrumentation(Document* document, const ScriptObject& context)
{
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document)) {
        if (InspectorCanvasAgent* canvasAgent = instrumentingAgents->inspectorCanvasAgent())
            return canvasAgent->wrapCanvas2DRenderingContextForInstrumentation(context);
    }
    return ScriptObject();
}

inline ScriptObject wrapWebGLRenderingContextForInstrumentation(Document* document, const ScriptObject& glContext)
{
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForDocument(document)) {
        if (InspectorCanvasAgent* canvasAgent = instrumentingAgents->inspectorCanvasAgent())
            return canvasAgent->wrapWebGLRenderingContextForInstrumentation(glContext);
    }
    return ScriptObject();
}

} // namespace InspectorInstrumentation

} // namespace WebCore

#endif // !defined(InspectorCanvasInstrumentation_h)
