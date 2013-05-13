/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(OGLHOOKS_C_INCL) && !defined(UNITY_BUILD)
#define OGLHOOKS_C_INCL

// this wraps opengl and replaces everything it does with Direct3D8 calls,
// because windows opengl drivers are crashtastic on some computers when savestates are used,
// and because this way lets us reuse existing code for D3D such as AVI capture.

// current status: very basic, but sufficient to render Rescue: The Beagles and Tumiki Fighters and the Generic demo.
// may be too incomplete for other games (some functions are still empty or not display-list capable).
// lines are rendered incorrectly (need to convert to quads so they can use width and z-buffer).

#include "../../external/d3d8.h"
#include "../global.h"
#include <vector>
#include <math.h>

bool ShouldSkipDrawing(bool destIsFrontBuffer, bool destIsBackBuffer); // extern (didn't want to include wintasee.h and everything it includes just for this one function)

#define GL_UNSTARTED ((GLenum)(-1))
#define GL_POINTS 0x0000
#define GL_LINES 0x0001
#define GL_LINE_LOOP 0x0002
#define GL_LINE_STRIP 0x0003
#define GL_TRIANGLES 0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_TRIANGLE_FAN 0x0006
#define GL_QUADS 0x0007
#define GL_QUAD_STRIP 0x0008
#define GL_POLYGON 0x0009

static const int GL_FRONT = 0x0404;
static const int GL_BACK = 0x0405;
static const int GL_FRONT_AND_BACK = 0x0408;
static const int GL_CW = 0x0900;
static const int GL_CCW = 0x0901;

static const int GL_ZERO = 0;
static const int GL_ONE = 1;
static const int GL_ADD = 0x0104;

static const int GL_TEXTURE_MAG_FILTER = 0x2800;
static const int GL_TEXTURE_MIN_FILTER = 0x2801;
static const int GL_TEXTURE_WRAP_S = 0x2802;
static const int GL_TEXTURE_WRAP_T = 0x2803;
static const int GL_TEXTURE_BORDER_COLOR = 0x1004;
static const int GL_TEXTURE_PRIORITY = 0x8066;
static const int GL_NEAREST = 0x2600;
static const int GL_LINEAR = 0x2601;
static const int GL_NEAREST_MIPMAP_NEAREST = 0x2700;
static const int GL_LINEAR_MIPMAP_NEAREST = 0x2701;
static const int GL_NEAREST_MIPMAP_LINEAR = 0x2702;
static const int GL_LINEAR_MIPMAP_LINEAR = 0x2703;
static const int GL_CLAMP = 0x2900;
static const int GL_REPEAT = 0x2901;

#define GL_NO_ERROR 0x0
#define GL_INVALID_VALUE 0x0501
#define GL_INVALID_ENUM 0x0500
#define GL_INVALID_OPERATION 0x0502
#define GL_STACK_OVERFLOW 0x0503
#define GL_STACK_UNDERFLOW 0x0504
#define GL_OUT_OF_MEMORY 0x0505

#define GL_BYTE 0x1400
#define GL_UNSIGNED_BYTE 0x1401
#define GL_SHORT 0x1402
#define GL_UNSIGNED_SHORT 0x1403
#define GL_INT 0x1404
#define GL_UNSIGNED_INT 0x1405
#define GL_FLOAT 0x1406
#define GL_2_BYTES 0x1407
#define GL_3_BYTES 0x1408
#define GL_4_BYTES 0x1409
#define GL_DOUBLE 0x140A

int OGLTypeToSize(GLenum type)
{
	switch(type)
	{
	default:
		return 0;
	case GL_BYTE:
	case GL_UNSIGNED_BYTE:
		return 1;
	case GL_SHORT:
	case GL_UNSIGNED_SHORT:
	case GL_2_BYTES:
		return 2;
	case GL_3_BYTES:
		return 3;
	case GL_INT:
	case GL_UNSIGNED_INT:
	case GL_FLOAT:
	case GL_4_BYTES:
		return 4;
	case GL_DOUBLE:
		return 8;
	}
}

#define OGLCLAMPTOBYTE(x) (((x) < 0) ? 0 : (((x) > 255) ? 255 : (x)))
static inline BYTE oglclamptobyte(GLbyte x) { return (BYTE)OGLCLAMPTOBYTE(x); }
static inline BYTE oglclamptobyte(GLubyte x) { return (BYTE)OGLCLAMPTOBYTE(x); }
static inline BYTE oglclamptobyte(GLshort x) { return (BYTE)OGLCLAMPTOBYTE(x >> 8); }
static inline BYTE oglclamptobyte(GLushort x) { return (BYTE)OGLCLAMPTOBYTE(x >> 8); }
static inline BYTE oglclamptobyte(GLint x) { return (BYTE)OGLCLAMPTOBYTE(x >> 24); }
static inline BYTE oglclamptobyte(GLuint x) { return (BYTE)OGLCLAMPTOBYTE(x >> 24); }
static inline BYTE oglclamptobyte(GLfloat x) { return (BYTE)OGLCLAMPTOBYTE(x * 255.0f); }
static inline BYTE oglclamptobyte(GLdouble x) { return (BYTE)OGLCLAMPTOBYTE(x * 255.0); }

typedef DWORD OGLCOLOR;
#define OGLCOLOR_RGBA(r,g,b,a) D3DCOLOR_RGBA(a,r,g,b)

static GLenum oglBeganMode = GL_UNSTARTED; // mode in-between calls to glBegin and glEnd
struct OpenGLImmediateVertex
{
	FLOAT x,y,z,w;
	FLOAT nx,ny,nz;
	OGLCOLOR c;
	FLOAT u,v;
};
static std::vector<OpenGLImmediateVertex> oglImmediateVertices;



static const int GL_MODELVIEW = 0x1700;
static const int GL_PROJECTION = 0x1701;
static const int GL_TEXTURE = 0x1702;

static std::vector<D3DMATRIX> oglMatrixStackMV; // GL_MODELVIEW
static std::vector<D3DMATRIX> oglMatrixStackP; // GL_PROJECTION
static std::vector<D3DMATRIX> oglMatrixStackT; // GL_TEXTURE
static std::vector<D3DMATRIX>* oglMatrixStack = &oglMatrixStackMV;
static D3DMATRIX oglMatrixMV; // GL_MODELVIEW
static D3DMATRIX oglMatrixP; // GL_PROJECTION
static D3DMATRIX oglMatrixT; // GL_TEXTURE
static D3DMATRIX* oglMatrix = &oglMatrixMV;
static int oglMatrixID = GL_MODELVIEW;
bool oglDirtyMV, oglDirtyP, oglDirtyT, oglUsedT;
bool* oglDirty = &oglDirtyMV;

static void oglMulD3DMats(D3DMATRIX& m, const D3DMATRIX& m1, const D3DMATRIX& m2)
{
	D3DMATRIX temp = {
		m1.m[0][0]*m2.m[0][0] + m1.m[0][1]*m2.m[1][0] + m1.m[0][2]*m2.m[2][0] + m1.m[0][3]*m2.m[3][0],
		m1.m[0][0]*m2.m[0][1] + m1.m[0][1]*m2.m[1][1] + m1.m[0][2]*m2.m[2][1] + m1.m[0][3]*m2.m[3][1],
		m1.m[0][0]*m2.m[0][2] + m1.m[0][1]*m2.m[1][2] + m1.m[0][2]*m2.m[2][2] + m1.m[0][3]*m2.m[3][2],
		m1.m[0][0]*m2.m[0][3] + m1.m[0][1]*m2.m[1][3] + m1.m[0][2]*m2.m[2][3] + m1.m[0][3]*m2.m[3][3],
		m1.m[1][0]*m2.m[0][0] + m1.m[1][1]*m2.m[1][0] + m1.m[1][2]*m2.m[2][0] + m1.m[1][3]*m2.m[3][0],
		m1.m[1][0]*m2.m[0][1] + m1.m[1][1]*m2.m[1][1] + m1.m[1][2]*m2.m[2][1] + m1.m[1][3]*m2.m[3][1],
		m1.m[1][0]*m2.m[0][2] + m1.m[1][1]*m2.m[1][2] + m1.m[1][2]*m2.m[2][2] + m1.m[1][3]*m2.m[3][2],
		m1.m[1][0]*m2.m[0][3] + m1.m[1][1]*m2.m[1][3] + m1.m[1][2]*m2.m[2][3] + m1.m[1][3]*m2.m[3][3],
		m1.m[2][0]*m2.m[0][0] + m1.m[2][1]*m2.m[1][0] + m1.m[2][2]*m2.m[2][0] + m1.m[2][3]*m2.m[3][0],
		m1.m[2][0]*m2.m[0][1] + m1.m[2][1]*m2.m[1][1] + m1.m[2][2]*m2.m[2][1] + m1.m[2][3]*m2.m[3][1],
		m1.m[2][0]*m2.m[0][2] + m1.m[2][1]*m2.m[1][2] + m1.m[2][2]*m2.m[2][2] + m1.m[2][3]*m2.m[3][2],
		m1.m[2][0]*m2.m[0][3] + m1.m[2][1]*m2.m[1][3] + m1.m[2][2]*m2.m[2][3] + m1.m[2][3]*m2.m[3][3],
		m1.m[3][0]*m2.m[0][0] + m1.m[3][1]*m2.m[1][0] + m1.m[3][2]*m2.m[2][0] + m1.m[3][3]*m2.m[3][0],
		m1.m[3][0]*m2.m[0][1] + m1.m[3][1]*m2.m[1][1] + m1.m[3][2]*m2.m[2][1] + m1.m[3][3]*m2.m[3][1],
		m1.m[3][0]*m2.m[0][2] + m1.m[3][1]*m2.m[1][2] + m1.m[3][2]*m2.m[2][2] + m1.m[3][3]*m2.m[3][2],
		m1.m[3][0]*m2.m[0][3] + m1.m[3][1]*m2.m[1][3] + m1.m[3][2]*m2.m[2][3] + m1.m[3][3]*m2.m[3][3],
	};
	m = temp;
}



static GLfloat oglLineWidth = 1.0f; // TODO move to serverstate

static GLenum oglError = GL_NO_ERROR;

static int oglMakingDisplayList = 0;
static bool oglAllowExecuteCommands = true;

struct OpenGLClientState // see glEnableClientState docs
{
	GLbitfield mask;

	// vertex array state
	__declspec(align(4)) struct ArrayState
	{
		bool colorArrayEnabled;
		GLint colorArraySize;
		GLenum colorArrayType;
		GLsizei colorArrayStride;
		const GLvoid* colorArrayPointer;
		int colorElementSize;
		
		//bool edgeFlagArrayEnabled;
		//GLsizei edgeFlagArrayStride;
		//const GLvoid* edgeFlagArrayPointer;
		//int edgeFlagElementSize;

		//bool indexArrayEnabled;
		//GLenum indexArrayType;
		//GLsizei indexArrayStride;
		//const GLvoid* indexArrayPointer;
		//int indexElementSize;

		bool normalArrayEnabled;
		GLenum normalArrayType;
		GLsizei normalArrayStride;
		const GLvoid* normalArrayPointer;
		int normalElementSize;

		bool texCoordArrayEnabled;
		GLint texCoordArraySize;
		GLenum texCoordArrayType;
		GLsizei texCoordArrayStride;
		const GLvoid* texCoordArrayPointer;
		int texCoordElementSize;

		bool vertexArrayEnabled;
		GLint vertexArraySize;
		GLenum vertexArrayType;
		GLsizei vertexArrayStride;
		const GLvoid* vertexArrayPointer;
		int vertexElementSize;

		int aggregateElementSize;

		ArrayState()
		{
			colorArrayEnabled = false;
			colorArraySize = 4;
			colorArrayType = GL_FLOAT;
			colorArrayStride = 0;
			colorArrayPointer = 0;
			
			//edgeFlagArrayEnabled = false;
			//edgeFlagArrayStride = 0;
			//edgeFlagArrayPointer = 0;

			//indexArrayEnabled = false;
			//indexArrayType = GL_FLOAT;
			//indexArrayStride = 0;
			//indexArrayPointer = 0;

			normalArrayEnabled = false;
			normalArrayType = GL_FLOAT;
			normalArrayStride = 0;
			normalArrayPointer = 0;

			texCoordArrayEnabled = false;
			texCoordArraySize = 4;
			texCoordArrayType = GL_FLOAT;
			texCoordArrayStride = 0;
			texCoordArrayPointer = 0;

			vertexArrayEnabled = false;
			vertexArraySize = 4;
			vertexArrayType = GL_FLOAT;
			vertexArrayStride = 0;
			vertexArrayPointer = 0;

			RecalculateSizes();
		}

		void RecalculateSizes()
		{
			int elemSize;
			
			elemSize = OGLTypeToSize(colorArrayType) * colorArraySize;
			if(!colorArrayStride)
				colorArrayStride = elemSize;
			colorElementSize = colorArrayEnabled ? elemSize : 0;

			//elemSize = sizeof(GLboolean);
			//if(!edgeFlagArrayStride)
			//	edgeFlagArrayStride = elemSize;
			//edgeFlagElementSize = edgeFlagArrayEnabled ? elemSize : 0;

			//elemSize = OGLTypeToSize(indexArrayType);
			//if(!indexArrayStride)
			//	indexArrayStride = elemSize;
			//indexElementSize = indexArrayEnabled ? elemSize : 0;

			elemSize = OGLTypeToSize(normalArrayType) * 3;
			if(!normalArrayStride)
				normalArrayStride = elemSize;
			normalElementSize = normalArrayEnabled ? elemSize : 0;

			elemSize = OGLTypeToSize(texCoordArrayType) * texCoordArraySize;
			if(!texCoordArrayStride)
				texCoordArrayStride = elemSize;
			texCoordElementSize = texCoordArrayEnabled ? elemSize : 0;

			elemSize = OGLTypeToSize(vertexArrayType) * vertexArraySize;
			if(!vertexArrayStride)
				vertexArrayStride = elemSize;
			vertexElementSize = vertexArrayEnabled ? elemSize : 0;

			aggregateElementSize = colorElementSize + /*edgeFlagElementSize + indexElementSize +*/ normalElementSize + texCoordElementSize + vertexElementSize;
		}
	} arrayState;

	struct PixelState // see glPixelStore docs
	{
		bool glpackswapbytes;
		bool glpacklsbfirst;
		int glpackrowlength;
		int glpackimageheight;
		int glpackskippixels;
		int glpackskiprows;
		int glpackskipimages;
		int glpackalignment;
		
		bool glunpackswapbytes;
		bool glunpacklsbfirst;
		int glunpackrowlength;
		int glunpackimageheight;
		int glunpackskippixels;
		int glunpackskiprows;
		int glunpackskipimages;
		int glunpackalignment;
	} pixelState;

	OpenGLClientState()
	{
		mask = ~0;
	}
};
static OpenGLClientState oglClientState;
static std::vector<OpenGLClientState> oglClientStateStack;


struct OpenGLServerState // see glEnableState docs
{
	struct EnableState
	{
		bool alphaTest;
		bool blend;
		bool cullFace;
		bool depthTest;
		bool scissorTest;
		bool texture1d;
		bool texture2d;
	} enable;
	
	OpenGLImmediateVertex current;
	
	struct ColorBuffer
	{
		GLenum srcBlend,dstBlend,blendOp;
		D3DCOLOR clearColor;
		FLOAT clearZ;
		DWORD clearStencil;
	} colorBuffer;

	struct Other // not sure where these are really stored, maybe some are in client
	{
		GLenum frontFaceMode;
		GLenum cullFaceMode;
		GLenum texFunctionMode;
		GLboolean zWriteOn;
		GLenum magFilter;
		GLenum minFilter;
	} other;
};
static OpenGLServerState oglServerState;
static std::vector<OpenGLServerState> oglServerStateStack;



static IDirect3D8* ogld3d8 = NULL;
static IDirect3DDevice8* ogld3d8Device = NULL;
static HDC oglCurrentHDC = (HDC)INVALID_HANDLE_VALUE;


struct OpenGLDisplayListEntry
{
	enum FuncID
	{
		// commented out means it doesn't go into display lists
		idglINVALID,
		idglBindTexture,
		//idglDeleteTextures,
		//idglGenTextures,
		idglAccum,
		idglAlphaFunc,
		idglBegin,
		idglBitmap,
		idglBlendFunc,
		idglCallList,
		idglCallLists,
		idglClear,
		idglClearAccum,
		idglClearColor,
		idglClearDepth,
		idglClearIndex,
		idglClearStencil,
		idglClipPlane,
		idglColor3b,
		idglColor3bv,
		idglColor3d,
		idglColor3dv,
		idglColor3f,
		idglColor3fv,
		idglColor3i,
		idglColor3iv,
		idglColor3s,
		idglColor3sv,
		idglColor3ub,
		idglColor3ubv,
		idglColor3ui,
		idglColor3uiv,
		idglColor3us,
		idglColor3usv,
		idglColor4b,
		idglColor4bv,
		idglColor4d,
		idglColor4dv,
		idglColor4f,
		idglColor4fv,
		idglColor4i,
		idglColor4iv,
		idglColor4s,
		idglColor4sv,
		idglColor4ub,
		idglColor4ubv,
		idglColor4ui,
		idglColor4uiv,
		idglColor4us,
		idglColor4usv,
		idglColorMask,
		idglColorMaterial,
		idglCopyPixels,
		idglCullFace,
		//idglDeleteLists,
		idglDepthFunc,
		idglDepthMask,
		idglDepthRange,
		idglDisable,
		idglDrawBuffer,
		idglDrawPixels,
		idglEdgeFlag,
		idglEdgeFlagv,
		idglEnable,
		idglEnd,
		//idglEndList,
		idglEvalCoord1d,
		idglEvalCoord1dv,
		idglEvalCoord1f,
		idglEvalCoord1fv,
		idglEvalCoord2d,
		idglEvalCoord2dv,
		idglEvalCoord2f,
		idglEvalCoord2fv,
		idglEvalMesh1,
		idglEvalMesh2,
		idglEvalPoint1,
		idglEvalPoint2,
		//idglFeedbackBuffer,
		//idglFinish,
		//idglFlush,
		idglFogf,
		idglFogfv,
		idglFogi,
		idglFogiv,
		idglFrontFace,
		idglFrustum,
		//idglGetBooleanv,
		//idglGetClipPlane,
		//idglGetDoublev,
		//idglGetFloatv,
		//idglGetIntegerv,
		//idglGetLightfv,
		//idglGetLightiv,
		//idglGetMapdv,
		//idglGetMapfv,
		//idglGetMapiv,
		//idglGetMaterialfv,
		//idglGetMaterialiv,
		//idglGetPixelMapfv,
		//idglGetPixelMapuiv,
		//idglGetPixelMapusv,
		//idglGetPolygonStipple,
		//idglGetTexEnvfv,
		//idglGetTexEnviv,
		//idglGetTexGendv,
		//idglGetTexGenfv,
		//idglGetTexGeniv,
		//idglGetTexImage,
		//idglGetTexLevelParameterfv,
		//idglGetTexLevelParameteriv,
		//idglGetTexParameterfv,
		//idglGetTexParameteriv,
		idglHint,
		idglIndexMask,
		idglIndexd,
		idglIndexdv,
		idglIndexf,
		idglIndexfv,
		idglIndexi,
		idglIndexiv,
		idglIndexs,
		idglIndexsv,
		idglInitNames,
		idglLightModelf,
		idglLightModelfv,
		idglLightModeli,
		idglLightModeliv,
		idglLightf,
		idglLightfv,
		idglLighti,
		idglLightiv,
		idglLineStipple,
		idglLineWidth,
		idglListBase,
		idglLoadIdentity,
		idglLoadMatrixd,
		idglLoadMatrixf,
		idglLoadName,
		idglLogicOp,
		idglMap1d,
		idglMap1f,
		idglMap2d,
		idglMap2f,
		idglMapGrid1d,
		idglMapGrid1f,
		idglMapGrid2d,
		idglMapGrid2f,
		idglMaterialf,
		idglMaterialfv,
		idglMateriali,
		idglMaterialiv,
		idglMatrixMode,
		idglMultMatrixd,
		idglMultMatrixf,
		//idglNewList,
		idglNormal3b,
		idglNormal3bv,
		idglNormal3d,
		idglNormal3dv,
		idglNormal3f,
		idglNormal3fv,
		idglNormal3i,
		idglNormal3iv,
		idglNormal3s,
		idglNormal3sv,
		idglOrtho,
		idglPassThrough,
		idglPixelMapfv,
		idglPixelMapuiv,
		idglPixelMapusv,
		//idglPixelStoref,
		//idglPixelStorei,
		idglPixelTransferf,
		idglPixelTransferi,
		idglPixelZoom,
		idglPointSize,
		idglPolygonMode,
		idglPolygonStipple,
		idglPopAttrib,
		idglPopMatrix,
		idglPopName,
		idglPushAttrib,
		idglPushMatrix,
		idglPushName,
		idglRasterPos2d,
		idglRasterPos2dv,
		idglRasterPos2f,
		idglRasterPos2fv,
		idglRasterPos2i,
		idglRasterPos2iv,
		idglRasterPos2s,
		idglRasterPos2sv,
		idglRasterPos3d,
		idglRasterPos3dv,
		idglRasterPos3f,
		idglRasterPos3fv,
		idglRasterPos3i,
		idglRasterPos3iv,
		idglRasterPos3s,
		idglRasterPos3sv,
		idglRasterPos4d,
		idglRasterPos4dv,
		idglRasterPos4f,
		idglRasterPos4fv,
		idglRasterPos4i,
		idglRasterPos4iv,
		idglRasterPos4s,
		idglRasterPos4sv,
		idglReadBuffer,
		//idglReadPixels,
		idglRectd,
		idglRectdv,
		idglRectf,
		idglRectfv,
		idglRecti,
		idglRectiv,
		idglRects,
		idglRectsv,
		idglRotated,
		idglRotatef,
		idglScaled,
		idglScalef,
		idglScissor,
		//idglSelectBuffer,
		idglShadeModel,
		idglStencilFunc,
		idglStencilMask,
		idglStencilOp,
		idglTexCoord1d,
		idglTexCoord1dv,
		idglTexCoord1f,
		idglTexCoord1fv,
		idglTexCoord1i,
		idglTexCoord1iv,
		idglTexCoord1s,
		idglTexCoord1sv,
		idglTexCoord2d,
		idglTexCoord2dv,
		idglTexCoord2f,
		idglTexCoord2fv,
		idglTexCoord2i,
		idglTexCoord2iv,
		idglTexCoord2s,
		idglTexCoord2sv,
		idglTexCoord3d,
		idglTexCoord3dv,
		idglTexCoord3f,
		idglTexCoord3fv,
		idglTexCoord3i,
		idglTexCoord3iv,
		idglTexCoord3s,
		idglTexCoord3sv,
		idglTexCoord4d,
		idglTexCoord4dv,
		idglTexCoord4f,
		idglTexCoord4fv,
		idglTexCoord4i,
		idglTexCoord4iv,
		idglTexCoord4s,
		idglTexCoord4sv,
		idglTexEnvf,
		idglTexEnvfv,
		idglTexEnvi,
		idglTexEnviv,
		idglTexGend,
		idglTexGendv,
		idglTexGenf,
		idglTexGenfv,
		idglTexGeni,
		idglTexGeniv,
		idglTexImage1D,
		idglTexImage2D,
		idglTexParameterf,
		idglTexParameterfv,
		idglTexParameteri,
		idglTexParameteriv,
		idglTranslated,
		idglTranslatef,
		idglVertex2d,
		idglVertex2dv,
		idglVertex2f,
		idglVertex2fv,
		idglVertex2i,
		idglVertex2iv,
		idglVertex2s,
		idglVertex2sv,
		idglVertex3d,
		idglVertex3dv,
		idglVertex3f,
		idglVertex3fv,
		idglVertex3i,
		idglVertex3iv,
		idglVertex3s,
		idglVertex3sv,
		idglVertex4d,
		idglVertex4dv,
		idglVertex4f,
		idglVertex4fv,
		idglVertex4i,
		idglVertex4iv,
		idglVertex4s,
		idglVertex4sv,
		idglViewport,
		//idglGenLists,
		//idglGetError,
		//idglGetString,
		//idglIsEnabled,
		//idglIsList,
		//idglRenderMode,
		//idglAreTexturesResident,
		idglArrayElement,
		//idglColorPointer,
		idglCopyTexImage1D,
		idglCopyTexImage2D,
		idglCopyTexSubImage1D,
		idglCopyTexSubImage2D,
		//idglDisableClientState,
//		idglDrawArrays,
//		idglDrawElements,
		idd3dDrawPrimitive,
		//idglEdgeFlagPointer,
		//idglEnableClientState,
		//idglGetPointerv,
		//idglIndexPointer,
		idglIndexub,
		idglIndexubv,
		//idglInterleavedArrays,
		//idglIsTexture,
		//idglNormalPointer,
		idglPolygonOffset,
		//idglPopClientAttrib,
		idglPrioritizeTextures,
		//idglPushClientAttrib,
		//idglTexCoordPointer,
		idglTexSubImage1D,
		idglTexSubImage2D,
		//idglVertexPointer,
	};

	union GLArg
	{
		GLenum glenum;
		GLboolean glboolean;
		GLbitfield glbitfield;
		GLbyte glbyte;		// 1-byte signed 
		GLshort glshort;	// 2-byte signed 
		GLint glint;		// 4-byte signed 
		GLubyte glubyte;	// 1-byte unsigned 
		GLushort glushort;	// 2-byte unsigned 
		GLuint gluint;		// 4-byte unsigned 
		GLsizei glsizei;	// 4-byte signed 
		GLfloat glfloat;	// single precision float 
		GLclampf glclampf;	// single precision float in [0,1] 
		GLdouble gldouble;	// double precision float 
		GLclampd glclampd;	// double precision float in [0,1] 
		void* ptr;
		DWORD dword;
	};

	FuncID id;
	std::vector<GLArg> args;
protected:
	char* buffer;
public:

	void Call();

	void StoreBuffer(const void* source, int numBytes, int alignment=4)
	{
		memcpy(AllocBuffer(numBytes,alignment), source, numBytes);
	}

	void* AllocBuffer(int numBytes, int alignment=4)
	{
#if _MSC_VER > 1310
		if(buffer)
			_aligned_free(buffer);
		buffer = (char*)_aligned_malloc(numBytes, alignment);
#else
		if(buffer)
			free(buffer);
		buffer = (char*)malloc(numBytes);
#endif
		return (void*)buffer;
	}

	void Clear()
	{
		if(id == idd3dDrawPrimitive)
			((IDirect3DVertexBuffer8*)args[4].ptr)->Release();
		id = idglINVALID;
		args.clear();
		if(buffer)
		{
#if _MSC_VER > 1310
			_aligned_free(buffer);
#else
			free(buffer);
#endif
			buffer = NULL;
		}
	}
};

struct OpenGLDisplayList
{
	bool valid;
	std::vector<OpenGLDisplayListEntry> entries;

	void Call()
	{
		int size = entries.size();
		for(int i = 0; i < size; i++)
			entries[i].Call();
	}
	void Clear()
	{
		int size = entries.size();
		for(int i = 1; i < size; i++)
			entries[i].Clear();
		if(size > 1)
			entries.resize(1);
		valid = false;
	}
};
static std::vector<OpenGLDisplayList> oglDisplayLists;


struct OpenGLTexture
{
	bool named;
	GLenum bound;

	GLsizei width;
	GLsizei height;
	//GLsizei depth;
	GLenum internalFormat;
	//GLint border;

	IDirect3DTexture8* d3dTexture;

	void ClearData()
	{
		if(d3dTexture)
		{
			d3dTexture->Release();
			d3dTexture = NULL;
		}
	}
	void Clear()
	{
		named = false;
		bound = 0;
		width = 0;
		height = 0;
		internalFormat = 0;
		ClearData();
	}
	OpenGLTexture() : d3dTexture(NULL)
	{
		Clear();
	}
};
static std::vector<OpenGLTexture> oglTextures;
int oglTexture1DTarget = 0;
int oglTexture2DTarget = 0;
float oglTextureOffsets [2] = {}; // because opengl and directx use different texel origins
int oglTextureDims [2] = {};

static const int GL_TEXTURE_1D = 0x0DE0;
static const int GL_TEXTURE_2D = 0x0DE1;
static const int GL_PROXY_TEXTURE_1D = 0x8063;
static const int GL_PROXY_TEXTURE_2D = 0x8064;

#define GL_BITMAP 0x1A00
#define GL_COLOR_INDEX 0x1900
#define GL_STENCIL_INDEX 0x1901
#define GL_DEPTH_COMPONENT 0x1902
#define GL_RED 0x1903
#define GL_GREEN 0x1904
#define GL_BLUE 0x1905
#define GL_ALPHA 0x1906
#define GL_RGB 0x1907
#define GL_RGBA 0x1908
#define GL_LUMINANCE 0x1909
#define GL_LUMINANCE_ALPHA 0x190A

static const int GL_ALPHA4 = 0x803B;
static const int GL_ALPHA8 = 0x803C;
static const int GL_ALPHA12 = 0x803D;
static const int GL_ALPHA16 = 0x803E;
static const int GL_LUMINANCE4 = 0x803F;
static const int GL_LUMINANCE8 = 0x8040;
static const int GL_LUMINANCE12 = 0x8041;
static const int GL_LUMINANCE16 = 0x8042;
static const int GL_LUMINANCE4_ALPHA4 = 0x8043;
static const int GL_LUMINANCE6_ALPHA2 = 0x8044;
static const int GL_LUMINANCE8_ALPHA8 = 0x8045;
static const int GL_LUMINANCE12_ALPHA4 = 0x8046;
static const int GL_LUMINANCE12_ALPHA12 = 0x8047;
static const int GL_LUMINANCE16_ALPHA16 = 0x8048;
static const int GL_INTENSITY = 0x8049;
static const int GL_INTENSITY4 = 0x804A;
static const int GL_INTENSITY8 = 0x804B;
static const int GL_INTENSITY12 = 0x804C;
static const int GL_INTENSITY16 = 0x804D;
static const int GL_R3_G3_B2 = 0x2A10;
static const int GL_RGB4 = 0x804F;
static const int GL_RGB5 = 0x8050;
static const int GL_RGB8 = 0x8051;
static const int GL_RGB10 = 0x8052;
static const int GL_RGB12 = 0x8053;
static const int GL_RGB16 = 0x8054;
static const int GL_RGBA2 = 0x8055;
static const int GL_RGBA4 = 0x8056;
static const int GL_RGB5_A1 = 0x8057;
static const int GL_RGBA8 = 0x8058;
static const int GL_RGB10_A2 = 0x8059;
static const int GL_RGBA12 = 0x805A;
static const int GL_RGBA16 = 0x805B;
static const int GL_RGB565 = 0x8D62;

static void InitGLState()
{
	oglAllowExecuteCommands = (ogld3d8Device != NULL);
	oglError = GL_NO_ERROR;
	oglBeganMode = GL_UNSTARTED;
	oglMatrixID = GL_MODELVIEW;
	oglMatrixStack = &oglMatrixStackMV;
	oglMatrix = &oglMatrixMV;
	oglDirty = &oglDirtyMV;
	oglDirtyMV = true;
	oglDirtyP = true;
	oglDirtyT = true;
	oglUsedT = false;
	D3DMATRIX ident = {
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1,
	};
	oglMatrixMV = ident;
	oglMatrixP = ident;
	oglMatrixT = ident;
	oglTexture1DTarget = 0;
	oglTexture2DTarget = 0;
	oglTextureOffsets[0] = 0;
	oglTextureOffsets[1] = 0;
	oglTextureDims[0] = 0;
	oglTextureDims[1] = 0;
}

