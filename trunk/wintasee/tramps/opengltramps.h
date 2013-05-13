/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#ifndef OGLTRAMPS_H_INCL
#define OGLTRAMPS_H_INCL


typedef unsigned int	GLenum;
typedef unsigned char	GLboolean;
typedef unsigned int	GLbitfield;
typedef void		GLvoid;
typedef signed char	GLbyte;		// 1-byte signed 
typedef short		GLshort;	// 2-byte signed 
typedef int		GLint;			// 4-byte signed 
typedef unsigned char	GLubyte;	// 1-byte unsigned 
typedef unsigned short	GLushort;	// 2-byte unsigned 
typedef unsigned int	GLuint;		// 4-byte unsigned 
typedef int		GLsizei;		// 4-byte signed 
typedef float		GLfloat;	// single precision float 
typedef float		GLclampf;	// single precision float in [0,1] 
typedef double		GLdouble;	// double precision float 
typedef double		GLclampd;	// double precision float in [0,1] 

#define GLAPI WINAPI

#define glBindTexture TrampglBindTexture
#define glDeleteTextures TrampglDeleteTextures
#define glGenTextures TrampglGenTextures
#define glLineWidth TrampglLineWidth
#define glAccum TrampglAccum
#define glAlphaFunc TrampglAlphaFunc
#define glBegin TrampglBegin
#define glBitmap TrampglBitmap
#define glBlendFunc TrampglBlendFunc
#define glCallList TrampglCallList
#define glCallLists TrampglCallLists
#define glClear TrampglClear
#define glClearAccum TrampglClearAccum
#define glClearColor TrampglClearColor
#define glClearDepth TrampglClearDepth
#define glClearIndex TrampglClearIndex
#define glClearStencil TrampglClearStencil
#define glClipPlane TrampglClipPlane
#define glColor3b TrampglColor3b
#define glColor3bv TrampglColor3bv
#define glColor3d TrampglColor3d
#define glColor3dv TrampglColor3dv
#define glColor3f TrampglColor3f
#define glColor3fv TrampglColor3fv
#define glColor3i TrampglColor3i
#define glColor3iv TrampglColor3iv
#define glColor3s TrampglColor3s
#define glColor3sv TrampglColor3sv
#define glColor3ub TrampglColor3ub
#define glColor3ubv TrampglColor3ubv
#define glColor3ui TrampglColor3ui
#define glColor3uiv TrampglColor3uiv
#define glColor3us TrampglColor3us
#define glColor3usv TrampglColor3usv
#define glColor4b TrampglColor4b
#define glColor4bv TrampglColor4bv
#define glColor4d TrampglColor4d
#define glColor4dv TrampglColor4dv
#define glColor4f TrampglColor4f
#define glColor4fv TrampglColor4fv
#define glColor4i TrampglColor4i
#define glColor4iv TrampglColor4iv
#define glColor4s TrampglColor4s
#define glColor4sv TrampglColor4sv
#define glColor4ub TrampglColor4ub
#define glColor4ubv TrampglColor4ubv
#define glColor4ui TrampglColor4ui
#define glColor4uiv TrampglColor4uiv
#define glColor4us TrampglColor4us
#define glColor4usv TrampglColor4usv
#define glColorMask TrampglColorMask
#define glColorMaterial TrampglColorMaterial
#define glCopyPixels TrampglCopyPixels
#define glCullFace TrampglCullFace
#define glDeleteLists TrampglDeleteLists
#define glDepthFunc TrampglDepthFunc
#define glDepthMask TrampglDepthMask
#define glDepthRange TrampglDepthRange
#define glDisable TrampglDisable
#define glDrawBuffer TrampglDrawBuffer
#define glDrawPixels TrampglDrawPixels
#define glEdgeFlag TrampglEdgeFlag
#define glEdgeFlagv TrampglEdgeFlagv
#define glEnable TrampglEnable
#define glEnd TrampglEnd
#define glEndList TrampglEndList
#define glEvalCoord1d TrampglEvalCoord1d
#define glEvalCoord1dv TrampglEvalCoord1dv
#define glEvalCoord1f TrampglEvalCoord1f
#define glEvalCoord1fv TrampglEvalCoord1fv
#define glEvalCoord2d TrampglEvalCoord2d
#define glEvalCoord2dv TrampglEvalCoord2dv
#define glEvalCoord2f TrampglEvalCoord2f
#define glEvalCoord2fv TrampglEvalCoord2fv
#define glEvalMesh1 TrampglEvalMesh1
#define glEvalMesh2 TrampglEvalMesh2
#define glEvalPoint1 TrampglEvalPoint1
#define glEvalPoint2 TrampglEvalPoint2
#define glFeedbackBuffer TrampglFeedbackBuffer
#define glFinish TrampglFinish
#define glFlush TrampglFlush
#define glFogf TrampglFogf
#define glFogfv TrampglFogfv
#define glFogi TrampglFogi
#define glFogiv TrampglFogiv
#define glFrontFace TrampglFrontFace
#define glFrustum TrampglFrustum
#define glGetBooleanv TrampglGetBooleanv
#define glGetClipPlane TrampglGetClipPlane
#define glGetDoublev TrampglGetDoublev
#define glGetFloatv TrampglGetFloatv
#define glGetIntegerv TrampglGetIntegerv
#define glGetLightfv TrampglGetLightfv
#define glGetLightiv TrampglGetLightiv
#define glGetMapdv TrampglGetMapdv
#define glGetMapfv TrampglGetMapfv
#define glGetMapiv TrampglGetMapiv
#define glGetMaterialfv TrampglGetMaterialfv
#define glGetMaterialiv TrampglGetMaterialiv
#define glGetPixelMapfv TrampglGetPixelMapfv
#define glGetPixelMapuiv TrampglGetPixelMapuiv
#define glGetPixelMapusv TrampglGetPixelMapusv
#define glGetPolygonStipple TrampglGetPolygonStipple
#define glGetTexEnvfv TrampglGetTexEnvfv
#define glGetTexEnviv TrampglGetTexEnviv
#define glGetTexGendv TrampglGetTexGendv
#define glGetTexGenfv TrampglGetTexGenfv
#define glGetTexGeniv TrampglGetTexGeniv
#define glGetTexImage TrampglGetTexImage
#define glGetTexLevelParameterfv TrampglGetTexLevelParameterfv
#define glGetTexLevelParameteriv TrampglGetTexLevelParameteriv
#define glGetTexParameterfv TrampglGetTexParameterfv
#define glGetTexParameteriv TrampglGetTexParameteriv
#define glHint TrampglHint
#define glIndexMask TrampglIndexMask
#define glIndexd TrampglIndexd
#define glIndexdv TrampglIndexdv
#define glIndexf TrampglIndexf
#define glIndexfv TrampglIndexfv
#define glIndexi TrampglIndexi
#define glIndexiv TrampglIndexiv
#define glIndexs TrampglIndexs
#define glIndexsv TrampglIndexsv
#define glInitNames TrampglInitNames
#define glLightModelf TrampglLightModelf
#define glLightModelfv TrampglLightModelfv
#define glLightModeli TrampglLightModeli
#define glLightModeliv TrampglLightModeliv
#define glLightf TrampglLightf
#define glLightfv TrampglLightfv
#define glLighti TrampglLighti
#define glLightiv TrampglLightiv
#define glLineStipple TrampglLineStipple
#define glLineWidth TrampglLineWidth
#define glListBase TrampglListBase
#define glLoadIdentity TrampglLoadIdentity
#define glLoadMatrixd TrampglLoadMatrixd
#define glLoadMatrixf TrampglLoadMatrixf
#define glLoadName TrampglLoadName
#define glLogicOp TrampglLogicOp
#define glMap1d TrampglMap1d
#define glMap1f TrampglMap1f
#define glMap2d TrampglMap2d
#define glMap2f TrampglMap2f
#define glMapGrid1d TrampglMapGrid1d
#define glMapGrid1f TrampglMapGrid1f
#define glMapGrid2d TrampglMapGrid2d
#define glMapGrid2f TrampglMapGrid2f
#define glMaterialf TrampglMaterialf
#define glMaterialfv TrampglMaterialfv
#define glMateriali TrampglMateriali
#define glMaterialiv TrampglMaterialiv
#define glMatrixMode TrampglMatrixMode
#define glMultMatrixd TrampglMultMatrixd
#define glMultMatrixf TrampglMultMatrixf
#define glNewList TrampglNewList
#define glNormal3b TrampglNormal3b
#define glNormal3bv TrampglNormal3bv
#define glNormal3d TrampglNormal3d
#define glNormal3dv TrampglNormal3dv
#define glNormal3f TrampglNormal3f
#define glNormal3fv TrampglNormal3fv
#define glNormal3i TrampglNormal3i
#define glNormal3iv TrampglNormal3iv
#define glNormal3s TrampglNormal3s
#define glNormal3sv TrampglNormal3sv
#define glOrtho TrampglOrtho
#define glPassThrough TrampglPassThrough
#define glPixelMapfv TrampglPixelMapfv
#define glPixelMapuiv TrampglPixelMapuiv
#define glPixelMapusv TrampglPixelMapusv
#define glPixelStoref TrampglPixelStoref
#define glPixelStorei TrampglPixelStorei
#define glPixelTransferf TrampglPixelTransferf
#define glPixelTransferi TrampglPixelTransferi
#define glPixelZoom TrampglPixelZoom
#define glPointSize TrampglPointSize
#define glPolygonMode TrampglPolygonMode
#define glPolygonStipple TrampglPolygonStipple
#define glPopAttrib TrampglPopAttrib
#define glPopMatrix TrampglPopMatrix
#define glPopName TrampglPopName
#define glPushAttrib TrampglPushAttrib
#define glPushMatrix TrampglPushMatrix
#define glPushName TrampglPushName
#define glRasterPos2d TrampglRasterPos2d
#define glRasterPos2dv TrampglRasterPos2dv
#define glRasterPos2f TrampglRasterPos2f
#define glRasterPos2fv TrampglRasterPos2fv
#define glRasterPos2i TrampglRasterPos2i
#define glRasterPos2iv TrampglRasterPos2iv
#define glRasterPos2s TrampglRasterPos2s
#define glRasterPos2sv TrampglRasterPos2sv
#define glRasterPos3d TrampglRasterPos3d
#define glRasterPos3dv TrampglRasterPos3dv
#define glRasterPos3f TrampglRasterPos3f
#define glRasterPos3fv TrampglRasterPos3fv
#define glRasterPos3i TrampglRasterPos3i
#define glRasterPos3iv TrampglRasterPos3iv
#define glRasterPos3s TrampglRasterPos3s
#define glRasterPos3sv TrampglRasterPos3sv
#define glRasterPos4d TrampglRasterPos4d
#define glRasterPos4dv TrampglRasterPos4dv
#define glRasterPos4f TrampglRasterPos4f
#define glRasterPos4fv TrampglRasterPos4fv
#define glRasterPos4i TrampglRasterPos4i
#define glRasterPos4iv TrampglRasterPos4iv
#define glRasterPos4s TrampglRasterPos4s
#define glRasterPos4sv TrampglRasterPos4sv
#define glReadBuffer TrampglReadBuffer
#define glReadPixels TrampglReadPixels
#define glRectd TrampglRectd
#define glRectdv TrampglRectdv
#define glRectf TrampglRectf
#define glRectfv TrampglRectfv
#define glRecti TrampglRecti
#define glRectiv TrampglRectiv
#define glRects TrampglRects
#define glRectsv TrampglRectsv
#define glRotated TrampglRotated
#define glRotatef TrampglRotatef
#define glScaled TrampglScaled
#define glScalef TrampglScalef
#define glScissor TrampglScissor
#define glSelectBuffer TrampglSelectBuffer
#define glShadeModel TrampglShadeModel
#define glStencilFunc TrampglStencilFunc
#define glStencilMask TrampglStencilMask
#define glStencilOp TrampglStencilOp
#define glTexCoord1d TrampglTexCoord1d
#define glTexCoord1dv TrampglTexCoord1dv
#define glTexCoord1f TrampglTexCoord1f
#define glTexCoord1fv TrampglTexCoord1fv
#define glTexCoord1i TrampglTexCoord1i
#define glTexCoord1iv TrampglTexCoord1iv
#define glTexCoord1s TrampglTexCoord1s
#define glTexCoord1sv TrampglTexCoord1sv
#define glTexCoord2d TrampglTexCoord2d
#define glTexCoord2dv TrampglTexCoord2dv
#define glTexCoord2f TrampglTexCoord2f
#define glTexCoord2fv TrampglTexCoord2fv
#define glTexCoord2i TrampglTexCoord2i
#define glTexCoord2iv TrampglTexCoord2iv
#define glTexCoord2s TrampglTexCoord2s
#define glTexCoord2sv TrampglTexCoord2sv
#define glTexCoord3d TrampglTexCoord3d
#define glTexCoord3dv TrampglTexCoord3dv
#define glTexCoord3f TrampglTexCoord3f
#define glTexCoord3fv TrampglTexCoord3fv
#define glTexCoord3i TrampglTexCoord3i
#define glTexCoord3iv TrampglTexCoord3iv
#define glTexCoord3s TrampglTexCoord3s
#define glTexCoord3sv TrampglTexCoord3sv
#define glTexCoord4d TrampglTexCoord4d
#define glTexCoord4dv TrampglTexCoord4dv
#define glTexCoord4f TrampglTexCoord4f
#define glTexCoord4fv TrampglTexCoord4fv
#define glTexCoord4i TrampglTexCoord4i
#define glTexCoord4iv TrampglTexCoord4iv
#define glTexCoord4s TrampglTexCoord4s
#define glTexCoord4sv TrampglTexCoord4sv
#define glTexEnvf TrampglTexEnvf
#define glTexEnvfv TrampglTexEnvfv
#define glTexEnvi TrampglTexEnvi
#define glTexEnviv TrampglTexEnviv
#define glTexGend TrampglTexGend
#define glTexGendv TrampglTexGendv
#define glTexGenf TrampglTexGenf
#define glTexGenfv TrampglTexGenfv
#define glTexGeni TrampglTexGeni
#define glTexGeniv TrampglTexGeniv
#define glTexImage1D TrampglTexImage1D
#define glTexImage2D TrampglTexImage2D
#define glTexParameterf TrampglTexParameterf
#define glTexParameterfv TrampglTexParameterfv
#define glTexParameteri TrampglTexParameteri
#define glTexParameteriv TrampglTexParameteriv
#define glTranslated TrampglTranslated
#define glTranslatef TrampglTranslatef
#define glVertex2d TrampglVertex2d
#define glVertex2dv TrampglVertex2dv
#define glVertex2f TrampglVertex2f
#define glVertex2fv TrampglVertex2fv
#define glVertex2i TrampglVertex2i
#define glVertex2iv TrampglVertex2iv
#define glVertex2s TrampglVertex2s
#define glVertex2sv TrampglVertex2sv
#define glVertex3d TrampglVertex3d
#define glVertex3dv TrampglVertex3dv
#define glVertex3f TrampglVertex3f
#define glVertex3fv TrampglVertex3fv
#define glVertex3i TrampglVertex3i
#define glVertex3iv TrampglVertex3iv
#define glVertex3s TrampglVertex3s
#define glVertex3sv TrampglVertex3sv
#define glVertex4d TrampglVertex4d
#define glVertex4dv TrampglVertex4dv
#define glVertex4f TrampglVertex4f
#define glVertex4fv TrampglVertex4fv
#define glVertex4i TrampglVertex4i
#define glVertex4iv TrampglVertex4iv
#define glVertex4s TrampglVertex4s
#define glVertex4sv TrampglVertex4sv
#define glViewport TrampglViewport
#define glGenLists TrampglGenLists
#define glGetError TrampglGetError
#define glGetString TrampglGetString
#define glIsEnabled TrampglIsEnabled
#define glIsList TrampglIsList
#define glRenderMode TrampglRenderMode
#define glAreTexturesResident TrampglAreTexturesResident
#define glArrayElement TrampglArrayElement
#define glColorPointer TrampglColorPointer
#define glCopyTexImage1D TrampglCopyTexImage1D
#define glCopyTexImage2D TrampglCopyTexImage2D
#define glCopyTexSubImage1D TrampglCopyTexSubImage1D
#define glCopyTexSubImage2D TrampglCopyTexSubImage2D
#define glDisableClientState TrampglDisableClientState
#define glDrawArrays TrampglDrawArrays
#define glDrawElements TrampglDrawElements
#define glEdgeFlagPointer TrampglEdgeFlagPointer
#define glEnableClientState TrampglEnableClientState
#define glGetPointerv TrampglGetPointerv
#define glIndexPointer TrampglIndexPointer
#define glIndexub TrampglIndexub
#define glIndexubv TrampglIndexubv
#define glInterleavedArrays TrampglInterleavedArrays
#define glIsTexture TrampglIsTexture
#define glNormalPointer TrampglNormalPointer
#define glPolygonOffset TrampglPolygonOffset
#define glPopClientAttrib TrampglPopClientAttrib
#define glPrioritizeTextures TrampglPrioritizeTextures
#define glPushClientAttrib TrampglPushClientAttrib
#define glTexCoordPointer TrampglTexCoordPointer
#define glTexSubImage1D TrampglTexSubImage1D
#define glTexSubImage2D TrampglTexSubImage2D
#define glVertexPointer TrampglVertexPointer
TRAMPFUNC void GLAPI glBindTexture(GLenum target, GLuint texture) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glDeleteTextures(GLsizei n, const GLuint *textures) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glGenTextures(GLsizei n, GLuint *textures) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glAccum (GLenum op, GLfloat value) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glAlphaFunc (GLenum func, GLclampf ref) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glBegin (GLenum mode) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glBitmap (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glBlendFunc (GLenum sfactor, GLenum dfactor) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glCallList (GLuint list) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glCallLists (GLsizei n, GLenum type, const GLvoid *lists) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glClear (GLbitfield mask) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glClearAccum (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glClearDepth (GLclampd depth) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glClearIndex (GLfloat c) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glClearStencil (GLint s) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glClipPlane (GLenum plane, const GLdouble *equation) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor3b (GLbyte red, GLbyte green, GLbyte blue) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor3bv (const GLbyte *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor3d (GLdouble red, GLdouble green, GLdouble blue) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor3dv (const GLdouble *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor3f (GLfloat red, GLfloat green, GLfloat blue) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor3fv (const GLfloat *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor3i (GLint red, GLint green, GLint blue) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor3iv (const GLint *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor3s (GLshort red, GLshort green, GLshort blue) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor3sv (const GLshort *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor3ub (GLubyte red, GLubyte green, GLubyte blue) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor3ubv (const GLubyte *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor3ui (GLuint red, GLuint green, GLuint blue) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor3uiv (const GLuint *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor3us (GLushort red, GLushort green, GLushort blue) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor3usv (const GLushort *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor4b (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor4bv (const GLbyte *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor4d (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor4dv (const GLdouble *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor4f (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor4fv (const GLfloat *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor4i (GLint red, GLint green, GLint blue, GLint alpha) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor4iv (const GLint *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor4s (GLshort red, GLshort green, GLshort blue, GLshort alpha) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor4sv (const GLshort *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor4ub (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor4ubv (const GLubyte *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor4ui (GLuint red, GLuint green, GLuint blue, GLuint alpha) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor4uiv (const GLuint *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor4us (GLushort red, GLushort green, GLushort blue, GLushort alpha) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColor4usv (const GLushort *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColorMaterial (GLenum face, GLenum mode) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glCopyPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glCullFace (GLenum mode) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glDeleteLists (GLuint list, GLsizei range) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glDepthFunc (GLenum func) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glDepthMask (GLboolean flag) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glDepthRange (GLclampd zNear, GLclampd zFar) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glDisable (GLenum cap) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glDrawBuffer (GLenum mode) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glDrawPixels (GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glEdgeFlag (GLboolean flag) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glEdgeFlagv (const GLboolean *flag) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glEnable (GLenum cap) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glEnd (void) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glEndList (void) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glEvalCoord1d (GLdouble u) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glEvalCoord1dv (const GLdouble *u) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glEvalCoord1f (GLfloat u) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glEvalCoord1fv (const GLfloat *u) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glEvalCoord2d (GLdouble u, GLdouble v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glEvalCoord2dv (const GLdouble *u) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glEvalCoord2f (GLfloat u, GLfloat v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glEvalCoord2fv (const GLfloat *u) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glEvalMesh1 (GLenum mode, GLint i1, GLint i2) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glEvalMesh2 (GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glEvalPoint1 (GLint i) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glEvalPoint2 (GLint i, GLint j) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glFeedbackBuffer (GLsizei size, GLenum type, GLfloat *buffer) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glFinish (void) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glFlush (void) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glFogf (GLenum pname, GLfloat param) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glFogfv (GLenum pname, const GLfloat *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glFogi (GLenum pname, GLint param) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glFogiv (GLenum pname, const GLint *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glFrontFace (GLenum mode) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glFrustum (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glGetBooleanv (GLenum pname, GLboolean *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glGetClipPlane (GLenum plane, GLdouble *equation) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glGetDoublev (GLenum pname, GLdouble *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glGetFloatv (GLenum pname, GLfloat *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glGetIntegerv (GLenum pname, GLint *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glGetLightfv (GLenum light, GLenum pname, GLfloat *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glGetLightiv (GLenum light, GLenum pname, GLint *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glGetMapdv (GLenum target, GLenum query, GLdouble *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glGetMapfv (GLenum target, GLenum query, GLfloat *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glGetMapiv (GLenum target, GLenum query, GLint *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glGetMaterialfv (GLenum face, GLenum pname, GLfloat *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glGetMaterialiv (GLenum face, GLenum pname, GLint *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glGetPixelMapfv (GLenum map, GLfloat *values) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glGetPixelMapuiv (GLenum map, GLuint *values) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glGetPixelMapusv (GLenum map, GLushort *values) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glGetPolygonStipple (GLubyte *mask) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glGetTexEnvfv (GLenum target, GLenum pname, GLfloat *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glGetTexEnviv (GLenum target, GLenum pname, GLint *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glGetTexGendv (GLenum coord, GLenum pname, GLdouble *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glGetTexGenfv (GLenum coord, GLenum pname, GLfloat *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glGetTexGeniv (GLenum coord, GLenum pname, GLint *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glGetTexImage (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glGetTexLevelParameterfv (GLenum target, GLint level, GLenum pname, GLfloat *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glGetTexLevelParameteriv (GLenum target, GLint level, GLenum pname, GLint *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glGetTexParameterfv (GLenum target, GLenum pname, GLfloat *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glGetTexParameteriv (GLenum target, GLenum pname, GLint *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glHint (GLenum target, GLenum mode) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glIndexMask (GLuint mask) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glIndexd (GLdouble c) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glIndexdv (const GLdouble *c) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glIndexf (GLfloat c) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glIndexfv (const GLfloat *c) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glIndexi (GLint c) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glIndexiv (const GLint *c) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glIndexs (GLshort c) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glIndexsv (const GLshort *c) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glInitNames (void) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glLightModelf (GLenum pname, GLfloat param) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glLightModelfv (GLenum pname, const GLfloat *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glLightModeli (GLenum pname, GLint param) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glLightModeliv (GLenum pname, const GLint *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glLightf (GLenum light, GLenum pname, GLfloat param) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glLightfv (GLenum light, GLenum pname, const GLfloat *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glLighti (GLenum light, GLenum pname, GLint param) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glLightiv (GLenum light, GLenum pname, const GLint *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glLineStipple (GLint factor, GLushort pattern) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glLineWidth (GLfloat width) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glListBase (GLuint base) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glLoadIdentity (void) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glLoadMatrixd (const GLdouble *m) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glLoadMatrixf (const GLfloat *m) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glLoadName (GLuint name) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glLogicOp (GLenum opcode) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glMap1d (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glMap1f (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glMap2d (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glMap2f (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glMapGrid1d (GLint un, GLdouble u1, GLdouble u2) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glMapGrid1f (GLint un, GLfloat u1, GLfloat u2) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glMapGrid2d (GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glMapGrid2f (GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glMaterialf (GLenum face, GLenum pname, GLfloat param) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glMaterialfv (GLenum face, GLenum pname, const GLfloat *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glMateriali (GLenum face, GLenum pname, GLint param) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glMaterialiv (GLenum face, GLenum pname, const GLint *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glMatrixMode (GLenum mode) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glMultMatrixd (const GLdouble *m) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glMultMatrixf (const GLfloat *m) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glNewList (GLuint list, GLenum mode) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glNormal3b (GLbyte nx, GLbyte ny, GLbyte nz) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glNormal3bv (const GLbyte *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glNormal3d (GLdouble nx, GLdouble ny, GLdouble nz) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glNormal3dv (const GLdouble *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glNormal3f (GLfloat nx, GLfloat ny, GLfloat nz) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glNormal3fv (const GLfloat *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glNormal3i (GLint nx, GLint ny, GLint nz) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glNormal3iv (const GLint *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glNormal3s (GLshort nx, GLshort ny, GLshort nz) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glNormal3sv (const GLshort *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glOrtho (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glPassThrough (GLfloat token) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glPixelMapfv (GLenum map, GLsizei mapsize, const GLfloat *values) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glPixelMapuiv (GLenum map, GLsizei mapsize, const GLuint *values) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glPixelMapusv (GLenum map, GLsizei mapsize, const GLushort *values) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glPixelStoref (GLenum pname, GLfloat param) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glPixelStorei (GLenum pname, GLint param) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glPixelTransferf (GLenum pname, GLfloat param) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glPixelTransferi (GLenum pname, GLint param) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glPixelZoom (GLfloat xfactor, GLfloat yfactor) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glPointSize (GLfloat size) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glPolygonMode (GLenum face, GLenum mode) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glPolygonStipple (const GLubyte *mask) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glPopAttrib (void) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glPopMatrix (void) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glPopName (void) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glPushAttrib (GLbitfield mask) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glPushMatrix (void) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glPushName (GLuint name) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRasterPos2d (GLdouble x, GLdouble y) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRasterPos2dv (const GLdouble *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRasterPos2f (GLfloat x, GLfloat y) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRasterPos2fv (const GLfloat *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRasterPos2i (GLint x, GLint y) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRasterPos2iv (const GLint *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRasterPos2s (GLshort x, GLshort y) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRasterPos2sv (const GLshort *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRasterPos3d (GLdouble x, GLdouble y, GLdouble z) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRasterPos3dv (const GLdouble *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRasterPos3f (GLfloat x, GLfloat y, GLfloat z) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRasterPos3fv (const GLfloat *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRasterPos3i (GLint x, GLint y, GLint z) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRasterPos3iv (const GLint *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRasterPos3s (GLshort x, GLshort y, GLshort z) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRasterPos3sv (const GLshort *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRasterPos4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRasterPos4dv (const GLdouble *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRasterPos4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRasterPos4fv (const GLfloat *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRasterPos4i (GLint x, GLint y, GLint z, GLint w) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRasterPos4iv (const GLint *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRasterPos4s (GLshort x, GLshort y, GLshort z, GLshort w) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRasterPos4sv (const GLshort *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glReadBuffer (GLenum mode) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRectd (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRectdv (const GLdouble *v1, const GLdouble *v2) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRectf (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRectfv (const GLfloat *v1, const GLfloat *v2) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRecti (GLint x1, GLint y1, GLint x2, GLint y2) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRectiv (const GLint *v1, const GLint *v2) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRects (GLshort x1, GLshort y1, GLshort x2, GLshort y2) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRectsv (const GLshort *v1, const GLshort *v2) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRotated (GLdouble angle, GLdouble x, GLdouble y, GLdouble z) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glScaled (GLdouble x, GLdouble y, GLdouble z) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glScalef (GLfloat x, GLfloat y, GLfloat z) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glScissor (GLint x, GLint y, GLsizei width, GLsizei height) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glSelectBuffer (GLsizei size, GLuint *buffer) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glShadeModel (GLenum mode) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glStencilFunc (GLenum func, GLint ref, GLuint mask) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glStencilMask (GLuint mask) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glStencilOp (GLenum fail, GLenum zfail, GLenum zpass) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord1d (GLdouble s) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord1dv (const GLdouble *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord1f (GLfloat s) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord1fv (const GLfloat *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord1i (GLint s) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord1iv (const GLint *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord1s (GLshort s) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord1sv (const GLshort *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord2d (GLdouble s, GLdouble t) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord2dv (const GLdouble *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord2f (GLfloat s, GLfloat t) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord2fv (const GLfloat *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord2i (GLint s, GLint t) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord2iv (const GLint *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord2s (GLshort s, GLshort t) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord2sv (const GLshort *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord3d (GLdouble s, GLdouble t, GLdouble r) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord3dv (const GLdouble *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord3f (GLfloat s, GLfloat t, GLfloat r) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord3fv (const GLfloat *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord3i (GLint s, GLint t, GLint r) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord3iv (const GLint *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord3s (GLshort s, GLshort t, GLshort r) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord3sv (const GLshort *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord4d (GLdouble s, GLdouble t, GLdouble r, GLdouble q) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord4dv (const GLdouble *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord4f (GLfloat s, GLfloat t, GLfloat r, GLfloat q) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord4fv (const GLfloat *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord4i (GLint s, GLint t, GLint r, GLint q) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord4iv (const GLint *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord4s (GLshort s, GLshort t, GLshort r, GLshort q) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoord4sv (const GLshort *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexEnvf (GLenum target, GLenum pname, GLfloat param) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexEnvfv (GLenum target, GLenum pname, const GLfloat *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexEnvi (GLenum target, GLenum pname, GLint param) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexEnviv (GLenum target, GLenum pname, const GLint *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexGend (GLenum coord, GLenum pname, GLdouble param) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexGendv (GLenum coord, GLenum pname, const GLdouble *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexGenf (GLenum coord, GLenum pname, GLfloat param) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexGenfv (GLenum coord, GLenum pname, const GLfloat *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexGeni (GLenum coord, GLenum pname, GLint param) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexGeniv (GLenum coord, GLenum pname, const GLint *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexImage1D (GLenum target, GLint level, GLint components, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexImage2D (GLenum target, GLint level, GLint components, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexParameterf (GLenum target, GLenum pname, GLfloat param) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexParameterfv (GLenum target, GLenum pname, const GLfloat *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexParameteri (GLenum target, GLenum pname, GLint param) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexParameteriv (GLenum target, GLenum pname, const GLint *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTranslated (GLdouble x, GLdouble y, GLdouble z) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTranslatef (GLfloat x, GLfloat y, GLfloat z) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glVertex2d (GLdouble x, GLdouble y) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glVertex2dv (const GLdouble *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glVertex2f (GLfloat x, GLfloat y) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glVertex2fv (const GLfloat *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glVertex2i (GLint x, GLint y) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glVertex2iv (const GLint *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glVertex2s (GLshort x, GLshort y) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glVertex2sv (const GLshort *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glVertex3d (GLdouble x, GLdouble y, GLdouble z) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glVertex3dv (const GLdouble *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glVertex3f (GLfloat x, GLfloat y, GLfloat z) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glVertex3fv (const GLfloat *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glVertex3i (GLint x, GLint y, GLint z) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glVertex3iv (const GLint *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glVertex3s (GLshort x, GLshort y, GLshort z) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glVertex3sv (const GLshort *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glVertex4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glVertex4dv (const GLdouble *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glVertex4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glVertex4fv (const GLfloat *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glVertex4i (GLint x, GLint y, GLint z, GLint w) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glVertex4iv (const GLint *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glVertex4s (GLshort x, GLshort y, GLshort z, GLshort w) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glVertex4sv (const GLshort *v) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glViewport (GLint x, GLint y, GLsizei width, GLsizei height) TRAMPOLINE_DEF_VOID
TRAMPFUNC GLuint GLAPI glGenLists (GLsizei range) TRAMPOLINE_DEF
TRAMPFUNC GLenum GLAPI glGetError (void) TRAMPOLINE_DEF
TRAMPFUNC const GLubyte * GLAPI glGetString (GLenum name) TRAMPOLINE_DEF
TRAMPFUNC GLboolean GLAPI glIsEnabled (GLenum cap) TRAMPOLINE_DEF
TRAMPFUNC GLboolean GLAPI glIsList (GLuint list) TRAMPOLINE_DEF
TRAMPFUNC GLint GLAPI glRenderMode (GLenum mode) TRAMPOLINE_DEF
TRAMPFUNC GLboolean GLAPI glAreTexturesResident (GLsizei n, const GLuint *textures, GLboolean *residences) TRAMPOLINE_DEF
TRAMPFUNC void GLAPI glArrayElement (GLint i) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glCopyTexImage1D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glCopyTexImage2D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glCopyTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glCopyTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glDisableClientState (GLenum array) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glDrawArrays (GLenum mode, GLint first, GLsizei count) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glDrawElements (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glEdgeFlagPointer (GLsizei stride, const GLvoid *pointer) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glEnableClientState (GLenum array) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glGetPointerv (GLenum pname, GLvoid* *params) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glIndexPointer (GLenum type, GLsizei stride, const GLvoid *pointer) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glIndexub (GLubyte c) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glIndexubv (const GLubyte *c) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glInterleavedArrays (GLenum format, GLsizei stride, const GLvoid *pointer) TRAMPOLINE_DEF_VOID
TRAMPFUNC GLboolean GLAPI glIsTexture (GLuint texture) TRAMPOLINE_DEF
TRAMPFUNC void GLAPI glNormalPointer (GLenum type, GLsizei stride, const GLvoid *pointer) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glPolygonOffset (GLfloat factor, GLfloat units) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glPopClientAttrib (void) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glPrioritizeTextures (GLsizei n, const GLuint *textures, const GLclampf *priorities) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glPushClientAttrib (GLbitfield mask) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI glVertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) TRAMPOLINE_DEF_VOID

// more internal windows opengl functions
#define glDebugEntry TrampglDebugEntry
#define GlmfBeginGlsBlock TrampGlmfBeginGlsBlock
#define GlmfEndGlsBlock TrampGlmfEndGlsBlock
#define GlmfCloseMetaFile TrampGlmfCloseMetaFile
#define GlmfEndPlayback TrampGlmfEndPlayback
#define GlmfInitPlayback TrampGlmfInitPlayback
#define GlmfPlayGlsRecord TrampGlmfPlayGlsRecord
#define wglChoosePixelFormat TrampwglChoosePixelFormat
#define wglCopyContext TrampwglCopyContext
#define wglCreateContext TrampwglCreateContext
#define wglCreateLayerContext TrampwglCreateLayerContext
#define wglDeleteContext TrampwglDeleteContext
#define wglDescribeLayerPlane TrampwglDescribeLayerPlane
#define wglDescribePixelFormat TrampwglDescribePixelFormat
#define wglGetCurrentContext TrampwglGetCurrentContext
#define wglGetCurrentDC TrampwglGetCurrentDC
#define wglGetDefaultProcAddress TrampwglGetDefaultProcAddress
#define wglGetLayerPaletteEntries TrampwglGetLayerPaletteEntries
#define wglGetPixelFormat TrampwglGetPixelFormat
#define wglGetProcAddress TrampwglGetProcAddress
#define wglMakeCurrent TrampwglMakeCurrent
#define wglRealizeLayerPalette TrampwglRealizeLayerPalette
#define wglSetLayerPaletteEntries TrampwglSetLayerPaletteEntries
#define wglSetPixelFormat TrampwglSetPixelFormat
#define wglShareLists TrampwglShareLists
#define wglSwapBuffers TrampwglSwapBuffers
#define wglSwapLayerBuffers TrampwglSwapLayerBuffers
#define wglSwapMultipleBuffers TrampwglSwapMultipleBuffers
#define wglUseFontBitmapsA TrampwglUseFontBitmapsA
#define wglUseFontBitmapsW TrampwglUseFontBitmapsW
#define wglUseFontOutlinesA TrampwglUseFontOutlinesA
#define wglUseFontOutlinesW TrampwglUseFontOutlinesW
TRAMPFUNC void GLAPI glDebugEntry(DWORD arg1, DWORD arg2) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI GlmfBeginGlsBlock(DWORD arg1) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI GlmfEndGlsBlock(DWORD arg1) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI GlmfCloseMetaFile(DWORD arg1) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI GlmfEndPlayback(DWORD arg1) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI GlmfInitPlayback(DWORD arg1, DWORD arg2, DWORD arg3) TRAMPOLINE_DEF_VOID
TRAMPFUNC void GLAPI GlmfPlayGlsRecord(DWORD arg1, DWORD arg2, DWORD arg3, DWORD arg4) TRAMPOLINE_DEF_VOID
TRAMPFUNC int WINAPI wglChoosePixelFormat(HDC, const PIXELFORMATDESCRIPTOR *) TRAMPOLINE_DEF
TRAMPFUNC BOOL WINAPI wglCopyContext(HGLRC, HGLRC, UINT) TRAMPOLINE_DEF
TRAMPFUNC HGLRC WINAPI wglCreateContext(HDC) TRAMPOLINE_DEF
TRAMPFUNC HGLRC WINAPI wglCreateLayerContext(HDC, int) TRAMPOLINE_DEF
TRAMPFUNC BOOL WINAPI wglDeleteContext(HGLRC) TRAMPOLINE_DEF
TRAMPFUNC BOOL WINAPI wglDescribeLayerPlane(HDC, int, int, UINT, LPLAYERPLANEDESCRIPTOR) TRAMPOLINE_DEF
TRAMPFUNC int WINAPI wglDescribePixelFormat(HDC hdc, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR ppfd) TRAMPOLINE_DEF
TRAMPFUNC HGLRC WINAPI wglGetCurrentContext(VOID) TRAMPOLINE_DEF
TRAMPFUNC HDC WINAPI wglGetCurrentDC(VOID) TRAMPOLINE_DEF
TRAMPFUNC PROC WINAPI wglGetDefaultProcAddress(LPCSTR) TRAMPOLINE_DEF
TRAMPFUNC int WINAPI wglGetLayerPaletteEntries(HDC, int, int, int, COLORREF *) TRAMPOLINE_DEF
TRAMPFUNC int WINAPI wglGetPixelFormat(HDC hdc) TRAMPOLINE_DEF
TRAMPFUNC PROC WINAPI wglGetProcAddress(LPCSTR) TRAMPOLINE_DEF
TRAMPFUNC BOOL WINAPI wglMakeCurrent(HDC, HGLRC) TRAMPOLINE_DEF
TRAMPFUNC BOOL WINAPI wglRealizeLayerPalette(HDC, int, BOOL) TRAMPOLINE_DEF
TRAMPFUNC int WINAPI wglSetLayerPaletteEntries(HDC, int, int, int, CONST COLORREF *) TRAMPOLINE_DEF
TRAMPFUNC BOOL WINAPI wglSetPixelFormat(HDC hdc, int format, CONST PIXELFORMATDESCRIPTOR * pfd) TRAMPOLINE_DEF
TRAMPFUNC BOOL WINAPI wglShareLists(HGLRC, HGLRC) TRAMPOLINE_DEF
TRAMPFUNC BOOL WINAPI wglSwapBuffers(HDC hdc) TRAMPOLINE_DEF
TRAMPFUNC BOOL WINAPI wglSwapLayerBuffers(HDC, UINT) TRAMPOLINE_DEF
TRAMPFUNC DWORD WINAPI wglSwapMultipleBuffers(UINT, CONST WGLSWAP *) TRAMPOLINE_DEF
TRAMPFUNC BOOL WINAPI wglUseFontBitmapsA(HDC, DWORD, DWORD, DWORD) TRAMPOLINE_DEF
TRAMPFUNC BOOL WINAPI wglUseFontBitmapsW(HDC, DWORD, DWORD, DWORD) TRAMPOLINE_DEF
TRAMPFUNC BOOL WINAPI wglUseFontOutlinesA(HDC, DWORD, DWORD, DWORD, FLOAT, FLOAT, int, LPGLYPHMETRICSFLOAT) TRAMPOLINE_DEF
TRAMPFUNC BOOL WINAPI wglUseFontOutlinesW(HDC, DWORD, DWORD, DWORD, FLOAT, FLOAT, int, LPGLYPHMETRICSFLOAT) TRAMPOLINE_DEF


#endif
