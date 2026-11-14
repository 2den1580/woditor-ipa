#include "host.h"
#include "host_asm.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#ifdef __ANDROID__
#include <android/log.h>
#endif

#include "wasm-rt-exceptions.h"
#include "wasm-rt-impl.h"

w2c_woditor* g_inst;
uint8_t* w2c_woditor_membase;
static w2c_woditor g_instance;
static struct w2c_env g_env;
static struct w2c_wasi__snapshot__preview1 g_wasi;
static host_config g_cfg;

void* host_log_file;

static void vlog(int error, const char* fmt, va_list ap) {
  static double t0;
  if (!t0) t0 = host_now_ms();
#ifdef __ANDROID__
  char msg[1024];
  vsnprintf(msg, sizeof msg, fmt, ap);
  __android_log_print(error ? ANDROID_LOG_ERROR : ANDROID_LOG_INFO, "Woditor", "[%6.2f] %s",
                      (host_now_ms() - t0) / 1000, msg);
#else
  (void)error;
  char msg[1024];
  vsnprintf(msg, sizeof msg, fmt, ap);
  fprintf(stderr, "[%7.2f] %s\n", (host_now_ms() - t0) / 1000, msg);
  fflush(stderr);
  if (host_log_file) {
    fprintf((FILE*)host_log_file, "[%7.2f] %s\n", (host_now_ms() - t0) / 1000, msg);
    fflush((FILE*)host_log_file);
  }
#endif
}

void host_log(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vlog(0, fmt, ap);
  va_end(ap);
}

static void status_log_text(int stage, const char* code, char* out, size_t n) {
  const char* bar = strchr(code, '|');
  const char* arg = bar ? bar + 1 : "";
  size_t kl = bar ? (size_t)(bar - code) : strlen(code);
  if (stage == LOAD_ARCHIVE) snprintf(out, n, "processing archive %s", code);
  else if (!strncmp(code, "start", kl)) snprintf(out, n, "Starting the engine…");
  else if (!strncmp(code, "check", kl)) snprintf(out, n, "Checking game files…");
  else if (!strncmp(code, "read", kl)) snprintf(out, n, "Reading %s…", arg);
  else if (!strncmp(code, "unpack", kl)) snprintf(out, n, "Unpacking %s…", arg);
  else if (!strncmp(code, "game", kl)) snprintf(out, n, "Starting the game…");
  else snprintf(out, n, "%s", code);
}

static double g_archive_t0;
static char g_archive_name[256];
static bool g_game_starting;
void host_load_status(int stage, int value, const char* fmt, ...) {
  char text[512];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(text, sizeof text, fmt, ap);
  va_end(ap);
  if (g_archive_t0) {
    host_log("archive %s processed in %.1f s", g_archive_name, (host_now_ms() - g_archive_t0) / 1000);
    g_archive_t0 = 0;
  }
  static char last_text[512];
  static int last_dec = -1;
  int dec = value >= 0 && stage == LOAD_TEXT ? value / 10 : -1;
  if (strcmp(text, last_text) || dec != last_dec) {
    char lt[600];
    status_log_text(stage, text, lt, sizeof lt);
    if (stage == LOAD_ARCHIVE) host_log("%s (%d MB)…", lt, value);
    else if (value >= 0 && stage == LOAD_TEXT) host_log("%s %d%%", lt, value);
    else host_log("%s", lt);
    snprintf(last_text, sizeof last_text, "%s", text);
    last_dec = dec;
  }
  if (stage == LOAD_GAME) g_game_starting = true;
  if (stage == LOAD_ARCHIVE) {
    g_archive_t0 = host_now_ms();
    snprintf(g_archive_name, sizeof g_archive_name, "%s", text);
  }
  plat_load_status(stage, text, value);
}

_Noreturn void host_fatal(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vlog(1, fmt, ap);
  va_end(ap);
  abort();
}

void host_unimplemented(const char* name) {
  host_fatal("import not implemented: %s", name);
}

u32 host_write_str(u32 dst, const char* s, u32 maxBytes) {
  if (!maxBytes) return 0;
  size_t n = strlen(s);
  if (n > maxBytes - 1) {
    n = maxBytes - 1;
    while (n && ((unsigned char)s[n] & 0xC0) == 0x80) n--;
  }
  memcpy(hp(dst), s, n);
  hw8(dst + (u32)n, 0);
  return (u32)n;
}