static void oglSetD3dCullMode()
{
	DWORD d3dcull = D3DCULL_NONE;
	if(oglServerState.enable.cullFace)
	{
		switch(MAKELONG(oglServerState.other.frontFaceMode, oglServerState.other.cullFaceMode))
		{
		case MAKELONG(GL_CCW, GL_BACK): d3dcull = D3DCULL_CW; break;
		case MAKELONG(GL_CW, GL_BACK): d3dcull = D3DCULL_CCW; break;
		case MAKELONG(GL_CCW, GL_FRONT): d3dcull = D3DCULL_CCW; break;
		case MAKELONG(GL_CW, GL_FRONT): d3dcull = D3DCULL_CW; break;
		case MAKELONG(GL_CCW, GL_FRONT_AND_BACK): d3dcull = D3DCULL_CCW; break; // NYI
		case MAKELONG(GL_CW, GL_FRONT_AND_BACK): d3dcull = D3DCULL_CW; break; // NYI
		}
	}
	ogld3d8Device->SetRenderState(D3DRS_CULLMODE,d3dcull);
}

static void oglSetD3dDepthTest()
{
	ogld3d8Device->SetRenderState(D3DRS_ZENABLE, oglServerState.enable.depthTest ? D3DZB_TRUE : D3DZB_FALSE);
	ogld3d8Device->SetRenderState(D3DRS_ZWRITEENABLE, oglServerState.other.zWriteOn && oglServerState.enable.depthTest);
}


static void InitDevice(bool fullscreen, HWND hwnd, int width=0, int height=0)
{
	if(!hwnd)
	{
		extern HWND gamehwnd;
		hwnd = gamehwnd;
	}
	if(ogld3d8Device)
	{
		ogld3d8Device->Release();
		ogld3d8Device = NULL;
	}
    D3DDISPLAYMODE displayMode;
	ogld3d8->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &displayMode);
	D3DPRESENT_PARAMETERS presentParams = {};
	presentParams.BackBufferFormat = displayMode.Format;
	presentParams.BackBufferWidth = (fullscreen&&width) ? width : displayMode.Width;
	presentParams.BackBufferHeight = (fullscreen&&height) ? height : displayMode.Height;
	if(!fullscreen && hwnd && (!width || !height))
	{
		RECT rect;
		if(GetClientRect(hwnd, &rect))
		{
			presentParams.BackBufferWidth = rect.right - rect.left;
			presentParams.BackBufferHeight = rect.bottom - rect.top;
		}
	}
	presentParams.Windowed = fullscreen ? 0 : 1;
	presentParams.SwapEffect = D3DSWAPEFFECT_DISCARD;//D3DSWAPEFFECT_COPY_VSYNC
	presentParams.EnableAutoDepthStencil = TRUE;
	presentParams.AutoDepthStencilFormat = D3DFMT_D16;//D3DFMT_D24X8
	HRESULT hres = ogld3d8->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &presentParams, &ogld3d8Device);
	if(FAILED(hres)){
		debuglog(LCF_OGL|LCF_D3D|LCF_ERROR,"OGL D3D CreateDevice FAILED with HRESULT 0x%X\n", hres);
	}
	if(ogld3d8Device)
	{
		ogld3d8Device->SetTextureStageState(0, D3DTSS_COLOROP,D3DTOP_MODULATE);
		ogld3d8Device->SetTextureStageState(0, D3DTSS_COLORARG1,D3DTA_TEXTURE);
		ogld3d8Device->SetTextureStageState(0, D3DTSS_COLORARG2,D3DTA_CURRENT);
		ogld3d8Device->SetTextureStageState(0, D3DTSS_ALPHAOP,D3DTOP_MODULATE);
		ogld3d8Device->SetTextureStageState(0, D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
		ogld3d8Device->SetTextureStageState(0, D3DTSS_ALPHAARG2,D3DTA_CURRENT);
		
		ogld3d8Device->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);
		ogld3d8Device->SetRenderState(D3DRS_LIGHTING,FALSE);
		ogld3d8Device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
		ogld3d8Device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		ogld3d8Device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL); // hack for tumuki fighters
		ogld3d8Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
		ogld3d8Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
		ogld3d8Device->SetTextureStageState(0, D3DTSS_MAGFILTER,D3DTEXF_POINT);
		ogld3d8Device->SetTextureStageState(0, D3DTSS_MINFILTER,D3DTEXF_POINT);
		ogld3d8Device->SetTextureStageState(0, D3DTSS_MIPFILTER,D3DTEXF_NONE);
		ogld3d8Device->SetTextureStageState(0, D3DTSS_ADDRESSU,D3DTADDRESS_WRAP);
		ogld3d8Device->SetTextureStageState(0, D3DTSS_ADDRESSV,D3DTADDRESS_WRAP);

		oglServerState.enable.alphaTest = false;
		oglServerState.enable.blend = false;
		oglServerState.enable.cullFace = false;
		oglServerState.other.frontFaceMode = GL_CCW;
		oglServerState.other.cullFaceMode = GL_BACK;
		oglServerState.enable.depthTest = false;
		oglServerState.other.zWriteOn = true;
		oglServerState.enable.scissorTest = false;
		oglServerState.enable.texture1d = false;
		oglServerState.enable.texture2d = false;
		oglServerState.colorBuffer.srcBlend = GL_ONE;
		oglServerState.colorBuffer.dstBlend = GL_ZERO;
		oglServerState.colorBuffer.blendOp = GL_ADD;
		oglServerState.colorBuffer.clearColor = D3DCOLOR_RGBA(0,0,0,0);
		oglServerState.colorBuffer.clearZ = 1.0f; // important
		oglServerState.colorBuffer.clearStencil = 0;
		oglServerState.other.magFilter = GL_NEAREST;

		oglSetD3dCullMode();
		oglSetD3dDepthTest();

		ogld3d8Device->BeginScene();
	}
}


HOOKFUNC IDirect3D8* WINAPI MyDirect3DCreate8(UINT SDKVersion);

static void InitOGLD3D(HWND hwnd=NULL)
{
	LoadLibrary("d3d8.dll"); // make sure d3d8 functions are available (hooking magic takes care of the rest, so there's no need to call GetProcAddress here)
	ogld3d8 = MyDirect3DCreate8(D3D_SDK_VERSION);
	InitDevice(false, hwnd);
	InitGLState();
};


// todo: delete
struct OpenGLInit
{
	OpenGLInit()
	{
		InitOGLD3D();
	}

	static OpenGLInit& Init()
	{
		static OpenGLInit inst;
		return inst;
	}
};


#define GLFUNCBOILERPLATE OpenGLInit& oglInit = OpenGLInit::Init();


static inline void glSetError(GLenum error)
{
	if(error != GL_NO_ERROR && error != oglError)
	{
		debuglog(LCF_OGL|LCF_ERROR, "OpenGL Error: 0x%04X\n", error);
#ifdef _DEBUG
		cmdprintf("SHORTTRACE: 3,120");
#endif
	}
	if(oglError == GL_NO_ERROR || error == GL_NO_ERROR)
		oglError = error;
}
#define OGLRETURNERROR(error) do { glSetError(error); return; } while(0)
#define OGLRETURNERRORVAL(error,val) do { glSetError(error); return val; } while(0)


#define OGLPDLE0(name) if(oglMakingDisplayList) { OpenGLDisplayListEntry entry; entry.id = OpenGLDisplayListEntry::name
#define OGLPDLEN(name,nargs) OGLPDLE0(name); OpenGLDisplayListEntry::GLArg arg; entry.args.reserve(nargs)
#define OGLPDLEA(t,v) arg.t = v; entry.args.push_back(arg)
#define OGLPDLEV(sizt,v,n) entry.StoreBuffer(v, (sizt)*(n), sizt)
#define OGLPDLEE() oglDisplayLists[0].entries.push_back(entry); } do{}while(0)
#define OGLPUSHDISPLAYLISTENTRY_0ARG(name) do { OGLPDLE0(name); OGLPDLEE(); } while(0)
#define OGLPUSHDISPLAYLISTENTRY_1ARG(name, t1,v1) do { OGLPDLEN(name,1); OGLPDLEA(t1,v1); OGLPDLEE(); } while(0)
#define OGLPUSHDISPLAYLISTENTRY_2ARG(name, t1,v1, t2,v2) do { OGLPDLEN(name,1); OGLPDLEA(t1,v1); OGLPDLEA(t2,v2); OGLPDLEE(); } while(0)
#define OGLPUSHDISPLAYLISTENTRY_3ARG(name, t1,v1, t2,v2, t3,v3) do { OGLPDLEN(name,1); OGLPDLEA(t1,v1); OGLPDLEA(t2,v2); OGLPDLEA(t3,v3); OGLPDLEE(); } while(0)
#define OGLPUSHDISPLAYLISTENTRY_4ARG(name, t1,v1, t2,v2, t3,v3, t4,v4) do { OGLPDLEN(name,1); OGLPDLEA(t1,v1); OGLPDLEA(t2,v2); OGLPDLEA(t3,v3); OGLPDLEA(t4,v4); OGLPDLEE(); } while(0)
#define OGLPUSHDISPLAYLISTENTRY_5ARG(name, t1,v1, t2,v2, t3,v3, t4,v4, t5,v5) do { OGLPDLEN(name,1); OGLPDLEA(t1,v1); OGLPDLEA(t2,v2); OGLPDLEA(t3,v3); OGLPDLEA(t4,v4); OGLPDLEA(t5,v5); OGLPDLEE(); } while(0)
#define OGLPUSHDISPLAYLISTENTRY_6ARG(name, t1,v1, t2,v2, t3,v3, t4,v4, t5,v5, t6,v6) do { OGLPDLEN(name,1); OGLPDLEA(t1,v1); OGLPDLEA(t2,v2); OGLPDLEA(t3,v3); OGLPDLEA(t4,v4); OGLPDLEA(t5,v5); OGLPDLEA(t6,v6); OGLPDLEE(); } while(0)
#define OGLPUSHDISPLAYLISTENTRY_7ARG(name, t1,v1, t2,v2, t3,v3, t4,v4, t5,v5, t6,v6, t7,v7) do { OGLPDLEN(name,1); OGLPDLEA(t1,v1); OGLPDLEA(t2,v2); OGLPDLEA(t3,v3); OGLPDLEA(t4,v4); OGLPDLEA(t5,v5); OGLPDLEA(t6,v6); OGLPDLEA(t7,v7); OGLPDLEE(); } while(0)
#define OGLPUSHDISPLAYLISTENTRY_8ARG(name, t1,v1, t2,v2, t3,v3, t4,v4, t5,v5, t6,v6, t7,v7, t8,v8) do { OGLPDLEN(name,1); OGLPDLEA(t1,v1); OGLPDLEA(t2,v2); OGLPDLEA(t3,v3); OGLPDLEA(t4,v4); OGLPDLEA(t5,v5); OGLPDLEA(t6,v6); OGLPDLEA(t7,v7); OGLPDLEA(t8,v8); OGLPDLEE(); } while(0)
#define OGLPUSHDISPLAYLISTENTRY_9ARG(name, t1,v1, t2,v2, t3,v3, t4,v4, t5,v5, t6,v6, t7,v7, t8,v8, t9,v9) do { OGLPDLEN(name,1); OGLPDLEA(t1,v1); OGLPDLEA(t2,v2); OGLPDLEA(t3,v3); OGLPDLEA(t4,v4); OGLPDLEA(t5,v5); OGLPDLEA(t6,v6); OGLPDLEA(t7,v7); OGLPDLEA(t8,v8); OGLPDLEA(t9,v9); OGLPDLEE(); } while(0)
#define OGLPUSHDISPLAYLISTENTRY_0ARGV(name, sizt,v,n) do { OGLPDLE0(name); OGLPDLEV(sizt,v,n); OGLPDLEE(); } while(0)
#define OGLPUSHDISPLAYLISTENTRY_1ARGV(name, t1,v1, sizt,v,n) do { OGLPDLEN(name,1); OGLPDLEA(t1,v1); OGLPDLEV(sizt,v,n); OGLPDLEE(); } while(0)
#define OGLPUSHDISPLAYLISTENTRY_2ARGV(name, t1,v1, t2,v2, sizt,v,n) do { OGLPDLEN(name,1); OGLPDLEA(t1,v1); OGLPDLEA(t2,v2); OGLPDLEV(sizt,v,n); OGLPDLEE(); } while(0)
#define OGLPUSHDISPLAYLISTENTRY_3ARGV(name, t1,v1, t2,v2, t3,v3, sizt,v,n) do { OGLPDLEN(name,1); OGLPDLEA(t1,v1); OGLPDLEA(t2,v2); OGLPDLEA(t3,v3); OGLPDLEV(sizt,v,n); OGLPDLEE(); } while(0)
#define OGLPUSHDISPLAYLISTENTRY_4ARGV(name, t1,v1, t2,v2, t3,v3, t4,v4, sizt,v,n) do { OGLPDLEN(name,1); OGLPDLEA(t1,v1); OGLPDLEA(t2,v2); OGLPDLEA(t3,v3); OGLPDLEA(t4,v4); OGLPDLEV(sizt,v,n); OGLPDLEE(); } while(0)
#define OGLPUSHDISPLAYLISTENTRY_5ARGV(name, t1,v1, t2,v2, t3,v3, t4,v4, t5,v5, sizt,v,n) do { OGLPDLEN(name,1); OGLPDLEA(t1,v1); OGLPDLEA(t2,v2); OGLPDLEA(t3,v3); OGLPDLEA(t4,v4); OGLPDLEA(t5,v5); OGLPDLEV(sizt,v,n); OGLPDLEE(); } while(0)
#define OGLPUSHDISPLAYLISTENTRY_6ARGV(name, t1,v1, t2,v2, t3,v3, t4,v4, t5,v5, t6,v6, sizt,v,n) do { OGLPDLEN(name,1); OGLPDLEA(t1,v1); OGLPDLEA(t2,v2); OGLPDLEA(t3,v3); OGLPDLEA(t4,v4); OGLPDLEA(t5,v5); OGLPDLEA(t6,v6); OGLPDLEV(sizt,v,n); OGLPDLEE(); } while(0)
#define OGLPUSHDISPLAYLISTENTRY_7ARGV(name, t1,v1, t2,v2, t3,v3, t4,v4, t5,v5, t6,v6, t7,v7, sizt,v,n) do { OGLPDLEN(name,1); OGLPDLEA(t1,v1); OGLPDLEA(t2,v2); OGLPDLEA(t3,v3); OGLPDLEA(t4,v4); OGLPDLEA(t5,v5); OGLPDLEA(t6,v6); OGLPDLEA(t7,v7); OGLPDLEV(sizt,v,n); OGLPDLEE(); } while(0)



HOOKFUNC void GLAPI MyglBegin(GLenum mode)
{
	debuglog(LCF_OGL, __FUNCTION__ "(mode=0x%X) called.\n", mode);
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(oglBeganMode != GL_UNSTARTED || (unsigned int)mode > GL_POLYGON)
			OGLRETURNERROR(GL_INVALID_OPERATION);
		oglBeganMode = mode;
		oglImmediateVertices.clear();
	}
	OGLPUSHDISPLAYLISTENTRY_1ARG(idglBegin, glenum,mode);
}

//static void OglGetD3dDrawInfo(GLenum mode, GLsizei count, DWORD& fvf, DWORD& elemSize, DWORD& primType, int& primCount, int& elemCount);
static void OglDrawToD3D(GLenum mode, GLsizei count, GLint first, GLenum type, const GLvoid* indices, bool renderNow, bool makeDisplayList);

HOOKFUNC void GLAPI MyglEnd()
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(oglBeganMode == GL_UNSTARTED)
			OGLRETURNERROR(GL_INVALID_OPERATION);

		if(oglImmediateVertices.size())
		{
			if(!ShouldSkipDrawing(false, true))
			{
				//DWORD elemSize;
				//DWORD fvf;
				//DWORD primType;
				//int primCount;
				//int elemCount;
				//OglGetD3dDrawInfo(oglBeganMode,oglImmediateVertices.size(), fvf,elemSize,primType,primCount,elemCount);

				OpenGLClientState::ArrayState prevArrayState = oglClientState.arrayState;

				// hack? maybe should leave colorarray disabled if no glcolor calls, etc.?
				oglClientState.arrayState.texCoordArrayEnabled = true;
				oglClientState.arrayState.colorArrayEnabled = true;
				oglClientState.arrayState.normalArrayEnabled = true;
				oglClientState.arrayState.vertexArrayEnabled = true;

				oglClientState.arrayState.vertexArraySize = 4;
				oglClientState.arrayState.vertexArrayType = GL_FLOAT;
				oglClientState.arrayState.vertexArrayStride = 0;
				oglClientState.arrayState.normalArrayType = GL_FLOAT;
				oglClientState.arrayState.normalArrayStride = 0;
				oglClientState.arrayState.colorArraySize = 4;
				oglClientState.arrayState.colorArrayType = GL_UNSIGNED_BYTE;
				oglClientState.arrayState.colorArrayStride = 0;
				oglClientState.arrayState.texCoordArraySize = 2;
				oglClientState.arrayState.texCoordArrayType = GL_FLOAT;
				oglClientState.arrayState.texCoordArrayStride = 0;
				oglClientState.arrayState.RecalculateSizes();
				int offset = 0;
				oglClientState.arrayState.vertexArrayPointer = (const GLvoid*)(&((char*)(&oglImmediateVertices[0]))[offset]);
				offset += oglClientState.arrayState.vertexArrayStride;
				oglClientState.arrayState.normalArrayPointer = (const GLvoid*)(&((char*)(&oglImmediateVertices[0]))[offset]);
				offset += oglClientState.arrayState.normalArrayStride;
				oglClientState.arrayState.colorArrayPointer = (const GLvoid*)(&((char*)(&oglImmediateVertices[0]))[offset]);
				offset += oglClientState.arrayState.colorArrayStride;
				oglClientState.arrayState.texCoordArrayPointer = (const GLvoid*)(&((char*)(&oglImmediateVertices[0]))[offset]);
				offset += oglClientState.arrayState.texCoordArrayStride;
				oglClientState.arrayState.texCoordArrayStride = offset;
				oglClientState.arrayState.colorArrayStride = offset;
				oglClientState.arrayState.normalArrayStride = offset;
				oglClientState.arrayState.vertexArrayStride = offset;
				OglDrawToD3D(oglBeganMode,oglImmediateVertices.size(), 0, 0,NULL, true,oglMakingDisplayList!=0);


				oglClientState.arrayState = prevArrayState;

				//ogld3d8Device->SetVertexShader(D3DFVF_XYZRHW|D3DFVF_NORMAL|D3DFVF_DIFFUSE|D3DFVF_TEX1); //fvf
				//ogld3d8Device->SetStreamSource(0, vb, elemSize);
				//ogld3d8Device->DrawPrimitiveUP((D3DPRIMITIVETYPE)primType, , 0, primCount);
			}

			oglImmediateVertices.clear();
		}
		oglBeganMode = GL_UNSTARTED;
	}
	OGLPUSHDISPLAYLISTENTRY_0ARG(idglEnd);
}

HOOKFUNC GLubyte* GLAPI MyglGetString(GLenum name)
{
	debuglog(LCF_OGL, __FUNCTION__ "(name=0x%X) called.\n", name);
	//GLFUNCBOILERPLATE;

	static const int GL_VENDOR = 0x1F00;
	static const int GL_RENDERER = 0x1F01;
	static const int GL_VERSION = 0x1F02;
	static const int GL_EXTENSIONS = 0x1F03;

	//debuglog(LCF_TODO|LCF_OGL, "GL_VENDOR: %s\n", glGetString(GL_VENDOR));
	//debuglog(LCF_TODO|LCF_OGL, "GL_RENDERER: %s\n", glGetString(GL_RENDERER));
	//debuglog(LCF_TODO|LCF_OGL, "GL_VERSION: %s\n", glGetString(GL_VERSION));
	//debuglog(LCF_TODO|LCF_OGL, "GL_EXTENSIONS: %s\n", glGetString(GL_EXTENSIONS));

	switch(name)
	{
	case GL_VENDOR:
		return (GLubyte*)"nitsuja?";
	case GL_RENDERER:
		return (GLubyte*)"wintasee";
	case GL_VERSION:
		return (GLubyte*)"1.0";
//	case GL_EXTENSIONS:
//		return (GLubyte*)"";
	}

	return (GLubyte*)NULL;
}

HOOKFUNC void GLAPI MyglHint(GLenum target, GLenum mode)
{
	debuglog(LCF_OGL, __FUNCTION__ "(target=0x%X, mode=0x%X) called.\n", target, mode);
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		// "The interpretation of hints depends on the implementation.
		//  Some implementations ignore glHint settings."
		// and... done.
	}
	OGLPUSHDISPLAYLISTENTRY_2ARG(idglHint, glenum,target, glenum,mode);
}

HOOKFUNC void GLAPI MyglBindTexture(GLenum target, GLuint texture)
{
	debuglog(LCF_OGL, __FUNCTION__ "(target=0x%X, texture=%d) called.\n", target, texture);
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(oglBeganMode != GL_UNSTARTED)
			OGLRETURNERROR(GL_INVALID_OPERATION);
		if(target != GL_TEXTURE_1D && target != GL_TEXTURE_2D)
			OGLRETURNERROR(GL_INVALID_ENUM);
		if(texture >= oglTextures.size())
			OGLRETURNERROR(GL_INVALID_OPERATION);
		OpenGLTexture& tex = oglTextures[texture];
		if(!tex.named)
			OGLRETURNERROR(GL_INVALID_OPERATION);
		if(tex.bound)
		{
//			if(tex.bound != target)
//				OGLRETURNERROR(GL_INVALID_OPERATION);
		}
		tex.bound = target;
		switch(target)
		{
			case GL_TEXTURE_1D: oglTexture1DTarget = texture; break;
			case GL_TEXTURE_2D: oglTexture2DTarget = texture; break;
		}
		if(tex.width && tex.width != oglTextureDims[0])
		{
			float newOffset = oglMatrixP.m[0][0] < 0 ? (-0.5f / tex.width) : 0;
			oglTextureDims[0] = tex.width;
			if(oglTextureOffsets[0] != newOffset)
			{
				oglTextureOffsets[0] = newOffset;
				oglDirtyT = true;
			}
		}
		if(tex.height && tex.height != oglTextureDims[1])
		{
			float newOffset = oglMatrixP.m[1][1] > 0 ? (-0.5f / tex.height) : 0;
			oglTextureDims[1] = tex.height;
			if(oglTextureOffsets[1] != newOffset)
			{
				oglTextureOffsets[1] = newOffset;
				oglDirtyT = true;
			}
		}
		ogld3d8Device->SetTexture(0, tex.d3dTexture);
	}
	OGLPUSHDISPLAYLISTENTRY_2ARG(idglBindTexture, glenum,target, gluint,texture);
}

HOOKFUNC void GLAPI MyglDeleteTextures(GLsizei n, const GLuint* textures)
{
	debuglog(LCF_OGL, __FUNCTION__ "(n=%d, textures=%d) called.\n", n, textures);
	//GLFUNCBOILERPLATE;
	if(n < 0)
		OGLRETURNERROR(GL_INVALID_VALUE);
	if(oglBeganMode != GL_UNSTARTED)
		OGLRETURNERROR(GL_INVALID_OPERATION);
	for(int i = 0; i < n; i++)
		oglTextures[textures[i]].Clear();
	while(oglTextures.size() > 1 && !oglTextures.back().named)
		oglTextures.pop_back();
}

HOOKFUNC void GLAPI MyglGenTextures(GLsizei n, GLuint* textures)
{
	debuglog(LCF_OGL, __FUNCTION__ "(n=%d, textures=0x%X) called.\n", n, textures);
	//GLFUNCBOILERPLATE;
	if(n < 0)
		OGLRETURNERROR(GL_INVALID_VALUE);
	if(oglBeganMode != GL_UNSTARTED)
		OGLRETURNERROR(GL_INVALID_OPERATION);
	OpenGLTexture tex;
	tex.named = true;
	tex.bound = 0;
	if(oglTextures.empty())
	{
		oglTextures.push_back(tex);
		oglTextures.back().bound = 1;
	}
	for(int i = 0; i < n; i++)
	{
		textures[i] = oglTextures.size();
		oglTextures.push_back(tex);
	}
}




HOOKFUNC void GLAPI MyglAccum (GLenum op, GLfloat value)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
	OGLPUSHDISPLAYLISTENTRY_2ARG(idglAccum, glenum,op, glfloat,value);
}

static const int GL_NEVER = 0x0200;
static const int GL_LESS = 0x0201;
static const int GL_EQUAL = 0x0202;
static const int GL_LEQUAL = 0x0203;
static const int GL_GREATER = 0x0204;
static const int GL_NOTEQUAL = 0x0205;
static const int GL_GEQUAL = 0x0206;
static const int GL_ALWAYS = 0x0207;

HOOKFUNC void GLAPI MyglAlphaFunc (GLenum func, GLclampf ref)
{
	debuglog(LCF_OGL, __FUNCTION__ "(func=0x%X, ref=%g) called.\n", func, ref);
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(oglBeganMode != GL_UNSTARTED)
			OGLRETURNERROR(GL_INVALID_OPERATION);
		switch(func)
		{
		case GL_NEVER: ogld3d8Device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_NEVER); break;
		case GL_LESS: ogld3d8Device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_LESS); break;
		case GL_EQUAL: ogld3d8Device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_EQUAL); break;
		case GL_LEQUAL: ogld3d8Device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_LESSEQUAL); break;
		case GL_GREATER: ogld3d8Device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER); break;
		case GL_NOTEQUAL: ogld3d8Device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_NOTEQUAL); break;
		case GL_GEQUAL: ogld3d8Device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL); break;
		case GL_ALWAYS: ogld3d8Device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_ALWAYS); break;
		}
		ogld3d8Device->SetRenderState(D3DRS_ALPHAREF, (DWORD)oglclamptobyte(ref));
	}
	OGLPUSHDISPLAYLISTENTRY_2ARG(idglAlphaFunc, glenum,func, glclampf,ref);
}

HOOKFUNC void GLAPI MyglBitmap (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
	OGLPUSHDISPLAYLISTENTRY_7ARG(idglBitmap, glsizei,width, glsizei,height, glfloat,xorig, glfloat,yorig, glfloat,xmove, glfloat,ymove, ptr,(void*)bitmap);
}

static DWORD OglBlendFactorToD3D(GLenum factor)
{
	static const int GL_SRC_COLOR = 0x0300;
	static const int GL_ONE_MINUS_SRC_COLOR = 0x0301;
	static const int GL_SRC_ALPHA = 0x0302;
	static const int GL_ONE_MINUS_SRC_ALPHA = 0x0303;
	static const int GL_DST_ALPHA = 0x0304;
	static const int GL_ONE_MINUS_DST_ALPHA = 0x0305;
	static const int GL_DST_COLOR = 0x0306;
	static const int GL_ONE_MINUS_DST_COLOR = 0x0307;
	static const int GL_SRC_ALPHA_SATURATE = 0x0308;
	switch(factor)
	{
	case GL_ZERO: return D3DBLEND_ZERO;
	default: case GL_ONE: return D3DBLEND_ONE;
	case GL_SRC_COLOR: return D3DBLEND_SRCCOLOR;
	case GL_ONE_MINUS_SRC_COLOR: return D3DBLEND_INVSRCCOLOR;
	case GL_SRC_ALPHA: return D3DBLEND_SRCALPHA;
	case GL_ONE_MINUS_SRC_ALPHA: return D3DBLEND_INVSRCALPHA;
	case GL_DST_ALPHA: return D3DBLEND_DESTALPHA;
	case GL_ONE_MINUS_DST_ALPHA: return D3DBLEND_INVDESTALPHA;
	case GL_DST_COLOR: return D3DBLEND_DESTCOLOR;
	case GL_ONE_MINUS_DST_COLOR: return D3DBLEND_INVDESTCOLOR;
	case GL_SRC_ALPHA_SATURATE: return D3DBLEND_SRCALPHASAT;
	}
}

HOOKFUNC void GLAPI MyglBlendFunc (GLenum sfactor, GLenum dfactor)
{
	debuglog(LCF_OGL, __FUNCTION__ "(sfactor=0x%X, dfactor=0x%X) called.\n", sfactor, dfactor);
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(oglBeganMode != GL_UNSTARTED)
			OGLRETURNERROR(GL_INVALID_OPERATION);
		ogld3d8Device->SetRenderState(D3DRS_SRCBLEND, OglBlendFactorToD3D(sfactor));
		ogld3d8Device->SetRenderState(D3DRS_DESTBLEND, OglBlendFactorToD3D(dfactor));
	}
	OGLPUSHDISPLAYLISTENTRY_2ARG(idglBlendFunc, glenum,sfactor, glenum,dfactor);
}

static void MyglCallList_internal(GLuint list)
{
	if(list >= oglDisplayLists.size() || !oglDisplayLists[list].valid)
		return;
	static int callDepth = 0;
	if(callDepth < 64) // enforce a GL_MAX_LIST_NESTING of 64
	{
		callDepth++;
		int wasMakingDisplayList = oglMakingDisplayList;
		oglMakingDisplayList = 0; // avoid embedding display lists (in case we're in compile and execute mode)

		oglDisplayLists[list].Call();

		oglMakingDisplayList = wasMakingDisplayList;
		callDepth--;
	}
}

HOOKFUNC void GLAPI MyglCallList (GLuint list)
{
	debuglog(LCF_OGL, __FUNCTION__ "(list=%d) called.\n", list);
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
		MyglCallList_internal(list);
	OGLPUSHDISPLAYLISTENTRY_1ARG(idglCallList, gluint,list);
}

HOOKFUNC void GLAPI MyglCallLists (GLsizei n, GLenum type, const GLvoid *lists)
{
	debuglog(LCF_OGL, __FUNCTION__ "(n=%d, type=0x%X, lists=0x%X) called.\n", n, type, lists);
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(n < 0)
			OGLRETURNERROR(GL_INVALID_VALUE);
		if(type != GL_BYTE && type != GL_UNSIGNED_BYTE && type != GL_SHORT && type != GL_UNSIGNED_SHORT && type != GL_INT && type != GL_UNSIGNED_INT,	GL_FLOAT && type != GL_2_BYTES && type != GL_3_BYTES && type != GL_4_BYTES)
			OGLRETURNERROR(GL_INVALID_ENUM);

		for(int i = 0; i < n; i++)
		{
			int list;
			switch(type)
			{
			case GL_BYTE: list = (GLuint)(((GLbyte*)lists)[i]); break;
			case GL_UNSIGNED_BYTE: list = (GLuint)(((GLubyte*)lists)[i]); break;
			case GL_SHORT: list = (GLuint)(((GLshort*)lists)[i]); break;
			case GL_UNSIGNED_SHORT: list = (GLuint)(((GLushort*)lists)[i]); break;
			case GL_INT: list = (GLuint)(((GLint*)lists)[i]); break;
			case GL_UNSIGNED_INT: list = (GLuint)(((GLuint*)lists)[i]); break;
			case GL_FLOAT: list = (GLuint)(((GLfloat*)lists)[i]); break;
			case GL_2_BYTES: list = (GLuint)(((GLbyte*)lists)[i*2])*256 + (GLuint)(((GLbyte*)lists)[i*2+1]); break;
			case GL_3_BYTES: list = (GLuint)(((GLbyte*)lists)[i*3])*65536 + (GLuint)(((GLbyte*)lists)[i*3+1])*256 + (GLuint)(((GLbyte*)lists)[i*3+2]); break;
			case GL_4_BYTES: list = (GLuint)(((GLbyte*)lists)[i*4])*16777216 + (GLuint)(((GLbyte*)lists)[i*4+1])*65536 + (GLuint)(((GLbyte*)lists)[i*4+2])*256 + (GLuint)(((GLbyte*)lists)[i*4+3]); break;
			}
			MyglCallList_internal(list);
		}
	}
	OGLPUSHDISPLAYLISTENTRY_2ARGV(idglCallLists, glsizei,n, glenum,type, OGLTypeToSize(type),lists,n);
}

HOOKFUNC void GLAPI MyglClear (GLbitfield mask)
{
	debuglog(LCF_OGL, __FUNCTION__ "(mask=0x%X) called.\n", mask);
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		static const int GL_DEPTH_BUFFER_BIT = 0x00000100;
		static const int GL_STENCIL_BUFFER_BIT = 0x00000400;
		static const int GL_COLOR_BUFFER_BIT = 0x00004000;
		DWORD flags = 0;
		if(mask & GL_DEPTH_BUFFER_BIT)
			flags |= D3DCLEAR_ZBUFFER;
		if(mask & GL_STENCIL_BUFFER_BIT)
			flags |= D3DCLEAR_STENCIL;
		if(mask & GL_COLOR_BUFFER_BIT)
			flags |= D3DCLEAR_TARGET;
		ogld3d8Device->Clear(0, NULL, flags, oglServerState.colorBuffer.clearColor, oglServerState.colorBuffer.clearZ, oglServerState.colorBuffer.clearStencil);
	}
	OGLPUSHDISPLAYLISTENTRY_1ARG(idglClear, glbitfield,mask);
}

