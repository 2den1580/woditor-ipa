#pragma once

#include <setjmp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "wasm-rt.h"
#include "woditor.h"

struct w2c_env { int unused; };
struct w2c_wasi__snapshot__preview1 { int unused; };

extern w2c_woditor* g_inst;

static inline uint8_t* host_mem(void) { return g_inst->w2c_memory.data; }
static inline uint8_t* hp(u32 off) { return g_inst->w2c_memory.data + off; }
static inline u32 hr32(u32 off) { u32 v; memcpy(&v, hp(off), 4); return v; }
static inline void hw32(u32 off, u32 v) { memcpy(hp(off), &v, 4); }
static inline void hw16(u32 off, uint16_t v) { memcpy(hp(off), &v, 2); }
static inline void hw64(u32 off, u64 v) { memcpy(hp(off), &v, 8); }
static inline void hwf64(u32 off, double v) { memcpy(hp(off), &v, 8); }
static inline void hw8(u32 off, uint8_t v) { *hp(off) = v; }

static inline const char* hstr(u32 off) { return off ? (const char*)hp(off) : ""; }

u32 host_write_str(u32 dst, const char* s, u32 maxBytes);
u32 host_malloc(u32 size);
void host_free(u32 ptr);
u32 host_strdup(const char* s);

u32 host_stack_save(void);
void host_stack_restore(u32 sp);
u32 host_stack_alloc(u32 size);
const wasm_rt_funcref_t* host_table_entry(u32 index);

void host_call_v(u32 index);
void host_call_vi(u32 index, u32 a);
void host_call_viiii(u32 index, u32 a, u32 b, u32 c, u32 d);
u32 host_call_iiii(u32 index, u32 a, u32 b, u32 c);

typedef struct host_eh_frame {
  jmp_buf jb;
  struct host_eh_frame* prev;
} host_eh_frame;
void host_eh_push(host_eh_frame* f);
void host_eh_pop(host_eh_frame* f);
void host_eh_caught(host_eh_frame* f, u32 sp);
_Noreturn void host_eh_throw(void);

void host_log(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
_Noreturn void host_fatal(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void host_unimplemented(const char* name);
double host_now_ms(void);
double host_date_ms(void);

typedef struct host_config {
  const char* root_dir;
  const char* game_dir;
  const char* project_id;
  const char* location;
  const char* lang;
  const char* game_pack;
  const char* game_pack_prefix;
  const char* cache_dir;
  const char* save_dir;

  const char* mod_dirs;
} host_config;
int host_run(const host_config* cfg);

bool host_main_loop_active(void);
void host_main_loop_iter(void);
double host_main_loop_interval_ms(void);

void plat_present(void);
void plat_pump_events(void);
void plat_set_game_size(int w, int h);
void plat_window_text(const char* text);
void plat_game_message(const char* text);
void plat_before_game_start(void);
void plat_vibrate(int input_type, int power, int time_ms, int effect);
void plat_sync_saves(int from_storage);
void plat_exit(int code);
_Noreturn void plat_engine_finished(void);
double plat_canvas_css_width(void);
int plat_load_proc(void* (**getproc)(const char*));
void plat_text_input(bool show, const char* text, int maxlen);

enum {
  LOAD_TEXT = 0,
  LOAD_ARCHIVE = 1,
  LOAD_GAME = 2,
};
void plat_load_status(int stage, const char* text, int value);
void host_load_status(int stage, int value, const char* fmt, ...);

void host_text_input_finish(const char* text, int state);

bool host_error_dialog_hook(u32 a0, u32 a1, u32 a2);

bool host_fs_move_aside(const char* virt_src, const char* virt_dst);
void host_console_write(const uint8_t* data, size_t n);

void host_fs_init(const host_config* cfg);
int host_fs_game_file_available(const char* virt_path);
int host_fs_read_source(const char* rel, u32* out_ptr, u32* out_len);

typedef struct host_pack_file host_pack_file;
void host_name_lower(const char* s, char* out, size_t n);
bool host_name_eq_ci(const char* a, const char* b);
bool host_pack_open(const char* zip, const char* prefix, const char* cache_dir);
bool host_pack_active(void);
bool host_pack_stat(const char* rel, bool* is_dir, u64* size);
time_t host_pack_mtime(void);
void host_pack_list(const char* rel_dir, void (*cb)(void* ud, const char* name), void* ud);
host_pack_file* host_pack_fopen(const char* rel);
host_pack_file* host_pack_fdup(host_pack_file* f);
void host_pack_fclose(host_pack_file* f);
u64 host_pack_fsize(host_pack_file* f);
int64_t host_pack_pread(host_pack_file* f, void* buf, u64 n, u64 off);

void host_gl_init(void);
extern unsigned host_gl_window_fbo;
extern void* host_log_file;
bool host_gl_take_dirty(void);
void host_gl_set_screen_size(int w, int h);
void host_gl_screen_size(int* w, int* h);
void host_gl_present(int win_w, int win_h);
void host_gl_present_last(int win_w, int win_h);
void host_gl_read_screen(uint8_t* rgba);
void host_timers_run(void);
void host_wget_run(void);

void host_input_key(bool down, int dom_key_code, const char* key, const char* code, bool shift, bool ctrl, bool alt);
void host_input_mouse(int type  , int x, int y, int button, int buttons);
void host_input_wheel(int x, int y, double dy);
typedef struct { int id; int x; int y; bool changed; } host_touch;
void host_input_touch(int type  , const host_touch* touches, int n);
void host_gamepad_set(int index, bool connected, const double* axes, int naxes, const double* buttons, int nbuttons,
                      const char* id);

void host_audio_mix(int16_t* out, int frames, int rate);
void host_midi_mix(float* acc, int frames, int rate);