u32 host_malloc(u32 size) { return w2c_woditor_malloc(g_inst, size); }
void host_free(u32 ptr) { w2c_woditor_free(g_inst, ptr); }

u32 host_strdup(const char* s) {
  u32 n = (u32)strlen(s) + 1;
  u32 p = host_malloc(n);
  if (p) memcpy(hp(p), s, n);
  return p;
}

u32 host_stack_save(void) { return w2c_woditor_emscripten_stack_get_current(g_inst); }
void host_stack_restore(u32 sp) { w2c_woditor_0x5Femscripten_stack_restore(g_inst, sp); }
u32 host_stack_alloc(u32 size) { return w2c_woditor_0x5Femscripten_stack_alloc(g_inst, size); }

const wasm_rt_funcref_t* host_table_entry(u32 index) {
  wasm_rt_funcref_table_t* t = &g_inst->w2c_0x5F_indirect_function_table;
  if (index >= t->size || !t->data[index].func) host_fatal("call to missing table function %u", index);
  return &t->data[index];
}

void host_call_v(u32 index) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  ((void (*)(void*))f->func)(f->module_instance);
}

void host_call_vi(u32 index, u32 a) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  ((void (*)(void*, u32))f->func)(f->module_instance, a);
}

void host_call_viiii(u32 index, u32 a, u32 b, u32 c, u32 d) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  ((void (*)(void*, u32, u32, u32, u32))f->func)(f->module_instance, a, b, c, d);
}

u32 host_call_iiii(u32 index, u32 a, u32 b, u32 c) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  return ((u32 (*)(void*, u32, u32, u32))f->func)(f->module_instance, a, b, c);
}

static host_eh_frame* g_eh_top;

void host_eh_push(host_eh_frame* f) {
  f->prev = g_eh_top;
  g_eh_top = f;
}

void host_eh_pop(host_eh_frame* f) { g_eh_top = f->prev; }

void host_eh_caught(host_eh_frame* f, u32 sp) {
  g_eh_top = f->prev;
  host_stack_restore(sp);
  w2c_woditor_setThrew(g_inst, 1, 0);
}

_Noreturn void host_eh_throw(void) {
  if (!g_eh_top) host_fatal("unhandled C++ exception in the engine");
  longjmp(g_eh_top->jb, 1);
}

static u32 g_exc_last;
static int g_uncaught;
static u32 g_caught[64];
static int g_ncaught;

#define EXC_TYPE(p) ((p) - 24 + 4)
#define EXC_DTOR(p) ((p) - 24 + 8)
#define EXC_CAUGHT(p) ((p) - 24 + 12)
#define EXC_RETHROWN(p) ((p) - 24 + 13)
#define EXC_ADJUSTED(p) ((p) - 24 + 16)

void w2c_env_0x5F_cxa_throw(struct w2c_env* env, u32 ptr, u32 type, u32 destructor) {
  hw32(EXC_ADJUSTED(ptr), 0);
  hw32(EXC_TYPE(ptr), type);
  hw32(EXC_DTOR(ptr), destructor);
  w2c_woditor_0x5F_cxa_increment_exception_refcount(g_inst, ptr);
  g_exc_last = ptr;
  g_uncaught++;
  host_eh_throw();
}

u32 w2c_env_0x5F_cxa_begin_catch(struct w2c_env* env, u32 ptr) {
  if (!*hp(EXC_CAUGHT(ptr))) {
    hw8(EXC_CAUGHT(ptr), 1);
    g_uncaught--;
  }
  hw8(EXC_RETHROWN(ptr), 0);
  if (g_ncaught >= (int)(sizeof(g_caught) / sizeof(g_caught[0]))) host_fatal("catch nesting too deep");
  g_caught[g_ncaught++] = ptr;
  return w2c_woditor_0x5F_cxa_get_exception_ptr(g_inst, ptr);
}