HOOKFUNC void GLAPI MyglClearAccum (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
	OGLPUSHDISPLAYLISTENTRY_4ARG(idglClearAccum, glfloat,red, glfloat,green, glfloat,blue, glfloat,alpha);
}

HOOKFUNC void GLAPI MyglClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.colorBuffer.clearColor = D3DCOLOR_RGBA(oglclamptobyte(red),oglclamptobyte(green),oglclamptobyte(blue),oglclamptobyte(alpha));
	}
	OGLPUSHDISPLAYLISTENTRY_4ARG(idglClearColor, glclampf,red, glclampf,green, glclampf,blue, glclampf,alpha);
}

HOOKFUNC void GLAPI MyglClearDepth (GLclampd depth)
{
	debuglog(LCF_OGL, __FUNCTION__ "(%g) called.\n", (float)depth);
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.colorBuffer.clearZ = (FLOAT)depth;
	}
	OGLPUSHDISPLAYLISTENTRY_1ARG(idglClearDepth, glclampd,depth);
}

HOOKFUNC void GLAPI MyglClearIndex (GLfloat c)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
	OGLPUSHDISPLAYLISTENTRY_1ARG(idglClearIndex, glfloat,c);
}

HOOKFUNC void GLAPI MyglClearStencil (GLint s)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.colorBuffer.clearStencil = s;
	}
	OGLPUSHDISPLAYLISTENTRY_1ARG(idglClearStencil, glint,s);
}

HOOKFUNC void GLAPI MyglClipPlane (GLenum plane, const GLdouble *equation)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
	OGLPUSHDISPLAYLISTENTRY_1ARGV(idglClipPlane, glenum,plane, sizeof(GLdouble),equation,4);
}

HOOKFUNC void GLAPI MyglColor3b (GLbyte red, GLbyte green, GLbyte blue)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(red),oglclamptobyte(green),oglclamptobyte(blue),255);
	}
	OGLPUSHDISPLAYLISTENTRY_3ARG(idglColor3b, glbyte,red, glbyte,green, glbyte,blue);
}

HOOKFUNC void GLAPI MyglColor3bv (const GLbyte *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(v[0]),oglclamptobyte(v[1]),oglclamptobyte(v[2]),255);
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglColor3bv, sizeof(GLbyte),v,3);
}

HOOKFUNC void GLAPI MyglColor3d (GLdouble red, GLdouble green, GLdouble blue)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(red),oglclamptobyte(green),oglclamptobyte(blue),255);
	}
	OGLPUSHDISPLAYLISTENTRY_3ARG(idglColor3d, gldouble,red, gldouble,green, gldouble,blue);
}

HOOKFUNC void GLAPI MyglColor3dv (const GLdouble *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(v[0]),oglclamptobyte(v[1]),oglclamptobyte(v[2]),255);
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglColor3dv, sizeof(GLdouble),v,3);
}

HOOKFUNC void GLAPI MyglColor3f (GLfloat red, GLfloat green, GLfloat blue)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(red),oglclamptobyte(green),oglclamptobyte(blue),255);
	}
	OGLPUSHDISPLAYLISTENTRY_3ARG(idglColor3f, glfloat,red, glfloat,green, glfloat,blue);
}

HOOKFUNC void GLAPI MyglColor3fv (const GLfloat *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(v[0]),oglclamptobyte(v[1]),oglclamptobyte(v[2]),255);
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglColor3fv, sizeof(GLfloat),v,3);
}

HOOKFUNC void GLAPI MyglColor3i (GLint red, GLint green, GLint blue)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(red),oglclamptobyte(green),oglclamptobyte(blue),255);
	}
	OGLPUSHDISPLAYLISTENTRY_3ARG(idglColor3i, glint,red, glint,green, glint,blue);
}

HOOKFUNC void GLAPI MyglColor3iv (const GLint *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(v[0]),oglclamptobyte(v[1]),oglclamptobyte(v[2]),255);
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglColor3iv, sizeof(GLint),v,3);
}

HOOKFUNC void GLAPI MyglColor3s (GLshort red, GLshort green, GLshort blue)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(red),oglclamptobyte(green),oglclamptobyte(blue),255);
	}
	OGLPUSHDISPLAYLISTENTRY_3ARG(idglColor3s, glshort,red, glshort,green, glshort,blue);
}

HOOKFUNC void GLAPI MyglColor3sv (const GLshort *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(v[0]),oglclamptobyte(v[1]),oglclamptobyte(v[2]),255);
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglColor3sv, sizeof(GLshort),v,3);
}

HOOKFUNC void GLAPI MyglColor3ub (GLubyte red, GLubyte green, GLubyte blue)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(red),oglclamptobyte(green),oglclamptobyte(blue),255);
	}
	OGLPUSHDISPLAYLISTENTRY_3ARG(idglColor3ub, glubyte,red, glubyte,green, glubyte,blue);
}

HOOKFUNC void GLAPI MyglColor3ubv (const GLubyte *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(v[0]),oglclamptobyte(v[1]),oglclamptobyte(v[2]),255);
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglColor3ubv, sizeof(GLubyte),v,3);
}

HOOKFUNC void GLAPI MyglColor3ui (GLuint red, GLuint green, GLuint blue)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(red),oglclamptobyte(green),oglclamptobyte(blue),255);
	}
	OGLPUSHDISPLAYLISTENTRY_3ARG(idglColor3ui, gluint,red, gluint,green, gluint,blue);
}

HOOKFUNC void GLAPI MyglColor3uiv (const GLuint *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(v[0]),oglclamptobyte(v[1]),oglclamptobyte(v[2]),255);
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglColor3uiv, sizeof(GLuint),v,3);
}

HOOKFUNC void GLAPI MyglColor3us (GLushort red, GLushort green, GLushort blue)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(red),oglclamptobyte(green),oglclamptobyte(blue),255);
	}
	OGLPUSHDISPLAYLISTENTRY_3ARG(idglColor3us, glushort,red, glushort,green, glushort,blue);
}

HOOKFUNC void GLAPI MyglColor3usv (const GLushort *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(v[0]),oglclamptobyte(v[1]),oglclamptobyte(v[2]),255);
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglColor3usv, sizeof(GLushort),v,3);
}

HOOKFUNC void GLAPI MyglColor4b (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(red),oglclamptobyte(green),oglclamptobyte(blue),oglclamptobyte(alpha));
	}
	OGLPUSHDISPLAYLISTENTRY_4ARG(idglColor4b, glbyte,red, glbyte,green, glbyte,blue, glbyte,alpha);
}

HOOKFUNC void GLAPI MyglColor4bv (const GLbyte *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(v[0]),oglclamptobyte(v[1]),oglclamptobyte(v[2]),oglclamptobyte(v[3]));
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglColor4bv, sizeof(GLbyte),v,4);
}

HOOKFUNC void GLAPI MyglColor4d (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(red),oglclamptobyte(green),oglclamptobyte(blue),oglclamptobyte(alpha));
	}
	OGLPUSHDISPLAYLISTENTRY_4ARG(idglColor4d, gldouble,red, gldouble,green, gldouble,blue, gldouble,alpha);
}

HOOKFUNC void GLAPI MyglColor4dv (const GLdouble *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(v[0]),oglclamptobyte(v[1]),oglclamptobyte(v[2]),oglclamptobyte(v[3]));
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglColor4dv, sizeof(GLdouble),v,4);
}

HOOKFUNC void GLAPI MyglColor4f (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	debuglog(LCF_OGL, __FUNCTION__ "(%g, %g, %g, %g) called.\n", red, green, blue, alpha);
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(red),oglclamptobyte(green),oglclamptobyte(blue),oglclamptobyte(alpha));
	}
	OGLPUSHDISPLAYLISTENTRY_4ARG(idglColor4f, glfloat,red, glfloat,green, glfloat,blue, glfloat,alpha);
}

HOOKFUNC void GLAPI MyglColor4fv (const GLfloat *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(v[0]),oglclamptobyte(v[1]),oglclamptobyte(v[2]),oglclamptobyte(v[3]));
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglColor4fv, sizeof(GLfloat),v,4);
}

HOOKFUNC void GLAPI MyglColor4i (GLint red, GLint green, GLint blue, GLint alpha)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(red),oglclamptobyte(green),oglclamptobyte(blue),oglclamptobyte(alpha));
	}
	OGLPUSHDISPLAYLISTENTRY_4ARG(idglColor4i, glint,red, glint,green, glint,blue, glint,alpha);
}

HOOKFUNC void GLAPI MyglColor4iv (const GLint *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(v[0]),oglclamptobyte(v[1]),oglclamptobyte(v[2]),oglclamptobyte(v[3]));
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglColor4iv, sizeof(GLint),v,4);
}

HOOKFUNC void GLAPI MyglColor4s (GLshort red, GLshort green, GLshort blue, GLshort alpha)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(red),oglclamptobyte(green),oglclamptobyte(blue),oglclamptobyte(alpha));
	}
	OGLPUSHDISPLAYLISTENTRY_4ARG(idglColor4s, glshort,red, glshort,green, glshort,blue, glshort,alpha);
}

HOOKFUNC void GLAPI MyglColor4sv (const GLshort *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(v[0]),oglclamptobyte(v[1]),oglclamptobyte(v[2]),oglclamptobyte(v[3]));
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglColor4sv, sizeof(GLshort),v,4);
}

HOOKFUNC void GLAPI MyglColor4ub (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(red),oglclamptobyte(green),oglclamptobyte(blue),oglclamptobyte(alpha));
	}
	OGLPUSHDISPLAYLISTENTRY_4ARG(idglColor4ub, glubyte,red, glubyte,green, glubyte,blue, glubyte,alpha);
}

HOOKFUNC void GLAPI MyglColor4ubv (const GLubyte *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(v[0]),oglclamptobyte(v[1]),oglclamptobyte(v[2]),oglclamptobyte(v[3]));
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglColor4ubv, sizeof(GLubyte),v,4);
}

HOOKFUNC void GLAPI MyglColor4ui (GLuint red, GLuint green, GLuint blue, GLuint alpha)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(red),oglclamptobyte(green),oglclamptobyte(blue),oglclamptobyte(alpha));
	}
	OGLPUSHDISPLAYLISTENTRY_4ARG(idglColor4ui, gluint,red, gluint,green, gluint,blue, gluint,alpha);
}

HOOKFUNC void GLAPI MyglColor4uiv (const GLuint *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(v[0]),oglclamptobyte(v[1]),oglclamptobyte(v[2]),oglclamptobyte(v[3]));
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglColor4uiv, sizeof(GLuint),v,4);
}

HOOKFUNC void GLAPI MyglColor4us (GLushort red, GLushort green, GLushort blue, GLushort alpha)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(red),oglclamptobyte(green),oglclamptobyte(blue),oglclamptobyte(alpha));
	}
	OGLPUSHDISPLAYLISTENTRY_4ARG(idglColor4us, glushort,red, glushort,green, glushort,blue, glushort,alpha);
}

HOOKFUNC void GLAPI MyglColor4usv (const GLushort *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.c = OGLCOLOR_RGBA(oglclamptobyte(v[0]),oglclamptobyte(v[1]),oglclamptobyte(v[2]),oglclamptobyte(v[3]));
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglColor4usv, sizeof(GLushort),v,4);
}

HOOKFUNC void GLAPI MyglColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
	OGLPUSHDISPLAYLISTENTRY_4ARG(idglColorMask, glboolean,red, glboolean,green, glboolean,blue, glboolean,alpha);
}

HOOKFUNC void GLAPI MyglColorMaterial (GLenum face, GLenum mode)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
	OGLPUSHDISPLAYLISTENTRY_2ARG(idglColorMaterial, glenum,face, glenum,mode);
}

HOOKFUNC void GLAPI MyglCopyPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
	OGLPUSHDISPLAYLISTENTRY_5ARG(idglCopyPixels, glint,x, glint,y, glsizei,width, glsizei,height, glenum,type);
}

HOOKFUNC void GLAPI MyglCullFace (GLenum mode)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		// see also MyglFrontFace
		oglServerState.other.cullFaceMode = mode;
		oglSetD3dCullMode();
	}
	OGLPUSHDISPLAYLISTENTRY_1ARG(idglCullFace, glenum,mode);
}

HOOKFUNC void GLAPI MyglDeleteLists (GLuint list, GLsizei range)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(range < 0)
		OGLRETURNERROR(GL_INVALID_VALUE);
	if(oglBeganMode != GL_UNSTARTED)
		OGLRETURNERROR(GL_INVALID_OPERATION);
	int minim = list;
	if(minim == 0)
		minim = 1;
	int maxim = list + range;
	if(maxim > (int)oglDisplayLists.size())
		maxim = oglDisplayLists.size();
	for(int i = minim; i < maxim; i++)
		oglDisplayLists[i].Clear();
	if(maxim == oglDisplayLists.size())
		oglDisplayLists.resize(minim);
	while(oglDisplayLists.size() > 1 && !oglDisplayLists.back().valid)
		oglDisplayLists.pop_back();
}

HOOKFUNC void GLAPI MyglDepthFunc (GLenum func)
{
	debuglog(LCF_OGL, __FUNCTION__ "(0x%X) called.\n", func);
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(oglBeganMode != GL_UNSTARTED)
			OGLRETURNERROR(GL_INVALID_OPERATION);
		switch(func)
		{
		case GL_NEVER: ogld3d8Device->SetRenderState(D3DRS_ZFUNC, D3DCMP_NEVER); break;
		case GL_LESS: ogld3d8Device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESS); break;
		case GL_EQUAL: ogld3d8Device->SetRenderState(D3DRS_ZFUNC, D3DCMP_EQUAL); break;
		case GL_LEQUAL: ogld3d8Device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL); break;
		case GL_GREATER: ogld3d8Device->SetRenderState(D3DRS_ZFUNC, D3DCMP_GREATER); break;
		case GL_NOTEQUAL: ogld3d8Device->SetRenderState(D3DRS_ZFUNC, D3DCMP_NOTEQUAL); break;
		case GL_GEQUAL: ogld3d8Device->SetRenderState(D3DRS_ZFUNC, D3DCMP_GREATEREQUAL); break;
		case GL_ALWAYS: ogld3d8Device->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS); break;
		}
	}
	OGLPUSHDISPLAYLISTENTRY_1ARG(idglDepthFunc, glenum,func);
}

HOOKFUNC void GLAPI MyglDepthMask (GLboolean flag)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.other.zWriteOn = flag;
		oglSetD3dDepthTest();
	}
	OGLPUSHDISPLAYLISTENTRY_1ARG(idglDepthMask, glboolean,flag);
}

HOOKFUNC void GLAPI MyglDepthRange (GLclampd zNear, GLclampd zFar)
{
	debuglog(LCF_OGL, __FUNCTION__ "(near=%g, far=%g) called.\n", (float)zNear, (float)zFar);
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(oglBeganMode != GL_UNSTARTED)
			OGLRETURNERROR(GL_INVALID_OPERATION);
		if(zNear < 0) zNear = 0;
		if(zNear > 1) zNear = 1;
		if(zFar < 0) zFar = 0;
		if(zFar > 1) zFar = 1;
		D3DVIEWPORT8 vp;
		ogld3d8Device->GetViewport(&vp);
		vp.MinZ = (float)zNear;
		vp.MaxZ = (float)zFar;
		ogld3d8Device->SetViewport(&vp);
	}
	OGLPUSHDISPLAYLISTENTRY_2ARG(idglDepthRange, glclampd,zNear, glclampd,zFar);
}

HOOKFUNC void GLAPI MyglDrawBuffer (GLenum mode)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
	OGLPUSHDISPLAYLISTENTRY_1ARG(idglDrawBuffer, glenum,mode);
}

HOOKFUNC void GLAPI MyglDrawPixels (GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
	OGLPUSHDISPLAYLISTENTRY_5ARG(idglDrawPixels, glsizei,width, glsizei,height, glenum,format, glenum,type, ptr,(void*)pixels);
}

HOOKFUNC void GLAPI MyglEdgeFlag (GLboolean flag)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
	OGLPUSHDISPLAYLISTENTRY_1ARG(idglEdgeFlag, glboolean,flag);
}

HOOKFUNC void GLAPI MyglEdgeFlagv (const GLboolean *flag)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglEdgeFlagv, sizeof(GLboolean),flag,1);
}


static const int GL_ALPHA_TEST = 0x0BC0;
static const int GL_BLEND = 0x0BE2;
static const int GL_CULL_FACE = 0x0B44;
static const int GL_DEPTH_TEST = 0x0B71;
static const int GL_COLOR_LOGIC_OP = 0x0BF2;
static const int GL_SCISSOR_TEST = 0x0C11;
static const int GL_STENCIL_TEST = 0x0B90;

HOOKFUNC void GLAPI MyglEnable (GLenum cap)
{
	debuglog(LCF_OGL, __FUNCTION__ "(cap=0x%X) called.\n", cap);
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(oglBeganMode != GL_UNSTARTED)
			OGLRETURNERROR(GL_INVALID_OPERATION);
		switch(cap)
		{
		case GL_ALPHA_TEST:
			oglServerState.enable.alphaTest = true;
			ogld3d8Device->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
			break;
		case GL_BLEND:
			oglServerState.enable.blend = true;
			ogld3d8Device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			break;
		case GL_CULL_FACE:
			oglServerState.enable.cullFace = true;
			oglSetD3dCullMode();
			break;
		case GL_DEPTH_TEST:
			oglServerState.enable.depthTest = true;
			oglSetD3dDepthTest();
			break;
		//case GL_COLOR_LOGIC_OP: oglServerState.colorBuffer. = true; break;
		case GL_SCISSOR_TEST:
			oglServerState.enable.scissorTest = true;
			break;
		//case GL_STENCIL_TEST: oglServerState.enable.stencilTest = true; break;
		case GL_TEXTURE_1D:
			oglServerState.enable.texture1d = true;
			break;
		case GL_TEXTURE_2D:
			oglServerState.enable.texture2d = true;
			{ // FIXME temp hack
				OpenGLTexture& tex = oglTextures[oglTexture2DTarget];
				ogld3d8Device->SetTexture(0, tex.d3dTexture);
			}
			break;
		}
	}
	OGLPUSHDISPLAYLISTENTRY_1ARG(idglEnable, glenum,cap);
}

HOOKFUNC void GLAPI MyglDisable (GLenum cap)
{
	debuglog(LCF_OGL, __FUNCTION__ "(cap=0x%X) called.\n", cap);
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(oglBeganMode != GL_UNSTARTED)
			OGLRETURNERROR(GL_INVALID_OPERATION);
		switch(cap)
		{
		case GL_ALPHA_TEST:
			oglServerState.enable.alphaTest = false;
			ogld3d8Device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
			break;
		case GL_BLEND:
			oglServerState.enable.blend = false;
			ogld3d8Device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			break;
		case GL_CULL_FACE:
			oglServerState.enable.cullFace = false;
			oglSetD3dCullMode();
			break;
		case GL_DEPTH_TEST:
			oglServerState.enable.depthTest = false;
			oglSetD3dDepthTest();
			break;
		//case GL_COLOR_LOGIC_OP: oglServerState.colorBuffer. = false; break;
		case GL_SCISSOR_TEST:
			oglServerState.enable.scissorTest = false;
			break;
		//case GL_STENCIL_TEST: oglServerState.enable.stencilTest = false; break;
		case GL_TEXTURE_1D:
			oglServerState.enable.texture1d = false;
			break;
		case GL_TEXTURE_2D:
			oglServerState.enable.texture2d = false;
			ogld3d8Device->SetTexture(0, NULL); // FIXME temp hack
			break;
		}
	}
	OGLPUSHDISPLAYLISTENTRY_1ARG(idglDisable, glenum,cap);
}

HOOKFUNC void GLAPI MyglEvalCoord1d (GLdouble u)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
	OGLPUSHDISPLAYLISTENTRY_1ARG(idglEvalCoord1d, gldouble,u);
}

HOOKFUNC void GLAPI MyglEvalCoord1dv (const GLdouble *u)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglEvalCoord1dv, sizeof(GLdouble),u,1);
}

HOOKFUNC void GLAPI MyglEvalCoord1f (GLfloat u)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
	OGLPUSHDISPLAYLISTENTRY_1ARG(idglEvalCoord1f, glfloat,u);
}

HOOKFUNC void GLAPI MyglEvalCoord1fv (const GLfloat *u)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglEvalCoord1fv, sizeof(GLfloat),u,1);
}

HOOKFUNC void GLAPI MyglEvalCoord2d (GLdouble u, GLdouble v)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
	OGLPUSHDISPLAYLISTENTRY_2ARG(idglEvalCoord2d, gldouble,u, gldouble,v);
}

HOOKFUNC void GLAPI MyglEvalCoord2dv (const GLdouble *u)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglEvalCoord2dv, sizeof(GLdouble),u,2);
}

HOOKFUNC void GLAPI MyglEvalCoord2f (GLfloat u, GLfloat v)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
	OGLPUSHDISPLAYLISTENTRY_2ARG(idglEvalCoord2f, glfloat,u, glfloat,v);
}

HOOKFUNC void GLAPI MyglEvalCoord2fv (const GLfloat *u)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglEvalCoord2fv, sizeof(GLfloat),u,2);
}

HOOKFUNC void GLAPI MyglEvalMesh1 (GLenum mode, GLint i1, GLint i2)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglEvalMesh2 (GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglEvalPoint1 (GLint i)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglEvalPoint2 (GLint i, GLint j)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglFeedbackBuffer (GLsizei size, GLenum type, GLfloat *buffer)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglFinish (void)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglFlush (void)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglFogf (GLenum pname, GLfloat param)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglFogfv (GLenum pname, const GLfloat *params)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglFogi (GLenum pname, GLint param)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglFogiv (GLenum pname, const GLint *params)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglFrontFace (GLenum mode)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		// see also MyglCullFace
		oglServerState.other.frontFaceMode = mode;
		oglSetD3dCullMode();
	}
	OGLPUSHDISPLAYLISTENTRY_1ARG(idglFrontFace, glenum,mode);
}

HOOKFUNC void GLAPI MyglFrustum (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(oglBeganMode != GL_UNSTARTED)
			OGLRETURNERROR(GL_INVALID_OPERATION);
		if(left == right || bottom == top || zNear <= 0 || zFar <= 0)
			OGLRETURNERROR(GL_INVALID_VALUE);
		D3DMATRIX mt = {
			(float)(2*zNear/(right-left)),0,(float)((right+left)/(right-left)),0,
			0,(float)(2*zNear/(top-bottom)),(float)((top+bottom)/(top-bottom)),0,
			0,0,(float)(-(zFar+zNear)/(zFar-zNear)),(float)(-2*zFar*zNear/(zFar-zNear)),
			0,0,-1,0,
		};
		oglMulD3DMats(*oglMatrix,*oglMatrix,mt);
		*oglDirty = true;
	}
	OGLPUSHDISPLAYLISTENTRY_6ARG(idglFrustum, gldouble,left, gldouble,right, gldouble,bottom, gldouble,top, gldouble,zNear, gldouble,zFar);
}

HOOKFUNC void GLAPI MyglGetBooleanv (GLenum pname, GLboolean *params)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglGetClipPlane (GLenum plane, GLdouble *equation)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglGetDoublev (GLenum pname, GLdouble *params)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglGetFloatv (GLenum pname, GLfloat *params)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglGetIntegerv (GLenum pname, GLint *params)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglGetLightfv (GLenum light, GLenum pname, GLfloat *params)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglGetLightiv (GLenum light, GLenum pname, GLint *params)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglGetMapdv (GLenum target, GLenum query, GLdouble *v)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglGetMapfv (GLenum target, GLenum query, GLfloat *v)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglGetMapiv (GLenum target, GLenum query, GLint *v)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglGetMaterialfv (GLenum face, GLenum pname, GLfloat *params)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglGetMaterialiv (GLenum face, GLenum pname, GLint *params)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglGetPixelMapfv (GLenum map, GLfloat *values)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglGetPixelMapuiv (GLenum map, GLuint *values)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglGetPixelMapusv (GLenum map, GLushort *values)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglGetPolygonStipple (GLubyte *mask)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglGetTexEnvfv (GLenum target, GLenum pname, GLfloat *params)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglGetTexEnviv (GLenum target, GLenum pname, GLint *params)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglGetTexGendv (GLenum coord, GLenum pname, GLdouble *params)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglGetTexGenfv (GLenum coord, GLenum pname, GLfloat *params)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglGetTexGeniv (GLenum coord, GLenum pname, GLint *params)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglGetTexImage (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglGetTexLevelParameterfv (GLenum target, GLint level, GLenum pname, GLfloat *params)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglGetTexLevelParameteriv (GLenum target, GLint level, GLenum pname, GLint *params)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglGetTexParameterfv (GLenum target, GLenum pname, GLfloat *params)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglGetTexParameteriv (GLenum target, GLenum pname, GLint *params)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglIndexMask (GLuint mask)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglIndexd (GLdouble c)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglIndexdv (const GLdouble *c)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglIndexf (GLfloat c)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglIndexfv (const GLfloat *c)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglIndexi (GLint c)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglIndexiv (const GLint *c)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglIndexs (GLshort c)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglIndexsv (const GLshort *c)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglInitNames (void)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglLightModelf (GLenum pname, GLfloat param)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglLightModelfv (GLenum pname, const GLfloat *params)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglLightModeli (GLenum pname, GLint param)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglLightModeliv (GLenum pname, const GLint *params)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglLightf (GLenum light, GLenum pname, GLfloat param)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglLightfv (GLenum light, GLenum pname, const GLfloat *params)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglLighti (GLenum light, GLenum pname, GLint param)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglLightiv (GLenum light, GLenum pname, const GLint *params)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglLineStipple (GLint factor, GLushort pattern)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglLineWidth (GLfloat width)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ "(width=%g) called.\n", width);
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(oglBeganMode != GL_UNSTARTED)
			OGLRETURNERROR(GL_INVALID_OPERATION); // not allowed between begin and end
		if(width <= 0.0f)
			OGLRETURNERROR(GL_INVALID_VALUE);
		oglLineWidth = width;
	}
	OGLPUSHDISPLAYLISTENTRY_1ARG(idglLineWidth, glfloat,width);
}

HOOKFUNC void GLAPI MyglListBase (GLuint base)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglLoadIdentity (void)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(oglBeganMode != GL_UNSTARTED)
			OGLRETURNERROR(GL_INVALID_OPERATION);
		D3DMATRIX mt = {
			1,0,0,0,
			0,1,0,0,
			0,0,1,0,
			0,0,0,1,
		};
		*oglMatrix = mt;
		*oglDirty = true;
	}
	OGLPUSHDISPLAYLISTENTRY_0ARG(idglLoadIdentity);
}

HOOKFUNC void GLAPI MyglLoadMatrixd (const GLdouble *m)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglLoadMatrixf (const GLfloat *m)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglLoadName (GLuint name)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglLogicOp (GLenum opcode)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglMap1d (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglMap1f (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglMap2d (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglMap2f (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglMapGrid1d (GLint un, GLdouble u1, GLdouble u2)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglMapGrid1f (GLint un, GLfloat u1, GLfloat u2)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglMapGrid2d (GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglMapGrid2f (GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglMaterialf (GLenum face, GLenum pname, GLfloat param)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglMaterialfv (GLenum face, GLenum pname, const GLfloat *params)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglMateriali (GLenum face, GLenum pname, GLint param)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglMaterialiv (GLenum face, GLenum pname, const GLint *params)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglMatrixMode (GLenum mode)
{
	debuglog(LCF_OGL, __FUNCTION__ "(mode=0x%X) called.\n", mode);
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(oglBeganMode != GL_UNSTARTED)
			OGLRETURNERROR(GL_INVALID_OPERATION);
		switch(mode)
		{
		case GL_MODELVIEW: oglMatrixStack = &oglMatrixStackMV; oglMatrix = &oglMatrixMV; oglDirty = &oglDirtyMV; break;
		case GL_PROJECTION: oglMatrixStack = &oglMatrixStackP; oglMatrix = &oglMatrixP; oglDirty = &oglDirtyP; break;
		case GL_TEXTURE: oglMatrixStack = &oglMatrixStackT; oglMatrix = &oglMatrixT; oglDirty = &oglDirtyT; break;
		default: OGLRETURNERROR(GL_INVALID_ENUM); break;
		}
		oglMatrixID = mode;
	}
	OGLPUSHDISPLAYLISTENTRY_1ARG(idglMatrixMode, glenum,mode);
}

HOOKFUNC void GLAPI MyglMultMatrixd (const GLdouble *m)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(oglBeganMode != GL_UNSTARTED)
			OGLRETURNERROR(GL_INVALID_OPERATION);
		D3DMATRIX mt = {
			(float)m[0],(float)m[1],(float)m[2],(float)m[3],
			(float)m[4],(float)m[5],(float)m[6],(float)m[7],
			(float)m[8],(float)m[9],(float)m[10],(float)m[11],
			(float)m[12],(float)m[13],(float)m[14],(float)m[15],
		};
		oglMulD3DMats(*oglMatrix,*oglMatrix,mt);
		*oglDirty = true;
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglMultMatrixd, sizeof(GLdouble),m,16);
}

HOOKFUNC void GLAPI MyglMultMatrixf (const GLfloat *m)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(oglBeganMode != GL_UNSTARTED)
			OGLRETURNERROR(GL_INVALID_OPERATION);
		oglMulD3DMats(*oglMatrix,*oglMatrix,*(D3DMATRIX*)m);
		*oglDirty = true;
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglMultMatrixf, sizeof(GLfloat),m,16);
}

HOOKFUNC void GLAPI MyglNewList (GLuint list, GLenum mode)
{
	debuglog(LCF_OGL, __FUNCTION__ "(list=%d, mode=0x%X) called.\n", list, mode);
	//GLFUNCBOILERPLATE;

	static const int GL_COMPILE = 0x1300;
	static const int GL_COMPILE_AND_EXECUTE = 0x1301;

	if(list == 0 || list >= oglDisplayLists.size())
		OGLRETURNERROR(GL_INVALID_VALUE);
	if(mode != GL_COMPILE && mode != GL_COMPILE_AND_EXECUTE)
		OGLRETURNERROR(GL_INVALID_ENUM);
	if(oglMakingDisplayList || oglBeganMode != GL_UNSTARTED)
		OGLRETURNERROR(GL_INVALID_OPERATION);

	if(oglDisplayLists.empty())
		oglDisplayLists.resize(1);
	oglDisplayLists[0].entries.clear(); // intentionally don't call Clear on each entry, since it should have been copied already
	oglMakingDisplayList = list;
	oglAllowExecuteCommands = (mode == GL_COMPILE_AND_EXECUTE) && (ogld3d8Device != NULL);
}

HOOKFUNC void GLAPI MyglEndList (void)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;

	if(oglMakingDisplayList == 0 || oglBeganMode != GL_UNSTARTED)
		OGLRETURNERROR(GL_INVALID_OPERATION);

	oglDisplayLists[oglMakingDisplayList] = oglDisplayLists[0];
	oglDisplayLists[oglMakingDisplayList].valid = true;
	oglMakingDisplayList = 0;
	oglAllowExecuteCommands = (ogld3d8Device != NULL);
}

HOOKFUNC void GLAPI MyglNormal3b (GLbyte nx, GLbyte ny, GLbyte nz)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.nx = (FLOAT)nx;
		oglServerState.current.ny = (FLOAT)ny;
		oglServerState.current.nz = (FLOAT)nx;
	}
}

HOOKFUNC void GLAPI MyglNormal3bv (const GLbyte *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.nx = (FLOAT)v[0];
		oglServerState.current.ny = (FLOAT)v[1];
		oglServerState.current.nz = (FLOAT)v[2];
	}
}

