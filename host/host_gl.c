#include "host.h"
#include "gl_api.h"

#include <stdio.h>
#include <stdlib.h>

#ifndef HOST_GL_DIRECT
#define X(name, type) type gp_##name;
GL_FUNCS
#undef X
#endif

#define MEMP(off) ((const void*)(off ? hp(off) : NULL))

static GLuint g_screen_fbo, g_screen_color, g_screen_depth;

static GLuint g_last_fbo, g_last_color;
static int g_screen_w = 640, g_screen_h = 480;
static bool g_want_depth = true, g_want_stencil;
static bool g_dirty;
static GLuint g_engine_draw_fb, g_engine_read_fb;
static GLuint g_array_buffer, g_element_buffer;
static bool g_gl_ready;

unsigned host_gl_window_fbo;

void host_gl_init(void) {
#ifndef HOST_GL_DIRECT
  void* (*getproc)(const char*) = NULL;
  if (!plat_load_proc(&getproc) || !getproc) host_fatal("no GL function loader");
#define X(name, type)                                    \
  gp_##name = (type)getproc(#name);                      \
  if (!gp_##name) host_fatal("missing GL function %s", #name);
  GL_FUNCS
#undef X
#endif
  host_log("GL: %s / %s / %s", (const char*)GLF(glGetString)(GL_VENDOR), (const char*)GLF(glGetString)(GL_RENDERER),
           (const char*)GLF(glGetString)(GL_VERSION));
}

static void bind_engine_fb(GLenum target, GLuint fb) {
  GLF(glBindFramebuffer)(target, fb ? fb : g_screen_fbo);
}

static void create_screen(void) {
  if (!g_gl_ready) return;
  if (g_screen_fbo) {
    GLF(glDeleteFramebuffers)(1, &g_screen_fbo);
    GLF(glDeleteRenderbuffers)(1, &g_screen_color);
    if (g_screen_depth) GLF(glDeleteRenderbuffers)(1, &g_screen_depth);
    g_screen_fbo = g_screen_color = g_screen_depth = 0;
  }
  if (g_last_fbo) {
    GLF(glDeleteFramebuffers)(1, &g_last_fbo);
    GLF(glDeleteRenderbuffers)(1, &g_last_color);
    g_last_fbo = g_last_color = 0;
  }
  GLint rb_prev;
  GLF(glGetIntegerv)(GL_RENDERBUFFER_BINDING, &rb_prev);
  GLF(glGenFramebuffers)(1, &g_screen_fbo);
  GLF(glGenRenderbuffers)(1, &g_screen_color);
  GLF(glBindRenderbuffer)(GL_RENDERBUFFER, g_screen_color);
  GLF(glRenderbufferStorage)(GL_RENDERBUFFER, GL_RGBA8, g_screen_w, g_screen_h);
  GLF(glBindFramebuffer)(GL_FRAMEBUFFER, g_screen_fbo);
  GLF(glFramebufferRenderbuffer)(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, g_screen_color);
  if (g_want_depth || g_want_stencil) {
    GLF(glGenRenderbuffers)(1, &g_screen_depth);
    GLF(glBindRenderbuffer)(GL_RENDERBUFFER, g_screen_depth);
    GLF(glRenderbufferStorage)(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, g_screen_w, g_screen_h);
    GLF(glFramebufferRenderbuffer)(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, g_screen_depth);
  }
  GLenum st = GLF(glCheckFramebufferStatus)(GL_FRAMEBUFFER);
  if (st != GL_FRAMEBUFFER_COMPLETE) host_fatal("game screen framebuffer incomplete: 0x%x", st);
  GLF(glGenFramebuffers)(1, &g_last_fbo);
  GLF(glGenRenderbuffers)(1, &g_last_color);
  GLF(glBindRenderbuffer)(GL_RENDERBUFFER, g_last_color);
  GLF(glRenderbufferStorage)(GL_RENDERBUFFER, GL_RGBA8, g_screen_w, g_screen_h);
  GLF(glBindFramebuffer)(GL_FRAMEBUFFER, g_last_fbo);
  GLF(glFramebufferRenderbuffer)(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, g_last_color);
  GLF(glClearColor)(0, 0, 0, 1);
  GLF(glClear)(GL_COLOR_BUFFER_BIT);
  GLF(glBindFramebuffer)(GL_FRAMEBUFFER, g_screen_fbo);
  GLF(glBindRenderbuffer)(GL_RENDERBUFFER, (GLuint)rb_prev);

  GLF(glClearColor)(0, 0, 0, 0);
  GLF(glClear)(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  bind_engine_fb(GL_DRAW_FRAMEBUFFER, g_engine_draw_fb);
  bind_engine_fb(GL_READ_FRAMEBUFFER, g_engine_read_fb);
  host_log("game screen %dx%d", g_screen_w, g_screen_h);
}

void host_gl_set_screen_size(int w, int h) {
  if (w <= 0 || h <= 0 || (w == g_screen_w && h == g_screen_h && g_screen_fbo)) return;
  g_screen_w = w;
  g_screen_h = h;
  create_screen();
}

void host_gl_screen_size(int* w, int* h) {
  *w = g_screen_w;
  *h = g_screen_h;
}

bool host_gl_take_dirty(void) {
  bool d = g_dirty;
  g_dirty = false;
  return d;
}

void host_gl_read_screen(uint8_t* rgba) {
  GLint read_prev;
  GLF(glGetIntegerv)(GL_READ_FRAMEBUFFER_BINDING, &read_prev);
  GLF(glBindFramebuffer)(GL_READ_FRAMEBUFFER, g_screen_fbo);
  GLF(glReadPixels)(0, 0, g_screen_w, g_screen_h, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
  GLF(glBindFramebuffer)(GL_READ_FRAMEBUFFER, (GLuint)read_prev);
}

static void blit_to_window(GLuint src, int win_w, int win_h) {
  GLF(glBindFramebuffer)(GL_DRAW_FRAMEBUFFER, host_gl_window_fbo);
  GLF(glClearColor)(0, 0, 0, 1);
  GLF(glClear)(GL_COLOR_BUFFER_BIT);

  double s = (double)win_w / g_screen_w < (double)win_h / g_screen_h ? (double)win_w / g_screen_w
                                                                      : (double)win_h / g_screen_h;
  int dw = (int)(g_screen_w * s), dh = (int)(g_screen_h * s);
  int dx = (win_w - dw) / 2, dy = (win_h - dh) / 2;
  GLenum filter = (s == (int)s) ? GL_NEAREST : GL_LINEAR;
  GLF(glBindFramebuffer)(GL_READ_FRAMEBUFFER, src);
  GLF(glBlitFramebuffer)(0, 0, g_screen_w, g_screen_h, dx, dy, dx + dw, dy + dh, GL_COLOR_BUFFER_BIT, filter);
}

typedef struct {
  GLboolean scissor, mask[4], depth_mask;
  GLfloat clear[4];
} gl_saved;

static void save_state(gl_saved* g) {
  g->scissor = GLF(glIsEnabled)(GL_SCISSOR_TEST);
  GLF(glGetBooleanv)(GL_COLOR_WRITEMASK, g->mask);
  GLF(glGetBooleanv)(GL_DEPTH_WRITEMASK, &g->depth_mask);
  GLF(glGetFloatv)(GL_COLOR_CLEAR_VALUE, g->clear);
  GLF(glDisable)(GL_SCISSOR_TEST);
  GLF(glColorMask)(1, 1, 1, 1);
  GLF(glDepthMask)(1);
}

static void restore_state(const gl_saved* g) {
  GLF(glClearColor)(g->clear[0], g->clear[1], g->clear[2], g->clear[3]);
  GLF(glColorMask)(g->mask[0], g->mask[1], g->mask[2], g->mask[3]);
  GLF(glDepthMask)(g->depth_mask);
  if (g->scissor) GLF(glEnable)(GL_SCISSOR_TEST);
  bind_engine_fb(GL_DRAW_FRAMEBUFFER, g_engine_draw_fb);
  bind_engine_fb(GL_READ_FRAMEBUFFER, g_engine_read_fb);
}

void host_gl_present(int win_w, int win_h) {
  if (!g_screen_fbo) return;
  gl_saved st;
  save_state(&st);
  if (win_w > 0 && win_h > 0) blit_to_window(g_screen_fbo, win_w, win_h);

  GLF(glBindFramebuffer)(GL_READ_FRAMEBUFFER, g_screen_fbo);
  GLF(glBindFramebuffer)(GL_DRAW_FRAMEBUFFER, g_last_fbo);
  GLF(glBlitFramebuffer)(0, 0, g_screen_w, g_screen_h, 0, 0, g_screen_w, g_screen_h, GL_COLOR_BUFFER_BIT, GL_NEAREST);

  GLF(glBindFramebuffer)(GL_DRAW_FRAMEBUFFER, g_screen_fbo);
  GLF(glClearColor)(0, 0, 0, 0);
  GLF(glClear)(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  restore_state(&st);
}

void host_gl_present_last(int win_w, int win_h) {
  if (!g_last_fbo || win_w <= 0 || win_h <= 0) return;
  gl_saved st;
  save_state(&st);
  blit_to_window(g_last_fbo, win_w, win_h);
  restore_state(&st);
}

u32 w2c_env_emscripten_webgl_create_context(struct w2c_env* env, u32 target, u32 attrs) {
  g_want_depth = *hp(attrs + 1) != 0;
  g_want_stencil = *hp(attrs + 2) != 0;
  host_log("WebGL context: alpha=%d depth=%d stencil=%d antialias=%d version=%u", *hp(attrs), g_want_depth,
           g_want_stencil, *hp(attrs + 3), hr32(attrs + 16));
  return 1;
}

u32 w2c_env_emscripten_webgl_make_context_current(struct w2c_env* env, u32 ctx) {
  if (!g_gl_ready) {
    g_gl_ready = true;
    create_screen();
  }
  return 0;
}

u32 w2c_env_eglMakeCurrent(struct w2c_env* env, u32 dpy, u32 draw, u32 read, u32 ctx) { return 1; }
u32 w2c_env_eglDestroyContext(struct w2c_env* env, u32 dpy, u32 ctx) { return 1; }
u32 w2c_env_eglDestroySurface(struct w2c_env* env, u32 dpy, u32 surface) { return 1; }
u32 w2c_env_eglTerminate(struct w2c_env* env, u32 dpy) { return 1; }

u32 w2c_env_eglQuerySurface(struct w2c_env* env, u32 dpy, u32 surface, u32 attribute, u32 value) {
  if (!value) return 0;
  switch (attribute) {
    case 12375: hw32(value, (u32)g_screen_w); return 1;
    case 12374: hw32(value, (u32)g_screen_h); return 1;
    case 12328: hw32(value, 62002); return 1;
    case 12432: case 12433: case 12434: hw32(value, (u32)-1); return 1;
    case 12422: hw32(value, 12420); return 1;
    case 12441: hw32(value, 12442); return 1;
    case 12435: hw32(value, 12437); return 1;
    default: return 1;
  }
}

void w2c_env_glActiveTexture(struct w2c_env* e, u32 t) { GLF(glActiveTexture)(t); }
void w2c_env_glAttachShader(struct w2c_env* e, u32 p, u32 s) { GLF(glAttachShader)(p, s); }
void w2c_env_glBindRenderbuffer(struct w2c_env* e, u32 t, u32 rb) { GLF(glBindRenderbuffer)(t, rb); }
void w2c_env_glBindTexture(struct w2c_env* e, u32 t, u32 tex) { GLF(glBindTexture)(t, tex); }
void w2c_env_glBlendEquationSeparate(struct w2c_env* e, u32 a, u32 b) { GLF(glBlendEquationSeparate)(a, b); }
void w2c_env_glBlendFuncSeparate(struct w2c_env* e, u32 a, u32 b, u32 c, u32 d) { GLF(glBlendFuncSeparate)(a, b, c, d); }
u32 w2c_env_glCheckFramebufferStatus(struct w2c_env* e, u32 t) { return GLF(glCheckFramebufferStatus)(t); }
void w2c_env_glClearColor(struct w2c_env* e, f32 r, f32 g, f32 b, f32 a) { GLF(glClearColor)(r, g, b, a); }
void w2c_env_glCompileShader(struct w2c_env* e, u32 s) { GLF(glCompileShader)(s); }
u32 w2c_env_glCreateProgram(struct w2c_env* e) { return GLF(glCreateProgram)(); }
u32 w2c_env_glCreateShader(struct w2c_env* e, u32 t) { return GLF(glCreateShader)(t); }
void w2c_env_glCullFace(struct w2c_env* e, u32 m) { GLF(glCullFace)(m); }
void w2c_env_glDeleteProgram(struct w2c_env* e, u32 p) { GLF(glDeleteProgram)(p); }
void w2c_env_glDeleteShader(struct w2c_env* e, u32 s) { GLF(glDeleteShader)(s); }
void w2c_env_glDepthFunc(struct w2c_env* e, u32 f) { GLF(glDepthFunc)(f); }
void w2c_env_glDepthMask(struct w2c_env* e, u32 f) { GLF(glDepthMask)((GLboolean)(f != 0)); }
void w2c_env_glDisable(struct w2c_env* e, u32 c) { GLF(glDisable)(c); }
void w2c_env_glDisableVertexAttribArray(struct w2c_env* e, u32 i) { GLF(glDisableVertexAttribArray)(i); }
void w2c_env_glEnable(struct w2c_env* e, u32 c) { GLF(glEnable)(c); }
void w2c_env_glEnableVertexAttribArray(struct w2c_env* e, u32 i) { GLF(glEnableVertexAttribArray)(i); }
void w2c_env_glFrontFace(struct w2c_env* e, u32 m) { GLF(glFrontFace)(m); }
void w2c_env_glLinkProgram(struct w2c_env* e, u32 p) { GLF(glLinkProgram)(p); }
void w2c_env_glPixelStorei(struct w2c_env* e, u32 n, u32 v) { GLF(glPixelStorei)(n, (GLint)v); }
void w2c_env_glRenderbufferStorage(struct w2c_env* e, u32 t, u32 f, u32 w, u32 h) { GLF(glRenderbufferStorage)(t, f, (GLsizei)w, (GLsizei)h); }
void w2c_env_glTexParameteri(struct w2c_env* e, u32 t, u32 n, u32 v) { GLF(glTexParameteri)(t, n, (GLint)v); }
void w2c_env_glUniform1f(struct w2c_env* e, u32 l, f32 v) { GLF(glUniform1f)((GLint)l, v); }
void w2c_env_glUniform1i(struct w2c_env* e, u32 l, u32 v) { GLF(glUniform1i)((GLint)l, (GLint)v); }
void w2c_env_glUniform4f(struct w2c_env* e, u32 l, f32 a, f32 b, f32 c, f32 d) { GLF(glUniform4f)((GLint)l, a, b, c, d); }
void w2c_env_glUseProgram(struct w2c_env* e, u32 p) { GLF(glUseProgram)(p); }
void w2c_env_glViewport(struct w2c_env* e, u32 x, u32 y, u32 w, u32 h) { GLF(glViewport)((GLint)x, (GLint)y, (GLsizei)w, (GLsizei)h); }

void w2c_env_glFramebufferRenderbuffer(struct w2c_env* e, u32 t, u32 a, u32 rt, u32 rb) {
  GLF(glFramebufferRenderbuffer)(t, a, rt, rb);
}
void w2c_env_glFramebufferTexture2D(struct w2c_env* e, u32 t, u32 a, u32 tt, u32 tex, u32 level) {
  GLF(glFramebufferTexture2D)(t, a, tt, tex, (GLint)level);
}

void w2c_env_glGenFramebuffers(struct w2c_env* e, u32 n, u32 ids) { GLF(glGenFramebuffers)((GLsizei)n, (GLuint*)hp(ids)); }
void w2c_env_glGenRenderbuffers(struct w2c_env* e, u32 n, u32 ids) { GLF(glGenRenderbuffers)((GLsizei)n, (GLuint*)hp(ids)); }
void w2c_env_glGenTextures(struct w2c_env* e, u32 n, u32 ids) { GLF(glGenTextures)((GLsizei)n, (GLuint*)hp(ids)); }
void w2c_env_glDeleteFramebuffers(struct w2c_env* e, u32 n, u32 ids) { GLF(glDeleteFramebuffers)((GLsizei)n, (const GLuint*)hp(ids)); }
void w2c_env_glDeleteRenderbuffers(struct w2c_env* e, u32 n, u32 ids) { GLF(glDeleteRenderbuffers)((GLsizei)n, (const GLuint*)hp(ids)); }
void w2c_env_glDeleteTextures(struct w2c_env* e, u32 n, u32 ids) { GLF(glDeleteTextures)((GLsizei)n, (const GLuint*)hp(ids)); }

void w2c_env_glDeleteBuffers(struct w2c_env* e, u32 n, u32 ids) {
  for (u32 i = 0; i < n; i++) {
    u32 id = hr32(ids + 4 * i);
    if (id == g_array_buffer) g_array_buffer = 0;
    if (id == g_element_buffer) g_element_buffer = 0;
  }
  GLF(glDeleteBuffers)((GLsizei)n, (const GLuint*)hp(ids));
}

void w2c_env_glBindBuffer(struct w2c_env* e, u32 target, u32 buffer) {
  if (target == GL_ARRAY_BUFFER) g_array_buffer = buffer;
  else if (target == GL_ELEMENT_ARRAY_BUFFER) g_element_buffer = buffer;
  if (buffer) host_log("engine bound buffer %u to 0x%x (unexpected)", buffer, target);
  GLF(glBindBuffer)(target, buffer);
}

void w2c_env_glBindFramebuffer(struct w2c_env* e, u32 target, u32 fb) {
  if (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER) g_engine_draw_fb = fb;
  if (target == GL_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER) g_engine_read_fb = fb;
  bind_engine_fb(target, fb);
}

static bool drawing_to_screen(void) { return g_engine_draw_fb == 0; }

void w2c_env_glClear(struct w2c_env* e, u32 mask) {
  GLF(glClear)(mask);
  if (drawing_to_screen()) g_dirty = true;
}

void w2c_env_glDrawArrays(struct w2c_env* e, u32 mode, u32 first, u32 count) {
  GLF(glDrawArrays)(mode, (GLint)first, (GLsizei)count);
  if (drawing_to_screen()) g_dirty = true;
}

void w2c_env_glDrawElements(struct w2c_env* e, u32 mode, u32 count, u32 type, u32 indices) {

  const void* p = g_element_buffer ? (const void*)(uintptr_t)indices : MEMP(indices);
  GLF(glDrawElements)(mode, (GLsizei)count, type, p);
  if (drawing_to_screen()) g_dirty = true;
}

void w2c_env_glVertexAttribPointer(struct w2c_env* e, u32 index, u32 size, u32 type, u32 norm, u32 stride, u32 ptr) {

  const void* p = g_array_buffer ? (const void*)(uintptr_t)ptr : MEMP(ptr);
  GLF(glVertexAttribPointer)(index, (GLint)size, type, (GLboolean)(norm != 0), (GLsizei)stride, p);
}

void w2c_env_glTexImage2D(struct w2c_env* e, u32 target, u32 level, u32 ifmt, u32 w, u32 h, u32 border, u32 fmt,
                          u32 type, u32 pixels) {
  GLF(glTexImage2D)(target, (GLint)level, (GLint)ifmt, (GLsizei)w, (GLsizei)h, (GLint)border, fmt, type, MEMP(pixels));
}

void w2c_env_glTexSubImage2D(struct w2c_env* e, u32 target, u32 level, u32 x, u32 y, u32 w, u32 h, u32 fmt, u32 type,
                             u32 pixels) {
  GLF(glTexSubImage2D)(target, (GLint)level, (GLint)x, (GLint)y, (GLsizei)w, (GLsizei)h, fmt, type, MEMP(pixels));
}

void w2c_env_glCompressedTexImage2D(struct w2c_env* e, u32 target, u32 level, u32 ifmt, u32 w, u32 h, u32 border,
                                    u32 size, u32 data) {
  GLF(glCompressedTexImage2D)(target, (GLint)level, ifmt, (GLsizei)w, (GLsizei)h, (GLint)border, (GLsizei)size, MEMP(data));
}

void w2c_env_glReadPixels(struct w2c_env* e, u32 x, u32 y, u32 w, u32 h, u32 fmt, u32 type, u32 pixels) {
  GLF(glReadPixels)((GLint)x, (GLint)y, (GLsizei)w, (GLsizei)h, fmt, type, (void*)hp(pixels));
}

void w2c_env_glShaderSource(struct w2c_env* e, u32 shader, u32 count, u32 strings, u32 lengths) {
  const GLchar* src[64];
  GLint lens[64];
  if (count > 64) host_fatal("glShaderSource: too many strings (%u)", count);
  for (u32 i = 0; i < count; i++) {
    src[i] = (const GLchar*)hp(hr32(strings + 4 * i));
    lens[i] = lengths ? (GLint)hr32(lengths + 4 * i) : -1;
  }
  GLF(glShaderSource)(shader, (GLsizei)count, src, lens);
}

void w2c_env_glGetShaderiv(struct w2c_env* e, u32 shader, u32 pname, u32 p) {
  if (p) GLF(glGetShaderiv)(shader, pname, (GLint*)hp(p));
}

void w2c_env_glGetShaderInfoLog(struct w2c_env* e, u32 shader, u32 maxLength, u32 length, u32 infoLog) {
  GLsizei n = 0;
  if (maxLength > 0 && infoLog) GLF(glGetShaderInfoLog)(shader, (GLsizei)maxLength, &n, (GLchar*)hp(infoLog));
  if (length) hw32(length, (u32)n);
}

u32 w2c_env_glGetAttribLocation(struct w2c_env* e, u32 program, u32 name) {
  return (u32)GLF(glGetAttribLocation)(program, hstr(name));
}

u32 w2c_env_glGetUniformLocation(struct w2c_env* e, u32 program, u32 name) {
  return (u32)GLF(glGetUniformLocation)(program, hstr(name));
}

void w2c_env_glUniform2fv(struct w2c_env* e, u32 l, u32 n, u32 v) { if (n) GLF(glUniform2fv)((GLint)l, (GLsizei)n, (const GLfloat*)hp(v)); }
void w2c_env_glUniform3fv(struct w2c_env* e, u32 l, u32 n, u32 v) { if (n) GLF(glUniform3fv)((GLint)l, (GLsizei)n, (const GLfloat*)hp(v)); }
void w2c_env_glUniform4fv(struct w2c_env* e, u32 l, u32 n, u32 v) { if (n) GLF(glUniform4fv)((GLint)l, (GLsizei)n, (const GLfloat*)hp(v)); }

void w2c_env_glGetIntegerv(struct w2c_env* e, u32 pname, u32 p) {
  if (!p) return;
  switch (pname) {
    case 36346: hw32(p, 1); return;
    case 36345: case 34814: hw32(p, 0); return;
    default: break;
  }
  GLF(glGetIntegerv)(pname, (GLint*)hp(p));

  if ((pname == GL_FRAMEBUFFER_BINDING || pname == GL_READ_FRAMEBUFFER_BINDING) && hr32(p) == g_screen_fbo) hw32(p, 0);
}

static u32 g_str_cache[8];

u32 w2c_env_glGetString(struct w2c_env* e, u32 name) {
  int slot = name == GL_VENDOR ? 0 : name == GL_RENDERER ? 1 : name == GL_VERSION ? 2 : name == GL_EXTENSIONS ? 3
             : name == GL_SHADING_LANGUAGE_VERSION ? 4 : name == 37445 ? 5 : name == 37446 ? 6 : -1;
  if (slot < 0) return 0;
  if (g_str_cache[slot]) return g_str_cache[slot];
  const char* s;
  switch (name) {

    case GL_VERSION: s = "OpenGL ES 3.0 (WebGL 2.0)"; break;
    case GL_SHADING_LANGUAGE_VERSION: s = "OpenGL ES GLSL ES 3.00 (WebGL GLSL ES 3.00)"; break;
    case 37445: s = (const char*)GLF(glGetString)(GL_VENDOR); break;
    case 37446: s = (const char*)GLF(glGetString)(GL_RENDERER); break;
    default: s = (const char*)GLF(glGetString)(name); break;
  }
  g_str_cache[slot] = host_strdup(s ? s : "");
  return g_str_cache[slot];
}