void w2c_env_0x5F_cxa_end_catch(struct w2c_env* env) {
  w2c_woditor_setThrew(g_inst, 0, 0);
  if (!g_ncaught) host_fatal("__cxa_end_catch without catch");
  u32 ptr = g_caught[--g_ncaught];
  w2c_woditor_0x5F_cxa_decrement_exception_refcount(g_inst, ptr);
  g_exc_last = 0;
}

void w2c_env_0x5F_cxa_rethrow(struct w2c_env* env) {
  if (!g_ncaught) host_fatal("no exception to throw");
  u32 ptr = g_caught[g_ncaught - 1];
  hw8(EXC_RETHROWN(ptr), 1);
  hw8(EXC_CAUGHT(ptr), 0);
  g_uncaught++;
  w2c_woditor_0x5F_cxa_increment_exception_refcount(g_inst, ptr);
  g_exc_last = ptr;
  host_eh_throw();
}

u32 w2c_env_0x5F_cxa_uncaught_exceptions(struct w2c_env* env) { return (u32)g_uncaught; }

void w2c_env_0x5F_resumeException(struct w2c_env* env, u32 ptr) {
  if (!g_exc_last) g_exc_last = ptr;
  host_eh_throw();
}

void w2c_env_0x5Femscripten_throw_longjmp(struct w2c_env* env) { host_eh_throw(); }

u32 w2c_env_llvm_eh_typeid_for(struct w2c_env* env, u32 type) { return type; }

static u32 find_matching_catch(const u32* types, int n) {
  u32 thrown = g_exc_last;
  if (!thrown) {
    w2c_woditor_0x5Femscripten_tempret_set(g_inst, 0);
    return 0;
  }
  hw32(EXC_ADJUSTED(thrown), thrown);
  u32 thrownType = hr32(EXC_TYPE(thrown));
  if (!thrownType) {
    w2c_woditor_0x5Femscripten_tempret_set(g_inst, 0);
    return thrown;
  }
  for (int i = 0; i < n; i++) {
    u32 caughtType = types[i];
    if (caughtType == 0 || caughtType == thrownType) break;
    if (w2c_woditor_0x5F_cxa_can_catch(g_inst, caughtType, thrownType, EXC_ADJUSTED(thrown))) {
      w2c_woditor_0x5Femscripten_tempret_set(g_inst, caughtType);
      return thrown;
    }
  }
  w2c_woditor_0x5Femscripten_tempret_set(g_inst, thrownType);
  return thrown;
}

u32 w2c_env_0x5F_cxa_find_matching_catch_2(struct w2c_env* env) { return find_matching_catch(NULL, 0); }

u32 w2c_env_0x5F_cxa_find_matching_catch_3(struct w2c_env* env, u32 a) {
  u32 t[1] = {a};
  return find_matching_catch(t, 1);
}

u32 w2c_env_0x5F_cxa_find_matching_catch_4(struct w2c_env* env, u32 a, u32 b) {
  u32 t[2] = {a, b};
  return find_matching_catch(t, 2);
}

double host_now_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

double host_date_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

f64 w2c_env_emscripten_get_now(struct w2c_env* env) { return host_now_ms(); }
f64 w2c_env_emscripten_date_now(struct w2c_env* env) { return host_date_ms(); }

u32 w2c_wasi__snapshot__preview1_clock_time_get(struct w2c_wasi__snapshot__preview1* w, u32 clk, u64 precision, u32 ptime) {
  if (clk > 3) return 28;
  double ms = clk == 0 ? host_date_ms() : host_now_ms();
  hw64(ptime, (u64)(ms * 1e6));
  return 0;
}

static void write_tm(u32 tmPtr, const struct tm* t, bool local) {
  hw32(tmPtr, t->tm_sec);
  hw32(tmPtr + 4, t->tm_min);
  hw32(tmPtr + 8, t->tm_hour);
  hw32(tmPtr + 12, t->tm_mday);
  hw32(tmPtr + 16, t->tm_mon);
  hw32(tmPtr + 20, t->tm_year);
  hw32(tmPtr + 24, t->tm_wday);
  hw32(tmPtr + 28, t->tm_yday);
  if (local) {
    hw32(tmPtr + 32, t->tm_isdst > 0);
    hw32(tmPtr + 36, (u32)t->tm_gmtoff);
  }
}

