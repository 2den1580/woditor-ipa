#include "host.h"

#include <stdio.h>
#include <stdlib.h>

enum { EV_KEYDOWN = 2, EV_KEYUP = 3, EV_MOUSEDOWN = 5, EV_MOUSEUP = 6, EV_MOUSEMOVE = 8, EV_WHEEL = 9,
       EV_TOUCHSTART = 22, EV_TOUCHEND = 23, EV_TOUCHMOVE = 24, EV_TOUCHCANCEL = 25, EV_MAX = 32 };

typedef struct {
  u32 func;
  u32 userData;
} callback;
static callback g_cb[EV_MAX];
static u32 g_key_ev, g_mouse_ev, g_wheel_ev, g_touch_ev;

static u32 reg(int type, u32 userData, u32 func) {
  if (getenv("WODITOR_REGLOG")) host_log("callback type %d -> table %u", type, func);
  g_cb[type].func = func;
  g_cb[type].userData = userData;
  return 0;
}

u32 w2c_env_emscripten_set_keydown_callback_on_thread(struct w2c_env* e, u32 t, u32 ud, u32 cap, u32 f, u32 th) { return reg(EV_KEYDOWN, ud, f); }
u32 w2c_env_emscripten_set_keyup_callback_on_thread(struct w2c_env* e, u32 t, u32 ud, u32 cap, u32 f, u32 th) { return reg(EV_KEYUP, ud, f); }
u32 w2c_env_emscripten_set_mousedown_callback_on_thread(struct w2c_env* e, u32 t, u32 ud, u32 cap, u32 f, u32 th) { return reg(EV_MOUSEDOWN, ud, f); }
u32 w2c_env_emscripten_set_mouseup_callback_on_thread(struct w2c_env* e, u32 t, u32 ud, u32 cap, u32 f, u32 th) { return reg(EV_MOUSEUP, ud, f); }
u32 w2c_env_emscripten_set_mousemove_callback_on_thread(struct w2c_env* e, u32 t, u32 ud, u32 cap, u32 f, u32 th) { return reg(EV_MOUSEMOVE, ud, f); }
u32 w2c_env_emscripten_set_wheel_callback_on_thread(struct w2c_env* e, u32 t, u32 ud, u32 cap, u32 f, u32 th) { return reg(EV_WHEEL, ud, f); }
u32 w2c_env_emscripten_set_touchstart_callback_on_thread(struct w2c_env* e, u32 t, u32 ud, u32 cap, u32 f, u32 th) { return reg(EV_TOUCHSTART, ud, f); }
u32 w2c_env_emscripten_set_touchend_callback_on_thread(struct w2c_env* e, u32 t, u32 ud, u32 cap, u32 f, u32 th) { return reg(EV_TOUCHEND, ud, f); }
u32 w2c_env_emscripten_set_touchmove_callback_on_thread(struct w2c_env* e, u32 t, u32 ud, u32 cap, u32 f, u32 th) { return reg(EV_TOUCHMOVE, ud, f); }
u32 w2c_env_emscripten_set_touchcancel_callback_on_thread(struct w2c_env* e, u32 t, u32 ud, u32 cap, u32 f, u32 th) { return reg(EV_TOUCHCANCEL, ud, f); }

static bool dispatch(int type, u32 ev) {
  if (!g_cb[type].func) return false;
  return host_call_iiii(g_cb[type].func, (u32)type, ev, g_cb[type].userData) != 0;
}

void host_input_key(bool down, int keyCode, const char* key, const char* code, bool shift, bool ctrl, bool alt) {
  if (!g_key_ev) g_key_ev = host_malloc(160);
  u32 p = g_key_ev;
  memset(hp(p), 0, 160);
  hwf64(p, host_now_ms());
  hw8(p + 12, ctrl);
  hw8(p + 13, shift);
  hw8(p + 14, alt);
  hw32(p + 24, (u32)keyCode);
  hw32(p + 28, (u32)keyCode);
  host_write_str(p + 32, key ? key : "", 32);
  host_write_str(p + 64, code ? code : "", 32);
  dispatch(down ? EV_KEYDOWN : EV_KEYUP, p);
}

static void fill_mouse(u32 p, int x, int y, int button, int buttons) {
  hwf64(p, host_now_ms());
  hw32(p + 8, (u32)x);
  hw32(p + 12, (u32)y);
  hw32(p + 16, (u32)x);
  hw32(p + 20, (u32)y);
  hw16(p + 28, (uint16_t)button);
  hw16(p + 30, (uint16_t)buttons);
  hw32(p + 40, (u32)x);
  hw32(p + 44, (u32)y);
}