HOOKFUNC void GLAPI MyglNormal3d (GLdouble nx, GLdouble ny, GLdouble nz)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.nx = (FLOAT)nx;
		oglServerState.current.ny = (FLOAT)ny;
		oglServerState.current.nz = (FLOAT)nx;
	}
}

HOOKFUNC void GLAPI MyglNormal3dv (const GLdouble *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.nx = (FLOAT)v[0];
		oglServerState.current.ny = (FLOAT)v[1];
		oglServerState.current.nz = (FLOAT)v[2];
	}
}

HOOKFUNC void GLAPI MyglNormal3f (GLfloat nx, GLfloat ny, GLfloat nz)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.nx = (FLOAT)nx;
		oglServerState.current.ny = (FLOAT)ny;
		oglServerState.current.nz = (FLOAT)nx;
	}
}

HOOKFUNC void GLAPI MyglNormal3fv (const GLfloat *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.nx = (FLOAT)v[0];
		oglServerState.current.ny = (FLOAT)v[1];
		oglServerState.current.nz = (FLOAT)v[2];
	}
}

HOOKFUNC void GLAPI MyglNormal3i (GLint nx, GLint ny, GLint nz)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.nx = (FLOAT)nx;
		oglServerState.current.ny = (FLOAT)ny;
		oglServerState.current.nz = (FLOAT)nx;
	}
}

HOOKFUNC void GLAPI MyglNormal3iv (const GLint *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.nx = (FLOAT)v[0];
		oglServerState.current.ny = (FLOAT)v[1];
		oglServerState.current.nz = (FLOAT)v[2];
	}
}

HOOKFUNC void GLAPI MyglNormal3s (GLshort nx, GLshort ny, GLshort nz)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.nx = (FLOAT)nx;
		oglServerState.current.ny = (FLOAT)ny;
		oglServerState.current.nz = (FLOAT)nx;
	}
}

HOOKFUNC void GLAPI MyglNormal3sv (const GLshort *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.nx = (FLOAT)v[0];
		oglServerState.current.ny = (FLOAT)v[1];
		oglServerState.current.nz = (FLOAT)v[2];
	}
}

HOOKFUNC void GLAPI MyglOrtho (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(oglBeganMode != GL_UNSTARTED)
			OGLRETURNERROR(GL_INVALID_OPERATION);
		if(left == right || bottom == top || zNear == zFar)
			OGLRETURNERROR(GL_INVALID_VALUE);
		D3DMATRIX mt = {
			(float)(2/(right-left)),0,0,(float)(-(right+left)/(right-left)),
			0,(float)(2/(top-bottom)),0,(float)(-(top+bottom)/(top-bottom)),
			0,0,(float)(2/(zFar-zNear)),(float)(-(zFar+zNear)/(zFar-zNear)),
			0,0,0,1,
		};
		oglMulD3DMats(*oglMatrix,*oglMatrix,mt);
		*oglDirty = true;
	}
	OGLPUSHDISPLAYLISTENTRY_6ARG(idglOrtho, gldouble,left, gldouble,right, gldouble,bottom, gldouble,top, gldouble,zNear, gldouble,zFar);
}

HOOKFUNC void GLAPI MyglPassThrough (GLfloat token)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglPixelMapfv (GLenum map, GLsizei mapsize, const GLfloat *values)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglPixelMapuiv (GLenum map, GLsizei mapsize, const GLuint *values)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglPixelMapusv (GLenum map, GLsizei mapsize, const GLushort *values)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglPixelStoref (GLenum pname, GLfloat param)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglPixelStorei (GLenum pname, GLint param)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglPixelTransferf (GLenum pname, GLfloat param)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglPixelTransferi (GLenum pname, GLint param)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglPixelZoom (GLfloat xfactor, GLfloat yfactor)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglPointSize (GLfloat size)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglPolygonMode (GLenum face, GLenum mode)
{
	debuglog(LCF_OGL, __FUNCTION__ "(face=0x%X, mode=0x%X) called.\n", face,mode);
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(oglBeganMode != GL_UNSTARTED)
			OGLRETURNERROR(GL_INVALID_OPERATION);
		static const int GL_POINT = 0x1B00;
		static const int GL_LINE = 0x1B01;
		static const int GL_FILL = 0x1B02;
		if(face == GL_FRONT || face == GL_FRONT_AND_BACK)
		{
			switch(mode)
			{
			case GL_POINT:
				ogld3d8Device->SetRenderState(D3DRS_FILLMODE,D3DFILL_POINT);
				break;
			case GL_LINE:
				ogld3d8Device->SetRenderState(D3DRS_FILLMODE,D3DFILL_WIREFRAME);
				break;
			case GL_FILL:
				ogld3d8Device->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);
				break;
			default:
				OGLRETURNERROR(GL_INVALID_ENUM);
				break;
			}
		}
	}
	OGLPUSHDISPLAYLISTENTRY_2ARG(idglTexParameterfv, glenum,face, glenum,mode);
}

HOOKFUNC void GLAPI MyglPolygonStipple (const GLubyte *mask)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglPopAttrib (void)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglPopMatrix (void)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(oglBeganMode != GL_UNSTARTED)
			OGLRETURNERROR(GL_INVALID_OPERATION);
		if(oglMatrixStack->empty())
			OGLRETURNERROR(GL_STACK_UNDERFLOW);
		*oglMatrix = oglMatrixStack->back();
		*oglDirty = true;
		oglMatrixStack->pop_back();
	}
	OGLPUSHDISPLAYLISTENTRY_0ARG(idglPopMatrix);
}

HOOKFUNC void GLAPI MyglPopName (void)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglPushAttrib (GLbitfield mask)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglPushMatrix (void)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(oglBeganMode != GL_UNSTARTED)
			OGLRETURNERROR(GL_INVALID_OPERATION);
		oglMatrixStack->push_back(*oglMatrix);
	}
	OGLPUSHDISPLAYLISTENTRY_0ARG(idglPushMatrix);
}

HOOKFUNC void GLAPI MyglPushName (GLuint name)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglRasterPos2d (GLdouble x, GLdouble y)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglRasterPos2dv (const GLdouble *v)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglRasterPos2f (GLfloat x, GLfloat y)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglRasterPos2fv (const GLfloat *v)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglRasterPos2i (GLint x, GLint y)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglRasterPos2iv (const GLint *v)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglRasterPos2s (GLshort x, GLshort y)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglRasterPos2sv (const GLshort *v)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglRasterPos3d (GLdouble x, GLdouble y, GLdouble z)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglRasterPos3dv (const GLdouble *v)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglRasterPos3f (GLfloat x, GLfloat y, GLfloat z)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglRasterPos3fv (const GLfloat *v)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglRasterPos3i (GLint x, GLint y, GLint z)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglRasterPos3iv (const GLint *v)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglRasterPos3s (GLshort x, GLshort y, GLshort z)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglRasterPos3sv (const GLshort *v)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglRasterPos4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglRasterPos4dv (const GLdouble *v)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglRasterPos4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglRasterPos4fv (const GLfloat *v)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglRasterPos4i (GLint x, GLint y, GLint z, GLint w)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglRasterPos4iv (const GLint *v)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglRasterPos4s (GLshort x, GLshort y, GLshort z, GLshort w)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglRasterPos4sv (const GLshort *v)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglReadBuffer (GLenum mode)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglRotated (GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(oglBeganMode != GL_UNSTARTED)
			OGLRETURNERROR(GL_INVALID_OPERATION);
		double lensq = x*x + y*y + z*z;
		if(lensq > 0.00001 && lensq < 0.99999 || lensq > 1.00001)
		{
			double len = sqrt(lensq);
			x /= len;
			y /= len;
			z /= len;
		}
		double radians = angle * (3.1415926535897932384626433832795 / 180.0);
		double c = cos(radians);
		double s = sin(radians);
		double c2 = 1 - c;
		D3DMATRIX mt = {
			(float)(x*x*c2+c),(float)(x*y*c2-z*s),(float)(x*z*c2+y*s),0,
			(float)(y*x*c2+z*s),(float)(y*y*c2+c),(float)(y*z*c2-x*s),0,
			(float)(x*z*c2-y*s),(float)(y*z*c2+x*s),(float)(z*z*c2+c),0,
			0,0,0,1,
		};
		oglMulD3DMats(*oglMatrix,*oglMatrix,mt);
		*oglDirty = true;
	}
	OGLPUSHDISPLAYLISTENTRY_4ARG(idglRotated, gldouble,angle, gldouble,x, gldouble,y, gldouble,z);
}

HOOKFUNC void GLAPI MyglRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	debuglog(LCF_OGL, __FUNCTION__ "(angle=%g, x=%g, y=%g, z=%g) called.\n", angle,x,y,z);
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(oglBeganMode != GL_UNSTARTED)
			OGLRETURNERROR(GL_INVALID_OPERATION);
		float lensq = x*x + y*y + z*z;
		if(lensq > 0.00001f && lensq < 0.99999f || lensq > 1.00001f)
		{
			float len = sqrtf(lensq);
			x /= len;
			y /= len;
			z /= len;
		}
		float radians = angle * (3.1415926535897932384626433832795f / 180.0f);
		float c = cosf(radians);
		float s = sinf(radians);
		float c2 = 1 - c;
		D3DMATRIX mt = {
			x*x*c2+c, x*y*c2-z*s, x*z*c2+y*s, 0,
			y*x*c2+z*s, y*y*c2+c, y*z*c2-x*s, 0,
			x*z*c2-y*s, y*z*c2+x*s, z*z*c2+c, 0,
			0, 0, 0, 1,
		};
		oglMulD3DMats(*oglMatrix,*oglMatrix,mt);
		*oglDirty = true;
	}
	OGLPUSHDISPLAYLISTENTRY_4ARG(idglRotatef, glfloat,angle, glfloat,x, glfloat,y, glfloat,z);
}

HOOKFUNC void GLAPI MyglScaled (GLdouble x, GLdouble y, GLdouble z)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(oglBeganMode != GL_UNSTARTED)
			OGLRETURNERROR(GL_INVALID_OPERATION);
		D3DMATRIX mt = {
			(float)x,0,0,0,
			0,(float)y,0,0,
			0,0,(float)z,0,
			0,0,0,1,
		};
		oglMulD3DMats(*oglMatrix,*oglMatrix,mt);
		*oglDirty = true;
	}
	OGLPUSHDISPLAYLISTENTRY_3ARG(idglScaled, gldouble,x, gldouble,y, gldouble,z);
}

HOOKFUNC void GLAPI MyglScalef (GLfloat x, GLfloat y, GLfloat z)
{
	debuglog(LCF_OGL, __FUNCTION__ "(%g, %g, %g) called.\n", x,y,z);
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(oglBeganMode != GL_UNSTARTED)
			OGLRETURNERROR(GL_INVALID_OPERATION);
		D3DMATRIX mt = {
			x,0,0,0,
			0,y,0,0,
			0,0,z,0,
			0,0,0,1,
		};
		oglMulD3DMats(*oglMatrix,*oglMatrix,mt);
		*oglDirty = true;
	}
	OGLPUSHDISPLAYLISTENTRY_3ARG(idglScalef, glfloat,x, glfloat,y, glfloat,z);
}

HOOKFUNC void GLAPI MyglScissor (GLint x, GLint y, GLsizei width, GLsizei height)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglSelectBuffer (GLsizei size, GLuint *buffer)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}

HOOKFUNC void GLAPI MyglShadeModel (GLenum mode)
{
	debuglog(LCF_OGL, __FUNCTION__ "(mode=0x%X) called.\n", mode);
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		static const int GL_FLAT = 0x1D00;
		static const int GL_SMOOTH = 0x1D01;
		if(oglBeganMode != GL_UNSTARTED)
			OGLRETURNERROR(GL_INVALID_OPERATION);
		switch(mode)
		{
		case GL_FLAT:
			ogld3d8Device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT);
			break;
		case GL_SMOOTH:
			ogld3d8Device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
			break;
		default:
			OGLRETURNERROR(GL_INVALID_ENUM);
			break;
		}
	}
	OGLPUSHDISPLAYLISTENTRY_1ARG(idglShadeModel, glenum,mode);
}

HOOKFUNC void GLAPI MyglStencilFunc (GLenum func, GLint ref, GLuint mask)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglStencilMask (GLuint mask)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglStencilOp (GLenum fail, GLenum zfail, GLenum zpass)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglTexCoord1d (GLdouble s)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)s;
		oglServerState.current.v = (FLOAT)0;
	}
	OGLPUSHDISPLAYLISTENTRY_1ARG(idglTexCoord1d, gldouble,s);
}

HOOKFUNC void GLAPI MyglTexCoord1dv (const GLdouble *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)v[0];
		oglServerState.current.v = (FLOAT)0;
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglTexCoord1dv, sizeof(GLdouble),v,1);
}

HOOKFUNC void GLAPI MyglTexCoord1f (GLfloat s)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)s;
		oglServerState.current.v = (FLOAT)0;
	}
	OGLPUSHDISPLAYLISTENTRY_1ARG(idglTexCoord1f, glfloat,s);
}

HOOKFUNC void GLAPI MyglTexCoord1fv (const GLfloat *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)v[0];
		oglServerState.current.v = (FLOAT)0;
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglTexCoord1fv, sizeof(GLfloat),v,1);
}

HOOKFUNC void GLAPI MyglTexCoord1i (GLint s)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)s;
		oglServerState.current.v = (FLOAT)0;
	}
	OGLPUSHDISPLAYLISTENTRY_1ARG(idglTexCoord1i, glint,s);
}

HOOKFUNC void GLAPI MyglTexCoord1iv (const GLint *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)v[0];
		oglServerState.current.v = (FLOAT)0;
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglTexCoord1iv, sizeof(GLint),v,1);
}

HOOKFUNC void GLAPI MyglTexCoord1s (GLshort s)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)s;
		oglServerState.current.v = (FLOAT)0;
	}
	OGLPUSHDISPLAYLISTENTRY_1ARG(idglTexCoord1s, glshort,s);
}

HOOKFUNC void GLAPI MyglTexCoord1sv (const GLshort *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)v[0];
		oglServerState.current.v = (FLOAT)0;
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglTexCoord1sv, sizeof(GLshort),v,1);
}

HOOKFUNC void GLAPI MyglTexCoord2d (GLdouble s, GLdouble t)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)s;
		oglServerState.current.v = (FLOAT)t;
	}
	OGLPUSHDISPLAYLISTENTRY_2ARG(idglTexCoord2d, gldouble,s, gldouble,t);
}

HOOKFUNC void GLAPI MyglTexCoord2dv (const GLdouble *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)v[0];
		oglServerState.current.v = (FLOAT)v[1];
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglTexCoord2dv, sizeof(GLdouble),v,2);
}

HOOKFUNC void GLAPI MyglTexCoord2f (GLfloat s, GLfloat t)
{
	debuglog(LCF_OGL, __FUNCTION__ "(%g, %g) called.\n", s, t);
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)s;
		oglServerState.current.v = (FLOAT)t;
	}
	OGLPUSHDISPLAYLISTENTRY_2ARG(idglTexCoord2f, glfloat,s, glfloat,t);
}

HOOKFUNC void GLAPI MyglTexCoord2fv (const GLfloat *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)v[0];
		oglServerState.current.v = (FLOAT)v[1];
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglTexCoord2fv, sizeof(GLfloat),v,2);
}

HOOKFUNC void GLAPI MyglTexCoord2i (GLint s, GLint t)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)s;
		oglServerState.current.v = (FLOAT)t;
	}
	OGLPUSHDISPLAYLISTENTRY_2ARG(idglTexCoord2i, glint,s, glint,t);
}

HOOKFUNC void GLAPI MyglTexCoord2iv (const GLint *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)v[0];
		oglServerState.current.v = (FLOAT)v[1];
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglTexCoord2iv, sizeof(GLint),v,2);
}

HOOKFUNC void GLAPI MyglTexCoord2s (GLshort s, GLshort t)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)s;
		oglServerState.current.v = (FLOAT)t;
	}
	OGLPUSHDISPLAYLISTENTRY_2ARG(idglTexCoord2s, glshort,s, glshort,t);
}

HOOKFUNC void GLAPI MyglTexCoord2sv (const GLshort *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)v[0];
		oglServerState.current.v = (FLOAT)v[1];
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglTexCoord2sv, sizeof(GLshort),v,2);
}

HOOKFUNC void GLAPI MyglTexCoord3d (GLdouble s, GLdouble t, GLdouble r)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)s;
		oglServerState.current.v = (FLOAT)t;
		//oglServerState.current.r = (FLOAT)r;
	}
	OGLPUSHDISPLAYLISTENTRY_3ARG(idglTexCoord3d, gldouble,s, gldouble,t, gldouble,r);
}

HOOKFUNC void GLAPI MyglTexCoord3dv (const GLdouble *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)v[0];
		oglServerState.current.v = (FLOAT)v[1];
		//oglServerState.current.r = (FLOAT)v[2];
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglTexCoord3dv, sizeof(GLdouble),v,3);
}

HOOKFUNC void GLAPI MyglTexCoord3f (GLfloat s, GLfloat t, GLfloat r)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)s;
		oglServerState.current.v = (FLOAT)t;
		//oglServerState.current.r = (FLOAT)r;
	}
	OGLPUSHDISPLAYLISTENTRY_3ARG(idglTexCoord3f, glfloat,s, glfloat,t, glfloat,r);
}

HOOKFUNC void GLAPI MyglTexCoord3fv (const GLfloat *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)v[0];
		oglServerState.current.v = (FLOAT)v[1];
		//oglServerState.current.q = (FLOAT)v[2];
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglTexCoord3fv, sizeof(GLfloat),v,3);
}

HOOKFUNC void GLAPI MyglTexCoord3i (GLint s, GLint t, GLint r)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)s;
		oglServerState.current.v = (FLOAT)t;
		//oglServerState.current.r = (FLOAT)r;
	}
	OGLPUSHDISPLAYLISTENTRY_3ARG(idglTexCoord3i, glint,s, glint,t, glint,r);
}

HOOKFUNC void GLAPI MyglTexCoord3iv (const GLint *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)v[0];
		oglServerState.current.v = (FLOAT)v[1];
		//oglServerState.current.r = (FLOAT)v[2];
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglTexCoord3iv, sizeof(GLint),v,3);
}

HOOKFUNC void GLAPI MyglTexCoord3s (GLshort s, GLshort t, GLshort r)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)s;
		oglServerState.current.v = (FLOAT)t;
		//oglServerState.current.r = (FLOAT)r;
	}
	OGLPUSHDISPLAYLISTENTRY_3ARG(idglTexCoord3s, glshort,s, glshort,t, glshort,r);
}

HOOKFUNC void GLAPI MyglTexCoord3sv (const GLshort *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)v[0];
		oglServerState.current.v = (FLOAT)v[1];
		//oglServerState.current.r = (FLOAT)v[2];
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglTexCoord3sv, sizeof(GLshort),v,3);
}

HOOKFUNC void GLAPI MyglTexCoord4d (GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)s;
		oglServerState.current.v = (FLOAT)t;
		//oglServerState.current.r = (FLOAT)r;
		//oglServerState.current.q = (FLOAT)q;
	}
	OGLPUSHDISPLAYLISTENTRY_4ARG(idglTexCoord4d, gldouble,s, gldouble,t, gldouble,r, gldouble,q);
}

HOOKFUNC void GLAPI MyglTexCoord4dv (const GLdouble *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)v[0];
		oglServerState.current.v = (FLOAT)v[1];
		//oglServerState.current.r = (FLOAT)v[2];
		//oglServerState.current.q = (FLOAT)v[3];
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglTexCoord4dv, sizeof(GLdouble),v,4);
}

HOOKFUNC void GLAPI MyglTexCoord4f (GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)s;
		oglServerState.current.v = (FLOAT)t;
		//oglServerState.current.r = (FLOAT)r;
		//oglServerState.current.q = (FLOAT)q;
	}
	OGLPUSHDISPLAYLISTENTRY_4ARG(idglTexCoord4f, glfloat,s, glfloat,t, glfloat,r, glfloat,q);
}

HOOKFUNC void GLAPI MyglTexCoord4fv (const GLfloat *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)v[0];
		oglServerState.current.v = (FLOAT)v[1];
		//oglServerState.current.r = (FLOAT)v[2];
		//oglServerState.current.q = (FLOAT)v[3];
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglTexCoord4fv, sizeof(GLfloat),v,4);
}

HOOKFUNC void GLAPI MyglTexCoord4i (GLint s, GLint t, GLint r, GLint q)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)s;
		oglServerState.current.v = (FLOAT)t;
		//oglServerState.current.r = (FLOAT)r;
		//oglServerState.current.q = (FLOAT)q;
	}
	OGLPUSHDISPLAYLISTENTRY_4ARG(idglTexCoord4i, glint,s, glint,t, glint,r, glint,q);
}

HOOKFUNC void GLAPI MyglTexCoord4iv (const GLint *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)v[0];
		oglServerState.current.v = (FLOAT)v[1];
		//oglServerState.current.r = (FLOAT)v[2];
		//oglServerState.current.q = (FLOAT)v[3];
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglTexCoord4iv, sizeof(GLint),v,4);
}

HOOKFUNC void GLAPI MyglTexCoord4s (GLshort s, GLshort t, GLshort r, GLshort q)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)s;
		oglServerState.current.v = (FLOAT)t;
		//oglServerState.current.r = (FLOAT)r;
		//oglServerState.current.q = (FLOAT)q;
	}
	OGLPUSHDISPLAYLISTENTRY_4ARG(idglTexCoord4i, glshort,s, glshort,t, glshort,r, glshort,q);
}

HOOKFUNC void GLAPI MyglTexCoord4sv (const GLshort *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.u = (FLOAT)v[0];
		oglServerState.current.v = (FLOAT)v[1];
		//oglServerState.current.r = (FLOAT)v[2];
		//oglServerState.current.q = (FLOAT)v[3];
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglTexCoord4sv, sizeof(GLshort),v,4);
}

static const int GL_TEXTURE_ENV = 0x2300;
static const int GL_TEXTURE_ENV_MODE = 0x2200;
static const int GL_TEXTURE_ENV_COLOR = 0x2201;
static const int GL_MODULATE = 0x2100;
static const int GL_DECAL = 0x2101;
//static const int GL_BLEND = 0x0BE2;
static const int GL_REPLACE = 0x1E01;
//static const int GL_ADD = 0x0104;

static void oglSetTextureFunction(GLenum mode)
{
	switch(mode)
	{
	case GL_MODULATE:
		ogld3d8Device->SetTextureStageState(0, D3DTSS_COLOROP,D3DTOP_MODULATE);
		ogld3d8Device->SetTextureStageState(0, D3DTSS_ALPHAOP,D3DTOP_MODULATE);
		break;
	case GL_DECAL:
		ogld3d8Device->SetTextureStageState(0, D3DTSS_COLOROP,D3DTOP_BLENDDIFFUSEALPHA);
		ogld3d8Device->SetTextureStageState(0, D3DTSS_ALPHAOP,D3DTOP_SELECTARG2);
		break;
	case GL_BLEND:
		ogld3d8Device->SetTextureStageState(0, D3DTSS_COLOROP,D3DTOP_BLENDFACTORALPHA);
		ogld3d8Device->SetTextureStageState(0, D3DTSS_ALPHAOP,D3DTOP_MODULATE);
		break;
	case GL_REPLACE:
		ogld3d8Device->SetTextureStageState(0, D3DTSS_COLOROP,D3DTOP_SELECTARG1);
		ogld3d8Device->SetTextureStageState(0, D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);
		break;
	case GL_ADD:
		ogld3d8Device->SetTextureStageState(0, D3DTSS_COLOROP,D3DTOP_ADD);
		ogld3d8Device->SetTextureStageState(0, D3DTSS_ALPHAOP,D3DTOP_MODULATE);
		break;
	}
	oglServerState.other.texFunctionMode = mode;
}

HOOKFUNC void GLAPI MyglTexEnvf (GLenum target, GLenum pname, GLfloat param)
{
	debuglog(LCF_OGL, __FUNCTION__ "(target=0x%X, pname=0x%X, param=%g=0x%X) called.\n", target, pname, param,(GLenum)param);
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		switch(target)
		{
		case GL_TEXTURE_ENV:
			switch(pname)
			{
			case GL_TEXTURE_ENV_MODE:
				oglSetTextureFunction((GLenum)param);
				break;
			case GL_TEXTURE_ENV_COLOR:
				ogld3d8Device->SetRenderState(D3DRS_TEXTUREFACTOR,D3DCOLOR_ARGB(oglclamptobyte(param),oglclamptobyte(param),oglclamptobyte(param),oglclamptobyte(param)));
				break;
			default:
				OGLRETURNERROR(GL_INVALID_ENUM);
				break;
			}
			break;
		default:
			OGLRETURNERROR(GL_INVALID_ENUM);
		}
	}
	OGLPUSHDISPLAYLISTENTRY_3ARG(idglTexEnvf, glenum,target, glenum,pname, glfloat,param);
}

HOOKFUNC void GLAPI MyglTexEnvfv (GLenum target, GLenum pname, const GLfloat *params)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		switch(target)
		{
		case GL_TEXTURE_ENV:
			switch(pname)
			{
			case GL_TEXTURE_ENV_MODE:
				oglSetTextureFunction((GLenum)(params[0]));
				break;
			case GL_TEXTURE_ENV_COLOR:
				ogld3d8Device->SetRenderState(D3DRS_TEXTUREFACTOR,D3DCOLOR_ARGB(oglclamptobyte(params[0]),oglclamptobyte(params[1]),oglclamptobyte(params[2]),oglclamptobyte(params[3])));
				break;
			default:
				OGLRETURNERROR(GL_INVALID_ENUM);
				break;
			}
			break;
		default:
			OGLRETURNERROR(GL_INVALID_ENUM);
		}
	}
}

HOOKFUNC void GLAPI MyglTexEnvi (GLenum target, GLenum pname, GLint param)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		switch(target)
		{
		case GL_TEXTURE_ENV:
			switch(pname)
			{
			case GL_TEXTURE_ENV_MODE:
				oglSetTextureFunction((GLenum)param);
				break;
			case GL_TEXTURE_ENV_COLOR:
				ogld3d8Device->SetRenderState(D3DRS_TEXTUREFACTOR,D3DCOLOR_ARGB(oglclamptobyte(param),oglclamptobyte(param),oglclamptobyte(param),oglclamptobyte(param)));
				break;
			default:
				OGLRETURNERROR(GL_INVALID_ENUM);
				break;
			}
			break;
		default:
			OGLRETURNERROR(GL_INVALID_ENUM);
		}
	}
	OGLPUSHDISPLAYLISTENTRY_3ARG(idglTexEnvi, glenum,target, glenum,pname, glint,param);
}

HOOKFUNC void GLAPI MyglTexEnviv (GLenum target, GLenum pname, const GLint *params)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		switch(target)
		{
		case GL_TEXTURE_ENV:
			switch(pname)
			{
			case GL_TEXTURE_ENV_MODE:
				oglSetTextureFunction((GLenum)(params[0]));
				break;
			case GL_TEXTURE_ENV_COLOR:
				ogld3d8Device->SetRenderState(D3DRS_TEXTUREFACTOR,D3DCOLOR_ARGB(oglclamptobyte(params[0]),oglclamptobyte(params[1]),oglclamptobyte(params[2]),oglclamptobyte(params[3])));
				break;
			default:
				OGLRETURNERROR(GL_INVALID_ENUM);
				break;
			}
			break;
		default:
			OGLRETURNERROR(GL_INVALID_ENUM);
		}
	}
}

HOOKFUNC void GLAPI MyglTexGend (GLenum coord, GLenum pname, GLdouble param)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglTexGendv (GLenum coord, GLenum pname, const GLdouble *params)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglTexGenf (GLenum coord, GLenum pname, GLfloat param)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglTexGenfv (GLenum coord, GLenum pname, const GLfloat *params)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglTexGeni (GLenum coord, GLenum pname, GLint param)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

HOOKFUNC void GLAPI MyglTexGeniv (GLenum coord, GLenum pname, const GLint *params)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}