u32 w2c_env_0x5Fgmtime_js(struct w2c_env* env, u64 time, u32 tmPtr) {
  time_t tt = (time_t)(int64_t)time;
  struct tm t;
  if (!gmtime_r(&tt, &t)) return 1;
  write_tm(tmPtr, &t, false);
  return 0;
}

u32 w2c_env_0x5Flocaltime_js(struct w2c_env* env, u64 time, u32 tmPtr) {
  time_t tt = (time_t)(int64_t)time;
  struct tm t;
  if (!localtime_r(&tt, &t)) return 1;
  write_tm(tmPtr, &t, true);
  return 0;
}

static long tz_offset_min(int year, int mon) {
  struct tm t = {0};
  t.tm_year = year;
  t.tm_mon = mon;
  t.tm_mday = 1;
  t.tm_hour = 12;
  t.tm_isdst = -1;
  time_t tt = mktime(&t);
  struct tm lt;
  localtime_r(&tt, &lt);
  return -lt.tm_gmtoff / 60;
}

void w2c_env_0x5Ftzset_js(struct w2c_env* env, u32 timezone, u32 daylight, u32 std_name, u32 dst_name) {
  time_t now = time(NULL);
  struct tm lt;
  localtime_r(&now, &lt);
  long winter = tz_offset_min(lt.tm_year, 0), summer = tz_offset_min(lt.tm_year, 6);
  long stdOff = winter > summer ? winter : summer;
  hw32(timezone, (u32)(60 * stdOff));
  hw32(daylight, winter != summer);
  char wn[17], sn[17];
  snprintf(wn, sizeof wn, "UTC%c%02ld%02ld", winter >= 0 ? '-' : '+', labs(winter) / 60, labs(winter) % 60);
  snprintf(sn, sizeof sn, "UTC%c%02ld%02ld", summer >= 0 ? '-' : '+', labs(summer) / 60, labs(summer) % 60);
  if (summer < winter) {
    host_write_str(std_name, wn, 17);
    host_write_str(dst_name, sn, 17);
  } else {
    host_write_str(std_name, sn, 17);
    host_write_str(dst_name, wn, 17);
  }
}

static double g_timer_deadline[3];

u32 w2c_env_0x5Fsetitimer_js(struct w2c_env* env, u32 which, f64 timeout_ms) {
  if (which > 2) return 0;
  g_timer_deadline[which] = timeout_ms > 0 ? host_now_ms() + timeout_ms : 0;
  return 0;
}

void host_timers_run(void) {
  for (u32 i = 0; i < 3; i++) {
    if (g_timer_deadline[i] && host_now_ms() >= g_timer_deadline[i]) {
      g_timer_deadline[i] = 0;
      w2c_woditor_0x5Femscripten_timeout(g_inst, i, host_now_ms());
    }
  }
}

static char g_env_strings[8][128];
static int g_env_count;

static void build_env(void) {
  const char* lang = g_cfg.lang && *g_cfg.lang ? g_cfg.lang : "ja_JP";
  g_env_count = 0;
  snprintf(g_env_strings[g_env_count++], 128, "USER=web_user");
  snprintf(g_env_strings[g_env_count++], 128, "LOGNAME=web_user");
  snprintf(g_env_strings[g_env_count++], 128, "PATH=/");
  snprintf(g_env_strings[g_env_count++], 128, "PWD=/");
  snprintf(g_env_strings[g_env_count++], 128, "HOME=/home/web_user");
  snprintf(g_env_strings[g_env_count++], 128, "LANG=%s.UTF-8", lang);
  snprintf(g_env_strings[g_env_count++], 128, "_=./this.program");
}

u32 w2c_wasi__snapshot__preview1_environ_sizes_get(struct w2c_wasi__snapshot__preview1* w, u32 pcount, u32 psize) {
  u32 size = 0;
  for (int i = 0; i < g_env_count; i++) size += (u32)strlen(g_env_strings[i]) + 1;
  hw32(pcount, (u32)g_env_count);
  hw32(psize, size);
  return 0;
}

u32 w2c_wasi__snapshot__preview1_environ_get(struct w2c_wasi__snapshot__preview1* w, u32 environ, u32 buf) {
  u32 off = 0;
  for (int i = 0; i < g_env_count; i++) {
    hw32(environ + 4 * i, buf + off);
    off += host_write_str(buf + off, g_env_strings[i], 0xFFFFFFFFu) + 1;
  }
  return 0;
}

