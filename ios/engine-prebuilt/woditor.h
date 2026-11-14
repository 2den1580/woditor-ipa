#ifndef WODITOR_H_GENERATED_
#define WODITOR_H_GENERATED_

#include "wasm-rt.h"

#include <stdint.h>

#ifndef WASM_RT_CORE_TYPES_DEFINED
#define WASM_RT_CORE_TYPES_DEFINED
typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;
typedef float f32;
typedef double f64;
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct w2c_env;
struct w2c_wasi__snapshot__preview1;

typedef struct w2c_woditor {
  struct w2c_env* w2c_env_instance;
  struct w2c_wasi__snapshot__preview1* w2c_wasi__snapshot__preview1_instance;
  u32 w2c_g0;
  u32 w2c_g1;
  u32 w2c_g2;
  u32 w2c_g3;
  wasm_rt_memory_t w2c_memory;
  wasm_rt_funcref_table_t w2c_0x5F_indirect_function_table;
} w2c_woditor;

void wasm2c_woditor_instantiate(w2c_woditor*, struct w2c_env*, struct w2c_wasi__snapshot__preview1*);
void wasm2c_woditor_free(w2c_woditor*);
wasm_rt_func_type_t wasm2c_woditor_get_func_type(uint32_t param_count, uint32_t result_count, ...);

void w2c_env_0x5F_asyncjs_0x5FinitializeJS(struct w2c_env*);

void w2c_env_0x5F_asyncjs_0x5FsyncFS(struct w2c_env*, u32);

u32 w2c_env_0x5F_asyncjs_0x5FtestFileChangedJS(struct w2c_env*, u32, u32);

u32 w2c_env_0x5F_cxa_begin_catch(struct w2c_env*, u32);

void w2c_env_0x5F_cxa_end_catch(struct w2c_env*);

u32 w2c_env_0x5F_cxa_find_matching_catch_2(struct w2c_env*);

u32 w2c_env_0x5F_cxa_find_matching_catch_3(struct w2c_env*, u32);

u32 w2c_env_0x5F_cxa_find_matching_catch_4(struct w2c_env*, u32, u32);

void w2c_env_0x5F_cxa_rethrow(struct w2c_env*);

void w2c_env_0x5F_cxa_throw(struct w2c_env*, u32, u32, u32);

u32 w2c_env_0x5F_cxa_uncaught_exceptions(struct w2c_env*);

void w2c_env_0x5F_resumeException(struct w2c_env*, u32);

u32 w2c_env_0x5F_syscall_chdir(struct w2c_env*, u32);

u32 w2c_env_0x5F_syscall_chmod(struct w2c_env*, u32, u32);

u32 w2c_env_0x5F_syscall_fchmod(struct w2c_env*, u32, u32);

u32 w2c_env_0x5F_syscall_fcntl64(struct w2c_env*, u32, u32, u32);

u32 w2c_env_0x5F_syscall_fstat64(struct w2c_env*, u32, u32);

u32 w2c_env_0x5F_syscall_ftruncate64(struct w2c_env*, u32, u64);

u32 w2c_env_0x5F_syscall_getcwd(struct w2c_env*, u32, u32);

u32 w2c_env_0x5F_syscall_getdents64(struct w2c_env*, u32, u32, u32);

u32 w2c_env_0x5F_syscall_ioctl(struct w2c_env*, u32, u32, u32);

u32 w2c_env_0x5F_syscall_linkat(struct w2c_env*, u32, u32, u32, u32, u32);

u32 w2c_env_0x5F_syscall_lstat64(struct w2c_env*, u32, u32);

u32 w2c_env_0x5F_syscall_mkdirat(struct w2c_env*, u32, u32, u32);

u32 w2c_env_0x5F_syscall_newfstatat(struct w2c_env*, u32, u32, u32, u32);

u32 w2c_env_0x5F_syscall_openat(struct w2c_env*, u32, u32, u32, u32);

u32 w2c_env_0x5F_syscall_readlinkat(struct w2c_env*, u32, u32, u32, u32);