void host_input_mouse(int type, int x, int y, int button, int buttons) {
  static int lx, ly;
  if (!g_mouse_ev) g_mouse_ev = host_malloc(64);
  u32 p = g_mouse_ev;
  memset(hp(p), 0, 64);
  fill_mouse(p, x, y, button, buttons);
  hw32(p + 32, (u32)(x - lx));
  hw32(p + 36, (u32)(y - ly));
  lx = x;
  ly = y;
  dispatch(type, p);
}

void host_input_wheel(int x, int y, double dy) {
  if (!g_wheel_ev) g_wheel_ev = host_malloc(96);
  u32 p = g_wheel_ev;
  memset(hp(p), 0, 96);
  fill_mouse(p, x, y, 0, 0);
  hwf64(p + 72, dy);
  dispatch(EV_WHEEL, p);
}

void host_input_touch(int type, const host_touch* t, int n) {
  if (!g_touch_ev) g_touch_ev = host_malloc(1552);
  u32 p = g_touch_ev;
  memset(hp(p), 0, 1552);
  hwf64(p, host_now_ms());
  if (n > 32) n = 32;
  for (int i = 0; i < n; i++) {
    u32 q = p + 16 + 48 * (u32)i;
    hw32(q, (u32)t[i].id);
    for (int k = 1; k <= 6; k++) hw32(q + 4 * k, (u32)(k & 1 ? t[i].x : t[i].y));
    hw8(q + 28, t[i].changed);
    hw8(q + 29, 1);
    hw32(q + 32, (u32)t[i].x);
    hw32(q + 36, (u32)t[i].y);
  }
  hw32(p + 8, (u32)n);
  dispatch(type, p);
}

typedef struct {
  bool connected;
  int naxes, nbuttons;
  double axes[64];
  double buttons[64];
  char id[64];
} pad_state;
static pad_state g_pads[4];
static int g_npads;

void host_gamepad_set(int index, bool connected, const double* axes, int naxes, const double* buttons, int nbuttons,
                      const char* id) {
  if (index < 0 || index >= 4) return;
  pad_state* s = &g_pads[index];
  s->connected = connected;
  s->naxes = naxes > 64 ? 64 : naxes;
  s->nbuttons = nbuttons > 64 ? 64 : nbuttons;
  if (axes) memcpy(s->axes, axes, sizeof(double) * (size_t)s->naxes);
  if (buttons) memcpy(s->buttons, buttons, sizeof(double) * (size_t)s->nbuttons);
  snprintf(s->id, sizeof s->id, "%s", id ? id : "Android gamepad");
  if (index + 1 > g_npads) g_npads = index + 1;
}

u32 w2c_env_emscripten_sample_gamepad_data(struct w2c_env* e) { return 0; }
u32 w2c_env_emscripten_get_num_gamepads(struct w2c_env* e) { return (u32)g_npads; }
u32 w2c_env_emscripten_set_gamepadconnected_callback_on_thread(struct w2c_env* e, u32 ud, u32 cap, u32 f, u32 th) { return 0; }
u32 w2c_env_emscripten_set_gamepaddisconnected_callback_on_thread(struct w2c_env* e, u32 ud, u32 cap, u32 f, u32 th) { return 0; }

u32 w2c_env_emscripten_get_gamepad_status(struct w2c_env* e, u32 index, u32 p) {
  if (index >= (u32)g_npads) return (u32)-5;
  pad_state* s = &g_pads[index];
  if (!s->connected) return (u32)-7;
  memset(hp(p), 0, 1240);
  hwf64(p, host_now_ms());
  hw32(p + 8, (u32)s->naxes);
  hw32(p + 12, (u32)s->nbuttons);
  for (int i = 0; i < s->naxes; i++) hwf64(p + 16 + 8 * (u32)i, s->axes[i]);
  for (int i = 0; i < s->nbuttons; i++) {
    hwf64(p + 528 + 8 * (u32)i, s->buttons[i]);
    hw8(p + 1040 + (u32)i, s->buttons[i] > 0.5);
  }
  hw8(p + 1104, 1);
  hw32(p + 1108, index);
  host_write_str(p + 1112, s->id, 64);
  host_write_str(p + 1176, "standard", 64);
  return 0;
}
