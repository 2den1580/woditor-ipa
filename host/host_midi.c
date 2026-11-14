#include "host.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>

#if __has_include("benten.h")
#include "benten.h"
#include "wasm-rt-exceptions.h"
#include "wasm-rt-impl.h"

struct w2c_wbg { float* obj[8]; };

enum { OBJ_SF2 = 1, OBJ_MID = 2, OBJ_LEFT = 3, OBJ_RIGHT = 4 };

static w2c_benten g_bt;
static struct w2c_wbg g_wbg;
static bool g_bt_ready, g_bt_broken, g_bt_playing;

void w2c_wbg_0x5F_wbindgen_copy_to_typed_array(struct w2c_wbg* w, u32 ptr, u32 len, u32 obj) {
  if (obj < 8 && w->obj[obj]) memcpy(w->obj[obj], g_bt.w2c_memory.data + ptr, len);
}
void w2c_wbg_0x5F_wbindgen_object_drop_ref(struct w2c_wbg* w, u32 obj) {}

static u8* bt_mem(u32 off) { return g_bt.w2c_memory.data + off; }

#define BT_TRY(on_trap)                                          \
  wasm_rt_jmp_buf saved_jb;                                      \
  memcpy(&saved_jb, &g_wasm_rt_jmp_buf, sizeof saved_jb);        \
  WASM_RT_UNWIND_TARGET* saved_ut = wasm_rt_get_unwind_target(); \
  wasm_rt_trap_t trap = wasm_rt_impl_try();                      \
  if (trap) {                                                    \
    BT_END();                                                    \
    host_log("MIDI: synth error (%s) — MIDI disabled", wasm_rt_strerror(trap)); \
    g_bt_broken = true;                                          \
    g_bt_playing = false;                                        \
    on_trap;                                                     \
  }
#define BT_END() (memcpy(&g_wasm_rt_jmp_buf, &saved_jb, sizeof saved_jb), wasm_rt_set_unwind_target(saved_ut))

static u32 bt_copy_in(const u8* src, u32 len) {
  u32 p = w2c_benten_0x5F_wbindgen_malloc(&g_bt, len ? len : 1, 1);
  memcpy(bt_mem(p), src, len);
  return p;
}

#define BT_RATE 44100
#define RING 16384
#define RING_AHEAD 4096
#define CHUNK 256
static float g_ring[RING * 2];
static atomic_uint g_wr, g_rd;
static atomic_uint g_gen, g_gen_at;
static _Atomic float g_volume = 0.5f;

enum { CMD_NONE, CMD_PLAY, CMD_STOP };
static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_cv = PTHREAD_COND_INITIALIZER;
static bool g_thread_on;
static int g_cmd;
static u8 *g_cmd_sf2, *g_cmd_mid;
static u32 g_cmd_sf2_len, g_cmd_mid_len;

static void flush_ring(void) {
  atomic_store_explicit(&g_gen_at, atomic_load_explicit(&g_wr, memory_order_relaxed), memory_order_relaxed);
  atomic_fetch_add_explicit(&g_gen, 1, memory_order_release);
}

static void bt_start(u8* sf2, u32 sf2_len, u8* mid, u32 mid_len) {
  if (!g_bt_ready && !g_bt_broken) {
#if UINTPTR_MAX <= 0xffffffff || WODITOR_MMAP_LIMITED

    extern uint64_t wasm_rt_mmap32_max, wasm_rt_mmap32_headroom;
    uint64_t max = wasm_rt_mmap32_max, room = wasm_rt_mmap32_headroom;
    wasm_rt_mmap32_max = 256u << 20;
    wasm_rt_mmap32_headroom = 0;
    wasm2c_benten_instantiate(&g_bt, &g_wbg);
    wasm_rt_mmap32_max = max;
    wasm_rt_mmap32_headroom = room;
#else
    wasm2c_benten_instantiate(&g_bt, &g_wbg);
#endif
    g_bt_ready = true;
  }
  if (g_bt_broken) return;
  BT_TRY(return);
  if (g_bt_playing) w2c_benten_bt_stop(&g_bt);
  g_bt_playing = false;
  u32 ps = bt_copy_in(sf2, sf2_len), pm = bt_copy_in(mid, mid_len);
  w2c_benten_bt_play(&g_bt, ps, sf2_len, OBJ_SF2, pm, mid_len, OBJ_MID);
  g_bt_playing = true;
  BT_END();
}