u32 w2c_env_0x5F_syscall_renameat(struct w2c_env*, u32, u32, u32, u32);

u32 w2c_env_0x5F_syscall_rmdir(struct w2c_env*, u32);

u32 w2c_env_0x5F_syscall_stat64(struct w2c_env*, u32, u32);

u32 w2c_env_0x5F_syscall_symlinkat(struct w2c_env*, u32, u32, u32);

u32 w2c_env_0x5F_syscall_unlinkat(struct w2c_env*, u32, u32, u32);

void w2c_env_0x5Fabort_js(struct w2c_env*);

void w2c_env_0x5Femscripten_runtime_keepalive_clear(struct w2c_env*);

void w2c_env_0x5Femscripten_throw_longjmp(struct w2c_env*);

u32 w2c_env_0x5Fgmtime_js(struct w2c_env*, u64, u32);

u32 w2c_env_0x5Flocaltime_js(struct w2c_env*, u64, u32);

u32 w2c_env_0x5Fmmap_js(struct w2c_env*, u32, u32, u32, u32, u64, u32, u32);

u32 w2c_env_0x5Fmunmap_js(struct w2c_env*, u32, u32, u32, u32, u32, u64);

u32 w2c_env_0x5Fsetitimer_js(struct w2c_env*, u32, f64);

void w2c_env_0x5Ftzset_js(struct w2c_env*, u32, u32, u32, u32);

void w2c_env_alBufferData(struct w2c_env*, u32, u32, u32, u32, u32);

void w2c_env_alDeleteBuffers(struct w2c_env*, u32, u32);

void w2c_env_alDeleteSources(struct w2c_env*, u32, u32);

void w2c_env_alGenBuffers(struct w2c_env*, u32, u32);

void w2c_env_alGenSources(struct w2c_env*, u32, u32);

u32 w2c_env_alGetError(struct w2c_env*);

void w2c_env_alGetSourcei(struct w2c_env*, u32, u32, u32);

void w2c_env_alSourcePlay(struct w2c_env*, u32);

void w2c_env_alSourceQueueBuffers(struct w2c_env*, u32, u32, u32);

void w2c_env_alSourceStop(struct w2c_env*, u32);

void w2c_env_alSourceUnqueueBuffers(struct w2c_env*, u32, u32, u32);

void w2c_env_alSourcef(struct w2c_env*, u32, u32, f32);

void w2c_env_alSourcefv(struct w2c_env*, u32, u32, u32);

void w2c_env_alSourcei(struct w2c_env*, u32, u32, u32);

u32 w2c_env_alcCloseDevice(struct w2c_env*, u32);

u32 w2c_env_alcCreateContext(struct w2c_env*, u32, u32);

void w2c_env_alcDestroyContext(struct w2c_env*, u32);

u32 w2c_env_alcGetString(struct w2c_env*, u32, u32);

u32 w2c_env_alcMakeContextCurrent(struct w2c_env*, u32);

u32 w2c_env_alcOpenDevice(struct w2c_env*, u32);

u32 w2c_env_checkTextInputJS(struct w2c_env*);

u32 w2c_env_eglDestroyContext(struct w2c_env*, u32, u32);

u32 w2c_env_eglDestroySurface(struct w2c_env*, u32, u32);

u32 w2c_env_eglMakeCurrent(struct w2c_env*, u32, u32, u32, u32);

u32 w2c_env_eglQuerySurface(struct w2c_env*, u32, u32, u32, u32);

u32 w2c_env_eglTerminate(struct w2c_env*, u32);

f64 w2c_env_emscripten_asm_const_double(struct w2c_env*, u32, u32, u32);

u32 w2c_env_emscripten_asm_const_int(struct w2c_env*, u32, u32, u32);

u32 w2c_env_emscripten_async_wget2_data(struct w2c_env*, u32, u32, u32, u32, u32, u32, u32, u32);

f64 w2c_env_emscripten_date_now(struct w2c_env*);

u32 w2c_env_emscripten_get_gamepad_status(struct w2c_env*, u32, u32);

f64 w2c_env_emscripten_get_now(struct w2c_env*);

