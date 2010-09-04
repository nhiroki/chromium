/**
 * \file fog.h
 * Fog operations.
 * 
 * \if subset
 * (No-op)
 *
 * \endif
 */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef FOG_H
#define FOG_H


#include "mtypes.h"


#if _HAVE_FULL_GL

extern void GLAPIENTRY
_mesa_Fogf(GLenum pname, GLfloat param);

extern void GLAPIENTRY
_mesa_Fogi(GLenum pname, GLint param );

extern void GLAPIENTRY
_mesa_Fogfv(GLenum pname, const GLfloat *params );

extern void GLAPIENTRY
_mesa_Fogiv(GLenum pname, const GLint *params );

extern void _mesa_init_fog( GLcontext * ctx );

#else

/** No-op */
#define _mesa_init_fog( c ) ((void)0)

#endif

#endif