static void oglTexImageND(int texture, GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels)
{
	if(type != GL_BYTE && type != GL_UNSIGNED_BYTE
	&& type != GL_SHORT && type != GL_UNSIGNED_SHORT
	&& type != GL_INT && type != GL_UNSIGNED_INT
	&& type != GL_FLOAT && type != GL_DOUBLE
	&& (type != GL_BITMAP || format != GL_COLOR_INDEX))
		OGLRETURNERROR(GL_INVALID_ENUM);
	if(format != GL_COLOR_INDEX
	&& format != GL_RED
	&& format != GL_GREEN
	&& format != GL_BLUE
	&& format != GL_ALPHA
	&& format != GL_RGB
	&& format != GL_RGBA
	&& format != GL_LUMINANCE
	&& format != GL_LUMINANCE_ALPHA)
		OGLRETURNERROR(GL_INVALID_ENUM);
	if(!(internalFormat >= 1 && internalFormat <= 4)
	&& !(internalFormat >= GL_ALPHA4 && internalFormat <= GL_RGBA16)
	&& !(internalFormat == GL_RGB565)
	&& !(internalFormat == GL_RGB || internalFormat == GL_RGBA))
	{
		debuglog(LCF_OGL|LCF_ERROR, "unsupported internalFormat 0x%X\n", internalFormat);
		OGLRETURNERROR(GL_INVALID_ENUM);
	}
	if(width < 0 || height < 0 || level < 0 || (border != 0 && border != 1))
		OGLRETURNERROR(GL_INVALID_VALUE);
	if(oglBeganMode != GL_UNSTARTED)
		OGLRETURNERROR(GL_INVALID_OPERATION);
	if((size_t)texture >= oglTextures.size())
		OGLRETURNERROR(GL_INVALID_OPERATION);
	OpenGLTexture& tex = oglTextures[texture];
	if(!tex.named || !tex.bound)
		OGLRETURNERROR(GL_INVALID_OPERATION);
	tex.ClearData();
	D3DFORMAT d3dfmt = D3DFMT_A8R8G8B8;
	switch(internalFormat)
	{
	//case GL_ALPHA4: break;
	case 1:
	case GL_ALPHA8: d3dfmt = D3DFMT_A8; break;
	//case GL_ALPHA12: break;
	//case GL_ALPHA16: break;
	//case GL_LUMINANCE4: break;
	case GL_LUMINANCE8: d3dfmt = D3DFMT_L8; break;
	//case GL_LUMINANCE12: break;
	//case GL_LUMINANCE16: break;
	case GL_LUMINANCE4_ALPHA4: d3dfmt = D3DFMT_A4L4; break;
	//case GL_LUMINANCE6_ALPHA2: break;
	case 2:
	case GL_LUMINANCE8_ALPHA8: d3dfmt = D3DFMT_A8L8; break;
	//case GL_LUMINANCE12_ALPHA4: break;
	//case GL_LUMINANCE12_ALPHA12: break;
	//case GL_LUMINANCE16_ALPHA16: break;
	//case GL_INTENSITY: break;
	//case GL_INTENSITY4: break;
	case GL_INTENSITY8: d3dfmt = D3DFMT_A8; break;
	//case GL_INTENSITY12: break;
	//case GL_INTENSITY16: break;
	case GL_R3_G3_B2: d3dfmt = D3DFMT_R3G3B2; break;
	case GL_RGB4: d3dfmt = D3DFMT_X4R4G4B4; break;
	case GL_RGB5: d3dfmt = D3DFMT_X1R5G5B5; break;
	case 3:
	case GL_RGB:
	case GL_RGB8: d3dfmt = /*D3DFMT_R8G8B8*/D3DFMT_X8R8G8B8; break;
	//case GL_RGB10: break;
	//case GL_RGB12: break;
	//case GL_RGB16: d3dfmt = D3DFMT_A16B16G16R16; break;
	//case GL_RGBA2: break;
	case GL_RGBA4: d3dfmt = D3DFMT_A4R4G4B4; break;
	case GL_RGB5_A1: d3dfmt = D3DFMT_A1R5G5B5; break;
	case 4:
	case GL_RGBA:
	case GL_RGBA8: d3dfmt = D3DFMT_A8R8G8B8; break;
	//case GL_RGB10_A2: d3dfmt = D3DFMT_A2R10G10B10; break;
	//case GL_RGBA12: break;
	//case GL_RGBA16: d3dfmt = D3DFMT_A16B16G16R16; break;
	case GL_RGB565: d3dfmt = D3DFMT_R5G6B5; break;
	default:
		debuglog(LCF_ERROR|LCF_OGL, "glTexImage: unsupported internalFormat 0x%X was requested.\n", internalFormat);
		//OGLRETURNERROR(GL_INVALID_OPERATION);
		break;
	}
	if(FAILED(ogld3d8Device->CreateTexture(width, height, 1, 0, d3dfmt, D3DPOOL_MANAGED, &tex.d3dTexture)))
	{
		tex.width = 0;
		tex.height = 0;
		tex.internalFormat = 0;
		if(target == GL_PROXY_TEXTURE_1D || target == GL_PROXY_TEXTURE_2D)
			return;
		OGLRETURNERROR(GL_INVALID_OPERATION);
	}
	tex.width = width;
	tex.height = height;
	tex.internalFormat = internalFormat;
	if(target == GL_PROXY_TEXTURE_1D || target == GL_PROXY_TEXTURE_2D)
		return;
	D3DLOCKED_RECT lock;
	if(SUCCEEDED(tex.d3dTexture->LockRect(0,&lock,NULL,0)))
	{
		//if(0) // FIXME TEMP TESTING
		//{
		//	for(int y = 0; y < height; y++)
		//		for(int x = 0; x < width*4; x++)
		//			((char*)lock.pBits + lock.Pitch * y)[x] = (rand() & 0x7f) | 0x10;
		//}
		//else
		if(pixels)
		{
			if(format == GL_RGBA && (d3dfmt == D3DFMT_A8R8G8B8 || d3dfmt == D3DFMT_X8R8G8B8))
			{
				//for(int y = 0; y < height; y++)
				//	memcpy((char*)lock.pBits + lock.Pitch * y, (char*)pixels + width * 4 * y, width * 4);
				for(int y = 0; y < height; y++)
				{
					for(int x = 0; x < width; x++)
					{
						((char*)lock.pBits + lock.Pitch * y)[x*4+0] = ((char*)pixels + width * 4 * y)[x*4+2];
						((char*)lock.pBits + lock.Pitch * y)[x*4+1] = ((char*)pixels + width * 4 * y)[x*4+1];
						((char*)lock.pBits + lock.Pitch * y)[x*4+2] = ((char*)pixels + width * 4 * y)[x*4+0];
						((char*)lock.pBits + lock.Pitch * y)[x*4+3] = ((char*)pixels + width * 4 * y)[x*4+3];
					}
				}
			}
			else if(format == GL_RGB && (d3dfmt == D3DFMT_X8R8G8B8))
			{
				for(int y = 0; y < height; y++)
				{
					for(int x = 0; x < width; x++)
					{
						((char*)lock.pBits + lock.Pitch * y)[x*4+0] = ((char*)pixels + width * 3 * y)[x*3+2];
						((char*)lock.pBits + lock.Pitch * y)[x*4+1] = ((char*)pixels + width * 3 * y)[x*3+1];
						((char*)lock.pBits + lock.Pitch * y)[x*4+2] = ((char*)pixels + width * 3 * y)[x*3+0];
						((char*)lock.pBits + lock.Pitch * y)[x*4+3] = ~0;
					}
				}
			}
/*			else if(format == GL_RGB && (d3dfmt == D3DFMT_R8G8B8))
			{
				for(int y = 0; y < height; y++)
					memcpy((char*)lock.pBits + lock.Pitch * y, (char*)pixels + width * 3 * y, width * 3);
			}
			else if((format == GL_RGB && (d3dfmt == D3DFMT_A1R5G5B5 || d3dfmt == D3DFMT_R5G6B5))
			|| (format == GL_RGBA && (d3dfmt == D3DFMT_A1R5G5B5 || d3dfmt == D3DFMT_A4R4G4B4)))
			{
				for(int y = 0; y < height; y++)
					memcpy((char*)lock.pBits + lock.Pitch * y, (char*)pixels + width * 2 * y, width * 2);
			}
			else if(d3dfmt == D3DFMT_R3G3B2 || d3dfmt == D3DFMT_A8 || d3dfmt == D3DFMT_L8 || d3dfmt == D3DFMT_A4L4)
			{
				for(int y = 0; y < height; y++)
					memcpy((char*)lock.pBits + lock.Pitch * y, (char*)pixels + width * y, width);
			}*/
			else
			{
				debuglog(LCF_ERROR|LCF_OGL, "glTexImage: unsupported format conversion (format=0x%X, internalFormat=0x%X, d3dfmt=0x%X).\n", format,internalFormat,d3dfmt);
				// some temp default
				for(int y = 0; y < height; y++)
					memset((char*)lock.pBits + lock.Pitch * y, 0x80, width * 4);
/*				switch(format)
				{
				case GL_RGBA:
					break;
				case GL_RGB:
					break;
				case GL_COLOR_INDEX:
					break;
				case GL_RED:
				case GL_GREEN:
				case GL_BLUE:
				case GL_ALPHA:
				case GL_LUMINANCE:
					break;
				case GL_LUMINANCE_ALPHA:
					break;
				}*/
			}
		}
		tex.d3dTexture->UnlockRect(0);
	}
}

HOOKFUNC void GLAPI MyglTexImage1D (GLenum target, GLint level, GLint internalFormat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(target != GL_TEXTURE_1D && target != GL_PROXY_TEXTURE_1D)
			OGLRETURNERROR(GL_INVALID_ENUM);
		oglTexImageND(oglTexture1DTarget, target,level,internalFormat,width,1,border,format,type,pixels);
	}
	OGLPUSHDISPLAYLISTENTRY_8ARG(idglTexImage1D, glenum,target, glint,level, glint,internalFormat, glsizei,width, glint,border, glenum,format, glenum,type, ptr,(void*)pixels);
}

HOOKFUNC void GLAPI MyglTexImage2D (GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
	debuglog(LCF_OGL, __FUNCTION__ "(target=0x%X (oglTexture2DTarget=%d), level=%d, components=0x%X, width=%d, height=%d, border=%d, format=0x%X, type=0x%X, pixel=0x%X) called.\n", target, oglTexture2DTarget, level, internalFormat, width, height, border, format, type, pixels);
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(target != GL_TEXTURE_2D && target != GL_PROXY_TEXTURE_2D)
			OGLRETURNERROR(GL_INVALID_ENUM);
		oglTexImageND(oglTexture2DTarget, target,level,internalFormat,width,height,border,format,type,pixels);
	}
	OGLPUSHDISPLAYLISTENTRY_9ARG(idglTexImage2D, glenum,target, glint,level, glint,internalFormat, glsizei,width, glsizei,height, glint,border, glenum,format, glenum,type, ptr,(void*)pixels);
}

static void OglTexParameter(GLenum pname, int param)
{
	switch(pname)
	{
	case GL_TEXTURE_MAG_FILTER:
		oglServerState.other.magFilter = param;
		switch(param)
		{
		case GL_NEAREST:
			ogld3d8Device->SetTextureStageState(0, D3DTSS_MAGFILTER,D3DTEXF_POINT);
			break;
		case GL_LINEAR:
			ogld3d8Device->SetTextureStageState(0, D3DTSS_MAGFILTER,D3DTEXF_LINEAR);
			break;
		}
		break;
	case GL_TEXTURE_MIN_FILTER:
		oglServerState.other.minFilter = param;
		switch(param)
		{
		case GL_NEAREST:
			ogld3d8Device->SetTextureStageState(0, D3DTSS_MINFILTER,D3DTEXF_POINT);
			ogld3d8Device->SetTextureStageState(0, D3DTSS_MIPFILTER,D3DTEXF_NONE);
			break;
		case GL_LINEAR:
			ogld3d8Device->SetTextureStageState(0, D3DTSS_MINFILTER,D3DTEXF_LINEAR);
			ogld3d8Device->SetTextureStageState(0, D3DTSS_MIPFILTER,D3DTEXF_NONE);
			break;
		case GL_NEAREST_MIPMAP_NEAREST:
			ogld3d8Device->SetTextureStageState(0, D3DTSS_MINFILTER,D3DTEXF_POINT);
			ogld3d8Device->SetTextureStageState(0, D3DTSS_MIPFILTER,D3DTEXF_POINT);
			break;
		case GL_LINEAR_MIPMAP_NEAREST:
			ogld3d8Device->SetTextureStageState(0, D3DTSS_MINFILTER,D3DTEXF_LINEAR);
			ogld3d8Device->SetTextureStageState(0, D3DTSS_MIPFILTER,D3DTEXF_POINT);
			break;
		case GL_NEAREST_MIPMAP_LINEAR:
			ogld3d8Device->SetTextureStageState(0, D3DTSS_MINFILTER,D3DTEXF_POINT);
			ogld3d8Device->SetTextureStageState(0, D3DTSS_MIPFILTER,D3DTEXF_LINEAR);
			break;
		case GL_LINEAR_MIPMAP_LINEAR:
			ogld3d8Device->SetTextureStageState(0, D3DTSS_MINFILTER,D3DTEXF_LINEAR);
			ogld3d8Device->SetTextureStageState(0, D3DTSS_MIPFILTER,D3DTEXF_LINEAR);
			break;
		}
		break;
	case GL_TEXTURE_WRAP_S:
		switch(param)
		{
		case GL_CLAMP:
			ogld3d8Device->SetTextureStageState(0, D3DTSS_ADDRESSU,D3DTADDRESS_CLAMP);
			break;
		case GL_REPEAT:
			ogld3d8Device->SetTextureStageState(0, D3DTSS_ADDRESSU,D3DTADDRESS_WRAP);
			break;
		}
		break;
	case GL_TEXTURE_WRAP_T:
		switch(param)
		{
		case GL_CLAMP:
			ogld3d8Device->SetTextureStageState(0, D3DTSS_ADDRESSV,D3DTADDRESS_CLAMP);
			break;
		case GL_REPEAT:
			ogld3d8Device->SetTextureStageState(0, D3DTSS_ADDRESSV,D3DTADDRESS_WRAP);
			break;
		}
		break;
	case GL_TEXTURE_BORDER_COLOR:
		debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ ": GL_TEXTURE_BORDER_COLOR is NYI.\n");
		break;
	case GL_TEXTURE_PRIORITY:
		debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ ": GL_TEXTURE_PRIORITY is NYI.\n");
		break;
	}
}

HOOKFUNC void GLAPI MyglTexParameterf (GLenum target, GLenum pname, GLfloat param)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(oglBeganMode != GL_UNSTARTED)
			OGLRETURNERROR(GL_INVALID_OPERATION);
		int texture;
		switch(target)
		{
			case GL_TEXTURE_1D: texture = oglTexture1DTarget; break;
			case GL_TEXTURE_2D: texture = oglTexture2DTarget; break;
			default: OGLRETURNERROR(GL_INVALID_ENUM); break;
		}
		OglTexParameter(pname,(int)param);
	}
	OGLPUSHDISPLAYLISTENTRY_3ARG(idglTexParameterf, glenum,target, glenum,pname, glfloat,param);
}

HOOKFUNC void GLAPI MyglTexParameterfv (GLenum target, GLenum pname, const GLfloat *params)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(oglBeganMode != GL_UNSTARTED)
			OGLRETURNERROR(GL_INVALID_OPERATION);
		int texture;
		switch(target)
		{
			case GL_TEXTURE_1D: texture = oglTexture1DTarget; break;
			case GL_TEXTURE_2D: texture = oglTexture2DTarget; break;
			default: OGLRETURNERROR(GL_INVALID_ENUM); break;
		}
		OglTexParameter(pname,(int)params[0]);
	}
	OGLPUSHDISPLAYLISTENTRY_2ARGV(idglTexParameterfv, glenum,target, glenum,pname, sizeof(GLfloat),params,1);
}

HOOKFUNC void GLAPI MyglTexParameteri (GLenum target, GLenum pname, GLint param)
{
	debuglog(LCF_OGL, __FUNCTION__ "(target=0x%X, pname=0x%X, param=%d=0x%X) called.\n", target, pname, param,param);
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(oglBeganMode != GL_UNSTARTED)
			OGLRETURNERROR(GL_INVALID_OPERATION);
		int texture;
		switch(target)
		{
			case GL_TEXTURE_1D: texture = oglTexture1DTarget; break;
			case GL_TEXTURE_2D: texture = oglTexture2DTarget; break;
			default: OGLRETURNERROR(GL_INVALID_ENUM); break;
		}
		OglTexParameter(pname,(int)param);
	}
	OGLPUSHDISPLAYLISTENTRY_3ARG(idglTexParameteri, glenum,target, glenum,pname, glint,param);
}

HOOKFUNC void GLAPI MyglTexParameteriv (GLenum target, GLenum pname, const GLint *params)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(oglBeganMode != GL_UNSTARTED)
			OGLRETURNERROR(GL_INVALID_OPERATION);
		int texture;
		switch(target)
		{
			case GL_TEXTURE_1D: texture = oglTexture1DTarget; break;
			case GL_TEXTURE_2D: texture = oglTexture2DTarget; break;
			default: OGLRETURNERROR(GL_INVALID_ENUM); break;
		}
		OglTexParameter(pname,(int)params[0]);
	}
	OGLPUSHDISPLAYLISTENTRY_2ARGV(idglTexParameteriv, glenum,target, glenum,pname, sizeof(GLint),params,1);
}

HOOKFUNC void GLAPI MyglTranslated (GLdouble x, GLdouble y, GLdouble z)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		D3DMATRIX mt = {
			1,0,0,(float)x,
			0,1,0,(float)y,
			0,0,1,(float)z,
			0,0,0,1
		};
		oglMulD3DMats(*oglMatrix,*oglMatrix,mt);
		*oglDirty = true;
	}
	OGLPUSHDISPLAYLISTENTRY_3ARG(idglTranslated, gldouble,x, gldouble,y, gldouble,z);
}

HOOKFUNC void GLAPI MyglTranslatef (GLfloat x, GLfloat y, GLfloat z)
{
	debuglog(LCF_OGL, __FUNCTION__ "(%g, %g, %g) called.\n", x,y,z);
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		D3DMATRIX mt = {
			1,0,0,x,
			0,1,0,y,
			0,0,1,z,
			0,0,0,1
		};
		oglMulD3DMats(*oglMatrix,*oglMatrix,mt);
		*oglDirty = true;
	}
	OGLPUSHDISPLAYLISTENTRY_3ARG(idglTranslatef, glfloat,x, glfloat,y, glfloat,z);
}

HOOKFUNC void GLAPI MyglVertex2d (GLdouble x, GLdouble y)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.x = (FLOAT)x;
		oglServerState.current.y = (FLOAT)y;
		oglServerState.current.z = (FLOAT)0;
		oglServerState.current.w = (FLOAT)1;
		oglImmediateVertices.push_back(oglServerState.current);
	}
	OGLPUSHDISPLAYLISTENTRY_2ARG(idglVertex2d, gldouble,x, gldouble,y);
}

HOOKFUNC void GLAPI MyglVertex2dv (const GLdouble *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.x = (FLOAT)v[0];
		oglServerState.current.y = (FLOAT)v[1];
		oglServerState.current.z = (FLOAT)0;
		oglServerState.current.w = (FLOAT)1;
		oglImmediateVertices.push_back(oglServerState.current);
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglVertex2dv, sizeof(GLdouble),v,2);
}

HOOKFUNC void GLAPI MyglVertex2f (GLfloat x, GLfloat y)
{
	debuglog(LCF_OGL, __FUNCTION__ "(%g, %g) called.\n", x, y);
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.x = (FLOAT)x;
		oglServerState.current.y = (FLOAT)y;
		oglServerState.current.z = (FLOAT)0;
		oglServerState.current.w = (FLOAT)1;
		oglImmediateVertices.push_back(oglServerState.current);
	}
	OGLPUSHDISPLAYLISTENTRY_2ARG(idglVertex2f, glfloat,x, glfloat,y);
}

HOOKFUNC void GLAPI MyglVertex2fv (const GLfloat *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.x = (FLOAT)v[0];
		oglServerState.current.y = (FLOAT)v[1];
		oglServerState.current.z = (FLOAT)0;
		oglServerState.current.w = (FLOAT)1;
		oglImmediateVertices.push_back(oglServerState.current);
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglVertex2fv, sizeof(GLfloat),v,2);
}

HOOKFUNC void GLAPI MyglVertex2i (GLint x, GLint y)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.x = (FLOAT)x;
		oglServerState.current.y = (FLOAT)y;
		oglServerState.current.z = (FLOAT)0;
		oglServerState.current.w = (FLOAT)1;
		oglImmediateVertices.push_back(oglServerState.current);
	}
	OGLPUSHDISPLAYLISTENTRY_2ARG(idglVertex2i, glint,x, glint,y);
}

HOOKFUNC void GLAPI MyglVertex2iv (const GLint *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.x = (FLOAT)v[0];
		oglServerState.current.y = (FLOAT)v[1];
		oglServerState.current.z = (FLOAT)0;
		oglServerState.current.w = (FLOAT)1;
		oglImmediateVertices.push_back(oglServerState.current);
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglVertex2iv, sizeof(GLint),v,2);
}

HOOKFUNC void GLAPI MyglVertex2s (GLshort x, GLshort y)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.x = (FLOAT)x;
		oglServerState.current.y = (FLOAT)y;
		oglServerState.current.z = (FLOAT)0;
		oglServerState.current.w = (FLOAT)1;
		oglImmediateVertices.push_back(oglServerState.current);
	}
	OGLPUSHDISPLAYLISTENTRY_2ARG(idglVertex2s, glshort,x, glshort,y);
}

HOOKFUNC void GLAPI MyglVertex2sv (const GLshort *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.x = (FLOAT)v[0];
		oglServerState.current.y = (FLOAT)v[1];
		oglServerState.current.z = (FLOAT)0;
		oglServerState.current.w = (FLOAT)1;
		oglImmediateVertices.push_back(oglServerState.current);
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglVertex2sv, sizeof(GLshort),v,2);
}

HOOKFUNC void GLAPI MyglVertex3d (GLdouble x, GLdouble y, GLdouble z)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.x = (FLOAT)x;
		oglServerState.current.y = (FLOAT)y;
		oglServerState.current.z = (FLOAT)z;
		oglServerState.current.w = (FLOAT)1;
		oglImmediateVertices.push_back(oglServerState.current);
	}
	OGLPUSHDISPLAYLISTENTRY_3ARG(idglVertex3d, gldouble,x, gldouble,y, gldouble,z);
}

HOOKFUNC void GLAPI MyglVertex3dv (const GLdouble *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.x = (FLOAT)v[0];
		oglServerState.current.y = (FLOAT)v[1];
		oglServerState.current.z = (FLOAT)v[2];
		oglServerState.current.w = (FLOAT)1;
		oglImmediateVertices.push_back(oglServerState.current);
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglVertex3dv, sizeof(GLdouble),v,3);
}

HOOKFUNC void GLAPI MyglVertex3f (GLfloat x, GLfloat y, GLfloat z)
{
	debuglog(LCF_OGL, __FUNCTION__ "(%g, %g, %g) called.\n", x,y,z);
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.x = (FLOAT)x;
		oglServerState.current.y = (FLOAT)y;
		oglServerState.current.z = (FLOAT)z;
		oglServerState.current.w = (FLOAT)1;
		oglImmediateVertices.push_back(oglServerState.current);
	}
	OGLPUSHDISPLAYLISTENTRY_3ARG(idglVertex3f, glfloat,x, glfloat,y, glfloat,z);
}

HOOKFUNC void GLAPI MyglVertex3fv (const GLfloat *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.x = (FLOAT)v[0];
		oglServerState.current.y = (FLOAT)v[1];
		oglServerState.current.z = (FLOAT)v[2];
		oglServerState.current.w = (FLOAT)1;
		oglImmediateVertices.push_back(oglServerState.current);
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglVertex3fv, sizeof(GLfloat),v,3);
}

HOOKFUNC void GLAPI MyglVertex3i (GLint x, GLint y, GLint z)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.x = (FLOAT)x;
		oglServerState.current.y = (FLOAT)y;
		oglServerState.current.z = (FLOAT)z;
		oglServerState.current.w = (FLOAT)1;
		oglImmediateVertices.push_back(oglServerState.current);
	}
	OGLPUSHDISPLAYLISTENTRY_3ARG(idglVertex3i, glint,x, glint,y, glint,z);
}

HOOKFUNC void GLAPI MyglVertex3iv (const GLint *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.x = (FLOAT)v[0];
		oglServerState.current.y = (FLOAT)v[1];
		oglServerState.current.z = (FLOAT)v[2];
		oglServerState.current.w = (FLOAT)1;
		oglImmediateVertices.push_back(oglServerState.current);
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglVertex3iv, sizeof(GLint),v,3);
}

HOOKFUNC void GLAPI MyglVertex3s (GLshort x, GLshort y, GLshort z)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.x = (FLOAT)x;
		oglServerState.current.y = (FLOAT)y;
		oglServerState.current.z = (FLOAT)z;
		oglServerState.current.w = (FLOAT)1;
		oglImmediateVertices.push_back(oglServerState.current);
	}
	OGLPUSHDISPLAYLISTENTRY_3ARG(idglVertex3s, glshort,x, glshort,y, glshort,z);
}

HOOKFUNC void GLAPI MyglVertex3sv (const GLshort *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.x = (FLOAT)v[0];
		oglServerState.current.y = (FLOAT)v[1];
		oglServerState.current.z = (FLOAT)v[2];
		oglServerState.current.w = (FLOAT)1;
		oglImmediateVertices.push_back(oglServerState.current);
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglVertex3sv, sizeof(GLshort),v,3);
}

HOOKFUNC void GLAPI MyglVertex4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.x = (FLOAT)x;
		oglServerState.current.y = (FLOAT)y;
		oglServerState.current.z = (FLOAT)z;
		oglServerState.current.w = (FLOAT)w;
		oglImmediateVertices.push_back(oglServerState.current);
	}
	OGLPUSHDISPLAYLISTENTRY_4ARG(idglVertex4d, gldouble,x, gldouble,y, gldouble,z, gldouble,w);
}

HOOKFUNC void GLAPI MyglVertex4dv (const GLdouble *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.x = (FLOAT)v[0];
		oglServerState.current.y = (FLOAT)v[1];
		oglServerState.current.z = (FLOAT)v[2];
		oglServerState.current.w = (FLOAT)v[3];
		oglImmediateVertices.push_back(oglServerState.current);
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglVertex4dv, sizeof(GLdouble),v,4);
}

HOOKFUNC void GLAPI MyglVertex4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.x = (FLOAT)x;
		oglServerState.current.y = (FLOAT)y;
		oglServerState.current.z = (FLOAT)z;
		oglServerState.current.w = (FLOAT)w;
		oglImmediateVertices.push_back(oglServerState.current);
	}
	OGLPUSHDISPLAYLISTENTRY_4ARG(idglVertex4f, glfloat,x, glfloat,y, glfloat,z, glfloat,w);
}

HOOKFUNC void GLAPI MyglVertex4fv (const GLfloat *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.x = (FLOAT)v[0];
		oglServerState.current.y = (FLOAT)v[1];
		oglServerState.current.z = (FLOAT)v[2];
		oglServerState.current.w = (FLOAT)v[3];
		oglImmediateVertices.push_back(oglServerState.current);
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglVertex4fv, sizeof(GLfloat),v,4);
}

HOOKFUNC void GLAPI MyglVertex4i (GLint x, GLint y, GLint z, GLint w)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.x = (FLOAT)x;
		oglServerState.current.y = (FLOAT)y;
		oglServerState.current.z = (FLOAT)z;
		oglServerState.current.w = (FLOAT)w;
		oglImmediateVertices.push_back(oglServerState.current);
	}
	OGLPUSHDISPLAYLISTENTRY_4ARG(idglVertex4i, glint,x, glint,y, glint,z, glint,w);
}

HOOKFUNC void GLAPI MyglVertex4iv (const GLint *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.x = (FLOAT)v[0];
		oglServerState.current.y = (FLOAT)v[1];
		oglServerState.current.z = (FLOAT)v[2];
		oglServerState.current.w = (FLOAT)v[3];
		oglImmediateVertices.push_back(oglServerState.current);
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglVertex4iv, sizeof(GLint),v,4);
}

HOOKFUNC void GLAPI MyglVertex4s (GLshort x, GLshort y, GLshort z, GLshort w)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.x = (FLOAT)x;
		oglServerState.current.y = (FLOAT)y;
		oglServerState.current.z = (FLOAT)z;
		oglServerState.current.w = (FLOAT)w;
		oglImmediateVertices.push_back(oglServerState.current);
	}
	OGLPUSHDISPLAYLISTENTRY_4ARG(idglVertex4s, glshort,x, glshort,y, glshort,z, glshort,w);
}

HOOKFUNC void GLAPI MyglVertex4sv (const GLshort *v)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		oglServerState.current.x = (FLOAT)v[0];
		oglServerState.current.y = (FLOAT)v[1];
		oglServerState.current.z = (FLOAT)v[2];
		oglServerState.current.w = (FLOAT)v[3];
		oglImmediateVertices.push_back(oglServerState.current);
	}
	OGLPUSHDISPLAYLISTENTRY_0ARGV(idglVertex4sv, sizeof(GLshort),v,4);
}


HOOKFUNC void GLAPI MyglRectd (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
	debuglog(LCF_OGL, __FUNCTION__ "(%g,%g,%g,%g) called.\n",(float)x1,(float)y1,(float)x2,(float)y2);
	////GLFUNCBOILERPLATE;
	//if(oglAllowExecuteCommands)
	//{
	//}
	MyglBegin(GL_POLYGON);
	MyglVertex2d(x1, y1);
	MyglVertex2d(x2, y1);
	MyglVertex2d(x2, y2);
	MyglVertex2d(x1, y2);
	MyglEnd();
}

HOOKFUNC void GLAPI MyglRectdv (const GLdouble *v1, const GLdouble *v2)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	////GLFUNCBOILERPLATE;
	//if(oglAllowExecuteCommands)
	//{
	//}
	MyglBegin(GL_POLYGON);
	MyglVertex2d(v1[0], v1[1]);
	MyglVertex2d(v2[0], v1[1]);
	MyglVertex2d(v2[0], v2[1]);
	MyglVertex2d(v1[0], v2[1]);
	MyglEnd();
}

HOOKFUNC void GLAPI MyglRectf (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
	debuglog(LCF_OGL, __FUNCTION__ "(%g,%g,%g,%g) called.\n",x1,y1,x2,y2);
	////GLFUNCBOILERPLATE;
	//if(oglAllowExecuteCommands)
	//{
	//}
	MyglBegin(GL_POLYGON);
	MyglVertex2f(x1, y1);
	MyglVertex2f(x2, y1);
	MyglVertex2f(x2, y2);
	MyglVertex2f(x1, y2);
	MyglEnd();
}

HOOKFUNC void GLAPI MyglRectfv (const GLfloat *v1, const GLfloat *v2)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	////GLFUNCBOILERPLATE;
	//if(oglAllowExecuteCommands)
	//{
	//}
	MyglBegin(GL_POLYGON);
	MyglVertex2f(v1[0], v1[1]);
	MyglVertex2f(v2[0], v1[1]);
	MyglVertex2f(v2[0], v2[1]);
	MyglVertex2f(v1[0], v2[1]);
	MyglEnd();
}

HOOKFUNC void GLAPI MyglRecti (GLint x1, GLint y1, GLint x2, GLint y2)
{
	debuglog(LCF_OGL, __FUNCTION__ "(%d,%d,%d,%d) called.\n",x1,y1,x2,y2);
	////GLFUNCBOILERPLATE;
	//if(oglAllowExecuteCommands)
	//{
	//}
	MyglBegin(GL_POLYGON);
	MyglVertex2i(x1, y1);
	MyglVertex2i(x2, y1);
	MyglVertex2i(x2, y2);
	MyglVertex2i(x1, y2);
	MyglEnd();
}

HOOKFUNC void GLAPI MyglRectiv (const GLint *v1, const GLint *v2)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	////GLFUNCBOILERPLATE;
	//if(oglAllowExecuteCommands)
	//{
	//}
	MyglBegin(GL_POLYGON);
	MyglVertex2i(v1[0], v1[1]);
	MyglVertex2i(v2[0], v1[1]);
	MyglVertex2i(v2[0], v2[1]);
	MyglVertex2i(v1[0], v2[1]);
	MyglEnd();
}

HOOKFUNC void GLAPI MyglRects (GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
	debuglog(LCF_OGL, __FUNCTION__ "(%d,%d,%d,%d) called.\n",(int)x1,(int)y1,(int)x2,(int)y2);
	////GLFUNCBOILERPLATE;
	//if(oglAllowExecuteCommands)
	//{
	//}
	MyglBegin(GL_POLYGON);
	MyglVertex2s(x1, y1);
	MyglVertex2s(x2, y1);
	MyglVertex2s(x2, y2);
	MyglVertex2s(x1, y2);
	MyglEnd();
}

HOOKFUNC void GLAPI MyglRectsv (const GLshort *v1, const GLshort *v2)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	////GLFUNCBOILERPLATE;
	//if(oglAllowExecuteCommands)
	//{
	//}
	MyglBegin(GL_POLYGON);
	MyglVertex2s(v1[0], v1[1]);
	MyglVertex2s(v2[0], v1[1]);
	MyglVertex2s(v2[0], v2[1]);
	MyglVertex2s(v1[0], v2[1]);
	MyglEnd();
}


HOOKFUNC void GLAPI MyglViewport (GLint x, GLint y, GLsizei width, GLsizei height)
{
	debuglog(LCF_OGL, __FUNCTION__ "(x=%d, y=%d, width=%d, height=%d) called.\n", x,y,width,height);
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
		if(oglBeganMode != GL_UNSTARTED)
			OGLRETURNERROR(GL_INVALID_OPERATION);
		if(width < 0 || height < 0)
			OGLRETURNERROR(GL_INVALID_VALUE);
		D3DVIEWPORT8 vp;
		ogld3d8Device->GetViewport(&vp);
		vp.X = x;
		vp.Y = y;
		vp.Width = width;
		vp.Height = height;
		ogld3d8Device->SetViewport(&vp);
	}
	OGLPUSHDISPLAYLISTENTRY_4ARG(idglViewport, glint,x, glint,y, glsizei,width, glsizei,height);
}

HOOKFUNC GLuint GLAPI MyglGenLists (GLsizei range)
{
	debuglog(LCF_OGL, __FUNCTION__ "(range=%d) called.\n", range);
	//GLFUNCBOILERPLATE;
	if(range < 0)
		OGLRETURNERRORVAL(GL_INVALID_VALUE, 0);
	if(oglBeganMode != GL_UNSTARTED)
		OGLRETURNERRORVAL(GL_INVALID_OPERATION, 0);
	int size = oglDisplayLists.size();
	if(size == 0)
		size = 1;
	GLuint index = (GLuint)size;
	size += range;
	oglDisplayLists.resize(size);
	return index;
}

HOOKFUNC GLenum GLAPI MyglGetError (void)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	GLenum rv = oglError;
	oglError = GL_NO_ERROR;
	return rv;
}

HOOKFUNC GLboolean GLAPI MyglIsEnabled (GLenum cap)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	return 0; // NYI
}

HOOKFUNC GLboolean GLAPI MyglIsList (GLuint list)
{
	debuglog(LCF_OGL, __FUNCTION__ "(list=%d) called.\n", list);
	//GLFUNCBOILERPLATE;
	if(oglBeganMode != GL_UNSTARTED)
		OGLRETURNERRORVAL(GL_INVALID_OPERATION, false); // not allowed between begin and end
	if(list >= oglDisplayLists.size())
		return false;
	return oglDisplayLists[list].valid;
}

HOOKFUNC GLint GLAPI MyglRenderMode (GLenum mode)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	return 0; // NYI
}