u32 w2c_env_emscripten_get_num_gamepads(struct w2c_env*);

void w2c_env_emscripten_pause_main_loop(struct w2c_env*);

u32 w2c_env_emscripten_resize_heap(struct w2c_env*, u32);

void w2c_env_emscripten_resume_main_loop(struct w2c_env*);

u32 w2c_env_emscripten_sample_gamepad_data(struct w2c_env*);

u32 w2c_env_emscripten_set_gamepadconnected_callback_on_thread(struct w2c_env*, u32, u32, u32, u32);

u32 w2c_env_emscripten_set_gamepaddisconnected_callback_on_thread(struct w2c_env*, u32, u32, u32, u32);

u32 w2c_env_emscripten_set_keydown_callback_on_thread(struct w2c_env*, u32, u32, u32, u32, u32);

u32 w2c_env_emscripten_set_keyup_callback_on_thread(struct w2c_env*, u32, u32, u32, u32, u32);

void w2c_env_emscripten_set_main_loop(struct w2c_env*, u32, u32, u32);

u32 w2c_env_emscripten_set_mousedown_callback_on_thread(struct w2c_env*, u32, u32, u32, u32, u32);

u32 w2c_env_emscripten_set_mousemove_callback_on_thread(struct w2c_env*, u32, u32, u32, u32, u32);

u32 w2c_env_emscripten_set_mouseup_callback_on_thread(struct w2c_env*, u32, u32, u32, u32, u32);

u32 w2c_env_emscripten_set_touchcancel_callback_on_thread(struct w2c_env*, u32, u32, u32, u32, u32);

u32 w2c_env_emscripten_set_touchend_callback_on_thread(struct w2c_env*, u32, u32, u32, u32, u32);

u32 w2c_env_emscripten_set_touchmove_callback_on_thread(struct w2c_env*, u32, u32, u32, u32, u32);

u32 w2c_env_emscripten_set_touchstart_callback_on_thread(struct w2c_env*, u32, u32, u32, u32, u32);

u32 w2c_env_emscripten_set_wheel_callback_on_thread(struct w2c_env*, u32, u32, u32, u32, u32);

void w2c_env_emscripten_sleep(struct w2c_env*, u32);

u32 w2c_env_emscripten_webgl_create_context(struct w2c_env*, u32, u32);

u32 w2c_env_emscripten_webgl_make_context_current(struct w2c_env*, u32);

void w2c_env_endTextInputJS(struct w2c_env*);

void w2c_env_exit(struct w2c_env*, u32);

void w2c_env_getLocationJS(struct w2c_env*, u32, u32);

void w2c_env_getProjectId(struct w2c_env*, u32, u32);

f64 w2c_env_getScreenRateJS(struct w2c_env*);

void w2c_env_getTextInputJS(struct w2c_env*, u32);

void w2c_env_glActiveTexture(struct w2c_env*, u32);

void w2c_env_glAttachShader(struct w2c_env*, u32, u32);

void w2c_env_glBindBuffer(struct w2c_env*, u32, u32);

void w2c_env_glBindFramebuffer(struct w2c_env*, u32, u32);

void w2c_env_glBindRenderbuffer(struct w2c_env*, u32, u32);

void w2c_env_glBindTexture(struct w2c_env*, u32, u32);

void w2c_env_glBlendEquationSeparate(struct w2c_env*, u32, u32);

void w2c_env_glBlendFuncSeparate(struct w2c_env*, u32, u32, u32, u32);

u32 w2c_env_glCheckFramebufferStatus(struct w2c_env*, u32);

void w2c_env_glClear(struct w2c_env*, u32);

void w2c_env_glClearColor(struct w2c_env*, f32, f32, f32, f32);

void w2c_env_glCompileShader(struct w2c_env*, u32);

void w2c_env_glCompressedTexImage2D(struct w2c_env*, u32, u32, u32, u32, u32, u32, u32, u32);

u32 w2c_env_glCreateProgram(struct w2c_env*);

u32 w2c_env_glCreateShader(struct w2c_env*, u32);

void w2c_env_glCullFace(struct w2c_env*, u32);