static void bt_halt(void) {
  if (!g_bt_ready || g_bt_broken || !g_bt_playing) return;
  BT_TRY(return);
  w2c_benten_bt_stop(&g_bt);
  g_bt_playing = false;
  BT_END();
}

static void render_ahead(void) {
  static float l[CHUNK], r[CHUNK];
  BT_TRY(return);
  for (;;) {
    unsigned wr = atomic_load_explicit(&g_wr, memory_order_relaxed);
    unsigned rd = atomic_load_explicit(&g_rd, memory_order_acquire);
    unsigned gen_at = atomic_load_explicit(&g_gen_at, memory_order_relaxed);
    unsigned filled = wr - rd;
    if (wr - gen_at < filled) filled = wr - gen_at;
    if (filled >= RING_AHEAD || wr - rd > RING - CHUNK) break;
    u32 bytes = CHUNK * 4;
    u32 pl = w2c_benten_0x5F_wbindgen_malloc(&g_bt, bytes, 4);
    memset(bt_mem(pl), 0, bytes);
    u32 pr = w2c_benten_0x5F_wbindgen_malloc(&g_bt, bytes, 4);
    memset(bt_mem(pr), 0, bytes);
    g_wbg.obj[OBJ_LEFT] = l;
    g_wbg.obj[OBJ_RIGHT] = r;
    w2c_benten_bt_process(&g_bt, pl, CHUNK, OBJ_LEFT, pr, CHUNK, OBJ_RIGHT);
    for (int i = 0; i < CHUNK; i++) {
      unsigned k = (wr + (unsigned)i) & (RING - 1);
      g_ring[2 * k] = l[i];
      g_ring[2 * k + 1] = r[i];
    }
    atomic_store_explicit(&g_wr, wr + CHUNK, memory_order_release);
  }
  BT_END();
}

static void* midi_thread(void* arg) {
  setpriority(PRIO_PROCESS, 0, -16);
  for (;;) {
    pthread_mutex_lock(&g_mu);
    if (g_cmd == CMD_NONE) {

      if (g_bt_playing) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_nsec += 10 * 1000000;
        if (ts.tv_nsec >= 1000000000) ts.tv_sec++, ts.tv_nsec -= 1000000000;
        pthread_cond_timedwait(&g_cv, &g_mu, &ts);
      } else {
        pthread_cond_wait(&g_cv, &g_mu);
      }
    }
    int cmd = g_cmd;
    u8 *sf2 = g_cmd_sf2, *mid = g_cmd_mid;
    u32 sf2_len = g_cmd_sf2_len, mid_len = g_cmd_mid_len;
    g_cmd = CMD_NONE;
    g_cmd_sf2 = g_cmd_mid = NULL;
    pthread_mutex_unlock(&g_mu);

    if (cmd != CMD_NONE) {
      bt_halt();
      flush_ring();
    }
    if (cmd == CMD_PLAY) {
      double t0 = host_now_ms();
      bt_start(sf2, sf2_len, mid, mid_len);
      if (g_bt_playing)
        host_log("MIDI: playing (%u bytes, soundfont %u bytes, prepared in %.0f ms)", mid_len, sf2_len,
                 host_now_ms() - t0);
    }
    free(sf2);
    free(mid);
    if (g_bt_playing) render_ahead();
  }
  return NULL;
}

