/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER “AS IS” AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"
#include "platform/graphics/filters/custom/CustomFilterGlobalContext.h"

#include "platform/graphics/GraphicsContext3D.h"
#include "platform/graphics/filters/custom/CustomFilterValidatedProgram.h"

namespace WebCore {

CustomFilterGlobalContext::CustomFilterGlobalContext()
{
}

CustomFilterGlobalContext::~CustomFilterGlobalContext()
{
    for (CustomFilterValidatedProgramsMap::iterator iter = m_programs.begin(); iter != m_programs.end(); ++iter)
        iter->value->detachFromGlobalContext();
}

ANGLEPlatformBridge* CustomFilterGlobalContext::webglShaderValidator()
{
    if (!m_webglShaderValidator)
        m_webglShaderValidator = createShaderValidator(SH_WEBGL_SPEC);
    return m_webglShaderValidator.get();
}

ANGLEPlatformBridge* CustomFilterGlobalContext::mixShaderValidator()
{
    if (!m_mixShaderValidator)
        m_mixShaderValidator = createShaderValidator(SH_CSS_SHADERS_SPEC);
    return m_mixShaderValidator.get();
}

PassOwnPtr<ANGLEPlatformBridge> CustomFilterGlobalContext::createShaderValidator(ShShaderSpec shaderSpec)
{
    OwnPtr<ANGLEPlatformBridge> validator = adoptPtr(new ANGLEPlatformBridge(SH_ESSL_OUTPUT, shaderSpec));
    ShBuiltInResources resources;
    ShInitBuiltInResources(&resources);
    validator->setResources(resources);
    return validator.release();
}

void CustomFilterGlobalContext::prepareContextIfNeeded()
{
    if (m_context.get())
        return;

    GraphicsContext3D::Attributes attributes;
    attributes.preserveDrawingBuffer = true;
    attributes.premultipliedAlpha = false;
    attributes.shareResources = true;
    attributes.preferDiscreteGPU = true;
    m_context = GraphicsContext3D::create(attributes);
    if (!m_context)
        return;
    m_context->makeContextCurrent();
    m_context->enable(GraphicsContext3D::DEPTH_TEST);
}

PassRefPtr<CustomFilterValidatedProgram> CustomFilterGlobalContext::getValidatedProgram(const CustomFilterProgramInfo& programInfo)
{
    CustomFilterValidatedProgramsMap::iterator iter = m_programs.find(programInfo);
    if (iter != m_programs.end())
        return iter->value;

    RefPtr<CustomFilterValidatedProgram> validatedProgram = CustomFilterValidatedProgram::create(this, programInfo);
    m_programs.set(programInfo, validatedProgram.get());
    return validatedProgram.release();
}

void CustomFilterGlobalContext::removeValidatedProgram(const CustomFilterValidatedProgram* program)
{
    CustomFilterValidatedProgramsMap::iterator iter = m_programs.find(program->programInfo());
    ASSERT_WITH_SECURITY_IMPLICATION(iter != m_programs.end());
    m_programs.remove(iter);

#ifndef NDEBUG
    // Check that there's no way we could have the same program under a different key.
    for (iter = m_programs.begin(); iter != m_programs.end(); ++iter)
        ASSERT(iter->value != program);
#endif
}

} // namespace WebCore