u32 w2c_env_emscripten_resize_heap(struct w2c_env* env, u32 requested) {
  const u64 maxHeap = 2147483648ull;
  u64 old = g_inst->w2c_memory.size;
  if (requested > maxHeap) return 0;
  for (int cut = 1; cut <= 4; cut *= 2) {
    u64 over = (u64)(old * (1 + 0.2 / cut));
    if (over > (u64)requested + 100663296ull) over = (u64)requested + 100663296ull;
    u64 want = requested > over ? requested : over;
    want = (want + 65535) & ~65535ull;
    if (want > maxHeap) want = maxHeap;
    u64 pages = (want - g_inst->w2c_memory.size + 65535) / 65536;
    if (wasm_rt_grow_memory(&g_inst->w2c_memory, pages) != (uint64_t)-1) return 1;
  }
  host_log("failed to grow the engine heap to %u bytes", requested);
  return 0;
}

void w2c_env_0x5Fabort_js(struct w2c_env* env) { host_fatal("engine called abort()"); }
void w2c_env_0x5Femscripten_runtime_keepalive_clear(struct w2c_env* env) {}
static bool g_exited;

__attribute__((weak)) _Noreturn void plat_engine_finished(void) { host_fatal("engine exited"); }

void w2c_env_exit(struct w2c_env* env, u32 code) {
  g_exited = true;
  host_log("engine exited, code %u", code);
  plat_exit((int)code);
}
void w2c_wasi__snapshot__preview1_proc_exit(struct w2c_wasi__snapshot__preview1* w, u32 code) {
  g_exited = true;
  host_log("engine exited (proc_exit), code %u", code);
  plat_exit((int)code);
}

static jmp_buf* g_unwind_target;
static u32 g_loop_func;
static int g_loop_fps;
static bool g_loop_paused;

void w2c_env_emscripten_set_main_loop(struct w2c_env* env, u32 func, u32 fps, u32 simulateInfiniteLoop) {
  g_loop_func = func;
  g_loop_fps = (int)fps;
  g_loop_paused = false;
  host_log("main loop: fps=%d", (int)fps);
  if (simulateInfiniteLoop && g_unwind_target) longjmp(*g_unwind_target, 1);
}

void w2c_env_emscripten_pause_main_loop(struct w2c_env* env) { g_loop_paused = true; }
void w2c_env_emscripten_resume_main_loop(struct w2c_env* env) { g_loop_paused = false; }

bool host_main_loop_active(void) { return g_loop_func && !g_loop_paused; }
double host_main_loop_interval_ms(void) { return g_loop_fps > 0 ? 1000.0 / g_loop_fps : 0; }

static void report_trap(wasm_rt_trap_t code) {
  if (g_exited) {
    host_log("engine finished (%s after exit)", wasm_rt_strerror(code));
    plat_engine_finished();
  }
  host_fatal("engine trap: %s", wasm_rt_strerror(code));
}

void host_main_loop_iter(void) {
  jmp_buf jb;
  jmp_buf* saved = g_unwind_target;
  plat_pump_events();
  host_timers_run();
  host_wget_run();
  if (!host_main_loop_active()) return;
  wasm_rt_trap_t trap = wasm_rt_impl_try();
  if (trap) report_trap(trap);
  g_unwind_target = &jb;
  if (!setjmp(jb)) host_call_v(g_loop_func);
  g_eh_top = NULL;
  g_unwind_target = saved;
  if (host_gl_take_dirty()) plat_present();
}

static void sleep_ms(double ms) {
  plat_pump_events();
  host_timers_run();
  host_wget_run();

  if (host_gl_take_dirty()) plat_present();
  if (ms > 0) {
    struct timespec ts = {(time_t)(ms / 1000), (long)(((long long)ms % 1000) * 1000000)};
    nanosleep(&ts, NULL);
  }
}

void w2c_env_emscripten_sleep(struct w2c_env* env, u32 ms) { sleep_ms(ms); }

