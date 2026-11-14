#pragma once

#if defined(__ANDROID__) || defined(__APPLE__)
#define HOST_GL_DIRECT 1
#endif

#if defined(__APPLE__)
#define GLES_SILENCE_DEPRECATION 1
#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>
#else
#ifndef HOST_GL_DIRECT
#define GL_GLES_PROTOTYPES 0
#endif
#include <GLES3/gl3.h>
#endif

#define GL_FUNCS                                                   \
  X(glActiveTexture, PFNGLACTIVETEXTUREPROC)                       \
  X(glAttachShader, PFNGLATTACHSHADERPROC)                         \
  X(glBindBuffer, PFNGLBINDBUFFERPROC)                             \
  X(glBindFramebuffer, PFNGLBINDFRAMEBUFFERPROC)                   \
  X(glBindRenderbuffer, PFNGLBINDRENDERBUFFERPROC)                 \
  X(glBindTexture, PFNGLBINDTEXTUREPROC)                           \
  X(glBlendEquationSeparate, PFNGLBLENDEQUATIONSEPARATEPROC)       \
  X(glBlendFuncSeparate, PFNGLBLENDFUNCSEPARATEPROC)               \
  X(glBlitFramebuffer, PFNGLBLITFRAMEBUFFERPROC)                   \
  X(glCheckFramebufferStatus, PFNGLCHECKFRAMEBUFFERSTATUSPROC)     \
  X(glClear, PFNGLCLEARPROC)                                       \
  X(glClearColor, PFNGLCLEARCOLORPROC)                             \
  X(glColorMask, PFNGLCOLORMASKPROC)                               \
  X(glCompileShader, PFNGLCOMPILESHADERPROC)                       \
  X(glCompressedTexImage2D, PFNGLCOMPRESSEDTEXIMAGE2DPROC)         \
  X(glCreateProgram, PFNGLCREATEPROGRAMPROC)                       \
  X(glCreateShader, PFNGLCREATESHADERPROC)                         \
  X(glCullFace, PFNGLCULLFACEPROC)                                 \
  X(glDeleteBuffers, PFNGLDELETEBUFFERSPROC)                       \
  X(glDeleteFramebuffers, PFNGLDELETEFRAMEBUFFERSPROC)             \
  X(glDeleteProgram, PFNGLDELETEPROGRAMPROC)                       \
  X(glDeleteRenderbuffers, PFNGLDELETERENDERBUFFERSPROC)           \
  X(glDeleteShader, PFNGLDELETESHADERPROC)                         \
  X(glDeleteTextures, PFNGLDELETETEXTURESPROC)                     \
  X(glDepthFunc, PFNGLDEPTHFUNCPROC)                               \
  X(glDepthMask, PFNGLDEPTHMASKPROC)                               \
  X(glDisable, PFNGLDISABLEPROC)                                   \
  X(glDisableVertexAttribArray, PFNGLDISABLEVERTEXATTRIBARRAYPROC) \
  X(glDrawArrays, PFNGLDRAWARRAYSPROC)                             \
  X(glDrawElements, PFNGLDRAWELEMENTSPROC)                         \
  X(glEnable, PFNGLENABLEPROC)                                     \
  X(glEnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAYPROC)   \
  X(glFinish, PFNGLFINISHPROC)                                     \
  X(glFramebufferRenderbuffer, PFNGLFRAMEBUFFERRENDERBUFFERPROC)   \
  X(glFramebufferTexture2D, PFNGLFRAMEBUFFERTEXTURE2DPROC)         \
  X(glFrontFace, PFNGLFRONTFACEPROC)                               \
  X(glGenFramebuffers, PFNGLGENFRAMEBUFFERSPROC)                   \
  X(glGenRenderbuffers, PFNGLGENRENDERBUFFERSPROC)                 \
  X(glGenTextures, PFNGLGENTEXTURESPROC)                           \
  X(glGetAttribLocation, PFNGLGETATTRIBLOCATIONPROC)               \
  X(glGetBooleanv, PFNGLGETBOOLEANVPROC)                           \
  X(glGetError, PFNGLGETERRORPROC)                                 \
  X(glGetFloatv, PFNGLGETFLOATVPROC)                               \
  X(glGetIntegerv, PFNGLGETINTEGERVPROC)                           \
  X(glGetShaderInfoLog, PFNGLGETSHADERINFOLOGPROC)                 \
  X(glGetShaderiv, PFNGLGETSHADERIVPROC)                           \
  X(glGetString, PFNGLGETSTRINGPROC)                               \
  X(glGetUniformLocation, PFNGLGETUNIFORMLOCATIONPROC)             \
  X(glIsEnabled, PFNGLISENABLEDPROC)                               \
  X(glLinkProgram, PFNGLLINKPROGRAMPROC)                           \
  X(glPixelStorei, PFNGLPIXELSTOREIPROC)                           \
  X(glReadPixels, PFNGLREADPIXELSPROC)                             \
  X(glRenderbufferStorage, PFNGLRENDERBUFFERSTORAGEPROC)           \
  X(glShaderSource, PFNGLSHADERSOURCEPROC)                         \
  X(glTexImage2D, PFNGLTEXIMAGE2DPROC)                             \
  X(glTexParameteri, PFNGLTEXPARAMETERIPROC)                       \
  X(glTexSubImage2D, PFNGLTEXSUBIMAGE2DPROC)                       \
  X(glUniform1f, PFNGLUNIFORM1FPROC)                               \
  X(glUniform1i, PFNGLUNIFORM1IPROC)                               \
  X(glUniform2fv, PFNGLUNIFORM2FVPROC)                             \
  X(glUniform3fv, PFNGLUNIFORM3FVPROC)                             \
  X(glUniform4f, PFNGLUNIFORM4FPROC)                               \
  X(glUniform4fv, PFNGLUNIFORM4FVPROC)                             \
  X(glUseProgram, PFNGLUSEPROGRAMPROC)                             \
  X(glVertexAttribPointer, PFNGLVERTEXATTRIBPOINTERPROC)           \
  X(glViewport, PFNGLVIEWPORTPROC)

#ifdef HOST_GL_DIRECT
#define GLF(n) n
#else
#define GLF(n) gp_##n
#define X(name, type) extern type gp_##name;
GL_FUNCS
#undef X
#endif