HOOKFUNC GLboolean GLAPI MyglAreTexturesResident (GLsizei n, const GLuint *textures, GLboolean *residences)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	return 0; // NYI
}
HOOKFUNC void GLAPI MyglArrayElement (GLint i)
{ // NYI
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}
HOOKFUNC void GLAPI MyglColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(size < 3 || size > 4 || stride < 0)
		OGLRETURNERROR(GL_INVALID_VALUE);
	if(type != GL_BYTE && type != GL_UNSIGNED_BYTE && type != GL_SHORT && type != GL_UNSIGNED_SHORT
	&& type != GL_INT && type != GL_UNSIGNED_INT && type != GL_FLOAT && type != GL_DOUBLE)
		OGLRETURNERROR(GL_INVALID_ENUM);
	oglClientState.arrayState.colorArraySize = size;
	oglClientState.arrayState.colorArrayType = type;
	oglClientState.arrayState.colorArrayStride = stride;
	oglClientState.arrayState.colorArrayPointer = pointer;
	oglClientState.arrayState.RecalculateSizes();
}
HOOKFUNC void GLAPI MyglCopyTexImage1D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}
HOOKFUNC void GLAPI MyglCopyTexImage2D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}
HOOKFUNC void GLAPI MyglCopyTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}
HOOKFUNC void GLAPI MyglCopyTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}
static DWORD OglGetD3dFVF(/*__out*/ DWORD& vertexStreamZeroStride)
{
	DWORD size = 0;
	DWORD fvf = 0;
	if(oglClientState.arrayState.vertexArrayEnabled)
	{
		int arraySize = oglClientState.arrayState.vertexArraySize;
		if(arraySize == 4 && oglClientState.arrayState.normalArrayEnabled)
			arraySize = 3;
		switch(arraySize)
		{
		case 2:
		case 3: fvf |= D3DFVF_XYZ; size += 3*sizeof(FLOAT); break;
		case 4: fvf |= D3DFVF_XYZRHW; size += 4*sizeof(FLOAT); break;
		}
	}
	if(oglClientState.arrayState.normalArrayEnabled)
	{
		fvf |= D3DFVF_NORMAL;
		size += 3*sizeof(FLOAT);
	}
	if(oglClientState.arrayState.colorArrayEnabled)
	{
		fvf |= D3DFVF_DIFFUSE;
		size += sizeof(D3DCOLOR);
	}
	if(oglClientState.arrayState.texCoordArrayEnabled)
	{
		fvf |= D3DFVF_TEX1;
		switch(oglClientState.arrayState.texCoordArraySize)
		{
		case 1: fvf |= D3DFVF_TEXCOORDSIZE1(0); size += 1*sizeof(FLOAT); break;
		default:
		case 2: fvf |= D3DFVF_TEXCOORDSIZE2(0); size += 2*sizeof(FLOAT); break;
		case 3: fvf |= D3DFVF_TEXCOORDSIZE3(0); size += 3*sizeof(FLOAT); break;
		case 4: fvf |= D3DFVF_TEXCOORDSIZE4(0); size += 4*sizeof(FLOAT); break;
		}
	}
	vertexStreamZeroStride = size;
	return fvf;
}

static void oglTransposeD3DMat(D3DMATRIX& m, const D3DMATRIX& m1)
{
	D3DMATRIX temp = {
		m1.m[0][0], m1.m[1][0], m1.m[2][0], m1.m[3][0],
		m1.m[0][1], m1.m[1][1], m1.m[2][1], m1.m[3][1],
		m1.m[0][2], m1.m[1][2], m1.m[2][2], m1.m[3][2],
		m1.m[0][3], m1.m[1][3], m1.m[2][3], m1.m[3][3],
	};
	m = temp;
}

static void OglApplyDirtyMatrices()
{
	// d3d multiplies the view and projection matrices
	// in a different order than opengl does.
	// we can't tell d3d to change the multiplication order,
	// but we can transpose both matrices to get the same effect.
	if(oglDirtyMV)
	{
		oglDirtyMV = false;
		D3DMATRIX mtemp = oglMatrixMV;
		oglTransposeD3DMat(mtemp, mtemp);
		ogld3d8Device->SetTransform(D3DTS_VIEW, &mtemp);
	}
	if(oglDirtyP)
	{
		oglDirtyP = false;
		D3DMATRIX mtemp = oglMatrixP;

		//debugprintf("PROJECTION MATRIX:\n"
		//	"(%g, %g, %g, %g,\n"
		//	" %g, %g, %g, %g,\n"
		//	" %g, %g, %g, %g,\n"
		//	" %g, %g, %g, %g)\n\n",
		//	mtemp.m[0][0], mtemp.m[0][1], mtemp.m[0][2], mtemp.m[0][3],
		//	mtemp.m[1][0], mtemp.m[1][1], mtemp.m[1][2], mtemp.m[1][3],
		//	mtemp.m[2][0], mtemp.m[2][1], mtemp.m[2][2], mtemp.m[2][3],
		//	mtemp.m[3][0], mtemp.m[3][1], mtemp.m[3][2], mtemp.m[3][3]);

		// opengl clip space goes from -1 to 1 on the z axis
		// directx clip space goes from 0 to 1 on the z axis
		// thus, we alter the projection matrix
		// to scale and offset the final clip space z coord
		//D3DMATRIX mt = {
		//	1,0,0,0,
		//	0,1,0,0,
		//	0,0,.5f,.5f, // zfinal = zfinal * 0.5f + 0.5f
		//	0,0,0,1
		//};
		//oglMulD3DMats(mtemp,mt,mtemp);
		// argh, I don't know why the above doesn't work (it works in Tumiki Fighters but not in Generic)
		// and I don't know why the following does work (although it looks too specific to work in all cases)
		// TODO: maybe a vertex shader could do this more sensibly.
		for(int i = 0; i < 4; i++)
			mtemp.m[2][i] *= -(mtemp.m[3][2] + 0.5f);
		mtemp.m[2][3] += mtemp.m[3][3] * 0.5f;

		oglTransposeD3DMat(mtemp, mtemp);
		ogld3d8Device->SetTransform(D3DTS_PROJECTION, &mtemp);
	}
	if(oglDirtyT)
	{
		oglDirtyT = false;

		// directx texture mapping has some annoying differences from opengl
		D3DMATRIX mtemp = oglMatrixT;
		D3DMATRIX mt = {
			1,0,0,oglTextureOffsets[0], // apply opengl -> directx texel origin fix
			0,1,0,oglTextureOffsets[1],
			0,0,1,0,
			0,0,0,1
		};
		oglMulD3DMats(mtemp,mt,mtemp);
		oglTransposeD3DMat(mtemp, mtemp);
		mtemp.m[0][2] = mtemp.m[0][3]; // move w to z since directx uses (u,v,1,0) instead of (u,v,0,1)
		mtemp.m[1][2] = mtemp.m[1][3];
		mtemp.m[2][0] = mtemp.m[3][0];
		mtemp.m[2][1] = mtemp.m[3][1];
		mtemp.m[2][2] = mtemp.m[3][3];

		ogld3d8Device->SetTransform(D3DTS_TEXTURE0, &mtemp);
		if(!oglUsedT)
		{
			oglUsedT = true;
			ogld3d8Device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
		}
	}
}

static bool oglD3dConstColorActive = false;
static OGLCOLOR oglD3dCurrentConstColor;

static void OglD3dDrawPrimitive(DWORD fvf, DWORD elemSize, DWORD primCount, DWORD primType, void* vertexBuffer)
{
	if(oglServerState.other.texFunctionMode != GL_REPLACE)
	{
		bool needsConstColor = !(fvf & D3DFVF_DIFFUSE);
		if(needsConstColor != oglD3dConstColorActive)
		{
			oglD3dConstColorActive = needsConstColor;
			DWORD arg = needsConstColor ? D3DTA_TFACTOR : D3DTA_CURRENT;
			ogld3d8Device->SetTextureStageState(0, D3DTSS_COLORARG2, arg);
			ogld3d8Device->SetTextureStageState(0, D3DTSS_ALPHAARG2, arg);
		}
		if(needsConstColor && oglD3dCurrentConstColor != oglServerState.current.c)
		{
			oglD3dCurrentConstColor = oglServerState.current.c;
			OGLCOLOR c = oglServerState.current.c;
			ogld3d8Device->SetRenderState(D3DRS_TEXTUREFACTOR, (c<<8)|(c>>24));
		}
	}

	OglApplyDirtyMatrices();
	IDirect3DVertexBuffer8* vb = (IDirect3DVertexBuffer8*)vertexBuffer;
//	debugprintf("fvf=0x%X, elemSize=%d=0x%X, primCount=%d=0x%X, primType=0x%X\n", fvf, elemSize,elemSize, primCount,primCount, primType);
	ogld3d8Device->SetVertexShader(fvf);
	ogld3d8Device->SetStreamSource(0, vb, elemSize);
	ogld3d8Device->DrawPrimitive((D3DPRIMITIVETYPE)primType, 0, primCount);
}

void FillVertexBufferFromIndex(BYTE*& buffer, int index)
{
	if(oglClientState.arrayState.vertexArrayEnabled)
	{
		char* src = (char*)oglClientState.arrayState.vertexArrayPointer + index * oglClientState.arrayState.vertexArrayStride;
		int arraySize = oglClientState.arrayState.vertexArraySize;
		if(arraySize == 4 && oglClientState.arrayState.normalArrayEnabled)
			arraySize = 3;
		//debugprintf("index = %d\n", index);
		//debugprintf("arraySize = %d\n", arraySize);
		//debugprintf("oglClientState.arrayState.vertexArrayPointer = 0x%X\n", oglClientState.arrayState.vertexArrayPointer);
		//debugprintf("oglClientState.arrayState.vertexArrayStride = 0x%X\n", oglClientState.arrayState.vertexArrayStride);
		switch(oglClientState.arrayState.vertexArrayType)
		{
		case GL_SHORT:
			for(int j = 0; j < arraySize; j++)
				*(((FLOAT*&)buffer)++) = (FLOAT)(((GLshort*)src)[j]);
			break;
		case GL_INT:
			for(int j = 0; j < arraySize; j++)
				*(((FLOAT*&)buffer)++) = (FLOAT)(((GLint*)src)[j]);
			break;
		case GL_FLOAT:
			for(int j = 0; j < arraySize; j++)
			{
				//debugprintf("vert[%d].pos[%d] = %g\n", index,j, (FLOAT)(((GLfloat*)src)[j]));
				*(((FLOAT*&)buffer)++) = (FLOAT)(((GLfloat*)src)[j]);
			}
			break;
		case GL_DOUBLE:
			for(int j = 0; j < arraySize; j++)
				*(((FLOAT*&)buffer)++) = (FLOAT)(((GLdouble*)src)[j]);
			break;
		}
		if(oglClientState.arrayState.vertexArraySize == 2)
			*(((FLOAT*&)buffer)++) = 0;
	}

	if(oglClientState.arrayState.normalArrayEnabled)
	{
		char* src = (char*)oglClientState.arrayState.normalArrayPointer + index * oglClientState.arrayState.normalArrayStride;
		switch(oglClientState.arrayState.normalArrayType)
		{
		case GL_BYTE:
			for(int j = 0; j < 3; j++)
				*(((FLOAT*&)buffer)++) = (FLOAT)(((GLbyte*)src)[j]);
			break;
		case GL_SHORT:
			for(int j = 0; j < 3; j++)
				*(((FLOAT*&)buffer)++) = (FLOAT)(((GLshort*)src)[j]);
			break;
		case GL_INT:
			for(int j = 0; j < 3; j++)
				*(((FLOAT*&)buffer)++) = (FLOAT)(((GLint*)src)[j]);
			break;
		case GL_FLOAT:
			for(int j = 0; j < 3; j++)
				*(((FLOAT*&)buffer)++) = (FLOAT)(((GLfloat*)src)[j]);
			break;
		case GL_DOUBLE:
			for(int j = 0; j < 3; j++)
				*(((FLOAT*&)buffer)++) = (FLOAT)(((GLdouble*)src)[j]);
			break;
		}
	}

	if(oglClientState.arrayState.colorArrayEnabled)
	{
		char* src = (char*)oglClientState.arrayState.colorArrayPointer + index * oglClientState.arrayState.colorArrayStride;
		switch(oglClientState.arrayState.colorArrayType)
		{
		case GL_BYTE:
			*(((BYTE*&)buffer)++) = (oglClientState.arrayState.colorArraySize == 3) ? 255 : oglclamptobyte(((GLbyte*)src)[3]);
			for(int j = 0; j < 3; j++)
				*(((BYTE*&)buffer)++) = oglclamptobyte(((GLbyte*)src)[j]);
			break;
		case GL_UNSIGNED_BYTE:
			*(((BYTE*&)buffer)++) = (oglClientState.arrayState.colorArraySize == 3) ? 255 : oglclamptobyte(((GLubyte*)src)[3]);
			for(int j = 0; j < 3; j++)
				*(((BYTE*&)buffer)++) = oglclamptobyte(((GLubyte*)src)[j]);
			break;
		case GL_SHORT:
			*(((BYTE*&)buffer)++) = (oglClientState.arrayState.colorArraySize == 3) ? 255 : oglclamptobyte(((GLshort*)src)[3]);
			for(int j = 0; j < 3; j++)
				*(((BYTE*&)buffer)++) = oglclamptobyte(((GLshort*)src)[j]);
			break;
		case GL_UNSIGNED_SHORT:
			*(((BYTE*&)buffer)++) = (oglClientState.arrayState.colorArraySize == 3) ? 255 : oglclamptobyte(((GLushort*)src)[3]);
			for(int j = 0; j < 3; j++)
				*(((BYTE*&)buffer)++) = oglclamptobyte(((GLushort*)src)[j]);
			break;
		case GL_INT:
			*(((BYTE*&)buffer)++) = (oglClientState.arrayState.colorArraySize == 3) ? 255 : oglclamptobyte(((GLint*)src)[3]);
			for(int j = 0; j < 3; j++)
				*(((BYTE*&)buffer)++) = oglclamptobyte(((GLint*)src)[j]);
			break;
		case GL_UNSIGNED_INT:
			*(((BYTE*&)buffer)++) = (oglClientState.arrayState.colorArraySize == 3) ? 255 : oglclamptobyte(((GLuint*)src)[3]);
			for(int j = 0; j < 3; j++)
				*(((BYTE*&)buffer)++) = oglclamptobyte(((GLuint*)src)[j]);
			break;
		case GL_FLOAT:
			*(((BYTE*&)buffer)++) = (oglClientState.arrayState.colorArraySize == 3) ? 255 : oglclamptobyte(((GLfloat*)src)[3]);
			for(int j = 0; j < 3; j++)
				*(((BYTE*&)buffer)++) = oglclamptobyte(((GLfloat*)src)[j]);
			break;
		case GL_DOUBLE:
			*(((BYTE*&)buffer)++) = (oglClientState.arrayState.colorArraySize == 3) ? 255 : oglclamptobyte(((GLdouble*)src)[3]);
			for(int j = 0; j < 3; j++)
				*(((BYTE*&)buffer)++) = oglclamptobyte(((GLdouble*)src)[j]);
			break;
		}
	}

	if(oglClientState.arrayState.texCoordArrayEnabled)
	{
		char* src = (char*)oglClientState.arrayState.texCoordArrayPointer + index * oglClientState.arrayState.texCoordArrayStride;
		switch(oglClientState.arrayState.texCoordArrayType)
		{
		case GL_SHORT:
			for(int j = 0; j < oglClientState.arrayState.texCoordArraySize; j++)
				*(((FLOAT*&)buffer)++) = (FLOAT)(((GLshort*)src)[j]);
			break;
		case GL_INT:
			for(int j = 0; j < oglClientState.arrayState.texCoordArraySize; j++)
				*(((FLOAT*&)buffer)++) = (FLOAT)(((GLint*)src)[j]);
			break;
		case GL_FLOAT:
			for(int j = 0; j < oglClientState.arrayState.texCoordArraySize; j++)
			{
				//debugprintf("vert[%d].uv[%d] = %g\n", index,j, (FLOAT)(((GLfloat*)src)[j]));
				*(((FLOAT*&)buffer)++) = (FLOAT)(((GLfloat*)src)[j]);
			}
			break;
		case GL_DOUBLE:
			for(int j = 0; j < oglClientState.arrayState.texCoordArraySize; j++)
				*(((FLOAT*&)buffer)++) = (FLOAT)(((GLdouble*)src)[j]);
			break;
		}
	}
}

static void OglGetD3dDrawInfo(GLenum mode, GLsizei count, DWORD& fvf, DWORD& elemSize, DWORD& primType, int& primCount, int& elemCount)
{
	elemSize = 0;
	fvf = OglGetD3dFVF(elemSize);
	primType;
	primCount;
	elemCount = count;
	switch(mode)
	{
	case GL_POINTS:
		primType = D3DPT_POINTLIST;
		primCount = elemCount;
		break;
	case GL_LINES:
		primType = D3DPT_LINELIST;
		primCount = elemCount >> 1;
		break;
	case GL_LINE_LOOP:
		primType = D3DPT_LINESTRIP;
		elemCount++;
		primCount = (elemCount < 3) ? (elemCount >> 1) : elemCount;
		break;
	case GL_LINE_STRIP:
		primType = D3DPT_LINESTRIP;
		primCount = (elemCount < 2) ? 0 : (elemCount-1);
		break;
	case GL_TRIANGLES:
		primType = D3DPT_TRIANGLELIST;
		primCount = elemCount / 3;
		break;
	case GL_TRIANGLE_STRIP:
	case GL_QUAD_STRIP:
		primType = D3DPT_TRIANGLESTRIP;
		primCount = (elemCount < 3) ? 0 : (elemCount-2);
		break;
	case GL_TRIANGLE_FAN:
	case GL_POLYGON:
		primType = D3DPT_TRIANGLEFAN;
		primCount = (elemCount < 3) ? 0 : (elemCount-2);
		break;
	case GL_QUADS:
		primType = D3DPT_TRIANGLELIST;
		elemCount = (elemCount * 3) >> 1;
		primCount = elemCount / 3;
		break;
	}
}

static void OglDrawToD3D(GLenum mode, GLsizei count, GLint first, GLenum type, const GLvoid* indices, bool renderNow, bool makeDisplayList)
{
	//if(mode == GL_QUADS)
	//	return;

	DWORD elemSize;
	DWORD fvf;
	DWORD primType;
	int primCount;
	int elemCount;
	OglGetD3dDrawInfo(mode,count, fvf,elemSize,primType,primCount,elemCount);

	//debugprintf("elemCount=%d, elemSize=%d=0x%X\n", elemCount, elemSize,elemSize);
	IDirect3DVertexBuffer8* vb;
	ogld3d8Device->CreateVertexBuffer(elemCount * elemSize, D3DUSAGE_WRITEONLY, fvf, D3DPOOL_MANAGED, &vb);

	if(!indices)
		type = 0;

	// fill vb
	BYTE* buffer;
	vb->Lock(0, 0, &buffer, 0);
	BYTE* bufferEnd = buffer + (elemCount * elemSize);
	if(mode != GL_QUADS)
	{
		switch(type)
		{
			default:
				for(int i = first; i < first+count; i++)
					FillVertexBufferFromIndex(buffer, i);
				break;
			case GL_UNSIGNED_BYTE:
				for(int i = 0; i < count; i++)
					FillVertexBufferFromIndex(buffer, (GLuint)(((GLubyte*)indices)[i]));
				break;
			case GL_UNSIGNED_SHORT:
				for(int i = 0; i < count; i++)
					FillVertexBufferFromIndex(buffer, (GLuint)(((GLushort*)indices)[i]));
				break;
			case GL_UNSIGNED_INT:
				for(int i = 0; i < count; i++)
					FillVertexBufferFromIndex(buffer, (GLuint)(((GLuint*)indices)[i]));
				break;
		}
		if(mode == GL_LINE_LOOP)
		{
			// readd first vert
			int index;
			switch(type)
			{
				default: index = first; break;
				case GL_UNSIGNED_BYTE: index = (GLuint)(((GLubyte*)indices)[0]); break;
				case GL_UNSIGNED_SHORT: index = (GLuint)(((GLushort*)indices)[0]); break;
				case GL_UNSIGNED_INT: index = (GLuint)(((GLuint*)indices)[0]); break;
			}
			FillVertexBufferFromIndex(buffer, index);
		}
	}
	else // mode == GL_QUADS
	{
		int quadFirstVertIndex = -1;
		for(int i = 0; i < count; i++)
		{
			int index;
			switch(type)
			{
				default: index = first + i; break;
				case GL_UNSIGNED_BYTE: index = (GLuint)(((GLubyte*)indices)[i]); break;
				case GL_UNSIGNED_SHORT: index = (GLuint)(((GLushort*)indices)[i]); break;
				case GL_UNSIGNED_INT: index = (GLuint)(((GLuint*)indices)[i]); break;
			}
			switch(i&3)
			{
			case 0:
				quadFirstVertIndex = index;
				//nobreak
			case 1:
				FillVertexBufferFromIndex(buffer, index);
				break;
			case 2:
				FillVertexBufferFromIndex(buffer, index);
				FillVertexBufferFromIndex(buffer, index);
				break;
			case 3:
				FillVertexBufferFromIndex(buffer, index);
				FillVertexBufferFromIndex(buffer, quadFirstVertIndex);
				break;
			}
		}
	}
	vb->Unlock();

	if(renderNow)
		OglD3dDrawPrimitive(fvf, elemSize, primCount, primType, (void*)vb);

	if(makeDisplayList)
		OGLPUSHDISPLAYLISTENTRY_5ARG(idd3dDrawPrimitive, dword,fvf, dword,elemSize, dword,primCount, dword,primType, ptr,(void*)vb);
	else
		vb->Release();
}
HOOKFUNC void GLAPI MyglDrawArrays (GLenum mode, GLint first, GLsizei count)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(count < 0)
		OGLRETURNERROR(GL_INVALID_VALUE);
	if(oglBeganMode != GL_UNSTARTED)
		OGLRETURNERROR(GL_INVALID_OPERATION);
	if(!ShouldSkipDrawing(false, true))
		OglDrawToD3D(mode, count, first, 0,0, oglAllowExecuteCommands, oglMakingDisplayList!=0);
}
HOOKFUNC void GLAPI MyglDrawElements (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
	debuglog(LCF_OGL, __FUNCTION__ "(mode=0x%X, count=%d, type=0x%X, indices=0x%X) called.\n", mode,count,type,indices);
	//GLFUNCBOILERPLATE;
	if(count < 0)
		OGLRETURNERROR(GL_INVALID_VALUE);
	if(type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT)
		OGLRETURNERROR(GL_INVALID_ENUM);
	if(oglBeganMode != GL_UNSTARTED)
		OGLRETURNERROR(GL_INVALID_OPERATION);
	if(!ShouldSkipDrawing(false, true))
		OglDrawToD3D(mode, count, 0, type,indices, oglAllowExecuteCommands, oglMakingDisplayList!=0);
}
HOOKFUNC void GLAPI MyglEdgeFlagPointer (GLsizei stride, const GLvoid *pointer)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	//oglClientState.arrayState.edgeFlagArrayStride = stride;
	//oglClientState.arrayState.edgeFlagArrayPointer = pointer;
	//oglClientState.arrayState.RecalculateSizes();
}
static const int GL_VERTEX_ARRAY = 0x8074;
static const int GL_NORMAL_ARRAY = 0x8075;
static const int GL_COLOR_ARRAY = 0x8076;
static const int GL_INDEX_ARRAY = 0x8077;
static const int GL_TEXTURE_COORD_ARRAY = 0x8078;
static const int GL_EDGE_FLAG_ARRAY = 0x8079;
HOOKFUNC void GLAPI MyglEnableClientState (GLenum array)
{
	debuglog(LCF_OGL, __FUNCTION__ "(0x%X) called.\n", array);
	//GLFUNCBOILERPLATE;
	switch(array)
	{
	case GL_VERTEX_ARRAY: oglClientState.arrayState.vertexArrayEnabled = true; break;
	case GL_NORMAL_ARRAY: oglClientState.arrayState.normalArrayEnabled = true; break;
	case GL_COLOR_ARRAY: oglClientState.arrayState.colorArrayEnabled = true; break;
	case GL_TEXTURE_COORD_ARRAY: oglClientState.arrayState.texCoordArrayEnabled = true; break;
	//case GL_INDEX_ARRAY: oglClientState.arrayState.indexArrayEnabled = true; break;
	//case GL_EDGE_FLAG_ARRAY: oglClientState.arrayState.edgeFlagArrayEnabled = true; break;
	}
	oglClientState.arrayState.RecalculateSizes();
}
HOOKFUNC void GLAPI MyglDisableClientState (GLenum array)
{
	debuglog(LCF_OGL, __FUNCTION__ "(0x%X) called.\n", array);
	//GLFUNCBOILERPLATE;
	switch(array)
	{
	case GL_VERTEX_ARRAY: oglClientState.arrayState.vertexArrayEnabled = false; break;
	case GL_NORMAL_ARRAY: oglClientState.arrayState.normalArrayEnabled = false; break;
	case GL_COLOR_ARRAY: oglClientState.arrayState.colorArrayEnabled = false; break;
	case GL_TEXTURE_COORD_ARRAY: oglClientState.arrayState.texCoordArrayEnabled = false; break;
	//case GL_INDEX_ARRAY: oglClientState.arrayState.indexArrayEnabled = false; break;
	//case GL_EDGE_FLAG_ARRAY: oglClientState.arrayState.edgeFlagArrayEnabled = false; break;
	}
	oglClientState.arrayState.RecalculateSizes();
}
HOOKFUNC void GLAPI MyglGetPointerv (GLenum pname, GLvoid* *params)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
}
HOOKFUNC void GLAPI MyglIndexPointer (GLenum type, GLsizei stride, const GLvoid *pointer)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	//oglClientState.arrayState.indexArrayType = type;
	//oglClientState.arrayState.indexArrayStride = stride;
	//oglClientState.arrayState.indexArrayPointer = pointer;
	//oglClientState.arrayState.RecalculateSizes();
}
HOOKFUNC void GLAPI MyglIndexub (GLubyte c)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}
HOOKFUNC void GLAPI MyglIndexubv (const GLubyte *c)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}
HOOKFUNC void GLAPI MyglInterleavedArrays (GLenum format, GLsizei stride, const GLvoid *pointer)
{
	debuglog(LCF_OGL, __FUNCTION__ "(format=0x%X, stride=0x%X, pointer=0x%X) called.\n", format,stride,pointer);
	//GLFUNCBOILERPLATE;
	
	static const int GL_V2F = 0x2A20;
	static const int GL_V3F = 0x2A21;
	static const int GL_C4UB_V2F = 0x2A22;
	static const int GL_C4UB_V3F = 0x2A23;
	static const int GL_C3F_V3F = 0x2A24;
	static const int GL_N3F_V3F = 0x2A25;
	static const int GL_C4F_N3F_V3F = 0x2A26;
	static const int GL_T2F_V3F = 0x2A27;
	static const int GL_T4F_V4F = 0x2A28;
	static const int GL_T2F_C4UB_V3F = 0x2A29;
	static const int GL_T2F_C3F_V3F = 0x2A2A;
	static const int GL_T2F_N3F_V3F = 0x2A2B;
	static const int GL_T2F_C4F_N3F_V3F = 0x2A2C;
	static const int GL_T4F_C4F_N3F_V4F = 0x2A2D;
	
	if(stride < 0)
		OGLRETURNERROR(GL_INVALID_VALUE);
	if(format < GL_V2F || format > GL_T4F_C4F_N3F_V4F)
		OGLRETURNERROR(GL_INVALID_ENUM);
	
	oglClientState.arrayState.texCoordArrayEnabled = (format==GL_T2F_V3F||format==GL_T4F_V4F||format==GL_T2F_C4UB_V3F||format==GL_T2F_C3F_V3F||format==GL_T2F_N3F_V3F||format==GL_T2F_C4F_N3F_V3F||format==GL_T4F_C4F_N3F_V4F);
	oglClientState.arrayState.colorArrayEnabled = (format==GL_C4UB_V2F||format==GL_C4UB_V3F||format==GL_C3F_V3F||format==GL_C4F_N3F_V3F||format==GL_C4F_N3F_V3F||format==GL_T2F_C4UB_V3F||format==GL_T2F_C3F_V3F||format==GL_T2F_C4F_N3F_V3F||format==GL_T4F_C4F_N3F_V4F);
	oglClientState.arrayState.normalArrayEnabled = (format==GL_N3F_V3F||format==GL_C4F_N3F_V3F||format==GL_T2F_N3F_V3F||format==GL_T2F_C4F_N3F_V3F||format==GL_T4F_C4F_N3F_V4F);
	oglClientState.arrayState.vertexArrayEnabled = true;
	//oglClientState.arrayState.edgeFlagArrayEnabled = false;
	//oglClientState.arrayState.indexArrayEnabled = false;

	if(oglClientState.arrayState.texCoordArrayEnabled)
	{
		oglClientState.arrayState.texCoordArraySize = (format==GL_T4F_V4F||format==GL_T4F_C4F_N3F_V4F) ? 4 : 2;
		oglClientState.arrayState.texCoordArrayType = GL_FLOAT;
		oglClientState.arrayState.texCoordArrayStride = 0;
	}
	if(oglClientState.arrayState.colorArrayEnabled)
	{
		oglClientState.arrayState.colorArraySize = (format==GL_C3F_V3F||format==GL_T2F_C3F_V3F) ? 3 : 4;
		oglClientState.arrayState.colorArrayType = (format==GL_C4UB_V2F||format==GL_C4UB_V3F||format==GL_T2F_C4UB_V3F) ? GL_UNSIGNED_BYTE : GL_FLOAT;
		oglClientState.arrayState.colorArrayStride = 0;
	}
	if(oglClientState.arrayState.normalArrayEnabled)
	{
		oglClientState.arrayState.normalArrayType = GL_FLOAT;
		oglClientState.arrayState.normalArrayStride = 0;
	}
	if(oglClientState.arrayState.vertexArrayEnabled)
	{
		oglClientState.arrayState.vertexArraySize = (format==GL_T4F_V4F||format==GL_T4F_C4F_N3F_V4F) ? 4 : (format==GL_V2F||format==GL_C4UB_V2F) ? 2 : 3;
		oglClientState.arrayState.vertexArrayType = GL_FLOAT;
		oglClientState.arrayState.vertexArrayStride = 0;
	}

	oglClientState.arrayState.RecalculateSizes();

	if(stride == 0)
		stride = oglClientState.arrayState.aggregateElementSize;

	int offset = 0;
	oglClientState.arrayState.texCoordArrayPointer = (const GLvoid*)(&((char*)pointer)[offset]);
	offset += oglClientState.arrayState.texCoordElementSize;
	oglClientState.arrayState.colorArrayPointer = (const GLvoid*)(&((char*)pointer)[offset]);
	offset += oglClientState.arrayState.colorElementSize;
	oglClientState.arrayState.normalArrayPointer = (const GLvoid*)(&((char*)pointer)[offset]);
	offset += oglClientState.arrayState.normalElementSize;
	oglClientState.arrayState.vertexArrayPointer = (const GLvoid*)(&((char*)pointer)[offset]);
	//offset += oglClientState.arrayState.vertexElementSize;

	//debugprintf("oglClientState.arrayState.texCoordArrayStride = %d\n", oglClientState.arrayState.texCoordArrayStride);
	//debugprintf("oglClientState.arrayState.texCoordArrayPointer = 0x%x\n", oglClientState.arrayState.texCoordArrayPointer);
	//debugprintf("oglClientState.arrayState.colorArrayStride = %d\n", oglClientState.arrayState.colorArrayStride);
	//debugprintf("oglClientState.arrayState.colorArrayPointer = 0x%x\n", oglClientState.arrayState.colorArrayPointer);
	//debugprintf("oglClientState.arrayState.normalArrayStride = %d\n", oglClientState.arrayState.normalArrayStride);
	//debugprintf("oglClientState.arrayState.normalArrayPointer = 0x%x\n", oglClientState.arrayState.normalArrayPointer);
	//debugprintf("oglClientState.arrayState.vertexArrayStride = %d\n", oglClientState.arrayState.vertexArrayStride);
	//debugprintf("oglClientState.arrayState.vertexArrayPointer = 0x%x\n", oglClientState.arrayState.vertexArrayPointer);
	//debugprintf("stride = %d\n", stride);

	oglClientState.arrayState.texCoordArrayStride = stride;
	oglClientState.arrayState.colorArrayStride = stride;
	oglClientState.arrayState.normalArrayStride = stride;
	oglClientState.arrayState.vertexArrayStride = stride;
}
HOOKFUNC GLboolean GLAPI MyglIsTexture (GLuint texture)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	return 0; // NYI
}
HOOKFUNC void GLAPI MyglNormalPointer (GLenum type, GLsizei stride, const GLvoid *pointer)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(stride < 0)
		OGLRETURNERROR(GL_INVALID_VALUE);
	if(type != GL_BYTE && type != GL_SHORT && type != GL_INT && type != GL_FLOAT && type != GL_DOUBLE)
		OGLRETURNERROR(GL_INVALID_ENUM);
	oglClientState.arrayState.normalArrayType = type;
	oglClientState.arrayState.normalArrayStride = stride;
	oglClientState.arrayState.normalArrayPointer = pointer;
	oglClientState.arrayState.RecalculateSizes();
}
HOOKFUNC void GLAPI MyglPolygonOffset (GLfloat factor, GLfloat units)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}
HOOKFUNC void GLAPI MyglPushClientAttrib (GLbitfield mask)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	oglClientStateStack.push_back(oglClientState);
	oglClientStateStack.back().mask = mask;
}
HOOKFUNC void GLAPI MyglPopClientAttrib (void)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglClientStateStack.empty())
		OGLRETURNERROR(GL_STACK_UNDERFLOW);
	GLbitfield mask = oglClientStateStack.back().mask;
	static const int GL_CLIENT_PIXEL_STORE_BIT = 0x1;
	static const int GL_CLIENT_VERTEX_ARRAY_BIT = 0x2;
	if(mask & GL_CLIENT_PIXEL_STORE_BIT)
		oglClientState.pixelState = oglClientStateStack.back().pixelState;
	if(mask & GL_CLIENT_VERTEX_ARRAY_BIT)
		oglClientState.arrayState = oglClientStateStack.back().arrayState;
	oglClientStateStack.pop_back();
}
HOOKFUNC void GLAPI MyglPrioritizeTextures (GLsizei n, const GLuint *textures, const GLclampf *priorities)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}
HOOKFUNC void GLAPI MyglTexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(size < 1 || size > 4 || stride < 0)
		OGLRETURNERROR(GL_INVALID_VALUE);
	if(type != GL_SHORT && type != GL_INT && type != GL_FLOAT && type != GL_DOUBLE)
		OGLRETURNERROR(GL_INVALID_ENUM);
	oglClientState.arrayState.texCoordArraySize = size;
	oglClientState.arrayState.texCoordArrayType = type;
	oglClientState.arrayState.texCoordArrayStride = stride;
	oglClientState.arrayState.texCoordArrayPointer = pointer;
	oglClientState.arrayState.RecalculateSizes();
}
HOOKFUNC void GLAPI MyglTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}
HOOKFUNC void GLAPI MyglTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(oglAllowExecuteCommands)
	{
	}
}
HOOKFUNC void GLAPI MyglVertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	debuglog(LCF_OGL, __FUNCTION__ " called.\n");
	//GLFUNCBOILERPLATE;
	if(size < 2 || size > 4 || stride < 0)
		OGLRETURNERROR(GL_INVALID_VALUE);
	if(type != GL_SHORT && type != GL_INT && type != GL_FLOAT && type != GL_DOUBLE)
		OGLRETURNERROR(GL_INVALID_ENUM);
	oglClientState.arrayState.vertexArraySize = size;
	oglClientState.arrayState.vertexArrayType = type;
	oglClientState.arrayState.vertexArrayStride = stride;
	oglClientState.arrayState.vertexArrayPointer = pointer;
	oglClientState.arrayState.RecalculateSizes();
}