void w2c_env_0x5F_asyncjs_0x5FinitializeJS(struct w2c_env* env) {

  host_log("initializeJS: projectId=%s", g_cfg.project_id);
  host_load_status(LOAD_TEXT, -1, "check");
}

void w2c_env_0x5F_asyncjs_0x5FsyncFS(struct w2c_env* env, u32 syncFrom) {
  if (getenv("WODITOR_FSLOG")) host_log("syncFS(%u)", syncFrom);
  plat_sync_saves((int)syncFrom);
}

static char g_cache_dir[256];

u32 w2c_env_0x5F_asyncjs_0x5FtestFileChangedJS(struct w2c_env* env, u32 url, u32 filepath) {

  char virt[1024];
  snprintf(virt, sizeof virt, "%s%s", g_cache_dir, hstr(filepath));
  int avail = host_fs_game_file_available(virt);
  host_log("testFileChanged %s -> %s", hstr(filepath), avail ? "yes" : "no");
  return avail ? 0 : 1;
}

static int asm_kind(u32 code) {
  for (const host_asm_entry* e = host_asm_table; e->code; e++)
    if (e->code == code) return e->kind;
  return ASM_UNKNOWN;
}

static double run_asm(u32 code) {
  switch (asm_kind(code)) {
    case ASM_CANVAS_WIDTH:
      return plat_canvas_css_width();
    case ASM_CHECK_FAILED_0:
      host_log("ERROR: BrowserWoditor.dat must be inside the encrypted archive (browserCheckFailed(0))");
      plat_game_message("browserCheckFailed(0)");
      return 0;
    case ASM_CHECK_FAILED_1:
      host_log("ERROR: the game is not set up for Browser Woditor (browserCheckFailed(1))");
      plat_game_message("browserCheckFailed(1)");
      return 0;
    case ASM_BEFORE_GAME_START:
      plat_before_game_start();
      return 0;
    case ASM_SLEEP_100:
      sleep_ms(100);
      return 0;
    default:
      host_log("unknown EM_ASM %u", code);
      return 0;
  }
}

u32 w2c_env_emscripten_asm_const_int(struct w2c_env* env, u32 code, u32 sig, u32 args) { return (u32)(int)run_asm(code); }
f64 w2c_env_emscripten_asm_const_double(struct w2c_env* env, u32 code, u32 sig, u32 args) { return run_asm(code); }

typedef struct {
  u32 handle, userdata, free, onload, onerror, onprogress;
  char rel[512];
} wget_req;
static wget_req g_wget[64];
static int g_nwget;
static u32 g_wget_next;

static void url_to_rel(const char* url, char* out, size_t n) {
  const char* s = url;
  const char* loc = g_cfg.location ? g_cfg.location : "";
  const char* slash = strrchr(loc, '/');
  size_t baselen = slash ? (size_t)(slash - loc + 1) : 0;
  if (baselen && !strncmp(s, loc, baselen)) s += baselen;
  else if (strstr(s, "://")) {
    s = strstr(s, "://") + 3;
    s = strchr(s, '/') ? strchr(s, '/') + 1 : s + strlen(s);
  }
  while (*s == '/' || (s[0] == '.' && s[1] == '/')) s += (*s == '/') ? 1 : 2;
  size_t o = 0;
  for (; *s && *s != '?' && *s != '#' && o + 1 < n; s++) {
    if (*s == '%' && s[1] && s[2]) {
      char hex[3] = {s[1], s[2], 0};
      out[o++] = (char)strtol(hex, NULL, 16);
      s += 2;
    } else {
      out[o++] = *s;
    }
  }
  out[o] = 0;
}

u32 w2c_env_emscripten_async_wget2_data(struct w2c_env* env, u32 url, u32 request, u32 param, u32 userdata, u32 free,
                                         u32 onload, u32 onerror, u32 onprogress) {
  if (g_nwget >= (int)(sizeof g_wget / sizeof g_wget[0])) host_fatal("too many concurrent downloads");
  wget_req* r = &g_wget[g_nwget++];
  r->handle = g_wget_next++;
  r->userdata = userdata;
  r->free = free;
  r->onload = onload;
  r->onerror = onerror;
  r->onprogress = onprogress;
  url_to_rel(hstr(url), r->rel, sizeof r->rel);
  host_log("download %s %s -> %s", hstr(request), hstr(url), r->rel);
  return r->handle;
}