static void post(int cmd, u8* sf2, u32 sf2_len, u8* mid, u32 mid_len) {
  pthread_mutex_lock(&g_mu);
  if (!g_thread_on) {
    pthread_t t;
    pthread_attr_t a;
    pthread_attr_init(&a);
    pthread_attr_setstacksize(&a, 1 << 20);
    pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
    g_thread_on = pthread_create(&t, &a, midi_thread, NULL) == 0;
    pthread_attr_destroy(&a);
    if (!g_thread_on) host_log("MIDI: synth thread failed to start");
  }
  free(g_cmd_sf2);
  free(g_cmd_mid);
  g_cmd = cmd;
  g_cmd_sf2 = sf2;
  g_cmd_sf2_len = sf2_len;
  g_cmd_mid = mid;
  g_cmd_mid_len = mid_len;
  pthread_cond_signal(&g_cv);
  pthread_mutex_unlock(&g_mu);
}

void w2c_env_playMidiFile(struct w2c_env* e, u32 sf2, u32 sf2Len, u32 mid, u32 len) {
  u8* s = malloc(sf2Len ? sf2Len : 1);
  u8* m = malloc(len ? len : 1);
  if (!s || !m) {
    free(s);
    free(m);
    return;
  }
  memcpy(s, hp(sf2), sf2Len);
  memcpy(m, hp(mid), len);
  post(CMD_PLAY, s, sf2Len, m, len);
}

void w2c_env_stopMidiFile(struct w2c_env* e) {
  if (g_thread_on) post(CMD_STOP, NULL, 0, NULL, 0);
}

void w2c_env_setVolumeMidiFile(struct w2c_env* e, f32 v) {
  atomic_store_explicit(&g_volume, v * 0.5f, memory_order_relaxed);
}

static float g_h[4][2];
static double g_t;
static unsigned g_seen_gen;
static int g_quiet;

static inline float cubic(float x0, float x1, float x2, float x3, float t) {
  return x1 + 0.5f * t * (x2 - x0 + t * (2 * x0 - 5 * x1 + 4 * x2 - x3 + t * (3 * (x1 - x2) + x3 - x0)));
}

void host_midi_mix(float* acc, int frames, int rate) {
  unsigned gen = atomic_load_explicit(&g_gen, memory_order_acquire);
  unsigned rd = atomic_load_explicit(&g_rd, memory_order_relaxed);
  if (gen != g_seen_gen) {
    g_seen_gen = gen;
    rd = atomic_load_explicit(&g_gen_at, memory_order_relaxed);
    memset(g_h, 0, sizeof g_h);
    g_t = 0;
    g_quiet = 4;
  }
  unsigned wr = atomic_load_explicit(&g_wr, memory_order_acquire);
  if (wr == rd && g_quiet >= 4) {
    atomic_store_explicit(&g_rd, rd, memory_order_release);
    return;
  }
  float vol = atomic_load_explicit(&g_volume, memory_order_relaxed);
  double step = (double)BT_RATE / (rate > 0 ? rate : BT_RATE);
  for (int f = 0; f < frames; f++) {
    while (g_t >= 1.0) {
      g_t -= 1.0;
      memmove(g_h[0], g_h[1], sizeof g_h[0] * 3);
      if (rd != wr) {
        unsigned k = rd & (RING - 1);
        g_h[3][0] = g_ring[2 * k];
        g_h[3][1] = g_ring[2 * k + 1];
        rd++;
        g_quiet = 0;
      } else {
        g_h[3][0] = g_h[3][1] = 0;
        if (g_quiet < 4) g_quiet++;
      }
    }
    float t = (float)g_t;
    acc[2 * f] += cubic(g_h[0][0], g_h[1][0], g_h[2][0], g_h[3][0], t) * vol;
    acc[2 * f + 1] += cubic(g_h[0][1], g_h[1][1], g_h[2][1], g_h[3][1], t) * vol;
    g_t += step;
  }
  atomic_store_explicit(&g_rd, rd, memory_order_release);
}

#else

void w2c_env_playMidiFile(struct w2c_env* e, u32 sf2, u32 sf2Len, u32 mid, u32 len) {
  host_log("MIDI: no synth (engine/benten.c) — no music");
}
void w2c_env_stopMidiFile(struct w2c_env* e) {}
void w2c_env_setVolumeMidiFile(struct w2c_env* e, f32 v) {}
void host_midi_mix(float* acc, int frames, int rate) {}

#endif