void w2c_env_glDeleteBuffers(struct w2c_env*, u32, u32);

void w2c_env_glDeleteFramebuffers(struct w2c_env*, u32, u32);

void w2c_env_glDeleteProgram(struct w2c_env*, u32);

void w2c_env_glDeleteRenderbuffers(struct w2c_env*, u32, u32);

void w2c_env_glDeleteShader(struct w2c_env*, u32);

void w2c_env_glDeleteTextures(struct w2c_env*, u32, u32);

void w2c_env_glDepthFunc(struct w2c_env*, u32);

void w2c_env_glDepthMask(struct w2c_env*, u32);

void w2c_env_glDisable(struct w2c_env*, u32);

void w2c_env_glDisableVertexAttribArray(struct w2c_env*, u32);

void w2c_env_glDrawArrays(struct w2c_env*, u32, u32, u32);

void w2c_env_glDrawElements(struct w2c_env*, u32, u32, u32, u32);

void w2c_env_glEnable(struct w2c_env*, u32);

void w2c_env_glEnableVertexAttribArray(struct w2c_env*, u32);

void w2c_env_glFramebufferRenderbuffer(struct w2c_env*, u32, u32, u32, u32);

void w2c_env_glFramebufferTexture2D(struct w2c_env*, u32, u32, u32, u32, u32);

void w2c_env_glFrontFace(struct w2c_env*, u32);

void w2c_env_glGenFramebuffers(struct w2c_env*, u32, u32);

void w2c_env_glGenRenderbuffers(struct w2c_env*, u32, u32);

void w2c_env_glGenTextures(struct w2c_env*, u32, u32);

u32 w2c_env_glGetAttribLocation(struct w2c_env*, u32, u32);

void w2c_env_glGetIntegerv(struct w2c_env*, u32, u32);

void w2c_env_glGetShaderInfoLog(struct w2c_env*, u32, u32, u32, u32);

void w2c_env_glGetShaderiv(struct w2c_env*, u32, u32, u32);

u32 w2c_env_glGetString(struct w2c_env*, u32);

u32 w2c_env_glGetUniformLocation(struct w2c_env*, u32, u32);

void w2c_env_glLinkProgram(struct w2c_env*, u32);

void w2c_env_glPixelStorei(struct w2c_env*, u32, u32);

void w2c_env_glReadPixels(struct w2c_env*, u32, u32, u32, u32, u32, u32, u32);

void w2c_env_glRenderbufferStorage(struct w2c_env*, u32, u32, u32, u32);

void w2c_env_glShaderSource(struct w2c_env*, u32, u32, u32, u32);

void w2c_env_glTexImage2D(struct w2c_env*, u32, u32, u32, u32, u32, u32, u32, u32, u32);

void w2c_env_glTexParameteri(struct w2c_env*, u32, u32, u32);

void w2c_env_glTexSubImage2D(struct w2c_env*, u32, u32, u32, u32, u32, u32, u32, u32, u32);

void w2c_env_glUniform1f(struct w2c_env*, u32, f32);

void w2c_env_glUniform1i(struct w2c_env*, u32, u32);

void w2c_env_glUniform2fv(struct w2c_env*, u32, u32, u32);

void w2c_env_glUniform3fv(struct w2c_env*, u32, u32, u32);

void w2c_env_glUniform4f(struct w2c_env*, u32, f32, f32, f32, f32);

void w2c_env_glUniform4fv(struct w2c_env*, u32, u32, u32);

void w2c_env_glUseProgram(struct w2c_env*, u32);

void w2c_env_glVertexAttribPointer(struct w2c_env*, u32, u32, u32, u32, u32, u32);

void w2c_env_glViewport(struct w2c_env*, u32, u32, u32, u32);

u32 w2c_env_invoke_i(struct w2c_env*, u32);

u32 w2c_env_invoke_iffddiiii(struct w2c_env*, u32, f32, f32, f64, f64, u32, u32, u32, u32);

u32 w2c_env_invoke_iffffffffii(struct w2c_env*, u32, f32, f32, f32, f32, f32, f32, f32, f32, u32, u32);

