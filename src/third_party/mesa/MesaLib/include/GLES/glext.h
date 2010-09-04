#ifndef __glext_h_
#define __glext_h_


#ifdef __cplusplus
extern "C" {
#endif

/*
** License Applicability. Except to the extent portions of this file are
** made subject to an alternative license as permitted in the SGI Free
** Software License B, Version 1.0 (the "License"), the contents of this
** file are subject only to the provisions of the License. You may not use
** this file except in compliance with the License. You may obtain a copy
** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
**
** http://oss.sgi.com/projects/FreeB
**
** Note that, as provided in the License, the Software is distributed on an
** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
**
** Original Code. The Original Code is: OpenGL Sample Implementation,
** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
** Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
** Copyright in any portions created by third parties is as indicated
** elsewhere herein. All Rights Reserved.
**
** Additional Notice Provisions: The application programming interfaces
** established by SGI in conjunction with the Original Code are The
** OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
** April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
** 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
** Window System(R) (Version 1.3), released October 19, 1998. This software
** was created using the OpenGL(R) version 1.2.1 Sample Implementation
** published by SGI, but has not been independently verified as being
** compliant with the OpenGL(R) version 1.2.1 Specification.
*/

#ifndef GL_APIENTRYP
#   define GL_APIENTRYP GL_APIENTRY*
#endif

/*------------------------------------------------------------------------*
 * OES extension tokens
 *------------------------------------------------------------------------*/

/* GL_OES_blend_equation_separate */
#ifndef GL_OES_blend_equation_separate
/* BLEND_EQUATION_RGB_OES same as BLEND_EQUATION_OES */
#define GL_BLEND_EQUATION_RGB_OES                               0x8009
#define GL_BLEND_EQUATION_ALPHA_OES                             0x883D
#endif

/* GL_OES_blend_func_separate */
#ifndef GL_OES_blend_func_separate
#define GL_BLEND_DST_RGB_OES                                    0x80C8
#define GL_BLEND_SRC_RGB_OES                                    0x80C9
#define GL_BLEND_DST_ALPHA_OES                                  0x80CA
#define GL_BLEND_SRC_ALPHA_OES                                  0x80CB
#endif

/* GL_OES_blend_subtract */
#ifndef GL_OES_blend_subtract
#define GL_BLEND_EQUATION_OES                                   0x8009
#define GL_FUNC_ADD_OES                                         0x8006
#define GL_FUNC_SUBTRACT_OES                                    0x800A
#define GL_FUNC_REVERSE_SUBTRACT_OES                            0x800B
#endif

/* GL_OES_compressed_ETC1_RGB8_texture */
#ifndef GL_OES_compressed_ETC1_RGB8_texture
#define GL_ETC1_RGB8_OES                                        0x8D64
#endif

/* OES_draw_texture */
#ifndef GL_OES_draw_texture
#define GL_TEXTURE_CROP_RECT_OES                                0x8B9D
#endif

/* OES_fixed_point */
#ifndef GL_OES_fixed_point
#define GL_FIXED_OES                                            0x140C
#endif

/* OES_framebuffer_object */
#ifndef GL_OES_framebuffer_object
#define GL_NONE_OES                                             0
#define GL_FRAMEBUFFER_OES                                      0x8D40
#define GL_RENDERBUFFER_OES                                     0x8D41
#define GL_RGBA4_OES                                            0x8056
#define GL_RGB5_A1_OES                                          0x8057
#define GL_RGB565_OES                                           0x8D62
#define GL_DEPTH_COMPONENT16_OES                                0x81A5
#define GL_RENDERBUFFER_WIDTH_OES                               0x8D42
#define GL_RENDERBUFFER_HEIGHT_OES                              0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT_OES                     0x8D44
#define GL_RENDERBUFFER_RED_SIZE_OES                            0x8D50
#define GL_RENDERBUFFER_GREEN_SIZE_OES                          0x8D51
#define GL_RENDERBUFFER_BLUE_SIZE_OES                           0x8D52
#define GL_RENDERBUFFER_ALPHA_SIZE_OES                          0x8D53
#define GL_RENDERBUFFER_DEPTH_SIZE_OES                          0x8D54
#define GL_RENDERBUFFER_STENCIL_SIZE_OES                        0x8D55
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_OES               0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_OES               0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_OES             0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_OES     0x8CD3
#define GL_COLOR_ATTACHMENT0_OES                                0x8CE0
#define GL_DEPTH_ATTACHMENT_OES                                 0x8D00
#define GL_STENCIL_ATTACHMENT_OES                               0x8D20
#define GL_FRAMEBUFFER_COMPLETE_OES                             0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES                0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_OES        0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_OES                0x8CD9
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS_OES                   0x8CDA
#define GL_FRAMEBUFFER_UNSUPPORTED_OES                          0x8CDD
#define GL_FRAMEBUFFER_BINDING_OES                              0x8CA6
#define GL_RENDERBUFFER_BINDING_OES                             0x8CA7
#define GL_MAX_RENDERBUFFER_SIZE_OES                            0x84E8
#define GL_INVALID_FRAMEBUFFER_OPERATION_OES                    0x0506
#endif

/* OES_matrix_get */
#ifndef GL_OES_matrix_get
#define GL_MODELVIEW_MATRIX_FLOAT_AS_INT_BITS_OES               0x898D
#define GL_PROJECTION_MATRIX_FLOAT_AS_INT_BITS_OES              0x898E
#define GL_TEXTURE_MATRIX_FLOAT_AS_INT_BITS_OES                 0x898F
#endif

/* OES_matrix_palette */
#ifndef GL_OES_matrix_palette
#define GL_MAX_VERTEX_UNITS_OES                                 0x86A4
#define GL_MAX_PALETTE_MATRICES_OES                             0x8842
#define GL_MATRIX_PALETTE_OES                                   0x8840
#define GL_MATRIX_INDEX_ARRAY_OES                               0x8844
#define GL_WEIGHT_ARRAY_OES                                     0x86AD
#define GL_CURRENT_PALETTE_MATRIX_OES                           0x8843
#define GL_MATRIX_INDEX_ARRAY_SIZE_OES                          0x8846
#define GL_MATRIX_INDEX_ARRAY_TYPE_OES                          0x8847
#define GL_MATRIX_INDEX_ARRAY_STRIDE_OES                        0x8848
#define GL_MATRIX_INDEX_ARRAY_POINTER_OES                       0x8849
#define GL_MATRIX_INDEX_ARRAY_BUFFER_BINDING_OES                0x8B9E
#define GL_WEIGHT_ARRAY_SIZE_OES                                0x86AB
#define GL_WEIGHT_ARRAY_TYPE_OES                                0x86A9
#define GL_WEIGHT_ARRAY_STRIDE_OES                              0x86AA
#define GL_WEIGHT_ARRAY_POINTER_OES                             0x86AC
#define GL_WEIGHT_ARRAY_BUFFER_BINDING_OES                      0x889E
#endif

/* GL_OES_stencil_wrap */
#ifndef GL_OES_stencil_wrap
#define GL_INCR_WRAP_OES                                        0x8507
#define GL_DECR_WRAP_OES                                        0x8508
#endif

/* GL_OES_texture_cube_map */
#ifndef GL_OES_texture_cube_map
#define GL_NORMAL_MAP_OES                                       0x8511
#define GL_REFLECTION_MAP_OES                                   0x8512
#define GL_TEXTURE_CUBE_MAP_OES                                 0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP_OES                         0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_OES                      0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_OES                      0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_OES                      0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_OES                      0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_OES                      0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_OES                      0x851A
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_OES                        0x851C
#define GL_TEXTURE_GEN_MODE_OES                                 0x2500
#define GL_TEXTURE_GEN_STR_OES                                  0x8D60
#endif

/* GL_OES_texture_mirrored_repeat */
#ifndef GL_OES_texture_mirrored_repeat
#define GL_MIRRORED_REPEAT_OES                                  0x8370
#endif

/* GL_OES_EGL_image */
#ifndef GL_OES_EGL_image
typedef void* GLeglImageOES;
#endif

/* GL_OES_depth24 */
#ifndef GL_OES_depth24
#define GL_DEPTH_COMPONENT24_OES                                0x81A6
#endif

/* GL_OES_depth32 */
#ifndef GL_OES_depth32
#define GL_DEPTH_COMPONENT32_OES                                0x81A7
#endif

/* GL_OES_mapbuffer */
#ifndef GL_OES_mapbuffer
#define GL_WRITE_ONLY_OES                                       0x88B9
#define GL_BUFFER_ACCESS_OES                                    0x88BB
#define GL_BUFFER_MAPPED_OES                                    0x88BC
#define GL_BUFFER_MAP_POINTER_OES                               0x88BD
#endif

/* GL_OES_rgb8_rgba8 */
#ifndef GL_OES_rgb8_rgba8
#define GL_RGB8_OES                                             0x8051
#define GL_RGBA8_OES                                            0x8058
#endif

/* GL_OES_stencil1 */
#ifndef GL_OES_stencil1
#define GL_STENCIL_INDEX1_OES                                   0x8D46
#endif

/* GL_OES_stencil4 */
#ifndef GL_OES_stencil4
#define GL_STENCIL_INDEX4_OES                                   0x8D47
#endif

/* GL_OES_stencil8 */
#ifndef GL_OES_stencil8
#define GL_STENCIL_INDEX8_OES                                   0x8D48
#endif

/* GL_AMD_compressed_3DC_texture */
#ifndef GL_AMD_compressed_3DC_texture
#define GL_3DC_X_AMD                                            0x87F9
#define GL_3DC_XY_AMD                                           0x87FA
#endif

/* GL_AMD_compressed_ATC_texture */
#ifndef GL_AMD_compressed_ATC_texture
#define GL_ATC_RGB_AMD                                          0x8C92
#define GL_ATC_RGBA_EXPLICIT_ALPHA_AMD                          0x8C93
#define GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD                      0x87EE
#endif

/* GL_EXT_texture_filter_anisotropic */
#ifndef GL_EXT_texture_filter_anisotropic
#define GL_TEXTURE_MAX_ANISOTROPY_EXT                           0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT                       0x84FF
#endif

/*------------------------------------------------------------------------*
 * OES extension functions
 *------------------------------------------------------------------------*/

/* GL_OES_blend_equation_separate */
#ifndef GL_OES_blend_equation_separate
#define GL_OES_blend_equation_separate 1
#ifdef GL_GLEXT_PROTOTYPES
GL_API void GL_APIENTRY glBlendEquationSeparateOES (GLenum modeRGB, GLenum modeAlpha);
#endif
typedef void (GL_APIENTRYP PFNGLBLENDEQUATIONSEPARATEOESPROC) (GLenum modeRGB, GLenum modeAlpha);
#endif

/* GL_OES_blend_func_separate */
#ifndef GL_OES_blend_func_separate
#define GL_OES_blend_func_separate 1
#ifdef GL_GLEXT_PROTOTYPES
GL_API void GL_APIENTRY glBlendFuncSeparateOES (GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
#endif
typedef void (GL_APIENTRYP PFNGLBLENDFUNCSEPARATEOESPROC) (GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
#endif

/* GL_OES_blend_subtract */
#ifndef GL_OES_blend_subtract
#define GL_OES_blend_subtract 1
#ifdef GL_GLEXT_PROTOTYPES
GL_API void GL_APIENTRY glBlendEquationOES (GLenum mode);
#endif
typedef void (GL_APIENTRYP PFNGLBLENDEQUATIONOESPROC) (GLenum mode);
#endif

/* GL_OES_byte_coordinates */
#ifndef GL_OES_byte_coordinates
#define GL_OES_byte_coordinates 1
#endif

/* GL_OES_compressed_ETC1_RGB8_texture */
#ifndef GL_OES_compressed_ETC1_RGB8_texture
#define GL_OES_compressed_ETC1_RGB8_texture 1
#endif

/* GL_OES_draw_texture */
#ifndef GL_OES_draw_texture
#define GL_OES_draw_texture 1
#ifdef GL_GLEXT_PROTOTYPES
GL_API void GL_APIENTRY glDrawTexsOES (GLshort x, GLshort y, GLshort z, GLshort width, GLshort height);
GL_API void GL_APIENTRY glDrawTexiOES (GLint x, GLint y, GLint z, GLint width, GLint height);
GL_API void GL_APIENTRY glDrawTexxOES (GLfixed x, GLfixed y, GLfixed z, GLfixed width, GLfixed height);
GL_API void GL_APIENTRY glDrawTexsvOES (const GLshort *coords);
GL_API void GL_APIENTRY glDrawTexivOES (const GLint *coords);
GL_API void GL_APIENTRY glDrawTexxvOES (const GLfixed *coords);
GL_API void GL_APIENTRY glDrawTexfOES (GLfloat x, GLfloat y, GLfloat z, GLfloat width, GLfloat height);
GL_API void GL_APIENTRY glDrawTexfvOES (const GLfloat *coords);
#endif
typedef void (GL_APIENTRYP PFNGLDRAWTEXSOESPROC) (GLshort x, GLshort y, GLshort z, GLshort width, GLshort height);
typedef void (GL_APIENTRYP PFNGLDRAWTEXIOESPROC) (GLint x, GLint y, GLint z, GLint width, GLint height);
typedef void (GL_APIENTRYP PFNGLDRAWTEXXOESPROC) (GLfixed x, GLfixed y, GLfixed z, GLfixed width, GLfixed height);
typedef void (GL_APIENTRYP PFNGLDRAWTEXSVOESPROC) (const GLshort *coords);
typedef void (GL_APIENTRYP PFNGLDRAWTEXIVOESPROC) (const GLint *coords);
typedef void (GL_APIENTRYP PFNGLDRAWTEXXVOESPROC) (const GLfixed *coords);
typedef void (GL_APIENTRYP PFNGLDRAWTEXFOESPROC) (GLfloat x, GLfloat y, GLfloat z, GLfloat width, GLfloat height);
typedef void (GL_APIENTRYP PFNGLDRAWTEXFVOESPROC) (const GLfloat *coords);
#endif

/* GL_OES_extended_matrix_palette */
#ifndef GL_OES_extended_matrix_palette
#define GL_OES_extended_matrix_palette 1
#endif

/* GL_OES_fixed_point */
#ifndef GL_OES_fixed_point
#define GL_OES_fixed_point 1
#ifdef GL_GLEXT_PROTOTYPES
GL_API void GL_APIENTRY glAlphaFuncxOES (GLenum func, GLclampx ref);
GL_API void GL_APIENTRY glClearColorxOES (GLclampx red, GLclampx green, GLclampx blue, GLclampx alpha);
GL_API void GL_APIENTRY glClearDepthxOES (GLclampx depth);
GL_API void GL_APIENTRY glClipPlanexOES (GLenum plane, const GLfixed *equation);
GL_API void GL_APIENTRY glColor4xOES (GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha);
GL_API void GL_APIENTRY glDepthRangexOES (GLclampx zNear, GLclampx zFar);
GL_API void GL_APIENTRY glFogxOES (GLenum pname, GLfixed param);
GL_API void GL_APIENTRY glFogxvOES (GLenum pname, const GLfixed *params);
GL_API void GL_APIENTRY glFrustumxOES (GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar);
GL_API void GL_APIENTRY glGetClipPlanexOES (GLenum pname, GLfixed eqn[4]);
GL_API void GL_APIENTRY glGetFixedvOES (GLenum pname, GLfixed *params);
GL_API void GL_APIENTRY glGetLightxvOES (GLenum light, GLenum pname, GLfixed *params);
GL_API void GL_APIENTRY glGetMaterialxvOES (GLenum face, GLenum pname, GLfixed *params);
GL_API void GL_APIENTRY glGetTexEnvxvOES (GLenum env, GLenum pname, GLfixed *params);
GL_API void GL_APIENTRY glGetTexParameterxvOES (GLenum target, GLenum pname, GLfixed *params);
GL_API void GL_APIENTRY glLightModelxOES (GLenum pname, GLfixed param);
GL_API void GL_APIENTRY glLightModelxvOES (GLenum pname, const GLfixed *params);
GL_API void GL_APIENTRY glLightxOES (GLenum light, GLenum pname, GLfixed param);
GL_API void GL_APIENTRY glLightxvOES (GLenum light, GLenum pname, const GLfixed *params);
GL_API void GL_APIENTRY glLineWidthxOES (GLfixed width);
GL_API void GL_APIENTRY glLoadMatrixxOES (const GLfixed *m);
GL_API void GL_APIENTRY glMaterialxOES (GLenum face, GLenum pname, GLfixed param);
GL_API void GL_APIENTRY glMaterialxvOES (GLenum face, GLenum pname, const GLfixed *params);
GL_API void GL_APIENTRY glMultMatrixxOES (const GLfixed *m);
GL_API void GL_APIENTRY glMultiTexCoord4xOES (GLenum target, GLfixed s, GLfixed t, GLfixed r, GLfixed q);
GL_API void GL_APIENTRY glNormal3xOES (GLfixed nx, GLfixed ny, GLfixed nz);
GL_API void GL_APIENTRY glOrthoxOES (GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar);
GL_API void GL_APIENTRY glPointParameterxOES (GLenum pname, GLfixed param);
GL_API void GL_APIENTRY glPointParameterxvOES (GLenum pname, const GLfixed *params);
GL_API void GL_APIENTRY glPointSizexOES (GLfixed size);
GL_API void GL_APIENTRY glPolygonOffsetxOES (GLfixed factor, GLfixed units);
GL_API void GL_APIENTRY glRotatexOES (GLfixed angle, GLfixed x, GLfixed y, GLfixed z);
GL_API void GL_APIENTRY glSampleCoveragexOES (GLclampx value, GLboolean invert);
GL_API void GL_APIENTRY glScalexOES (GLfixed x, GLfixed y, GLfixed z);
GL_API void GL_APIENTRY glTexEnvxOES (GLenum target, GLenum pname, GLfixed param);
GL_API void GL_APIENTRY glTexEnvxvOES (GLenum target, GLenum pname, const GLfixed *params);
GL_API void GL_APIENTRY glTexParameterxOES (GLenum target, GLenum pname, GLfixed param);
GL_API void GL_APIENTRY glTexParameterxvOES (GLenum target, GLenum pname, const GLfixed *params);
GL_API void GL_APIENTRY glTranslatexOES (GLfixed x, GLfixed y, GLfixed z);
#endif
typedef void (GL_APIENTRYP PFNGLALPHAFUNCXOESPROC) (GLenum func, GLclampx ref);
typedef void (GL_APIENTRYP PFNGLCLEARCOLORXOESPROC) (GLclampx red, GLclampx green, GLclampx blue, GLclampx alpha);
typedef void (GL_APIENTRYP PFNGLCLEARDEPTHXOESPROC) (GLclampx depth);
typedef void (GL_APIENTRYP PFNGLCLIPPLANEXOESPROC) (GLenum plane, const GLfixed *equation);
typedef void (GL_APIENTRYP PFNGLCOLOR4XOESPROC) (GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha);
typedef void (GL_APIENTRYP PFNGLDEPTHRANGEXOESPROC) (GLclampx zNear, GLclampx zFar);
typedef void (GL_APIENTRYP PFNGLFOGXOESPROC) (GLenum pname, GLfixed param);
typedef void (GL_APIENTRYP PFNGLFOGXVOESPROC) (GLenum pname, const GLfixed *params);
typedef void (GL_APIENTRYP PFNGLFRUSTUMXOESPROC) (GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar);
typedef void (GL_APIENTRYP PFNGLGETCLIPPLANEXOESPROC) (GLenum pname, GLfixed eqn[4]);
typedef void (GL_APIENTRYP PFNGLGETFIXEDVOESPROC) (GLenum pname, GLfixed *params);
typedef void (GL_APIENTRYP PFNGLGETLIGHTXVOESPROC) (GLenum light, GLenum pname, GLfixed *params);
typedef void (GL_APIENTRYP PFNGLGETMATERIALXVOESPROC) (GLenum face, GLenum pname, GLfixed *params);
typedef void (GL_APIENTRYP PFNGLGETTEXENVXVOESPROC) (GLenum env, GLenum pname, GLfixed *params);
typedef void (GL_APIENTRYP PFNGLGETTEXPARAMETERXVOESPROC) (GLenum target, GLenum pname, GLfixed *params);
typedef void (GL_APIENTRYP PFNGLLIGHTMODELXOESPROC) (GLenum pname, GLfixed param);
typedef void (GL_APIENTRYP PFNGLLIGHTMODELXVOESPROC) (GLenum pname, const GLfixed *params);
typedef void (GL_APIENTRYP PFNGLLIGHTXOESPROC) (GLenum light, GLenum pname, GLfixed param);
typedef void (GL_APIENTRYP PFNGLLIGHTXVOESPROC) (GLenum light, GLenum pname, const GLfixed *params);
typedef void (GL_APIENTRYP PFNGLLINEWIDTHXOESPROC) (GLfixed width);
typedef void (GL_APIENTRYP PFNGLLOADMATRIXXOESPROC) (const GLfixed *m);
typedef void (GL_APIENTRYP PFNGLMATERIALXOESPROC) (GLenum face, GLenum pname, GLfixed param);
typedef void (GL_APIENTRYP PFNGLMATERIALXVOESPROC) (GLenum face, GLenum pname, const GLfixed *params);
typedef void (GL_APIENTRYP PFNGLMULTMATRIXXOESPROC) (const GLfixed *m);
typedef void (GL_APIENTRYP PFNGLMULTITEXCOORD4XOESPROC) (GLenum target, GLfixed s, GLfixed t, GLfixed r, GLfixed q);
typedef void (GL_APIENTRYP PFNGLNORMAL3XOESPROC) (GLfixed nx, GLfixed ny, GLfixed nz);
typedef void (GL_APIENTRYP PFNGLORTHOXOESPROC) (GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar);
typedef void (GL_APIENTRYP PFNGLPOINTPARAMETERXOESPROC) (GLenum pname, GLfixed param);
typedef void (GL_APIENTRYP PFNGLPOINTPARAMETERXVOESPROC) (GLenum pname, const GLfixed *params);
typedef void (GL_APIENTRYP PFNGLPOINTSIZEXOESPROC) (GLfixed size);
typedef void (GL_APIENTRYP PFNGLPOLYGONOFFSETXOESPROC) (GLfixed factor, GLfixed units);
typedef void (GL_APIENTRYP PFNGLROTATEXOESPROC) (GLfixed angle, GLfixed x, GLfixed y, GLfixed z);
typedef void (GL_APIENTRYP PFNGLSAMPLECOVERAGEXOESPROC) (GLclampx value, GLboolean invert);
typedef void (GL_APIENTRYP PFNGLSCALEXOESPROC) (GLfixed x, GLfixed y, GLfixed z);
typedef void (GL_APIENTRYP PFNGLTEXENVXOESPROC) (GLenum target, GLenum pname, GLfixed param);
typedef void (GL_APIENTRYP PFNGLTEXENVXVOESPROC) (GLenum target, GLenum pname, const GLfixed *params);
typedef void (GL_APIENTRYP PFNGLTEXPARAMETERXOESPROC) (GLenum target, GLenum pname, GLfixed param);
typedef void (GL_APIENTRYP PFNGLTEXPARAMETERXVOESPROC) (GLenum target, GLenum pname, const GLfixed *params);
typedef void (GL_APIENTRYP PFNGLTRANSLATEXOESPROC) (GLfixed x, GLfixed y, GLfixed z);
#endif

/* GL_OES_framebuffer_object */
#ifndef GL_OES_framebuffer_object
#define GL_OES_framebuffer_object 1
#ifdef GL_GLEXT_PROTOTYPES
GL_API GLboolean GL_APIENTRY glIsRenderbufferOES (GLuint renderbuffer);
GL_API void GL_APIENTRY glBindRenderbufferOES (GLenum target, GLuint renderbuffer);
GL_API void GL_APIENTRY glDeleteRenderbuffersOES (GLsizei n, const GLuint* renderbuffers);
GL_API void GL_APIENTRY glGenRenderbuffersOES (GLsizei n, GLuint* renderbuffers);
GL_API void GL_APIENTRY glRenderbufferStorageOES (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
GL_API void GL_APIENTRY glGetRenderbufferParameterivOES (GLenum target, GLenum pname, GLint* params);
GL_API GLboolean GL_APIENTRY glIsFramebufferOES (GLuint framebuffer);
GL_API void GL_APIENTRY glBindFramebufferOES (GLenum target, GLuint framebuffer);
GL_API void GL_APIENTRY glDeleteFramebuffersOES (GLsizei n, const GLuint* framebuffers);
GL_API void GL_APIENTRY glGenFramebuffersOES (GLsizei n, GLuint* framebuffers);
GL_API GLenum GL_APIENTRY glCheckFramebufferStatusOES (GLenum target);
GL_API void GL_APIENTRY glFramebufferRenderbufferOES (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
GL_API void GL_APIENTRY glFramebufferTexture2DOES (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
GL_API void GL_APIENTRY glGetFramebufferAttachmentParameterivOES (GLenum target, GLenum attachment, GLenum pname, GLint* params);
GL_API void GL_APIENTRY glGenerateMipmapOES (GLenum target);
#endif
typedef GLboolean (GL_APIENTRYP PFNGLISRENDERBUFFEROESPROC) (GLuint renderbuffer);
typedef void (GL_APIENTRYP PFNGLBINDRENDERBUFFEROESPROC) (GLenum target, GLuint renderbuffer);
typedef void (GL_APIENTRYP PFNGLDELETERENDERBUFFERSOESPROC) (GLsizei n, const GLuint* renderbuffers);
typedef void (GL_APIENTRYP PFNGLGENRENDERBUFFERSOESPROC) (GLsizei n, GLuint* renderbuffers);
typedef void (GL_APIENTRYP PFNGLRENDERBUFFERSTORAGEOESPROC) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (GL_APIENTRYP PFNGLGETRENDERBUFFERPARAMETERIVOESPROC) (GLenum target, GLenum pname, GLint* params);
typedef GLboolean (GL_APIENTRYP PFNGLISFRAMEBUFFEROESPROC) (GLuint framebuffer);
typedef void (GL_APIENTRYP PFNGLBINDFRAMEBUFFEROESPROC) (GLenum target, GLuint framebuffer);
typedef void (GL_APIENTRYP PFNGLDELETEFRAMEBUFFERSOESPROC) (GLsizei n, const GLuint* framebuffers);
typedef void (GL_APIENTRYP PFNGLGENFRAMEBUFFERSOESPROC) (GLsizei n, GLuint* framebuffers);
typedef GLenum (GL_APIENTRYP PFNGLCHECKFRAMEBUFFERSTATUSOESPROC) (GLenum target);
typedef void (GL_APIENTRYP PFNGLFRAMEBUFFERRENDERBUFFEROESPROC) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
typedef void (GL_APIENTRYP PFNGLFRAMEBUFFERTEXTURE2DOESPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (GL_APIENTRYP PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVOESPROC) (GLenum target, GLenum attachment, GLenum pname, GLint* params);
typedef void (GL_APIENTRYP PFNGLGENERATEMIPMAPOESPROC) (GLenum target);
#endif

/* GL_OES_matrix_get */
#ifndef GL_OES_matrix_get
#define GL_OES_matrix_get 1
#endif

/* GL_OES_matrix_palette */
#ifndef GL_OES_matrix_palette
#define GL_OES_matrix_palette 1
#ifdef GL_GLEXT_PROTOTYPES
GL_API void GL_APIENTRY glCurrentPaletteMatrixOES (GLuint matrixpaletteindex);
GL_API void GL_APIENTRY glLoadPaletteFromModelViewMatrixOES (void);
GL_API void GL_APIENTRY glMatrixIndexPointerOES (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GL_API void GL_APIENTRY glWeightPointerOES (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
#endif
typedef void (GL_APIENTRYP PFNGLCURRENTPALETTEMATRIXOESPROC) (GLuint matrixpaletteindex);
typedef void (GL_APIENTRYP PFNGLLOADPALETTEFROMMODELVIEWMATRIXOESPROC) (void);
typedef void (GL_APIENTRYP PFNGLMATRIXINDEXPOINTEROESPROC) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
typedef void (GL_APIENTRYP PFNGLWEIGHTPOINTEROESPROC) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
#endif

/* GL_OES_query_matrix */
#ifndef GL_OES_query_matrix
#define GL_OES_query_matrix 1
#ifdef GL_GLEXT_PROTOTYPES
GL_API GLbitfield GL_APIENTRY glQueryMatrixxOES (GLfixed mantissa[16], GLint exponent[16]);
#endif
typedef GLbitfield (GL_APIENTRYP PFNGLQUERYMATRIXXOESPROC) (GLfixed mantissa[16], GLint exponent[16]);
#endif

/* GL_OES_single_precision */
#ifndef GL_OES_single_precision
#define GL_OES_single_precision 1
#ifdef GL_GLEXT_PROTOTYPES
GL_API void GL_APIENTRY glDepthRangefOES (GLclampf zNear, GLclampf zFar);
GL_API void GL_APIENTRY glFrustumfOES (GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);
GL_API void GL_APIENTRY glOrthofOES (GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);
GL_API void GL_APIENTRY glClipPlanefOES (GLenum plane, const GLfloat *equation);
GL_API void GL_APIENTRY glGetClipPlanefOES (GLenum pname, GLfloat eqn[4]);
GL_API void GL_APIENTRY glClearDepthfOES (GLclampf depth);
#endif
typedef void (GL_APIENTRYP PFNGLDEPTHRANGEFOESPROC) (GLclampf zNear, GLclampf zFar);
typedef void (GL_APIENTRYP PFNGLFRUSTUMFOESPROC) (GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);
typedef void (GL_APIENTRYP PFNGLORTHOFOESPROC) (GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);
typedef void (GL_APIENTRYP PFNGLCLIPPLANEFOESPROC) (GLenum plane, const GLfloat *equation);
typedef void (GL_APIENTRYP PFNGLGETCLIPPLANEFOESPROC) (GLenum pname, GLfloat eqn[4]);
typedef void (GL_APIENTRYP PFNGLCLEARDEPTHFOESPROC) (GLclampf depth);
#endif

/* GL_OES_stencil_wrap */
#ifndef GL_OES_stencil_wrap
#define GL_OES_stencil_wrap 1
#endif

/* GL_OES_texture_cube_map */
#ifndef GL_OES_texture_cube_map
#define GL_OES_texture_cube_map 1
#ifdef GL_GLEXT_PROTOTYPES
GL_API void GL_APIENTRY glTexGenfOES (GLenum coord, GLenum pname, GLfloat param);
GL_API void GL_APIENTRY glTexGenfvOES (GLenum coord, GLenum pname, const GLfloat *params);
GL_API void GL_APIENTRY glTexGeniOES (GLenum coord, GLenum pname, GLint param);
GL_API void GL_APIENTRY glTexGenivOES (GLenum coord, GLenum pname, const GLint *params);
GL_API void GL_APIENTRY glTexGenxOES (GLenum coord, GLenum pname, GLfixed param);
GL_API void GL_APIENTRY glTexGenxvOES (GLenum coord, GLenum pname, const GLfixed *params);
GL_API void GL_APIENTRY glGetTexGenfvOES (GLenum coord, GLenum pname, GLfloat *params);
GL_API void GL_APIENTRY glGetTexGenivOES (GLenum coord, GLenum pname, GLint *params);
GL_API void GL_APIENTRY glGetTexGenxvOES (GLenum coord, GLenum pname, GLfixed *params);
#endif
typedef void (GL_APIENTRYP PFNGLTEXGENFOESPROC) (GLenum coord, GLenum pname, GLfloat param);
typedef void (GL_APIENTRYP PFNGLTEXGENFVOESPROC) (GLenum coord, GLenum pname, const GLfloat *params);
typedef void (GL_APIENTRYP PFNGLTEXGENIOESPROC) (GLenum coord, GLenum pname, GLint param);
typedef void (GL_APIENTRYP PFNGLTEXGENIVOESPROC) (GLenum coord, GLenum pname, const GLint *params);
typedef void (GL_APIENTRYP PFNGLTEXGENXOESPROC) (GLenum coord, GLenum pname, GLfixed param);
typedef void (GL_APIENTRYP PFNGLTEXGENXVOESPROC) (GLenum coord, GLenum pname, const GLfixed *params);
typedef void (GL_APIENTRYP PFNGLGETTEXGENFVOESPROC) (GLenum coord, GLenum pname, GLfloat *params);
typedef void (GL_APIENTRYP PFNGLGETTEXGENIVOESPROC) (GLenum coord, GLenum pname, GLint *params);
typedef void (GL_APIENTRYP PFNGLGETTEXGENXVOESPROC) (GLenum coord, GLenum pname, GLfixed *params);
#endif

/* GL_OES_texture_env_crossbar */
#ifndef GL_OES_texture_env_crossbar
#define GL_OES_texture_env_crossbar 1
#endif

/* GL_OES_texture_mirrored_repeat */
#ifndef GL_OES_texture_mirrored_repeat
#define GL_OES_texture_mirrored_repeat 1
#endif

/* GL_OES_EGL_image */
#ifndef GL_OES_EGL_image
#define GL_OES_EGL_image 1
#ifdef GL_GLEXT_PROTOTYPES
GL_API void GL_APIENTRY glEGLImageTargetTexture2DOES (GLenum target, GLeglImageOES image);
GL_API void GL_APIENTRY glEGLImageTargetRenderbufferStorageOES (GLenum target, GLeglImageOES image);
#endif
typedef void (GL_APIENTRYP PFNGLEGLIMAGETARGETTEXTURE2DOESPROC) (GLenum target, GLeglImageOES image);
typedef void (GL_APIENTRYP PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC) (GLenum target, GLeglImageOES image);
#endif

/* GL_OES_depth24 */
#ifndef GL_OES_depth24
#define GL_OES_depth24 1
#endif

/* GL_OES_depth32 */
#ifndef GL_OES_depth32
#define GL_OES_depth32 1
#endif

/* GL_OES_element_index_uint */
#ifndef GL_OES_element_index_uint
#define GL_OES_element_index_uint 1
#endif

/* GL_OES_fbo_render_mipmap */
#ifndef GL_OES_fbo_render_mipmap
#define GL_OES_fbo_render_mipmap 1
#endif

/* GL_OES_mapbuffer */
#ifndef GL_OES_mapbuffer
#define GL_OES_mapbuffer 1
#ifdef GL_GLEXT_PROTOTYPES
GL_API void* GL_APIENTRY glMapBufferOES (GLenum target, GLenum access);
GL_API GLboolean GL_APIENTRY glUnmapBufferOES (GLenum target);
GL_API void GL_APIENTRY glGetBufferPointervOES (GLenum target, GLenum pname, void** params);
#endif
typedef void* (GL_APIENTRYP PFNGLMAPBUFFEROESPROC) (GLenum target, GLenum access);
typedef GLboolean (GL_APIENTRYP PFNGLUNMAPBUFFEROESPROC) (GLenum target);
typedef void (GL_APIENTRYP PFNGLGETBUFFERPOINTERVOESPROC) (GLenum target, GLenum pname, void** params);
#endif

/* GL_OES_rgb8_rgba8 */
#ifndef GL_OES_rgb8_rgba8
#define GL_OES_rgb8_rgba8 1
#endif

/* GL_OES_stencil1 */
#ifndef GL_OES_stencil1
#define GL_OES_stencil1 1
#endif

/* GL_OES_stencil4 */
#ifndef GL_OES_stencil4
#define GL_OES_stencil4 1
#endif

/* GL_OES_stencil8 */
#ifndef GL_OES_stencil8
#define GL_OES_stencil8 1
#endif

/* GL_AMD_compressed_3DC_texture */
#ifndef GL_AMD_compressed_3DC_texture
#define GL_AMD_compressed_3DC_texture 1
#endif

/* GL_AMD_compressed_ATC_texture */
#ifndef GL_AMD_compressed_ATC_texture
#define GL_AMD_compressed_ATC_texture 1
#endif

/* GL_EXT_texture_filter_anisotropic */
#ifndef GL_EXT_texture_filter_anisotropic
#define GL_EXT_texture_filter_anisotropic 1
#endif

#ifdef __cplusplus
}
#endif

#endif /* __glext_h_ */