void host_wget_run(void) {
  while (g_nwget) {
    wget_req r = g_wget[0];
    memmove(&g_wget[0], &g_wget[1], sizeof(wget_req) * (size_t)(--g_nwget));
    u32 ptr = 0, len = 0;
    if (host_fs_read_source(r.rel, &ptr, &len)) {
      if (r.onprogress) host_call_viiii(r.onprogress, r.handle, r.userdata, len, len);
      if (r.onload) host_call_viiii(r.onload, r.handle, r.userdata, ptr, len);
      if (r.free) host_free(ptr);
    } else {
      host_log("no file to download: %s", r.rel);
      if (r.onerror) {
        u32 sp = host_stack_save();
        u32 text = host_stack_alloc(16);
        host_write_str(text, "Not Found", 16);
        host_call_viiii(r.onerror, r.handle, r.userdata, 404, text);
        host_stack_restore(sp);
      }
    }
  }
}

void w2c_env_wget2StartJS(struct w2c_env* env, u32 text) {}
void w2c_env_wget2ProgressJS(struct w2c_env* env, u32 loaded, u32 total, u32 filename) {}
void w2c_env_wget2ErrorJS(struct w2c_env* env, u32 code) { host_log("download error %u", code); }

static bool read_cxx_string(u32 p, char* out, size_t n) {
  u64 size = g_inst->w2c_memory.size;
  if (p < 1024 || p + 12 > size) return false;
  u32 cap = hr32(p + 8), data, len;
  if (cap & 0x80000000u) {
    data = hr32(p);
    len = hr32(p + 4);
  } else {
    data = p;
    len = *hp(p + 11);
  }
  if (len >= n || (u64)data + len > size) return false;
  memcpy(out, hp(data), len);
  out[len] = 0;
  return true;
}

bool host_error_dialog_hook(u32 a0, u32 a1, u32 a2) {

  if (a0 < 1024 || (u64)a0 + 12 > g_inst->w2c_memory.size) return false;
  u32 d = (hr32(a0 + 8) & 0x80000000u) ? hr32(a0) : a0;
  if ((u64)d >= g_inst->w2c_memory.size || *hp(d) != '[') return false;
  char title[64], m1[1024] = "", m2[1024] = "";
  if (!read_cxx_string(a0, title, sizeof title) || !strstr(title, "Error]")) return false;
  read_cxx_string(a1, m1, sizeof m1);
  read_cxx_string(a2, m2, sizeof m2);
  static char last[2200];
  char all[2200];
  snprintf(all, sizeof all, "%s|%s|%s", title, m1, m2);
  bool repeat = !strcmp(all, last);
  snprintf(last, sizeof last, "%s", all);
  if (!repeat) host_log("engine error dialog: %s / %s / %s", title, m1, m2);

  const char* suf = ".anotherGame";
  char* lb = strchr(m2, '[');
  char* rb = lb ? strrchr(lb, ']') : NULL;
  if (!strcmp(title, "[Save Error]") && lb && rb) {
    *rb = 0;
    const char* target = lb + 1;
    size_t tl = strlen(target), sl = strlen(suf);
    if (tl > sl && !strcmp(target + tl - sl, suf)) {
      char src[1024];
      snprintf(src, sizeof src, "%.*s", (int)(tl - sl), target);
      if (host_fs_move_aside(src, target)) {
        host_log("save from another game moved aside: %s -> %s", src, target);
        char msg[1200];
        const char* base = strrchr(src, '/');
        snprintf(msg, sizeof msg, "anotherGame|%s", base ? base + 1 : src);
        plat_game_message(msg);
        last[0] = 0;
        return true;
      }
    }
  }
  if (!repeat) plat_game_message(all);
  return false;
}

void w2c_env_getLocationJS(struct w2c_env* env, u32 ptr, u32 size) {
  host_write_str(ptr, g_cfg.location ? g_cfg.location : "https://localhost/index.html", size);
}

void w2c_env_getProjectId(struct w2c_env* env, u32 ptr, u32 size) { host_write_str(ptr, g_cfg.project_id, size); }

f64 w2c_env_getScreenRateJS(struct w2c_env* env) { return 1.0; }