u32 w2c_env_invoke_ii(struct w2c_env*, u32, u32);

u32 w2c_env_invoke_iii(struct w2c_env*, u32, u32, u32);

u32 w2c_env_invoke_iiidddiiii(struct w2c_env*, u32, u32, u32, f64, f64, f64, u32, u32, u32, u32);

u32 w2c_env_invoke_iiii(struct w2c_env*, u32, u32, u32, u32);

u32 w2c_env_invoke_iiiii(struct w2c_env*, u32, u32, u32, u32, u32);

u32 w2c_env_invoke_iiiiii(struct w2c_env*, u32, u32, u32, u32, u32, u32);

u32 w2c_env_invoke_iiiiiii(struct w2c_env*, u32, u32, u32, u32, u32, u32, u32);

u32 w2c_env_invoke_iiiiiiii(struct w2c_env*, u32, u32, u32, u32, u32, u32, u32, u32);

u32 w2c_env_invoke_iiiiiiiii(struct w2c_env*, u32, u32, u32, u32, u32, u32, u32, u32, u32);

u32 w2c_env_invoke_iiiiiiiiii(struct w2c_env*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32);

u32 w2c_env_invoke_iiiiiiiiiiii(struct w2c_env*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32);

u32 w2c_env_invoke_iiiiij(struct w2c_env*, u32, u32, u32, u32, u32, u64);

u32 w2c_env_invoke_iiiij(struct w2c_env*, u32, u32, u32, u32, u64);

u32 w2c_env_invoke_iiij(struct w2c_env*, u32, u32, u32, u64);

u32 w2c_env_invoke_iiijiiii(struct w2c_env*, u32, u32, u32, u64, u32, u32, u32, u32);

u32 w2c_env_invoke_iij(struct w2c_env*, u32, u32, u64);

u32 w2c_env_invoke_iiji(struct w2c_env*, u32, u32, u64, u32);

u64 w2c_env_invoke_ji(struct w2c_env*, u32, u32);

void w2c_env_invoke_v(struct w2c_env*, u32);

void w2c_env_invoke_vd(struct w2c_env*, u32, f64);

void w2c_env_invoke_vi(struct w2c_env*, u32, u32);

void w2c_env_invoke_vii(struct w2c_env*, u32, u32, u32);

void w2c_env_invoke_viiddd(struct w2c_env*, u32, u32, u32, f64, f64, f64);

void w2c_env_invoke_viii(struct w2c_env*, u32, u32, u32, u32);

void w2c_env_invoke_viiidd(struct w2c_env*, u32, u32, u32, u32, f64, f64);

void w2c_env_invoke_viiii(struct w2c_env*, u32, u32, u32, u32, u32);

void w2c_env_invoke_viiiii(struct w2c_env*, u32, u32, u32, u32, u32, u32);

void w2c_env_invoke_viiiiii(struct w2c_env*, u32, u32, u32, u32, u32, u32, u32);

void w2c_env_invoke_viiiiiii(struct w2c_env*, u32, u32, u32, u32, u32, u32, u32, u32);

void w2c_env_invoke_viiiiiiii(struct w2c_env*, u32, u32, u32, u32, u32, u32, u32, u32, u32);

void w2c_env_invoke_viiiiiiiii(struct w2c_env*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32);

void w2c_env_invoke_viiiiiiiiii(struct w2c_env*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32);

void w2c_env_invoke_viiiiiiiiiiiii(struct w2c_env*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32);

void w2c_env_invoke_viiiiiiiiiiiiiii(struct w2c_env*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32);

void w2c_env_invoke_viiiiiiiiiiiiiiii(struct w2c_env*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32);

void w2c_env_invoke_viiiiiiiiiiiiiiiiii(struct w2c_env*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32);

void w2c_env_invoke_viiiiiiiiiiiiiiiiiii(struct w2c_env*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32);

void w2c_env_invoke_viiiiiiiiiiiiiiiiiiiii(struct w2c_env*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32);

void w2c_env_invoke_viiiiiiiiiiiiiiiiiiiiiiii(struct w2c_env*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32);