// internal opengl

HOOKFUNC void GLAPI MyglDebugEntry(DWORD arg1, DWORD arg2) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
}

HOOKFUNC void GLAPI MyGlmfBeginGlsBlock(DWORD arg1) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
}

HOOKFUNC void GLAPI MyGlmfEndGlsBlock(DWORD arg1) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
}

HOOKFUNC void GLAPI MyGlmfCloseMetaFile(DWORD arg1) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
}

HOOKFUNC void GLAPI MyGlmfEndPlayback(DWORD arg1) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
}

HOOKFUNC void GLAPI MyGlmfInitPlayback(DWORD arg1, DWORD arg2, DWORD arg3) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
}

HOOKFUNC void GLAPI MyGlmfPlayGlsRecord(DWORD arg1, DWORD arg2, DWORD arg3, DWORD arg4) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
}

HOOKFUNC int WINAPI MywglChoosePixelFormat(HDC hdc, const PIXELFORMATDESCRIPTOR* ppfd) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	return wglChoosePixelFormat(hdc, ppfd);
}

HOOKFUNC BOOL WINAPI MywglCopyContext(HGLRC context1, HGLRC context2, UINT i) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//return wglCopyContext(context1,context2,i);
	return 0; // NYI
}

HOOKFUNC HGLRC WINAPI MywglCreateContext(HDC hdc) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//return wglCreateContext(hdc);

	HWND hwnd = WindowFromDC(hdc);
	if(!ogld3d8)
		InitOGLD3D(hwnd);
	else
		InitDevice(false, hwnd);
	oglCurrentHDC = hdc;

	return (HGLRC)ogld3d8;
}

HOOKFUNC HGLRC WINAPI MywglCreateLayerContext(HDC hdc, int i) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//return wglCreateLayerContext(hdc, i);
//	//GLFUNCBOILERPLATE;

	HWND hwnd = WindowFromDC(hdc);
	if(!ogld3d8)
		InitOGLD3D(hwnd);
	else if(hdc != oglCurrentHDC)
		InitDevice(false, hwnd);
	oglCurrentHDC = hdc;

	return (HGLRC)ogld3d8;
}

HOOKFUNC BOOL WINAPI MywglDeleteContext(HGLRC context) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//return wglDeleteContext(context);
//	return 0;
	if(context == (HGLRC)ogld3d8)
	{
		if(ogld3d8Device)
		{
			ogld3d8Device->Release();
			ogld3d8Device = NULL;
		}
		if(ogld3d8)
		{
			ogld3d8->Release();
			ogld3d8 = NULL;
		}
		oglCurrentHDC = (HDC)INVALID_HANDLE_VALUE;
		oglAllowExecuteCommands = false;
		oglMakingDisplayList = 0;
	}
	return TRUE;
}

HOOKFUNC BOOL WINAPI MywglDescribeLayerPlane(HDC hdc, int i1, int i2, UINT i3, LPLAYERPLANEDESCRIPTOR pd) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	// NYI
	return wglDescribeLayerPlane(hdc,i1,i2,i3,pd);
}

HOOKFUNC int WINAPI MywglDescribePixelFormat(HDC hdc, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR ppfd) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	// NYI
	return wglDescribePixelFormat(hdc, iPixelFormat, nBytes, ppfd);
}

HOOKFUNC HGLRC WINAPI MywglGetCurrentContext(VOID) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//return wglGetCurrentContext();
	return (HGLRC)ogld3d8;
}

HOOKFUNC HDC WINAPI MywglGetCurrentDC(VOID) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	//return wglGetCurrentDC();
	return oglCurrentHDC;
}

HOOKFUNC PROC WINAPI MywglGetDefaultProcAddress(LPCSTR name) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ "(\"%s\") called.\n", name);
	//return wglGetDefaultProcAddress(name);
	return NULL; // extensions are NYI
}

HOOKFUNC int WINAPI MywglGetLayerPaletteEntries(HDC hdc, int i1, int i2, int i3, COLORREF* cr) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	// NYI
	return wglGetLayerPaletteEntries(hdc,i1,i2,i3,cr);
}

HOOKFUNC int WINAPI MywglGetPixelFormat(HDC hdc) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	return wglGetPixelFormat(hdc);
}

HOOKFUNC PROC WINAPI MywglGetProcAddress(LPCSTR name) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ "(\"%s\") called.\n", name);
	//return wglGetProcAddress(name);
	return NULL; // extensions are NYI
}

HOOKFUNC BOOL WINAPI MywglMakeCurrent(HDC hdc, HGLRC context) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ "(hdc=0x%X, context=0x%X) called.\n", hdc, context);
	//return wglMakeCurrent(hdc,context);
	if(context)
	{
		HWND hwnd = WindowFromDC(hdc);
		if(!ogld3d8)
			InitOGLD3D(hwnd);
		else if(hdc != oglCurrentHDC)
			InitDevice(false, hwnd);

		oglCurrentHDC = hdc;
		//oglCurrentHDC = CreateCompatibleDC(hdc);
	}
	else if(ogld3d8) // TODO: should be in tls
	{
		if(!oglCurrentHDC)
			oglCurrentHDC = (HDC)INVALID_HANDLE_VALUE;
		if(oglCurrentHDC != (HDC)INVALID_HANDLE_VALUE)
		{
			// FIXME
			CloseHandle(oglCurrentHDC); // exception?
			//DeleteDC(oglCurrentHDC); // crashes
			oglCurrentHDC = (HDC)INVALID_HANDLE_VALUE;
		}
		if(ogld3d8Device)
		{
			ogld3d8Device->Release();
			ogld3d8Device = NULL;
		}
		oglAllowExecuteCommands = false;
		oglMakingDisplayList = 0;
	}
	return TRUE;
}

HOOKFUNC BOOL WINAPI MywglRealizeLayerPalette(HDC hdc, int i, BOOL b) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	return TRUE;
//	return wglRealizeLayerPalette(hdc,i,b);
}

HOOKFUNC int WINAPI MywglSetLayerPaletteEntries(HDC hdc, int i1, int i2, int i3, CONST COLORREF* cr) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	return i3;
//	return wglSetLayerPaletteEntries(hdc,i1,i2,i3,cr);
}

HOOKFUNC BOOL WINAPI MySetPixelFormat(HDC hdc, int format, CONST PIXELFORMATDESCRIPTOR * pfd);
HOOKFUNC BOOL WINAPI MywglSetPixelFormat(HDC hdc, int format, CONST PIXELFORMATDESCRIPTOR * pfd)
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	extern int depth_SetPixelFormat;
	if(depth_SetPixelFormat)
		return wglSetPixelFormat(hdc, format, pfd);
	depth_SetPixelFormat++;
	BOOL rv = MySetPixelFormat(hdc, format, pfd);
	depth_SetPixelFormat--;
	return rv;
}

HOOKFUNC BOOL WINAPI MywglShareLists(HGLRC context1, HGLRC context2) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
//	return wglShareLists(context1, context2);
	return TRUE;
}

HOOKFUNC BOOL MySwapBuffers(HDC hdc);
HOOKFUNC BOOL WINAPI MywglSwapBuffers(HDC hdc) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
//	return TRUE;
	extern int depth_SwapBuffers;
	if(depth_SwapBuffers)
		return wglSwapBuffers(hdc);
	depth_SwapBuffers++;
	BOOL rv = MySwapBuffers(hdc);
	depth_SwapBuffers--;
	return rv;
}

HOOKFUNC BOOL WINAPI MywglSwapLayerBuffers(HDC hdc, UINT i) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
//	return TRUE;
	extern int depth_SwapBuffers;
	if(depth_SwapBuffers || !(i&1))
		return wglSwapLayerBuffers(hdc,i);
	depth_SwapBuffers++;
	BOOL rv = MySwapBuffers(hdc);
	depth_SwapBuffers--;
	return rv;
}

HOOKFUNC DWORD WINAPI MywglSwapMultipleBuffers(UINT i, CONST WGLSWAP * s) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
//	return 0;
	extern int depth_SwapBuffers;
	if(depth_SwapBuffers)
		return wglSwapMultipleBuffers(i,s);
	depth_SwapBuffers++;
	BOOL rv = (i > 0) ? MySwapBuffers(s[0].hdc) : FALSE;
	//if(i > 1)
	//	wglSwapMultipleBuffers(i-1,s+1);
	depth_SwapBuffers--;
	return rv;
}

HOOKFUNC BOOL WINAPI MywglUseFontBitmapsA(HDC hdc, DWORD d1, DWORD d2, DWORD d3) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	return TRUE; // NYI
//	return wglUseFontBitmapsA(hdc,d1,d2,d3);
}

HOOKFUNC BOOL WINAPI MywglUseFontBitmapsW(HDC hdc, DWORD d1, DWORD d2, DWORD d3) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	return TRUE; // NYI
//	return wglUseFontBitmapsW(hdc,d1,d2,d3);
}

HOOKFUNC BOOL WINAPI MywglUseFontOutlinesA(HDC hdc, DWORD d1, DWORD d2, DWORD d3, FLOAT f1, FLOAT f2, int i, LPGLYPHMETRICSFLOAT lpgm) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	return TRUE; // NYI
//	return wglUseFontOutlinesA(hdc,d1,d2,d3,f1,f2,i,lpgm);
}

HOOKFUNC BOOL WINAPI MywglUseFontOutlinesW(HDC hdc, DWORD d1, DWORD d2, DWORD d3, FLOAT f1, FLOAT f2, int i, LPGLYPHMETRICSFLOAT lpgm) 
{
	debuglog(LCF_TODO|LCF_OGL, __FUNCTION__ " called.\n");
	return TRUE; // NYI
//	return wglUseFontOutlinesA(hdc,d1,d2,d3,f1,f2,i,lpgm);
}


void OpenGLDisplayListEntry::Call()
{
	switch(id)
	{
		case idglBindTexture: MyglBindTexture(args[0].glenum, args[1].gluint); break;
		case idglLineWidth: MyglLineWidth(args[0].glfloat); break;
		case idglAccum: MyglAccum(args[0].glenum, args[1].glfloat); break;
		case idglAlphaFunc: MyglAlphaFunc(args[0].glenum, args[1].glclampf); break;
		case idglBegin: MyglBegin(args[0].glenum); break;
		case idglBitmap: MyglBitmap(args[0].glsizei, args[1].glsizei, args[2].glfloat, args[3].glfloat, args[4].glfloat, args[5].glfloat, (const GLubyte *)buffer); break;
		case idglBlendFunc: MyglBlendFunc(args[0].glenum, args[1].glenum); break;
		case idglCallList: MyglCallList(args[0].gluint); break;
		case idglCallLists: MyglCallLists(args[0].glsizei, args[1].glenum, (const GLvoid *)buffer); break;
		case idglClear: MyglClear(args[0].glbitfield); break;
		case idglClearAccum: MyglClearAccum(args[0].glfloat, args[1].glfloat, args[2].glfloat, args[3].glfloat); break;
		case idglClearColor: MyglClearColor(args[0].glclampf, args[1].glclampf, args[2].glclampf, args[3].glclampf); break;
		case idglClearDepth: MyglClearDepth(args[0].glclampd); break;
		case idglClearIndex: MyglClearIndex(args[0].glfloat); break;
		case idglClearStencil: MyglClearStencil(args[0].glint); break;
		case idglClipPlane: MyglClipPlane(args[0].glenum, (const GLdouble *)buffer); break;
		case idglColor3b: MyglColor3b(args[0].glbyte, args[1].glbyte, args[2].glbyte); break;
		case idglColor3bv: MyglColor3bv((const GLbyte*)buffer); break;
		case idglColor3d: MyglColor3d(args[0].gldouble, args[1].gldouble, args[2].gldouble); break;
		case idglColor3dv: MyglColor3dv((const GLdouble*)buffer); break;
		case idglColor3f: MyglColor3f(args[0].glfloat, args[1].glfloat, args[2].glfloat); break;
		case idglColor3fv: MyglColor3fv((const GLfloat*)buffer); break;
		case idglColor3i: MyglColor3i(args[0].glint, args[1].glint, args[2].glint); break;
		case idglColor3iv: MyglColor3iv((const GLint*)buffer); break;
		case idglColor3s: MyglColor3s(args[0].glshort, args[1].glshort, args[2].glshort); break;
		case idglColor3sv: MyglColor3sv((const GLshort*)buffer); break;
		case idglColor3ub: MyglColor3ub(args[0].glubyte, args[1].glubyte, args[2].glubyte); break;
		case idglColor3ubv: MyglColor3ubv((const GLubyte*)buffer); break;
		case idglColor3ui: MyglColor3ui(args[0].gluint, args[1].gluint, args[2].gluint); break;
		case idglColor3uiv: MyglColor3uiv((const GLuint*)buffer); break;
		case idglColor3us: MyglColor3us(args[0].glushort, args[1].glushort, args[2].glushort); break;
		case idglColor3usv: MyglColor3usv((const GLushort*)buffer); break;
		case idglColor4b: MyglColor4b(args[0].glbyte, args[1].glbyte, args[2].glbyte, args[3].glbyte); break;
		case idglColor4bv: MyglColor4bv((const GLbyte*)buffer); break;
		case idglColor4d: MyglColor4d(args[0].gldouble, args[1].gldouble, args[2].gldouble, args[3].gldouble); break;
		case idglColor4dv: MyglColor4dv((const GLdouble*)buffer); break;
		case idglColor4f: MyglColor4f(args[0].glfloat, args[1].glfloat, args[2].glfloat, args[3].glfloat); break;
		case idglColor4fv: MyglColor4fv((const GLfloat*)buffer); break;
		case idglColor4i: MyglColor4i(args[0].glint, args[1].glint, args[2].glint, args[3].glint); break;
		case idglColor4iv: MyglColor4iv((const GLint*)buffer); break;
		case idglColor4s: MyglColor4s(args[0].glshort, args[1].glshort, args[2].glshort, args[3].glshort); break;
		case idglColor4sv: MyglColor4sv((const GLshort*)buffer); break;
		case idglColor4ub: MyglColor4ub(args[0].glubyte, args[1].glubyte, args[2].glubyte, args[3].glubyte); break;
		case idglColor4ubv: MyglColor4ubv((const GLubyte*)buffer); break;
		case idglColor4ui: MyglColor4ui(args[0].gluint, args[1].gluint, args[2].gluint, args[3].gluint); break;
		case idglColor4uiv: MyglColor4uiv((const GLuint*)buffer); break;
		case idglColor4us: MyglColor4us(args[0].glushort, args[1].glushort, args[2].glushort, args[3].glushort); break;
		case idglColor4usv: MyglColor4usv((const GLushort*)buffer); break;
		case idglColorMask: MyglColorMask(args[0].glboolean, args[1].glboolean, args[2].glboolean, args[3].glboolean); break;
			//NYI
		//case idglColorMaterial: MyglColorMaterial(); break;
		//case idglCopyPixels: MyglCopyPixels(); break;
		case idglCullFace: MyglCullFace(args[0].glenum); break;
		case idglDepthFunc: MyglDepthFunc(args[0].glenum); break;
		case idglDepthMask: MyglDepthMask(args[0].glboolean); break;
		case idglDepthRange: MyglDepthRange(args[0].glclampd, args[1].glclampd); break;
		//case idglDrawBuffer: MyglDrawBuffer(); break;
		//case idglDrawPixels: MyglDrawPixels(); break;
		//case idglEdgeFlag: MyglEdgeFlag(); break;
		//case idglEdgeFlagv: MyglEdgeFlagv(); break;
		case idglEnable: MyglEnable(args[0].glenum); break;
		case idglDisable: MyglDisable(args[0].glenum); break;
		case idglEnd: MyglEnd(); break;
		//case idglEvalCoord1d: MyglEvalCoord1d(); break;
		//case idglEvalCoord1dv: MyglEvalCoord1dv(); break;
		//case idglEvalCoord1f: MyglEvalCoord1f(); break;
		//case idglEvalCoord1fv: MyglEvalCoord1fv(); break;
		//case idglEvalCoord2d: MyglEvalCoord2d(); break;
		//case idglEvalCoord2dv: MyglEvalCoord2dv(); break;
		//case idglEvalCoord2f: MyglEvalCoord2f(); break;
		//case idglEvalCoord2fv: MyglEvalCoord2fv(); break;
		//case idglEvalMesh1: MyglEvalMesh1(); break;
		//case idglEvalMesh2: MyglEvalMesh2(); break;
		//case idglEvalPoint1: MyglEvalPoint1(); break;
		//case idglEvalPoint2: MyglEvalPoint2(); break;
		//case idglFogf: MyglFogf(); break;
		//case idglFogfv: MyglFogfv(); break;
		//case idglFogi: MyglFogi(); break;
		//case idglFogiv: MyglFogiv(); break;
		case idglFrontFace: MyglFrontFace(args[0].glenum); break;
		case idglFrustum: MyglFrustum(args[0].gldouble,args[1].gldouble,args[2].gldouble,args[3].gldouble,args[4].gldouble,args[5].gldouble); break;
		case idglHint: MyglHint(args[0].glenum, args[1].glenum); break;
		//case idglIndexMask: MyglIndexMask(); break;
		//case idglIndexd: MyglIndexd(); break;
		//case idglIndexdv: MyglIndexdv(); break;
		//case idglIndexf: MyglIndexf(); break;
		//case idglIndexfv: MyglIndexfv(); break;
		//case idglIndexi: MyglIndexi(); break;
		//case idglIndexiv: MyglIndexiv(); break;
		//case idglIndexs: MyglIndexs(); break;
		//case idglIndexsv: MyglIndexsv(); break;
		//case idglInitNames: MyglInitNames(); break;
		//case idglLightModelf: MyglLightModelf(); break;
		//case idglLightModelfv: MyglLightModelfv(); break;
		//case idglLightModeli: MyglLightModeli(); break;
		//case idglLightModeliv: MyglLightModeliv(); break;
		//case idglLightf: MyglLightf(); break;
		//case idglLightfv: MyglLightfv(); break;
		//case idglLighti: MyglLighti(); break;
		//case idglLightiv: MyglLightiv(); break;
		//case idglLineStipple: MyglLineStipple(); break;
		//case idglLineWidth: MyglLineWidth(); break;
		//case idglListBase: MyglListBase(); break;
		case idglLoadIdentity: MyglLoadIdentity(); break;
		//case idglLoadMatrixd: MyglLoadMatrixd(); break;
		//case idglLoadMatrixf: MyglLoadMatrixf(); break;
		//case idglLoadName: MyglLoadName(); break;
		case idglLogicOp: MyglLogicOp(args[0].glenum); break;
		//case idglMap1d: MyglMap1d(); break;
		//case idglMap1f: MyglMap1f(); break;
		//case idglMap2d: MyglMap2d(); break;
		//case idglMap2f: MyglMap2f(); break;
		//case idglMapGrid1d: MyglMapGrid1d(); break;
		//case idglMapGrid1f: MyglMapGrid1f(); break;
		//case idglMapGrid2d: MyglMapGrid2d(); break;
		//case idglMapGrid2f: MyglMapGrid2f(); break;
		//case idglMaterialf: MyglMaterialf(); break;
		//case idglMaterialfv: MyglMaterialfv(); break;
		//case idglMateriali: MyglMateriali(); break;
		//case idglMaterialiv: MyglMaterialiv(); break;
		case idglMatrixMode: MyglMatrixMode(args[0].glenum); break;
		case idglMultMatrixd: MyglMultMatrixd((const GLdouble*)buffer); break;
		case idglMultMatrixf: MyglMultMatrixf((const GLfloat*)buffer); break;
		case idglNormal3b: MyglNormal3b(args[0].glbyte, args[1].glbyte, args[2].glbyte); break;
		case idglNormal3bv: MyglNormal3bv((const GLbyte*)buffer); break;
		case idglNormal3d: MyglNormal3d(args[0].gldouble, args[1].gldouble, args[2].gldouble); break;
		case idglNormal3dv: MyglNormal3dv((const GLdouble*)buffer); break;
		case idglNormal3f: MyglNormal3f(args[0].glfloat, args[1].glfloat, args[2].glfloat); break;
		case idglNormal3fv: MyglNormal3fv((const GLfloat*)buffer); break;
		case idglNormal3i: MyglNormal3i(args[0].glint, args[1].glint, args[2].glint); break;
		case idglNormal3iv: MyglNormal3iv((const GLint*)buffer); break;
		case idglNormal3s: MyglNormal3s(args[0].glshort, args[1].glshort, args[2].glshort); break;
		case idglNormal3sv: MyglNormal3sv((const GLshort*)buffer); break;
		case idglOrtho: MyglOrtho(args[0].gldouble,args[1].gldouble,args[2].gldouble,args[3].gldouble,args[4].gldouble,args[5].gldouble); break;
		//case idglPassThrough: MyglPassThrough(); break;
		//case idglPixelMapfv: MyglPixelMapfv(); break;
		//case idglPixelMapuiv: MyglPixelMapuiv(); break;
		//case idglPixelMapusv: MyglPixelMapusv(); break;
		//case idglPixelTransferf: MyglPixelTransferf(); break;
		//case idglPixelTransferi: MyglPixelTransferi(); break;
		//case idglPixelZoom: MyglPixelZoom(); break;
		//case idglPointSize: MyglPointSize(); break;
		case idglPolygonMode: MyglPolygonMode(args[0].glenum, args[1].glenum); break;
		//case idglPolygonStipple: MyglPolygonStipple(); break;
		case idglPopAttrib: MyglPopAttrib(); break;
		case idglPopMatrix: MyglPopMatrix(); break;
		//case idglPopName: MyglPopName(); break;
		case idglPushAttrib: MyglPushAttrib(args[0].glbitfield); break;
		case idglPushMatrix: MyglPushMatrix(); break;
		//case idglPushName: MyglPushName(); break;
		//case idglRasterPos2d: MyglRasterPos2d(); break;
		//case idglRasterPos2dv: MyglRasterPos2dv(); break;
		//case idglRasterPos2f: MyglRasterPos2f(); break;
		//case idglRasterPos2fv: MyglRasterPos2fv(); break;
		//case idglRasterPos2i: MyglRasterPos2i(); break;
		//case idglRasterPos2iv: MyglRasterPos2iv(); break;
		//case idglRasterPos2s: MyglRasterPos2s(); break;
		//case idglRasterPos2sv: MyglRasterPos2sv(); break;
		//case idglRasterPos3d: MyglRasterPos3d(); break;
		//case idglRasterPos3dv: MyglRasterPos3dv(); break;
		//case idglRasterPos3f: MyglRasterPos3f(); break;
		//case idglRasterPos3fv: MyglRasterPos3fv(); break;
		//case idglRasterPos3i: MyglRasterPos3i(); break;
		//case idglRasterPos3iv: MyglRasterPos3iv(); break;
		//case idglRasterPos3s: MyglRasterPos3s(); break;
		//case idglRasterPos3sv: MyglRasterPos3sv(); break;
		//case idglRasterPos4d: MyglRasterPos4d(); break;
		//case idglRasterPos4dv: MyglRasterPos4dv(); break;
		//case idglRasterPos4f: MyglRasterPos4f(); break;
		//case idglRasterPos4fv: MyglRasterPos4fv(); break;
		//case idglRasterPos4i: MyglRasterPos4i(); break;
		//case idglRasterPos4iv: MyglRasterPos4iv(); break;
		//case idglRasterPos4s: MyglRasterPos4s(); break;
		//case idglRasterPos4sv: MyglRasterPos4sv(); break;
		case idglReadBuffer: MyglReadBuffer(args[0].glenum); break;
		//case idglRectd: MyglRectd(); break;
		//case idglRectdv: MyglRectdv(); break;
		//case idglRectf: MyglRectf(); break;
		//case idglRectfv: MyglRectfv(); break;
		//case idglRecti: MyglRecti(); break;
		//case idglRectiv: MyglRectiv(); break;
		//case idglRects: MyglRects(); break;
		//case idglRectsv: MyglRectsv(); break;
		case idglRotated: MyglRotated(args[0].gldouble, args[1].gldouble, args[2].gldouble, args[3].gldouble); break;
		case idglRotatef: MyglRotatef(args[0].glfloat, args[1].glfloat, args[2].glfloat, args[3].glfloat); break;
		case idglScaled: MyglScaled(args[0].gldouble, args[1].gldouble, args[2].gldouble); break;
		case idglScalef: MyglScalef(args[0].glfloat, args[1].glfloat, args[2].glfloat); break;
		//case idglScissor: MyglScissor(); break;
		case idglShadeModel: MyglShadeModel(args[0].glenum); break;
		//case idglStencilFunc: MyglStencilFunc(); break;
		//case idglStencilMask: MyglStencilMask(); break;
		//case idglStencilOp: MyglStencilOp(); break;
		case idglTexCoord1d: MyglTexCoord1d(args[0].gldouble); break;
		case idglTexCoord1dv: MyglTexCoord1dv((const GLdouble*)buffer); break;
		case idglTexCoord1f: MyglTexCoord1f(args[0].glfloat); break;
		case idglTexCoord1fv: MyglTexCoord1fv((const GLfloat*)buffer); break;
		case idglTexCoord1i: MyglTexCoord1i(args[0].glint); break;
		case idglTexCoord1iv: MyglTexCoord1iv((const GLint*)buffer); break;
		case idglTexCoord1s: MyglTexCoord1s(args[0].glshort); break;
		case idglTexCoord1sv: MyglTexCoord1sv((const GLshort*)buffer); break;
		case idglTexCoord2d: MyglTexCoord2d(args[0].gldouble, args[1].gldouble); break;
		case idglTexCoord2dv: MyglTexCoord2dv((const GLdouble*)buffer); break;
		case idglTexCoord2f: MyglTexCoord2f(args[0].glfloat, args[1].glfloat); break;
		case idglTexCoord2fv: MyglTexCoord2fv((const GLfloat*)buffer); break;
		case idglTexCoord2i: MyglTexCoord2i(args[0].glint, args[1].glint); break;
		case idglTexCoord2iv: MyglTexCoord2iv((const GLint*)buffer); break;
		case idglTexCoord2s: MyglTexCoord2s(args[0].glshort, args[1].glint); break;
		case idglTexCoord2sv: MyglTexCoord2sv((const GLshort*)buffer); break;
		case idglTexCoord3d: MyglTexCoord3d(args[0].gldouble, args[1].gldouble, args[2].gldouble); break;
		case idglTexCoord3dv: MyglTexCoord3dv((const GLdouble*)buffer); break;
		case idglTexCoord3f: MyglTexCoord3f(args[0].glfloat, args[1].glfloat, args[2].glfloat); break;
		case idglTexCoord3fv: MyglTexCoord3fv((const GLfloat*)buffer); break;
		case idglTexCoord3i: MyglTexCoord3i(args[0].glint, args[1].glint, args[2].glint); break;
		case idglTexCoord3iv: MyglTexCoord3iv((const GLint*)buffer); break;
		case idglTexCoord3s: MyglTexCoord3s(args[0].glshort, args[1].glint, args[2].glint); break;
		case idglTexCoord3sv: MyglTexCoord3sv((const GLshort*)buffer); break;
		case idglTexCoord4d: MyglTexCoord4d(args[0].gldouble, args[1].gldouble, args[2].gldouble, args[3].gldouble); break;
		case idglTexCoord4dv: MyglTexCoord4dv((const GLdouble*)buffer); break;
		case idglTexCoord4f: MyglTexCoord4f(args[0].glfloat, args[1].glfloat, args[2].glfloat, args[3].glfloat); break;
		case idglTexCoord4fv: MyglTexCoord4fv((const GLfloat*)buffer); break;
		case idglTexCoord4i: MyglTexCoord4i(args[0].glint, args[1].glint, args[2].glint, args[3].glint); break;
		case idglTexCoord4iv: MyglTexCoord4iv((const GLint*)buffer); break;
		case idglTexCoord4s: MyglTexCoord4s(args[0].glshort, args[1].glint, args[2].glint, args[3].glint); break;
		case idglTexCoord4sv: MyglTexCoord4sv((const GLshort*)buffer); break;
		case idglTexEnvf: MyglTexEnvf(args[0].glenum,args[1].glenum,args[2].glfloat); break;
		//case idglTexEnvfv: MyglTexEnvfv(); break;
		case idglTexEnvi: MyglTexEnvi(args[0].glenum,args[1].glenum,args[2].glint); break;
		//case idglTexEnviv: MyglTexEnviv(); break;
		//case idglTexGend: MyglTexGend(); break;
		//case idglTexGendv: MyglTexGendv(); break;
		//case idglTexGenf: MyglTexGenf(); break;
		//case idglTexGenfv: MyglTexGenfv(); break;
		//case idglTexGeni: MyglTexGeni(); break;
		//case idglTexGeniv: MyglTexGeniv(); break;
		case idglTexImage1D: MyglTexImage1D(args[0].glenum, args[1].glint, args[2].glint, args[3].glsizei, args[4].glint, args[5].glenum, args[6].glenum, args[7].ptr); break;
		case idglTexImage2D: MyglTexImage2D(args[0].glenum, args[1].glint, args[2].glint, args[3].glsizei, args[4].glsizei, args[5].glint, args[6].glenum, args[7].glenum, args[8].ptr); break;
		case idglTexParameterf: MyglTexParameterf(args[0].glenum, args[1].glenum, args[2].glfloat); break;
		case idglTexParameterfv: MyglTexParameterfv(args[0].glenum, args[1].glenum, (const GLfloat*)buffer); break;
		case idglTexParameteri: MyglTexParameteri(args[0].glenum, args[1].glenum, args[2].glint); break;
		case idglTexParameteriv: MyglTexParameteriv(args[0].glenum, args[1].glenum, (const GLint*)buffer); break;
		case idglTranslated: MyglTranslated(args[0].gldouble, args[1].gldouble, args[2].gldouble); break;
		case idglTranslatef: MyglTranslatef(args[0].glfloat, args[1].glfloat, args[2].glfloat); break;
		case idglVertex2d: MyglVertex2d(args[0].gldouble,args[1].gldouble); break;
		case idglVertex2dv: MyglVertex2dv((const GLdouble*)buffer); break;
		case idglVertex2f: MyglVertex2f(args[0].glfloat,args[1].glfloat); break;
		case idglVertex2fv: MyglVertex2fv((const GLfloat*)buffer); break;
		case idglVertex2i: MyglVertex2i(args[0].glint,args[1].glint); break;
		case idglVertex2iv: MyglVertex2iv((const GLint*)buffer); break;
		case idglVertex2s: MyglVertex2s(args[0].glshort,args[1].glshort); break;
		case idglVertex2sv: MyglVertex2sv((const GLshort*)buffer); break;
		case idglVertex3d: MyglVertex3d(args[0].gldouble,args[1].gldouble,args[2].gldouble); break;
		case idglVertex3dv: MyglVertex3dv((const GLdouble*)buffer); break;
		case idglVertex3f: MyglVertex3f(args[0].glfloat,args[1].glfloat,args[2].glfloat); break;
		case idglVertex3fv: MyglVertex3fv((const GLfloat*)buffer); break;
		case idglVertex3i: MyglVertex3i(args[0].glint,args[1].glint,args[2].glint); break;
		case idglVertex3iv: MyglVertex3iv((const GLint*)buffer); break;
		case idglVertex3s: MyglVertex3s(args[0].glshort,args[1].glshort,args[2].glshort); break;
		case idglVertex3sv: MyglVertex3sv((const GLshort*)buffer); break;
		case idglVertex4d: MyglVertex4d(args[0].gldouble,args[1].gldouble,args[2].gldouble,args[3].gldouble); break;
		case idglVertex4dv: MyglVertex4dv((const GLdouble*)buffer); break;
		case idglVertex4f: MyglVertex4f(args[0].glfloat,args[1].glfloat,args[2].glfloat,args[3].glfloat); break;
		case idglVertex4fv: MyglVertex4fv((const GLfloat*)buffer); break;
		case idglVertex4i: MyglVertex4i(args[0].glint,args[1].glint,args[2].glint,args[3].glint); break;
		case idglVertex4iv: MyglVertex4iv((const GLint*)buffer); break;
		case idglVertex4s: MyglVertex4s(args[0].glshort,args[1].glshort,args[2].glshort,args[3].glshort); break;
		case idglVertex4sv: MyglVertex4sv((const GLshort*)buffer); break;
		case idglViewport: MyglViewport(args[0].glint,args[1].glint,args[2].glsizei,args[3].glsizei); break;
		//case idglArrayElement: MyglArrayElement(); break;
		//case idglCopyTexImage1D: MyglCopyTexImage1D(); break;
		//case idglCopyTexImage2D: MyglCopyTexImage2D(); break;
		//case idglCopyTexSubImage1D: MyglCopyTexSubImage1D(); break;
		//case idglCopyTexSubImage2D: MyglCopyTexSubImage2D(); break;
		////case idglDrawArrays: MyglDrawArrays(); break;
		////case idglDrawElements: MyglDrawElements(); break;
		case idd3dDrawPrimitive:
			OglD3dDrawPrimitive(args[0].dword, args[1].dword, args[2].dword, args[3].dword, args[4].ptr);
			break;
		//case idglIndexub: MyglIndexub(); break;
		//case idglIndexubv: MyglIndexubv(); break;
		//case idglPolygonOffset: MyglPolygonOffset(); break;
		//case idglPrioritizeTextures: MyglPrioritizeTextures(); break;
		//case idglTexSubImage1D: MyglTexSubImage1D(); break;
		//case idglTexSubImage2D: MyglTexSubImage2D(); break;
			//NYI
	}
}