void w2c_env_setGameSizeJS(struct w2c_env* env, u32 w, u32 h) {
  host_log("game size %ux%u", w, h);
  host_load_status(LOAD_GAME, -1, "game");
  host_gl_set_screen_size((int)w, (int)h);
  plat_set_game_size((int)w, (int)h);
}

void w2c_env_setWindowTextJS(struct w2c_env* env, u32 text) {
  if (text) plat_window_text(hstr(text));
}

void w2c_env_passGameMessageJS(struct w2c_env* env, u32 msg) {
  if (msg) plat_game_message(hstr(msg));
}

void w2c_env_playVibrationJS(struct w2c_env* env, u32 inputType, u32 power, u32 time, u32 effect) {
  plat_vibrate((int)inputType, (int)power, (int)time, (int)effect);
}

static int g_ti_state;
static char g_ti_text[1024];
static int g_ti_maxlen;

void host_text_input_finish(const char* text, int state) {
  snprintf(g_ti_text, sizeof g_ti_text, "%s", text ? text : "");
  g_ti_state = state;
}

void w2c_env_makeTextInputJS(struct w2c_env* env, u32 maxlen) { g_ti_maxlen = (int)maxlen; }
void w2c_env_setTextInputJS(struct w2c_env* env, u32 buf) { snprintf(g_ti_text, sizeof g_ti_text, "%s", hstr(buf)); }
void w2c_env_getTextInputJS(struct w2c_env* env, u32 buf) { host_write_str(buf, g_ti_text, (u32)strlen(g_ti_text) + 1); }
u32 w2c_env_checkTextInputJS(struct w2c_env* env) { return (u32)g_ti_state; }
void w2c_env_setTextInputColorJS(struct w2c_env* env, u32 color) {}
void w2c_env_setTextInputPositionJS(struct w2c_env* env, u32 x, u32 y) {}

void w2c_env_startTextInputJS(struct w2c_env* env) {
  g_ti_state = 0;
  plat_text_input(true, g_ti_text, g_ti_maxlen);
}

void w2c_env_endTextInputJS(struct w2c_env* env) {
  plat_text_input(false, "", 0);
  g_ti_text[0] = 0;
}

static char g_line[4096];
static size_t g_line_len;

void host_console_write(const uint8_t* data, size_t n) {
  for (size_t i = 0; i < n; i++) {
    char c = (char)data[i];
    if (c == '\n' || g_line_len + 1 >= sizeof g_line) {
      g_line[g_line_len] = 0;
      host_log("[engine] %s", g_line);
      g_line_len = 0;
      if (c == '\n') continue;
    }
    if (c != '\r') g_line[g_line_len++] = c;
  }
}

int host_run(const host_config* cfg) {
  g_cfg = *cfg;
  snprintf(g_cache_dir, sizeof g_cache_dir, "/data0/%s-GameCache", cfg->project_id);
  build_env();
  host_load_status(LOAD_TEXT, -1, "start");
  host_fs_init(cfg);

  wasm_rt_init();
  wasm2c_woditor_instantiate(&g_instance, &g_env, &g_wasi);
  g_inst = &g_instance;
  w2c_woditor_membase = g_inst->w2c_memory.data;
  host_log("engine created, memory %llu MB", (unsigned long long)(g_inst->w2c_memory.size >> 20));

  wasm_rt_trap_t trap = wasm_rt_impl_try();
  if (trap) report_trap(trap);

  w2c_woditor_0x5F_wasm_call_ctors(g_inst);

  u32 prog = host_stack_alloc(16);
  host_write_str(prog, "./this.program", 16);
  u32 argv = host_stack_alloc(8);
  hw32(argv, prog);
  hw32(argv + 4, 0);

  jmp_buf jb;
  g_unwind_target = &jb;
  if (!setjmp(jb)) {
    u32 ret = w2c_woditor_main(g_inst, 1, argv);
    host_log("main() returned %u", ret);
    g_unwind_target = NULL;
    return (int)ret;
  }

  g_eh_top = NULL;
  g_unwind_target = NULL;
  host_log("main() handed over to the main loop");

  if (!g_game_starting) host_load_status(LOAD_GAME, -1, "game");
  return 0;
}