void w2c_env_invoke_viiiiiiiiiiiiiiiiiiiiiiiiii(struct w2c_env*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32);

void w2c_env_invoke_viiiiijjii(struct w2c_env*, u32, u32, u32, u32, u32, u32, u64, u64, u32, u32);

void w2c_env_invoke_viiij(struct w2c_env*, u32, u32, u32, u32, u64);

void w2c_env_invoke_vijj(struct w2c_env*, u32, u32, u64, u64);

void w2c_env_invoke_vji(struct w2c_env*, u32, u64, u32);

u32 w2c_env_llvm_eh_typeid_for(struct w2c_env*, u32);

void w2c_env_makeTextInputJS(struct w2c_env*, u32);

void w2c_env_passGameMessageJS(struct w2c_env*, u32);

void w2c_env_playMidiFile(struct w2c_env*, u32, u32, u32, u32);

void w2c_env_playVibrationJS(struct w2c_env*, u32, u32, u32, u32);

void w2c_env_setGameSizeJS(struct w2c_env*, u32, u32);

void w2c_env_setTextInputColorJS(struct w2c_env*, u32);

void w2c_env_setTextInputJS(struct w2c_env*, u32);

void w2c_env_setTextInputPositionJS(struct w2c_env*, u32, u32);

void w2c_env_setVolumeMidiFile(struct w2c_env*, f32);

void w2c_env_setWindowTextJS(struct w2c_env*, u32);

void w2c_env_startTextInputJS(struct w2c_env*);

void w2c_env_stopMidiFile(struct w2c_env*);

void w2c_env_wget2ErrorJS(struct w2c_env*, u32);

void w2c_env_wget2ProgressJS(struct w2c_env*, u32, u32, u32);

void w2c_env_wget2StartJS(struct w2c_env*, u32);

u32 w2c_wasi__snapshot__preview1_clock_time_get(struct w2c_wasi__snapshot__preview1*, u32, u64, u32);

u32 w2c_wasi__snapshot__preview1_environ_get(struct w2c_wasi__snapshot__preview1*, u32, u32);

u32 w2c_wasi__snapshot__preview1_environ_sizes_get(struct w2c_wasi__snapshot__preview1*, u32, u32);

u32 w2c_wasi__snapshot__preview1_fd_close(struct w2c_wasi__snapshot__preview1*, u32);

u32 w2c_wasi__snapshot__preview1_fd_fdstat_get(struct w2c_wasi__snapshot__preview1*, u32, u32);

u32 w2c_wasi__snapshot__preview1_fd_read(struct w2c_wasi__snapshot__preview1*, u32, u32, u32, u32);

u32 w2c_wasi__snapshot__preview1_fd_seek(struct w2c_wasi__snapshot__preview1*, u32, u64, u32, u32);

u32 w2c_wasi__snapshot__preview1_fd_write(struct w2c_wasi__snapshot__preview1*, u32, u32, u32, u32);

void w2c_wasi__snapshot__preview1_proc_exit(struct w2c_wasi__snapshot__preview1*, u32);

wasm_rt_memory_t* w2c_woditor_memory(w2c_woditor* instance);

void w2c_woditor_0x5F_wasm_call_ctors(w2c_woditor*);

void w2c_woditor_free(w2c_woditor*, u32);

u32 w2c_woditor_malloc(w2c_woditor*, u32);

u32 w2c_woditor_main(w2c_woditor*, u32, u32);

void w2c_woditor_js_pause_main_loop(w2c_woditor*);

void w2c_woditor_js_resume_main_loop(w2c_woditor*);

u32 w2c_woditor_fflush(w2c_woditor*, u32);

wasm_rt_funcref_table_t* w2c_woditor_0x5F_indirect_function_table(w2c_woditor* instance);

void w2c_woditor_0x5F_funcs_on_exit(w2c_woditor*);

u32 w2c_woditor_emscripten_builtin_memalign(w2c_woditor*, u32, u32);

void w2c_woditor_0x5Femscripten_timeout(w2c_woditor*, u32, f64);

void w2c_woditor_setThrew(w2c_woditor*, u32, u32);