bool PresentOGLD3D()
{
	if(!ogld3d8Device)
		return false;
	debuglog(LCF_OGL|LCF_FRAME, __FUNCTION__ " called.\n");
	ogld3d8Device->EndScene();
	ogld3d8Device->Present(NULL, NULL, NULL, NULL);
	ogld3d8Device->BeginScene();
	return true;
}

void ApplyOGLIntercepts()
{
	static const InterceptDescriptor intercepts [] = 
	{
		MAKE_INTERCEPT(1, OPENGL32, glBindTexture),
		MAKE_INTERCEPT(1, OPENGL32, glDeleteTextures),
		MAKE_INTERCEPT(1, OPENGL32, glGenTextures),
		MAKE_INTERCEPT(1, OPENGL32, glAccum),
		MAKE_INTERCEPT(1, OPENGL32, glAlphaFunc),
		MAKE_INTERCEPT(1, OPENGL32, glBegin),
		MAKE_INTERCEPT(1, OPENGL32, glBitmap),
		MAKE_INTERCEPT(1, OPENGL32, glBlendFunc),
		MAKE_INTERCEPT(1, OPENGL32, glCallList),
		MAKE_INTERCEPT(1, OPENGL32, glCallLists),
		MAKE_INTERCEPT(1, OPENGL32, glClear),
		MAKE_INTERCEPT(1, OPENGL32, glClearAccum),
		MAKE_INTERCEPT(1, OPENGL32, glClearColor),
		MAKE_INTERCEPT(1, OPENGL32, glClearDepth),
		MAKE_INTERCEPT(1, OPENGL32, glClearIndex),
		MAKE_INTERCEPT(1, OPENGL32, glClearStencil),
		MAKE_INTERCEPT(1, OPENGL32, glClipPlane),
		MAKE_INTERCEPT(1, OPENGL32, glColor3b),
		MAKE_INTERCEPT(1, OPENGL32, glColor3bv),
		MAKE_INTERCEPT(1, OPENGL32, glColor3d),
		MAKE_INTERCEPT(1, OPENGL32, glColor3dv),
		MAKE_INTERCEPT(1, OPENGL32, glColor3f),
		MAKE_INTERCEPT(1, OPENGL32, glColor3fv),
		MAKE_INTERCEPT(1, OPENGL32, glColor3i),
		MAKE_INTERCEPT(1, OPENGL32, glColor3iv),
		MAKE_INTERCEPT(1, OPENGL32, glColor3s),
		MAKE_INTERCEPT(1, OPENGL32, glColor3sv),
		MAKE_INTERCEPT(1, OPENGL32, glColor3ub),
		MAKE_INTERCEPT(1, OPENGL32, glColor3ubv),
		MAKE_INTERCEPT(1, OPENGL32, glColor3ui),
		MAKE_INTERCEPT(1, OPENGL32, glColor3uiv),
		MAKE_INTERCEPT(1, OPENGL32, glColor3us),
		MAKE_INTERCEPT(1, OPENGL32, glColor3usv),
		MAKE_INTERCEPT(1, OPENGL32, glColor4b),
		MAKE_INTERCEPT(1, OPENGL32, glColor4bv),
		MAKE_INTERCEPT(1, OPENGL32, glColor4d),
		MAKE_INTERCEPT(1, OPENGL32, glColor4dv),
		MAKE_INTERCEPT(1, OPENGL32, glColor4f),
		MAKE_INTERCEPT(1, OPENGL32, glColor4fv),
		MAKE_INTERCEPT(1, OPENGL32, glColor4i),
		MAKE_INTERCEPT(1, OPENGL32, glColor4iv),
		MAKE_INTERCEPT(1, OPENGL32, glColor4s),
		MAKE_INTERCEPT(1, OPENGL32, glColor4sv),
		MAKE_INTERCEPT(1, OPENGL32, glColor4ub),
		MAKE_INTERCEPT(1, OPENGL32, glColor4ubv),
		MAKE_INTERCEPT(1, OPENGL32, glColor4ui),
		MAKE_INTERCEPT(1, OPENGL32, glColor4uiv),
		MAKE_INTERCEPT(1, OPENGL32, glColor4us),
		MAKE_INTERCEPT(1, OPENGL32, glColor4usv),
		MAKE_INTERCEPT(1, OPENGL32, glColorMask),
		MAKE_INTERCEPT(1, OPENGL32, glColorMaterial),
		MAKE_INTERCEPT(1, OPENGL32, glCopyPixels),
		MAKE_INTERCEPT(1, OPENGL32, glCullFace),
		MAKE_INTERCEPT(1, OPENGL32, glDeleteLists),
		MAKE_INTERCEPT(1, OPENGL32, glDepthFunc),
		MAKE_INTERCEPT(1, OPENGL32, glDepthMask),
		MAKE_INTERCEPT(1, OPENGL32, glDepthRange),
		MAKE_INTERCEPT(1, OPENGL32, glDisable),
		MAKE_INTERCEPT(1, OPENGL32, glDrawBuffer),
		MAKE_INTERCEPT(1, OPENGL32, glDrawPixels),
		MAKE_INTERCEPT(1, OPENGL32, glEdgeFlag),
		MAKE_INTERCEPT(1, OPENGL32, glEdgeFlagv),
		MAKE_INTERCEPT(1, OPENGL32, glEnable),
		MAKE_INTERCEPT(1, OPENGL32, glEnd),
		MAKE_INTERCEPT(1, OPENGL32, glEndList),
		MAKE_INTERCEPT(1, OPENGL32, glEvalCoord1d),
		MAKE_INTERCEPT(1, OPENGL32, glEvalCoord1dv),
		MAKE_INTERCEPT(1, OPENGL32, glEvalCoord1f),
		MAKE_INTERCEPT(1, OPENGL32, glEvalCoord1fv),
		MAKE_INTERCEPT(1, OPENGL32, glEvalCoord2d),
		MAKE_INTERCEPT(1, OPENGL32, glEvalCoord2dv),
		MAKE_INTERCEPT(1, OPENGL32, glEvalCoord2f),
		MAKE_INTERCEPT(1, OPENGL32, glEvalCoord2fv),
		MAKE_INTERCEPT(1, OPENGL32, glEvalMesh1),
		MAKE_INTERCEPT(1, OPENGL32, glEvalMesh2),
		MAKE_INTERCEPT(1, OPENGL32, glEvalPoint1),
		MAKE_INTERCEPT(1, OPENGL32, glEvalPoint2),
		MAKE_INTERCEPT(1, OPENGL32, glFeedbackBuffer),
		MAKE_INTERCEPT(1, OPENGL32, glFinish),
		MAKE_INTERCEPT(1, OPENGL32, glFlush),
		MAKE_INTERCEPT(1, OPENGL32, glFogf),
		MAKE_INTERCEPT(1, OPENGL32, glFogfv),
		MAKE_INTERCEPT(1, OPENGL32, glFogi),
		MAKE_INTERCEPT(1, OPENGL32, glFogiv),
		MAKE_INTERCEPT(1, OPENGL32, glFrontFace),
		MAKE_INTERCEPT(1, OPENGL32, glFrustum),
		MAKE_INTERCEPT(1, OPENGL32, glGetBooleanv),
		MAKE_INTERCEPT(1, OPENGL32, glGetClipPlane),
		MAKE_INTERCEPT(1, OPENGL32, glGetDoublev),
		MAKE_INTERCEPT(1, OPENGL32, glGetFloatv),
		MAKE_INTERCEPT(1, OPENGL32, glGetIntegerv),
		MAKE_INTERCEPT(1, OPENGL32, glGetLightfv),
		MAKE_INTERCEPT(1, OPENGL32, glGetLightiv),
		MAKE_INTERCEPT(1, OPENGL32, glGetMapdv),
		MAKE_INTERCEPT(1, OPENGL32, glGetMapfv),
		MAKE_INTERCEPT(1, OPENGL32, glGetMapiv),
		MAKE_INTERCEPT(1, OPENGL32, glGetMaterialfv),
		MAKE_INTERCEPT(1, OPENGL32, glGetMaterialiv),
		MAKE_INTERCEPT(1, OPENGL32, glGetPixelMapfv),
		MAKE_INTERCEPT(1, OPENGL32, glGetPixelMapuiv),
		MAKE_INTERCEPT(1, OPENGL32, glGetPixelMapusv),
		MAKE_INTERCEPT(1, OPENGL32, glGetPolygonStipple),
		MAKE_INTERCEPT(1, OPENGL32, glGetTexEnvfv),
		MAKE_INTERCEPT(1, OPENGL32, glGetTexEnviv),
		MAKE_INTERCEPT(1, OPENGL32, glGetTexGendv),
		MAKE_INTERCEPT(1, OPENGL32, glGetTexGenfv),
		MAKE_INTERCEPT(1, OPENGL32, glGetTexGeniv),
		MAKE_INTERCEPT(1, OPENGL32, glGetTexImage),
		MAKE_INTERCEPT(1, OPENGL32, glGetTexLevelParameterfv),
		MAKE_INTERCEPT(1, OPENGL32, glGetTexLevelParameteriv),
		MAKE_INTERCEPT(1, OPENGL32, glGetTexParameterfv),
		MAKE_INTERCEPT(1, OPENGL32, glGetTexParameteriv),
		MAKE_INTERCEPT(1, OPENGL32, glHint),
		MAKE_INTERCEPT(1, OPENGL32, glIndexMask),
		MAKE_INTERCEPT(1, OPENGL32, glIndexd),
		MAKE_INTERCEPT(1, OPENGL32, glIndexdv),
		MAKE_INTERCEPT(1, OPENGL32, glIndexf),
		MAKE_INTERCEPT(1, OPENGL32, glIndexfv),
		MAKE_INTERCEPT(1, OPENGL32, glIndexi),
		MAKE_INTERCEPT(1, OPENGL32, glIndexiv),
		MAKE_INTERCEPT(1, OPENGL32, glIndexs),
		MAKE_INTERCEPT(1, OPENGL32, glIndexsv),
		MAKE_INTERCEPT(1, OPENGL32, glInitNames),
		MAKE_INTERCEPT(1, OPENGL32, glLightModelf),
		MAKE_INTERCEPT(1, OPENGL32, glLightModelfv),
		MAKE_INTERCEPT(1, OPENGL32, glLightModeli),
		MAKE_INTERCEPT(1, OPENGL32, glLightModeliv),
		MAKE_INTERCEPT(1, OPENGL32, glLightf),
		MAKE_INTERCEPT(1, OPENGL32, glLightfv),
		MAKE_INTERCEPT(1, OPENGL32, glLighti),
		MAKE_INTERCEPT(1, OPENGL32, glLightiv),
		MAKE_INTERCEPT(1, OPENGL32, glLineStipple),
		MAKE_INTERCEPT(1, OPENGL32, glLineWidth),
		MAKE_INTERCEPT(1, OPENGL32, glListBase),
		MAKE_INTERCEPT(1, OPENGL32, glLoadIdentity),
		MAKE_INTERCEPT(1, OPENGL32, glLoadMatrixd),
		MAKE_INTERCEPT(1, OPENGL32, glLoadMatrixf),
		MAKE_INTERCEPT(1, OPENGL32, glLoadName),
		MAKE_INTERCEPT(1, OPENGL32, glLogicOp),
		MAKE_INTERCEPT(1, OPENGL32, glMap1d),
		MAKE_INTERCEPT(1, OPENGL32, glMap1f),
		MAKE_INTERCEPT(1, OPENGL32, glMap2d),
		MAKE_INTERCEPT(1, OPENGL32, glMap2f),
		MAKE_INTERCEPT(1, OPENGL32, glMapGrid1d),
		MAKE_INTERCEPT(1, OPENGL32, glMapGrid1f),
		MAKE_INTERCEPT(1, OPENGL32, glMapGrid2d),
		MAKE_INTERCEPT(1, OPENGL32, glMapGrid2f),
		MAKE_INTERCEPT(1, OPENGL32, glMaterialf),
		MAKE_INTERCEPT(1, OPENGL32, glMaterialfv),
		MAKE_INTERCEPT(1, OPENGL32, glMateriali),
		MAKE_INTERCEPT(1, OPENGL32, glMaterialiv),
		MAKE_INTERCEPT(1, OPENGL32, glMatrixMode),
		MAKE_INTERCEPT(1, OPENGL32, glMultMatrixd),
		MAKE_INTERCEPT(1, OPENGL32, glMultMatrixf),
		MAKE_INTERCEPT(1, OPENGL32, glNewList),
		MAKE_INTERCEPT(1, OPENGL32, glNormal3b),
		MAKE_INTERCEPT(1, OPENGL32, glNormal3bv),
		MAKE_INTERCEPT(1, OPENGL32, glNormal3d),
		MAKE_INTERCEPT(1, OPENGL32, glNormal3dv),
		MAKE_INTERCEPT(1, OPENGL32, glNormal3f),
		MAKE_INTERCEPT(1, OPENGL32, glNormal3fv),
		MAKE_INTERCEPT(1, OPENGL32, glNormal3i),
		MAKE_INTERCEPT(1, OPENGL32, glNormal3iv),
		MAKE_INTERCEPT(1, OPENGL32, glNormal3s),
		MAKE_INTERCEPT(1, OPENGL32, glNormal3sv),
		MAKE_INTERCEPT(1, OPENGL32, glOrtho),
		MAKE_INTERCEPT(1, OPENGL32, glPassThrough),
		MAKE_INTERCEPT(1, OPENGL32, glPixelMapfv),
		MAKE_INTERCEPT(1, OPENGL32, glPixelMapuiv),
		MAKE_INTERCEPT(1, OPENGL32, glPixelMapusv),
		MAKE_INTERCEPT(1, OPENGL32, glPixelStoref),
		MAKE_INTERCEPT(1, OPENGL32, glPixelStorei),
		MAKE_INTERCEPT(1, OPENGL32, glPixelTransferf),
		MAKE_INTERCEPT(1, OPENGL32, glPixelTransferi),
		MAKE_INTERCEPT(1, OPENGL32, glPixelZoom),
		MAKE_INTERCEPT(1, OPENGL32, glPointSize),
		MAKE_INTERCEPT(1, OPENGL32, glPolygonMode),
		MAKE_INTERCEPT(1, OPENGL32, glPolygonStipple),
		MAKE_INTERCEPT(1, OPENGL32, glPopAttrib),
		MAKE_INTERCEPT(1, OPENGL32, glPopMatrix),
		MAKE_INTERCEPT(1, OPENGL32, glPopName),
		MAKE_INTERCEPT(1, OPENGL32, glPushAttrib),
		MAKE_INTERCEPT(1, OPENGL32, glPushMatrix),
		MAKE_INTERCEPT(1, OPENGL32, glPushName),
		MAKE_INTERCEPT(1, OPENGL32, glRasterPos2d),
		MAKE_INTERCEPT(1, OPENGL32, glRasterPos2dv),
		MAKE_INTERCEPT(1, OPENGL32, glRasterPos2f),
		MAKE_INTERCEPT(1, OPENGL32, glRasterPos2fv),
		MAKE_INTERCEPT(1, OPENGL32, glRasterPos2i),
		MAKE_INTERCEPT(1, OPENGL32, glRasterPos2iv),
		MAKE_INTERCEPT(1, OPENGL32, glRasterPos2s),
		MAKE_INTERCEPT(1, OPENGL32, glRasterPos2sv),
		MAKE_INTERCEPT(1, OPENGL32, glRasterPos3d),
		MAKE_INTERCEPT(1, OPENGL32, glRasterPos3dv),
		MAKE_INTERCEPT(1, OPENGL32, glRasterPos3f),
		MAKE_INTERCEPT(1, OPENGL32, glRasterPos3fv),
		MAKE_INTERCEPT(1, OPENGL32, glRasterPos3i),
		MAKE_INTERCEPT(1, OPENGL32, glRasterPos3iv),
		MAKE_INTERCEPT(1, OPENGL32, glRasterPos3s),
		MAKE_INTERCEPT(1, OPENGL32, glRasterPos3sv),
		MAKE_INTERCEPT(1, OPENGL32, glRasterPos4d),
		MAKE_INTERCEPT(1, OPENGL32, glRasterPos4dv),
		MAKE_INTERCEPT(1, OPENGL32, glRasterPos4f),
		MAKE_INTERCEPT(1, OPENGL32, glRasterPos4fv),
		MAKE_INTERCEPT(1, OPENGL32, glRasterPos4i),
		MAKE_INTERCEPT(1, OPENGL32, glRasterPos4iv),
		MAKE_INTERCEPT(1, OPENGL32, glRasterPos4s),
		MAKE_INTERCEPT(1, OPENGL32, glRasterPos4sv),
		MAKE_INTERCEPT(1, OPENGL32, glReadBuffer),
		MAKE_INTERCEPT(1, OPENGL32, glReadPixels),
		MAKE_INTERCEPT(1, OPENGL32, glRectd),
		MAKE_INTERCEPT(1, OPENGL32, glRectdv),
		MAKE_INTERCEPT(1, OPENGL32, glRectf),
		MAKE_INTERCEPT(1, OPENGL32, glRectfv),
		MAKE_INTERCEPT(1, OPENGL32, glRecti),
		MAKE_INTERCEPT(1, OPENGL32, glRectiv),
		MAKE_INTERCEPT(1, OPENGL32, glRects),
		MAKE_INTERCEPT(1, OPENGL32, glRectsv),
		MAKE_INTERCEPT(1, OPENGL32, glRotated),
		MAKE_INTERCEPT(1, OPENGL32, glRotatef),
		MAKE_INTERCEPT(1, OPENGL32, glScaled),
		MAKE_INTERCEPT(1, OPENGL32, glScalef),
		MAKE_INTERCEPT(1, OPENGL32, glScissor),
		MAKE_INTERCEPT(1, OPENGL32, glSelectBuffer),
		MAKE_INTERCEPT(1, OPENGL32, glShadeModel),
		MAKE_INTERCEPT(1, OPENGL32, glStencilFunc),
		MAKE_INTERCEPT(1, OPENGL32, glStencilMask),
		MAKE_INTERCEPT(1, OPENGL32, glStencilOp),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord1d),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord1dv),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord1f),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord1fv),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord1i),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord1iv),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord1s),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord1sv),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord2d),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord2dv),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord2f),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord2fv),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord2i),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord2iv),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord2s),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord2sv),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord3d),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord3dv),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord3f),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord3fv),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord3i),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord3iv),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord3s),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord3sv),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord4d),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord4dv),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord4f),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord4fv),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord4i),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord4iv),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord4s),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoord4sv),
		MAKE_INTERCEPT(1, OPENGL32, glTexEnvf),
		MAKE_INTERCEPT(1, OPENGL32, glTexEnvfv),
		MAKE_INTERCEPT(1, OPENGL32, glTexEnvi),
		MAKE_INTERCEPT(1, OPENGL32, glTexEnviv),
		MAKE_INTERCEPT(1, OPENGL32, glTexGend),
		MAKE_INTERCEPT(1, OPENGL32, glTexGendv),
		MAKE_INTERCEPT(1, OPENGL32, glTexGenf),
		MAKE_INTERCEPT(1, OPENGL32, glTexGenfv),
		MAKE_INTERCEPT(1, OPENGL32, glTexGeni),
		MAKE_INTERCEPT(1, OPENGL32, glTexGeniv),
		MAKE_INTERCEPT(1, OPENGL32, glTexImage1D),
		MAKE_INTERCEPT(1, OPENGL32, glTexImage2D),
		MAKE_INTERCEPT(1, OPENGL32, glTexParameterf),
		MAKE_INTERCEPT(1, OPENGL32, glTexParameterfv),
		MAKE_INTERCEPT(1, OPENGL32, glTexParameteri),
		MAKE_INTERCEPT(1, OPENGL32, glTexParameteriv),
		MAKE_INTERCEPT(1, OPENGL32, glTranslated),
		MAKE_INTERCEPT(1, OPENGL32, glTranslatef),
		MAKE_INTERCEPT(1, OPENGL32, glVertex2d),
		MAKE_INTERCEPT(1, OPENGL32, glVertex2dv),
		MAKE_INTERCEPT(1, OPENGL32, glVertex2f),
		MAKE_INTERCEPT(1, OPENGL32, glVertex2fv),
		MAKE_INTERCEPT(1, OPENGL32, glVertex2i),
		MAKE_INTERCEPT(1, OPENGL32, glVertex2iv),
		MAKE_INTERCEPT(1, OPENGL32, glVertex2s),
		MAKE_INTERCEPT(1, OPENGL32, glVertex2sv),
		MAKE_INTERCEPT(1, OPENGL32, glVertex3d),
		MAKE_INTERCEPT(1, OPENGL32, glVertex3dv),
		MAKE_INTERCEPT(1, OPENGL32, glVertex3f),
		MAKE_INTERCEPT(1, OPENGL32, glVertex3fv),
		MAKE_INTERCEPT(1, OPENGL32, glVertex3i),
		MAKE_INTERCEPT(1, OPENGL32, glVertex3iv),
		MAKE_INTERCEPT(1, OPENGL32, glVertex3s),
		MAKE_INTERCEPT(1, OPENGL32, glVertex3sv),
		MAKE_INTERCEPT(1, OPENGL32, glVertex4d),
		MAKE_INTERCEPT(1, OPENGL32, glVertex4dv),
		MAKE_INTERCEPT(1, OPENGL32, glVertex4f),
		MAKE_INTERCEPT(1, OPENGL32, glVertex4fv),
		MAKE_INTERCEPT(1, OPENGL32, glVertex4i),
		MAKE_INTERCEPT(1, OPENGL32, glVertex4iv),
		MAKE_INTERCEPT(1, OPENGL32, glVertex4s),
		MAKE_INTERCEPT(1, OPENGL32, glVertex4sv),
		MAKE_INTERCEPT(1, OPENGL32, glViewport),
		MAKE_INTERCEPT(1, OPENGL32, glGenLists),
		MAKE_INTERCEPT(1, OPENGL32, glGetError),
		MAKE_INTERCEPT(1, OPENGL32, glGetString),
		MAKE_INTERCEPT(1, OPENGL32, glIsEnabled),
		MAKE_INTERCEPT(1, OPENGL32, glIsList),
		MAKE_INTERCEPT(1, OPENGL32, glRenderMode),
		MAKE_INTERCEPT(1, OPENGL32, glAreTexturesResident),
		MAKE_INTERCEPT(1, OPENGL32, glArrayElement),
		MAKE_INTERCEPT(1, OPENGL32, glColorPointer),
		MAKE_INTERCEPT(1, OPENGL32, glCopyTexImage1D),
		MAKE_INTERCEPT(1, OPENGL32, glCopyTexImage2D),
		MAKE_INTERCEPT(1, OPENGL32, glCopyTexSubImage1D),
		MAKE_INTERCEPT(1, OPENGL32, glCopyTexSubImage2D),
		MAKE_INTERCEPT(1, OPENGL32, glDisableClientState),
		MAKE_INTERCEPT(1, OPENGL32, glDrawArrays),
		MAKE_INTERCEPT(1, OPENGL32, glDrawElements),
		MAKE_INTERCEPT(1, OPENGL32, glEdgeFlagPointer),
		MAKE_INTERCEPT(1, OPENGL32, glEnableClientState),
		MAKE_INTERCEPT(1, OPENGL32, glGetPointerv),
		MAKE_INTERCEPT(1, OPENGL32, glIndexPointer),
		MAKE_INTERCEPT(1, OPENGL32, glIndexub),
		MAKE_INTERCEPT(1, OPENGL32, glIndexubv),
		MAKE_INTERCEPT(1, OPENGL32, glInterleavedArrays),
		MAKE_INTERCEPT(1, OPENGL32, glIsTexture),
		MAKE_INTERCEPT(1, OPENGL32, glNormalPointer),
		MAKE_INTERCEPT(1, OPENGL32, glPolygonOffset),
		MAKE_INTERCEPT(1, OPENGL32, glPopClientAttrib),
		MAKE_INTERCEPT(1, OPENGL32, glPrioritizeTextures),
		MAKE_INTERCEPT(1, OPENGL32, glPushClientAttrib),
		MAKE_INTERCEPT(1, OPENGL32, glTexCoordPointer),
		MAKE_INTERCEPT(1, OPENGL32, glTexSubImage1D),
		MAKE_INTERCEPT(1, OPENGL32, glTexSubImage2D),
		MAKE_INTERCEPT(1, OPENGL32, glVertexPointer),
		// internal opengl
		MAKE_INTERCEPT(1, OPENGL32, glDebugEntry),
		MAKE_INTERCEPT(1, OPENGL32, GlmfBeginGlsBlock),
		MAKE_INTERCEPT(1, OPENGL32, GlmfEndGlsBlock),
		MAKE_INTERCEPT(1, OPENGL32, GlmfCloseMetaFile),
		MAKE_INTERCEPT(1, OPENGL32, GlmfEndPlayback),
		MAKE_INTERCEPT(1, OPENGL32, GlmfInitPlayback),
		MAKE_INTERCEPT(1, OPENGL32, GlmfPlayGlsRecord),
		MAKE_INTERCEPT(1, OPENGL32, wglChoosePixelFormat),
		MAKE_INTERCEPT(1, OPENGL32, wglCopyContext),
		MAKE_INTERCEPT(1, OPENGL32, wglCreateContext),
		MAKE_INTERCEPT(1, OPENGL32, wglCreateLayerContext),
		MAKE_INTERCEPT(1, OPENGL32, wglDeleteContext),
		MAKE_INTERCEPT(1, OPENGL32, wglDescribeLayerPlane),
		MAKE_INTERCEPT(1, OPENGL32, wglDescribePixelFormat),
		MAKE_INTERCEPT(1, OPENGL32, wglGetCurrentContext),
		MAKE_INTERCEPT(1, OPENGL32, wglGetCurrentDC),
		MAKE_INTERCEPT(1, OPENGL32, wglGetDefaultProcAddress),
		MAKE_INTERCEPT(1, OPENGL32, wglGetLayerPaletteEntries),
		MAKE_INTERCEPT(1, OPENGL32, wglGetPixelFormat),
		MAKE_INTERCEPT(1, OPENGL32, wglGetProcAddress),
		MAKE_INTERCEPT(1, OPENGL32, wglMakeCurrent),
		MAKE_INTERCEPT(1, OPENGL32, wglRealizeLayerPalette),
		MAKE_INTERCEPT(1, OPENGL32, wglSetLayerPaletteEntries),
		MAKE_INTERCEPT(1, OPENGL32, wglSetPixelFormat),
		MAKE_INTERCEPT(1, OPENGL32, wglShareLists),
		MAKE_INTERCEPT(1, OPENGL32, wglSwapBuffers),
		MAKE_INTERCEPT(1, OPENGL32, wglSwapLayerBuffers),
		MAKE_INTERCEPT(1, OPENGL32, wglSwapMultipleBuffers),
		MAKE_INTERCEPT(1, OPENGL32, wglUseFontBitmapsA),
		MAKE_INTERCEPT(1, OPENGL32, wglUseFontBitmapsW),
		MAKE_INTERCEPT(1, OPENGL32, wglUseFontOutlinesA),
		MAKE_INTERCEPT(1, OPENGL32, wglUseFontOutlinesW),
	};
	ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
}


#endif
