/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#include "../intercept.h"

#define GLAPI WINAPI

namespace Hooks
{
    typedef unsigned int GLenum;
    typedef unsigned char GLboolean;
    typedef unsigned int GLbitfield;
    typedef void GLvoid;
    typedef signed char GLbyte; // 1-byte signed
    typedef short GLshort; // 2-byte signed
    typedef int GLint; // 4-byte signed
    typedef unsigned char GLubyte; // 1-byte unsigned
    typedef unsigned short GLushort; // 2-byte unsigned
    typedef unsigned int GLuint; // 4-byte unsigned
    typedef int GLsizei; // 4-byte signed
    typedef float GLfloat; // single precision float
    typedef float GLclampf; // single precision float in [0,1]
    typedef double GLdouble; // double precision float
    typedef double GLclampd; // double precision float in [0,1]

    HOOK_FUNCTION_DECLARE(void, GLAPI, glBindTexture, GLenum target, GLuint texture);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glDeleteTextures, GLsizei n, const GLuint* textures);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glGenTextures, GLsizei n, GLuint* textures);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glAccum, GLenum op, GLfloat value);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glAlphaFunc, GLenum func, GLclampf ref);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glBegin, GLenum mode);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glBitmap,
                          GLsizei width,
                          GLsizei height,
                          GLfloat xorig,
                          GLfloat yorig,
                          GLfloat xmove,
                          GLfloat ymove,
                          const GLubyte* bitmap);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glBlendFunc, GLenum sfactor, GLenum dfactor);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glCallList, GLuint list);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glCallLists, GLsizei n, GLenum type, const GLvoid* lists);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glClear, GLbitfield mask);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glClearAccum,
                          GLfloat red,
                          GLfloat green,
                          GLfloat blue,
                          GLfloat alpha);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glClearColor,
                          GLclampf red,
                          GLclampf green,
                          GLclampf blue,
                          GLclampf alpha);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glClearDepth, GLclampd depth);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glClearIndex, GLfloat c);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glClearStencil, GLint s);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glClipPlane, GLenum plane, const GLdouble* equation);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glColor3b, GLbyte red, GLbyte green, GLbyte blue);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glColor3bv, const GLbyte* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glColor3d, GLdouble red, GLdouble green, GLdouble blue);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glColor3dv, const GLdouble* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glColor3f, GLfloat red, GLfloat green, GLfloat blue);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glColor3fv, const GLfloat* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glColor3i, GLint red, GLint green, GLint blue);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glColor3iv, const GLint* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glColor3s, GLshort red, GLshort green, GLshort blue);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glColor3sv, const GLshort* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glColor3ub, GLubyte red, GLubyte green, GLubyte blue);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glColor3ubv, const GLubyte* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glColor3ui, GLuint red, GLuint green, GLuint blue);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glColor3uiv, const GLuint* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glColor3us, GLushort red, GLushort green, GLushort blue);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glColor3usv, const GLushort* v);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glColor4b,
                          GLbyte red,
                          GLbyte green,
                          GLbyte blue,
                          GLbyte alpha);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glColor4bv, const GLbyte* v);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glColor4d,
                          GLdouble red,
                          GLdouble green,
                          GLdouble blue,
                          GLdouble alpha);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glColor4dv, const GLdouble* v);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glColor4f,
                          GLfloat red,
                          GLfloat green,
                          GLfloat blue,
                          GLfloat alpha);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glColor4fv, const GLfloat* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glColor4i, GLint red, GLint green, GLint blue, GLint alpha);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glColor4iv, const GLint* v);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glColor4s,
                          GLshort red,
                          GLshort green,
                          GLshort blue,
                          GLshort alpha);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glColor4sv, const GLshort* v);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glColor4ub,
                          GLubyte red,
                          GLubyte green,
                          GLubyte blue,
                          GLubyte alpha);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glColor4ubv, const GLubyte* v);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glColor4ui,
                          GLuint red,
                          GLuint green,
                          GLuint blue,
                          GLuint alpha);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glColor4uiv, const GLuint* v);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glColor4us,
                          GLushort red,
                          GLushort green,
                          GLushort blue,
                          GLushort alpha);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glColor4usv, const GLushort* v);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glColorMask,
                          GLboolean red,
                          GLboolean green,
                          GLboolean blue,
                          GLboolean alpha);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glColorMaterial, GLenum face, GLenum mode);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glCopyPixels,
                          GLint x,
                          GLint y,
                          GLsizei width,
                          GLsizei height,
                          GLenum type);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glCullFace, GLenum mode);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glDeleteLists, GLuint list, GLsizei range);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glDepthFunc, GLenum func);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glDepthMask, GLboolean flag);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glDepthRange, GLclampd zNear, GLclampd zFar);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glDisable, GLenum cap);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glDrawBuffer, GLenum mode);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glDrawPixels,
                          GLsizei width,
                          GLsizei height,
                          GLenum format,
                          GLenum type,
                          const GLvoid* pixels);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glEdgeFlag, GLboolean flag);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glEdgeFlagv, const GLboolean* flag);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glEnable, GLenum cap);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glEnd);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glEndList);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glEvalCoord1d, GLdouble u);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glEvalCoord1dv, const GLdouble* u);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glEvalCoord1f, GLfloat u);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glEvalCoord1fv, const GLfloat* u);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glEvalCoord2d, GLdouble u, GLdouble v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glEvalCoord2dv, const GLdouble* u);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glEvalCoord2f, GLfloat u, GLfloat v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glEvalCoord2fv, const GLfloat* u);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glEvalMesh1, GLenum mode, GLint i1, GLint i2);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glEvalMesh2,
                          GLenum mode,
                          GLint i1,
                          GLint i2,
                          GLint j1,
                          GLint j2);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glEvalPoint1, GLint i);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glEvalPoint2, GLint i, GLint j);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glFeedbackBuffer,
                          GLsizei size,
                          GLenum type,
                          GLfloat* buffer);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glFinish);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glFlush);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glFogf, GLenum pname, GLfloat param);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glFogfv, GLenum pname, const GLfloat* params);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glFogi, GLenum pname, GLint param);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glFogiv, GLenum pname, const GLint* params);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glFrontFace, GLenum mode);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glFrustum,
                          GLdouble left,
                          GLdouble right,
                          GLdouble bottom,
                          GLdouble top,
                          GLdouble zNear,
                          GLdouble zFar);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glGetBooleanv, GLenum pname, GLboolean* params);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glGetClipPlane, GLenum plane, GLdouble* equation);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glGetDoublev, GLenum pname, GLdouble* params);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glGetFloatv, GLenum pname, GLfloat* params);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glGetIntegerv, GLenum pname, GLint* params);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glGetLightfv, GLenum light, GLenum pname, GLfloat* params);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glGetLightiv, GLenum light, GLenum pname, GLint* params);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glGetMapdv, GLenum target, GLenum query, GLdouble* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glGetMapfv, GLenum target, GLenum query, GLfloat* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glGetMapiv, GLenum target, GLenum query, GLint* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glGetMaterialfv, GLenum face, GLenum pname, GLfloat* params);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glGetMaterialiv, GLenum face, GLenum pname, GLint* params);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glGetPixelMapfv, GLenum map, GLfloat* values);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glGetPixelMapuiv, GLenum map, GLuint* values);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glGetPixelMapusv, GLenum map, GLushort* values);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glGetPolygonStipple, GLubyte* mask);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glGetTexEnvfv, GLenum target, GLenum pname, GLfloat* params);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glGetTexEnviv, GLenum target, GLenum pname, GLint* params);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glGetTexGendv, GLenum coord, GLenum pname, GLdouble* params);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glGetTexGenfv, GLenum coord, GLenum pname, GLfloat* params);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glGetTexGeniv, GLenum coord, GLenum pname, GLint* params);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glGetTexImage,
                          GLenum target,
                          GLint level,
                          GLenum format,
                          GLenum type,
                          GLvoid* pixels);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glGetTexLevelParameterfv,
                          GLenum target,
                          GLint level,
                          GLenum pname,
                          GLfloat* params);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glGetTexLevelParameteriv,
                          GLenum target,
                          GLint level,
                          GLenum pname,
                          GLint* params);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glGetTexParameterfv,
                          GLenum target,
                          GLenum pname,
                          GLfloat* params);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glGetTexParameteriv,
                          GLenum target,
                          GLenum pname,
                          GLint* params);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glHint, GLenum target, GLenum mode);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glIndexMask, GLuint mask);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glIndexd, GLdouble c);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glIndexdv, const GLdouble* c);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glIndexf, GLfloat c);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glIndexfv, const GLfloat* c);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glIndexi, GLint c);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glIndexiv, const GLint* c);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glIndexs, GLshort c);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glIndexsv, const GLshort* c);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glInitNames);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glLightModelf, GLenum pname, GLfloat param);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glLightModelfv, GLenum pname, const GLfloat* params);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glLightModeli, GLenum pname, GLint param);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glLightModeliv, GLenum pname, const GLint* params);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glLightf, GLenum light, GLenum pname, GLfloat param);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glLightfv,
                          GLenum light,
                          GLenum pname,
                          const GLfloat* params);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glLighti, GLenum light, GLenum pname, GLint param);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glLightiv, GLenum light, GLenum pname, const GLint* params);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glLineStipple, GLint factor, GLushort pattern);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glLineWidth, GLfloat width);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glListBase, GLuint base);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glLoadIdentity);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glLoadMatrixd, const GLdouble* m);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glLoadMatrixf, const GLfloat* m);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glLoadName, GLuint name);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glLogicOp, GLenum opcode);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glMap1d,
                          GLenum target,
                          GLdouble u1,
                          GLdouble u2,
                          GLint stride,
                          GLint order,
                          const GLdouble* points);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glMap1f,
                          GLenum target,
                          GLfloat u1,
                          GLfloat u2,
                          GLint stride,
                          GLint order,
                          const GLfloat* points);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glMap2d,
                          GLenum target,
                          GLdouble u1,
                          GLdouble u2,
                          GLint ustride,
                          GLint uorder,
                          GLdouble v1,
                          GLdouble v2,
                          GLint vstride,
                          GLint vorder,
                          const GLdouble* points);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glMap2f,
                          GLenum target,
                          GLfloat u1,
                          GLfloat u2,
                          GLint ustride,
                          GLint uorder,
                          GLfloat v1,
                          GLfloat v2,
                          GLint vstride,
                          GLint vorder,
                          const GLfloat* points);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glMapGrid1d, GLint un, GLdouble u1, GLdouble u2);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glMapGrid1f, GLint un, GLfloat u1, GLfloat u2);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glMapGrid2d,
                          GLint un,
                          GLdouble u1,
                          GLdouble u2,
                          GLint vn,
                          GLdouble v1,
                          GLdouble v2);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glMapGrid2f,
                          GLint un,
                          GLfloat u1,
                          GLfloat u2,
                          GLint vn,
                          GLfloat v1,
                          GLfloat v2);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glMaterialf, GLenum face, GLenum pname, GLfloat param);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glMaterialfv,
                          GLenum face,
                          GLenum pname,
                          const GLfloat* params);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glMateriali, GLenum face, GLenum pname, GLint param);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glMaterialiv,
                          GLenum face,
                          GLenum pname,
                          const GLint* params);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glMatrixMode, GLenum mode);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glMultMatrixd, const GLdouble* m);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glMultMatrixf, const GLfloat* m);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glNewList, GLuint list, GLenum mode);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glNormal3b, GLbyte nx, GLbyte ny, GLbyte nz);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glNormal3bv, const GLbyte* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glNormal3d, GLdouble nx, GLdouble ny, GLdouble nz);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glNormal3dv, const GLdouble* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glNormal3f, GLfloat nx, GLfloat ny, GLfloat nz);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glNormal3fv, const GLfloat* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glNormal3i, GLint nx, GLint ny, GLint nz);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glNormal3iv, const GLint* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glNormal3s, GLshort nx, GLshort ny, GLshort nz);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glNormal3sv, const GLshort* v);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glOrtho,
                          GLdouble left,
                          GLdouble right,
                          GLdouble bottom,
                          GLdouble top,
                          GLdouble zNear,
                          GLdouble zFar);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glPassThrough, GLfloat token);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glPixelMapfv,
                          GLenum map,
                          GLsizei mapsize,
                          const GLfloat* values);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glPixelMapuiv,
                          GLenum map,
                          GLsizei mapsize,
                          const GLuint* values);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glPixelMapusv,
                          GLenum map,
                          GLsizei mapsize,
                          const GLushort* values);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glPixelStoref, GLenum pname, GLfloat param);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glPixelStorei, GLenum pname, GLint param);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glPixelTransferf, GLenum pname, GLfloat param);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glPixelTransferi, GLenum pname, GLint param);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glPixelZoom, GLfloat xfactor, GLfloat yfactor);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glPointSize, GLfloat size);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glPolygonMode, GLenum face, GLenum mode);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glPolygonStipple, const GLubyte* mask);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glPopAttrib);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glPopMatrix);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glPopName);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glPushAttrib, GLbitfield mask);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glPushMatrix);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glPushName, GLuint name);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRasterPos2d, GLdouble x, GLdouble y);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRasterPos2dv, const GLdouble* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRasterPos2f, GLfloat x, GLfloat y);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRasterPos2fv, const GLfloat* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRasterPos2i, GLint x, GLint y);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRasterPos2iv, const GLint* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRasterPos2s, GLshort x, GLshort y);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRasterPos2sv, const GLshort* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRasterPos3d, GLdouble x, GLdouble y, GLdouble z);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRasterPos3dv, const GLdouble* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRasterPos3f, GLfloat x, GLfloat y, GLfloat z);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRasterPos3fv, const GLfloat* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRasterPos3i, GLint x, GLint y, GLint z);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRasterPos3iv, const GLint* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRasterPos3s, GLshort x, GLshort y, GLshort z);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRasterPos3sv, const GLshort* v);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glRasterPos4d,
                          GLdouble x,
                          GLdouble y,
                          GLdouble z,
                          GLdouble w);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRasterPos4dv, const GLdouble* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRasterPos4f, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRasterPos4fv, const GLfloat* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRasterPos4i, GLint x, GLint y, GLint z, GLint w);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRasterPos4iv, const GLint* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRasterPos4s, GLshort x, GLshort y, GLshort z, GLshort w);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRasterPos4sv, const GLshort* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glReadBuffer, GLenum mode);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glReadPixels,
                          GLint x,
                          GLint y,
                          GLsizei width,
                          GLsizei height,
                          GLenum format,
                          GLenum type,
                          GLvoid* pixels);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRectd, GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRectdv, const GLdouble* v1, const GLdouble* v2);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRectf, GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRectfv, const GLfloat* v1, const GLfloat* v2);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRecti, GLint x1, GLint y1, GLint x2, GLint y2);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRectiv, const GLint* v1, const GLint* v2);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRects, GLshort x1, GLshort y1, GLshort x2, GLshort y2);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRectsv, const GLshort* v1, const GLshort* v2);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glRotated,
                          GLdouble angle,
                          GLdouble x,
                          GLdouble y,
                          GLdouble z);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glRotatef, GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glScaled, GLdouble x, GLdouble y, GLdouble z);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glScalef, GLfloat x, GLfloat y, GLfloat z);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glScissor, GLint x, GLint y, GLsizei width, GLsizei height);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glSelectBuffer, GLsizei size, GLuint* buffer);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glShadeModel, GLenum mode);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glStencilFunc, GLenum func, GLint ref, GLuint mask);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glStencilMask, GLuint mask);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glStencilOp, GLenum fail, GLenum zfail, GLenum zpass);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord1d, GLdouble s);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord1dv, const GLdouble* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord1f, GLfloat s);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord1fv, const GLfloat* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord1i, GLint s);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord1iv, const GLint* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord1s, GLshort s);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord1sv, const GLshort* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord2d, GLdouble s, GLdouble t);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord2dv, const GLdouble* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord2f, GLfloat s, GLfloat t);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord2fv, const GLfloat* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord2i, GLint s, GLint t);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord2iv, const GLint* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord2s, GLshort s, GLshort t);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord2sv, const GLshort* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord3d, GLdouble s, GLdouble t, GLdouble r);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord3dv, const GLdouble* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord3f, GLfloat s, GLfloat t, GLfloat r);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord3fv, const GLfloat* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord3i, GLint s, GLint t, GLint r);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord3iv, const GLint* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord3s, GLshort s, GLshort t, GLshort r);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord3sv, const GLshort* v);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glTexCoord4d,
                          GLdouble s,
                          GLdouble t,
                          GLdouble r,
                          GLdouble q);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord4dv, const GLdouble* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord4f, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord4fv, const GLfloat* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord4i, GLint s, GLint t, GLint r, GLint q);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord4iv, const GLint* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord4s, GLshort s, GLshort t, GLshort r, GLshort q);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexCoord4sv, const GLshort* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexEnvf, GLenum target, GLenum pname, GLfloat param);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glTexEnvfv,
                          GLenum target,
                          GLenum pname,
                          const GLfloat* params);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexEnvi, GLenum target, GLenum pname, GLint param);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glTexEnviv,
                          GLenum target,
                          GLenum pname,
                          const GLint* params);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexGend, GLenum coord, GLenum pname, GLdouble param);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glTexGendv,
                          GLenum coord,
                          GLenum pname,
                          const GLdouble* params);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexGenf, GLenum coord, GLenum pname, GLfloat param);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glTexGenfv,
                          GLenum coord,
                          GLenum pname,
                          const GLfloat* params);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexGeni, GLenum coord, GLenum pname, GLint param);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexGeniv, GLenum coord, GLenum pname, const GLint* params);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glTexImage1D,
                          GLenum target,
                          GLint level,
                          GLint components,
                          GLsizei width,
                          GLint border,
                          GLenum format,
                          GLenum type,
                          const GLvoid* pixels);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glTexImage2D,
                          GLenum target,
                          GLint level,
                          GLint components,
                          GLsizei width,
                          GLsizei height,
                          GLint border,
                          GLenum format,
                          GLenum type,
                          const GLvoid* pixels);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexParameterf, GLenum target, GLenum pname, GLfloat param);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glTexParameterfv,
                          GLenum target,
                          GLenum pname,
                          const GLfloat* params);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTexParameteri, GLenum target, GLenum pname, GLint param);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glTexParameteriv,
                          GLenum target,
                          GLenum pname,
                          const GLint* params);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTranslated, GLdouble x, GLdouble y, GLdouble z);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glTranslatef, GLfloat x, GLfloat y, GLfloat z);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glVertex2d, GLdouble x, GLdouble y);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glVertex2dv, const GLdouble* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glVertex2f, GLfloat x, GLfloat y);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glVertex2fv, const GLfloat* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glVertex2i, GLint x, GLint y);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glVertex2iv, const GLint* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glVertex2s, GLshort x, GLshort y);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glVertex2sv, const GLshort* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glVertex3d, GLdouble x, GLdouble y, GLdouble z);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glVertex3dv, const GLdouble* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glVertex3f, GLfloat x, GLfloat y, GLfloat z);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glVertex3fv, const GLfloat* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glVertex3i, GLint x, GLint y, GLint z);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glVertex3iv, const GLint* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glVertex3s, GLshort x, GLshort y, GLshort z);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glVertex3sv, const GLshort* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glVertex4d, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glVertex4dv, const GLdouble* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glVertex4f, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glVertex4fv, const GLfloat* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glVertex4i, GLint x, GLint y, GLint z, GLint w);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glVertex4iv, const GLint* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glVertex4s, GLshort x, GLshort y, GLshort z, GLshort w);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glVertex4sv, const GLshort* v);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glViewport, GLint x, GLint y, GLsizei width, GLsizei height);
    HOOK_FUNCTION_DECLARE(GLuint, GLAPI, glGenLists, GLsizei range);
    HOOK_FUNCTION_DECLARE(GLenum, GLAPI, glGetError);
    HOOK_FUNCTION_DECLARE(const GLubyte*, GLAPI, glGetString, GLenum name);
    HOOK_FUNCTION_DECLARE(GLboolean, GLAPI, glIsEnabled, GLenum cap);
    HOOK_FUNCTION_DECLARE(GLboolean, GLAPI, glIsList, GLuint list);
    HOOK_FUNCTION_DECLARE(GLint, GLAPI, glRenderMode, GLenum mode);
    HOOK_FUNCTION_DECLARE(GLboolean,
                          GLAPI,
                          glAreTexturesResident,
                          GLsizei n,
                          const GLuint* textures,
                          GLboolean* residences);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glArrayElement, GLint i);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glColorPointer,
                          GLint size,
                          GLenum type,
                          GLsizei stride,
                          const GLvoid* pointer);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glCopyTexImage1D,
                          GLenum target,
                          GLint level,
                          GLenum internalFormat,
                          GLint x,
                          GLint y,
                          GLsizei width,
                          GLint border);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glCopyTexImage2D,
                          GLenum target,
                          GLint level,
                          GLenum internalFormat,
                          GLint x,
                          GLint y,
                          GLsizei width,
                          GLsizei height,
                          GLint border);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glCopyTexSubImage1D,
                          GLenum target,
                          GLint level,
                          GLint xoffset,
                          GLint x,
                          GLint y,
                          GLsizei width);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glCopyTexSubImage2D,
                          GLenum target,
                          GLint level,
                          GLint xoffset,
                          GLint yoffset,
                          GLint x,
                          GLint y,
                          GLsizei width,
                          GLsizei height);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glDisableClientState, GLenum array);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glDrawArrays, GLenum mode, GLint first, GLsizei count);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glDrawElements,
                          GLenum mode,
                          GLsizei count,
                          GLenum type,
                          const GLvoid* indices);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glEdgeFlagPointer, GLsizei stride, const GLvoid* pointer);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glEnableClientState, GLenum array);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glGetPointerv, GLenum pname, GLvoid** params);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glIndexPointer,
                          GLenum type,
                          GLsizei stride,
                          const GLvoid* pointer);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glIndexub, GLubyte c);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glIndexubv, const GLubyte* c);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glInterleavedArrays,
                          GLenum format,
                          GLsizei stride,
                          const GLvoid* pointer);
    HOOK_FUNCTION_DECLARE(GLboolean, GLAPI, glIsTexture, GLuint texture);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glNormalPointer,
                          GLenum type,
                          GLsizei stride,
                          const GLvoid* pointer);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glPolygonOffset, GLfloat factor, GLfloat units);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glPopClientAttrib);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glPrioritizeTextures,
                          GLsizei n,
                          const GLuint* textures,
                          const GLclampf* priorities);
    HOOK_FUNCTION_DECLARE(void, GLAPI, glPushClientAttrib, GLbitfield mask);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glTexCoordPointer,
                          GLint size,
                          GLenum type,
                          GLsizei stride,
                          const GLvoid* pointer);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glTexSubImage1D,
                          GLenum target,
                          GLint level,
                          GLint xoffset,
                          GLsizei width,
                          GLenum format,
                          GLenum type,
                          const GLvoid* pixels);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glTexSubImage2D,
                          GLenum target,
                          GLint level,
                          GLint xoffset,
                          GLint yoffset,
                          GLsizei width,
                          GLsizei height,
                          GLenum format,
                          GLenum type,
                          const GLvoid* pixels);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          glVertexPointer,
                          GLint size,
                          GLenum type,
                          GLsizei stride,
                          const GLvoid* pointer);

    // more internal windows opengl functions
    HOOK_FUNCTION_DECLARE(void, GLAPI, glDebugEntry, DWORD arg1, DWORD arg2);
    HOOK_FUNCTION_DECLARE(void, GLAPI, GlmfBeginGlsBlock, DWORD arg1);
    HOOK_FUNCTION_DECLARE(void, GLAPI, GlmfEndGlsBlock, DWORD arg1);
    HOOK_FUNCTION_DECLARE(void, GLAPI, GlmfCloseMetaFile, DWORD arg1);
    HOOK_FUNCTION_DECLARE(void, GLAPI, GlmfEndPlayback, DWORD arg1);
    HOOK_FUNCTION_DECLARE(void, GLAPI, GlmfInitPlayback, DWORD arg1, DWORD arg2, DWORD arg3);
    HOOK_FUNCTION_DECLARE(void,
                          GLAPI,
                          GlmfPlayGlsRecord,
                          DWORD arg1,
                          DWORD arg2,
                          DWORD arg3,
                          DWORD arg4);
    HOOK_FUNCTION_DECLARE(int, WINAPI, wglChoosePixelFormat, HDC, const PIXELFORMATDESCRIPTOR*);
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, wglCopyContext, HGLRC, HGLRC, UINT);
    HOOK_FUNCTION_DECLARE(HGLRC, WINAPI, wglCreateContext, HDC);
    HOOK_FUNCTION_DECLARE(HGLRC, WINAPI, wglCreateLayerContext, HDC, int);
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, wglDeleteContext, HGLRC);
    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          wglDescribeLayerPlane,
                          HDC,
                          int,
                          int,
                          UINT,
                          LPLAYERPLANEDESCRIPTOR);
    HOOK_FUNCTION_DECLARE(int,
                          WINAPI,
                          wglDescribePixelFormat,
                          HDC hdc,
                          int iPixelFormat,
                          UINT nBytes,
                          LPPIXELFORMATDESCRIPTOR ppfd);
    HOOK_FUNCTION_DECLARE(HGLRC, WINAPI, wglGetCurrentContext);
    HOOK_FUNCTION_DECLARE(HDC, WINAPI, wglGetCurrentDC);
    HOOK_FUNCTION_DECLARE(PROC, WINAPI, wglGetDefaultProcAddress, LPCSTR);
    HOOK_FUNCTION_DECLARE(int, WINAPI, wglGetLayerPaletteEntries, HDC, int, int, int, COLORREF*);
    HOOK_FUNCTION_DECLARE(int, WINAPI, wglGetPixelFormat, HDC hdc);
    HOOK_FUNCTION_DECLARE(PROC, WINAPI, wglGetProcAddress, LPCSTR);
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, wglMakeCurrent, HDC, HGLRC);
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, wglRealizeLayerPalette, HDC, int, BOOL);
    HOOK_FUNCTION_DECLARE(int,
                          WINAPI,
                          wglSetLayerPaletteEntries,
                          HDC,
                          int,
                          int,
                          int,
                          CONST COLORREF*);
    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          wglSetPixelFormat,
                          HDC hdc,
                          int format,
                          CONST PIXELFORMATDESCRIPTOR* pfd);
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, wglShareLists, HGLRC, HGLRC);
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, wglSwapBuffers, HDC hdc);
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, wglSwapLayerBuffers, HDC, UINT);
    HOOK_FUNCTION_DECLARE(DWORD, WINAPI, wglSwapMultipleBuffers, UINT, CONST WGLSWAP*);
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, wglUseFontBitmapsA, HDC, DWORD, DWORD, DWORD);
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, wglUseFontBitmapsW, HDC, DWORD, DWORD, DWORD);
    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          wglUseFontOutlinesA,
                          HDC,
                          DWORD,
                          DWORD,
                          DWORD,
                          FLOAT,
                          FLOAT,
                          int,
                          LPGLYPHMETRICSFLOAT);
    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          wglUseFontOutlinesW,
                          HDC,
                          DWORD,
                          DWORD,
                          DWORD,
                          FLOAT,
                          FLOAT,
                          int,
                          LPGLYPHMETRICSFLOAT);

    void ApplyOGLIntercepts();
}