void w2c_woditor_0x5Femscripten_tempret_set(w2c_woditor*, u32);

void w2c_woditor_0x5Femscripten_stack_restore(w2c_woditor*, u32);

u32 w2c_woditor_0x5Femscripten_stack_alloc(w2c_woditor*, u32);

u32 w2c_woditor_emscripten_stack_get_current(w2c_woditor*);

void w2c_woditor_0x5F_cxa_decrement_exception_refcount(w2c_woditor*, u32);

void w2c_woditor_0x5F_cxa_increment_exception_refcount(w2c_woditor*, u32);

u32 w2c_woditor_0x5F_cxa_can_catch(w2c_woditor*, u32, u32, u32);

u32 w2c_woditor_0x5F_cxa_get_exception_ptr(w2c_woditor*, u32);

void w2c_woditor_dynCall_vi(w2c_woditor*, u32, u32);

void w2c_woditor_dynCall_vii(w2c_woditor*, u32, u32, u32);

void w2c_woditor_dynCall_viiiii(w2c_woditor*, u32, u32, u32, u32, u32, u32);

void w2c_woditor_dynCall_viiii(w2c_woditor*, u32, u32, u32, u32, u32);

void w2c_woditor_dynCall_viiiiiii(w2c_woditor*, u32, u32, u32, u32, u32, u32, u32, u32);

void w2c_woditor_dynCall_viii(w2c_woditor*, u32, u32, u32, u32);

u32 w2c_woditor_dynCall_ii(w2c_woditor*, u32, u32);

u32 w2c_woditor_dynCall_iiiii(w2c_woditor*, u32, u32, u32, u32, u32);

u32 w2c_woditor_dynCall_iiji(w2c_woditor*, u32, u32, u64, u32);

u64 w2c_woditor_dynCall_ji(w2c_woditor*, u32, u32);

u32 w2c_woditor_dynCall_iii(w2c_woditor*, u32, u32, u32);

void w2c_woditor_dynCall_v(w2c_woditor*, u32);

u32 w2c_woditor_dynCall_iiii(w2c_woditor*, u32, u32, u32, u32);

u32 w2c_woditor_dynCall_iiiiii(w2c_woditor*, u32, u32, u32, u32, u32, u32);

u32 w2c_woditor_dynCall_iiiiiii(w2c_woditor*, u32, u32, u32, u32, u32, u32, u32);

void w2c_woditor_dynCall_viiiiii(w2c_woditor*, u32, u32, u32, u32, u32, u32, u32);

void w2c_woditor_dynCall_vijji(w2c_woditor*, u32, u32, u64, u64, u32);

u32 w2c_woditor_dynCall_i(w2c_woditor*, u32);

void w2c_woditor_dynCall_vd(w2c_woditor*, u32, f64);

void w2c_woditor_dynCall_viiiiiiiii(w2c_woditor*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32);

void w2c_woditor_dynCall_viiij(w2c_woditor*, u32, u32, u32, u32, u64);

void w2c_woditor_dynCall_viiidd(w2c_woditor*, u32, u32, u32, u32, f64, f64);

u32 w2c_woditor_dynCall_iiiiiiiiii(w2c_woditor*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32);

u32 w2c_woditor_dynCall_iiiiiiii(w2c_woditor*, u32, u32, u32, u32, u32, u32, u32, u32);

void w2c_woditor_dynCall_vji(w2c_woditor*, u32, u64, u32);

void w2c_woditor_dynCall_viiiiiiiiii(w2c_woditor*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32);

void w2c_woditor_dynCall_viiiiiiiiiiiiiiiiiiiiiiiiii(w2c_woditor*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32);

u32 w2c_woditor_dynCall_iiidddiiii(w2c_woditor*, u32, u32, u32, f64, f64, f64, u32, u32, u32, u32);

u32 w2c_woditor_dynCall_iffddiiii(w2c_woditor*, u32, f32, f32, f64, f64, u32, u32, u32, u32);

u32 w2c_woditor_dynCall_iij(w2c_woditor*, u32, u32, u64);

u32 w2c_woditor_dynCall_iiiiij(w2c_woditor*, u32, u32, u32, u32, u32, u64);

u32 w2c_woditor_dynCall_iiiiiiiii(w2c_woditor*, u32, u32, u32, u32, u32, u32, u32, u32, u32);

void w2c_woditor_dynCall_viiiiiiiiiiiiiiii(w2c_woditor*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32);

void w2c_woditor_dynCall_viiiiiiiiiiiiiiiiiii(w2c_woditor*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32);

void w2c_woditor_dynCall_viiiiiiiiiiiiiiiiiiiii(w2c_woditor*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32);

void w2c_woditor_dynCall_viiddd(w2c_woditor*, u32, u32, u32, f64, f64, f64);

void w2c_woditor_dynCall_viiiiiiii(w2c_woditor*, u32, u32, u32, u32, u32, u32, u32, u32, u32);

void w2c_woditor_dynCall_viiiiiiiiiiiii(w2c_woditor*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32);

void w2c_woditor_dynCall_viiiiiiiiiiiiiii(w2c_woditor*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32);

void w2c_woditor_dynCall_viiiiiiiiiiiiiiiiii(w2c_woditor*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32);

void w2c_woditor_dynCall_viiiiiiiiiiiiiiiiiiiiiiii(w2c_woditor*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32);

void w2c_woditor_dynCall_vijj(w2c_woditor*, u32, u32, u64, u64);

u32 w2c_woditor_dynCall_iffffffffii(w2c_woditor*, u32, f32, f32, f32, f32, f32, f32, f32, f32, u32, u32);

u32 w2c_woditor_dynCall_iiijiiii(w2c_woditor*, u32, u32, u32, u64, u32, u32, u32, u32);

void w2c_woditor_dynCall_viiiiijjii(w2c_woditor*, u32, u32, u32, u32, u32, u32, u64, u64, u32, u32);

u32 w2c_woditor_dynCall_iiiij(w2c_woditor*, u32, u32, u32, u32, u64);

u32 w2c_woditor_dynCall_iidiiiii(w2c_woditor*, u32, u32, f64, u32, u32, u32, u32, u32);

u64 w2c_woditor_dynCall_jiji(w2c_woditor*, u32, u32, u64, u32);

u32 w2c_woditor_dynCall_iiij(w2c_woditor*, u32, u32, u32, u64);

u32 w2c_woditor_dynCall_iiiiiiiiiiii(w2c_woditor*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32);

void w2c_woditor_dynCall_vijjiii(w2c_woditor*, u32, u32, u64, u64, u32, u32, u32);

void w2c_woditor_dynCall_vijjii(w2c_woditor*, u32, u32, u64, u64, u32, u32);

void w2c_woditor_dynCall_viijji(w2c_woditor*, u32, u32, u32, u64, u64, u32);

u32 w2c_woditor_dynCall_iiiijji(w2c_woditor*, u32, u32, u32, u32, u64, u64, u32);

u64 w2c_woditor_dynCall_jij(w2c_woditor*, u32, u32, u64);

f64 w2c_woditor_dynCall_dij(w2c_woditor*, u32, u32, u64);

void w2c_woditor_dynCall_viijii(w2c_woditor*, u32, u32, u32, u64, u32, u32);

u32 w2c_woditor_dynCall_iiiiid(w2c_woditor*, u32, u32, u32, u32, u32, f64);

u32 w2c_woditor_dynCall_iiiiijj(w2c_woditor*, u32, u32, u32, u32, u32, u64, u64);

u32 w2c_woditor_dynCall_iiiiiijj(w2c_woditor*, u32, u32, u32, u32, u32, u32, u64, u64);

void w2c_woditor_asyncify_start_unwind(w2c_woditor*, u32);

void w2c_woditor_asyncify_stop_unwind(w2c_woditor*);

void w2c_woditor_asyncify_start_rewind(w2c_woditor*, u32);

void w2c_woditor_asyncify_stop_rewind(w2c_woditor*);

u32 w2c_woditor_asyncify_get_state(w2c_woditor*);

#ifdef __cplusplus
}
#endif

#endif
