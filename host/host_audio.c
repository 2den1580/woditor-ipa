#include "host.h"

#include <math.h>
#include <pthread.h>
#include <stdlib.h>

enum {
  AL_SOURCE_RELATIVE = 0x202, AL_PITCH = 0x1003, AL_POSITION = 0x1004, AL_LOOPING = 0x1007, AL_BUFFER = 0x1009,
  AL_GAIN = 0x100A, AL_SOURCE_STATE = 0x1010, AL_INITIAL = 0x1011, AL_PLAYING = 0x1012, AL_PAUSED = 0x1013,
  AL_STOPPED = 0x1014, AL_BUFFERS_QUEUED = 0x1015, AL_BUFFERS_PROCESSED = 0x1016, AL_SEC_OFFSET = 0x1024,
  AL_SAMPLE_OFFSET = 0x1025, AL_BYTE_OFFSET = 0x1026, AL_SOURCE_TYPE = 0x1027, AL_STATIC = 0x1028,
  AL_STREAMING = 0x1029, AL_UNDETERMINED = 0x1030,
  AL_INVALID_NAME = 0xA001, AL_INVALID_ENUM = 0xA002, AL_INVALID_VALUE = 0xA003, AL_INVALID_OPERATION = 0xA004,
};

typedef struct {
  bool used;
  int channels;
  int freq;
  int16_t* data;
  u32 frames;
  int refs;
} al_buffer;

#define MAX_QUEUE 64
typedef struct {
  bool used;
  int state;
  int type;
  u32 queue[MAX_QUEUE];
  int nqueue;
  int processed;
  int cur;
  double pos;
  bool looping;
  float gain;
  float pitch;
  float pan;
} al_source;

#define MAX_BUF 4096
#define MAX_SRC 256
static al_buffer g_buf[MAX_BUF];
static al_source g_src[MAX_SRC];
static int g_err;
static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
static float g_master = 1.0f;

#define LOCK() pthread_mutex_lock(&g_mu)
#define UNLOCK() pthread_mutex_unlock(&g_mu)
static void set_err(int e) { if (!g_err) g_err = e; }

static al_buffer* buf_get(u32 id) { return id && id < MAX_BUF && g_buf[id].used ? &g_buf[id] : NULL; }
static al_source* src_get(u32 id) { return id && id < MAX_SRC && g_src[id].used ? &g_src[id] : NULL; }

u32 w2c_env_alcOpenDevice(struct w2c_env* e, u32 name) { return 1; }
u32 w2c_env_alcCloseDevice(struct w2c_env* e, u32 dev) { return 1; }
u32 w2c_env_alcCreateContext(struct w2c_env* e, u32 dev, u32 attrs) { return 1; }
void w2c_env_alcDestroyContext(struct w2c_env* e, u32 ctx) {}
u32 w2c_env_alcMakeContextCurrent(struct w2c_env* e, u32 ctx) { return 1; }

u32 w2c_env_alcGetString(struct w2c_env* e, u32 dev, u32 param) {
  static u32 cache[8];
  int slot;
  const char* s;
  switch (param) {
    case 0: slot = 0; s = "No Error"; break;
    case 0x1004: slot = 1; s = "Web Audio"; break;
    case 0x1005: slot = 2; s = "Web Audio"; break;
    case 0x1006: slot = 3; s = ""; break;
    default: return 0;
  }
  if (!cache[slot]) cache[slot] = host_strdup(s);
  return cache[slot];
}

u32 w2c_env_alGetError(struct w2c_env* e) {
  LOCK();
  int r = g_err;
  g_err = 0;
  UNLOCK();
  return (u32)r;
}

void w2c_env_alGenBuffers(struct w2c_env* e, u32 n, u32 ids) {
  LOCK();
  for (u32 i = 0; i < n; i++) {
    u32 id = 1;
    while (id < MAX_BUF && g_buf[id].used) id++;
    if (id >= MAX_BUF) {
      set_err(AL_INVALID_VALUE);
      break;
    }
    memset(&g_buf[id], 0, sizeof g_buf[id]);
    g_buf[id].used = true;
    g_buf[id].channels = 1;
    hw32(ids + 4 * i, id);
  }
  UNLOCK();
}

void w2c_env_alDeleteBuffers(struct w2c_env* e, u32 n, u32 ids) {
  LOCK();
  for (u32 i = 0; i < n; i++) {
    al_buffer* b = buf_get(hr32(ids + 4 * i));
    if (!b) continue;
    if (b->refs) {
      set_err(AL_INVALID_OPERATION);
      continue;
    }
    free(b->data);
    memset(b, 0, sizeof *b);
  }
  UNLOCK();
}

void w2c_env_alBufferData(struct w2c_env* e, u32 id, u32 format, u32 data, u32 size, u32 freq) {
  int ch, bytes;
  switch (format) {
    case 0x1100: ch = 1; bytes = 1; break;
    case 0x1101: ch = 1; bytes = 2; break;
    case 0x1102: ch = 2; bytes = 1; break;
    case 0x1103: ch = 2; bytes = 2; break;
    case 0x10010: ch = 1; bytes = 4; break;
    case 0x10011: ch = 2; bytes = 4; break;
    default: set_err(AL_INVALID_ENUM); return;
  }
  u32 samples = size / (u32)bytes;
  int16_t* pcm = malloc(sizeof(int16_t) * (samples ? samples : 1));
  const uint8_t* p = hp(data);
  for (u32 i = 0; i < samples; i++) {
    if (bytes == 1) pcm[i] = (int16_t)((p[i] - 128) << 8);
    else if (bytes == 2) memcpy(&pcm[i], p + 2 * i, 2);
    else {
      float f;
      memcpy(&f, p + 4 * i, 4);
      f = f > 1 ? 1 : f < -1 ? -1 : f;
      pcm[i] = (int16_t)(f * 32767);
    }
  }
  LOCK();
  al_buffer* b = buf_get(id);
  if (!b || !freq) {
    set_err(b ? AL_INVALID_VALUE : AL_INVALID_NAME);
    UNLOCK();
    free(pcm);
    return;
  }
  free(b->data);
  b->data = pcm;
  b->channels = ch;
  b->freq = (int)freq;
  b->frames = samples / (u32)ch;
  UNLOCK();
}

void w2c_env_alGenSources(struct w2c_env* e, u32 n, u32 ids) {
  LOCK();
  for (u32 i = 0; i < n; i++) {
    u32 id = 1;
    while (id < MAX_SRC && g_src[id].used) id++;
    if (id >= MAX_SRC) {
      set_err(AL_INVALID_VALUE);
      break;
    }
    al_source* s = &g_src[id];
    memset(s, 0, sizeof *s);
    s->used = true;
    s->state = AL_INITIAL;
    s->type = AL_UNDETERMINED;
    s->gain = 1;
    s->pitch = 1;
    hw32(ids + 4 * i, id);
  }
  UNLOCK();
}

static void clear_queue(al_source* s) {
  for (int i = 0; i < s->nqueue; i++) {
    al_buffer* b = buf_get(s->queue[i]);
    if (b) b->refs--;
  }
  s->nqueue = s->processed = s->cur = 0;
  s->pos = 0;
}

void w2c_env_alDeleteSources(struct w2c_env* e, u32 n, u32 ids) {
  LOCK();
  for (u32 i = 0; i < n; i++) {
    al_source* s = src_get(hr32(ids + 4 * i));
    if (!s) continue;
    clear_queue(s);
    s->used = false;
  }
  UNLOCK();
}

static void rewind_source(al_source* s) {
  s->cur = 0;
  s->pos = 0;
  s->processed = 0;
}

void w2c_env_alSourcePlay(struct w2c_env* e, u32 id) {
  LOCK();
  al_source* s = src_get(id);
  if (!s) set_err(AL_INVALID_NAME);
  else {
    if (s->state != AL_PAUSED && s->state != AL_PLAYING) rewind_source(s);
    s->state = s->nqueue ? AL_PLAYING : AL_STOPPED;
  }
  UNLOCK();
}

void w2c_env_alSourceStop(struct w2c_env* e, u32 id) {
  LOCK();
  al_source* s = src_get(id);
  if (!s) set_err(AL_INVALID_NAME);
  else if (s->state != AL_INITIAL) {
    s->state = AL_STOPPED;
    s->processed = s->nqueue;
    s->cur = s->nqueue;
    s->pos = 0;
  }
  UNLOCK();
}

void w2c_env_alSourceQueueBuffers(struct w2c_env* e, u32 id, u32 n, u32 ids) {
  LOCK();
  al_source* s = src_get(id);
  if (!s || s->type == AL_STATIC) {
    set_err(s ? AL_INVALID_OPERATION : AL_INVALID_NAME);
    UNLOCK();
    return;
  }
  s->type = AL_STREAMING;
  for (u32 i = 0; i < n; i++) {
    u32 bid = hr32(ids + 4 * i);
    al_buffer* b = buf_get(bid);
    if (!b || s->nqueue >= MAX_QUEUE) {
      set_err(b ? AL_INVALID_OPERATION : AL_INVALID_NAME);
      break;
    }
    b->refs++;
    s->queue[s->nqueue++] = bid;
  }
  UNLOCK();
}

void w2c_env_alSourceUnqueueBuffers(struct w2c_env* e, u32 id, u32 n, u32 ids) {
  LOCK();
  al_source* s = src_get(id);
  if (!s) set_err(AL_INVALID_NAME);
  else if ((int)n > s->processed) set_err(AL_INVALID_VALUE);
  else {
    for (u32 i = 0; i < n; i++) {
      u32 bid = s->queue[0];
      al_buffer* b = buf_get(bid);
      if (b) b->refs--;
      hw32(ids + 4 * i, bid);
      memmove(&s->queue[0], &s->queue[1], sizeof(u32) * (size_t)(--s->nqueue));
      s->processed--;
      s->cur--;
    }
    if (s->cur < 0) s->cur = 0;
  }
  UNLOCK();
}

void w2c_env_alSourcei(struct w2c_env* e, u32 id, u32 param, u32 value) {
  LOCK();
  al_source* s = src_get(id);
  if (!s) {
    set_err(AL_INVALID_NAME);
    UNLOCK();
    return;
  }
  switch (param) {
    case AL_BUFFER:
      if (s->state == AL_PLAYING || s->state == AL_PAUSED) {
        set_err(AL_INVALID_OPERATION);
        break;
      }
      clear_queue(s);
      if (value) {
        al_buffer* b = buf_get(value);
        if (!b) {
          set_err(AL_INVALID_NAME);
          break;
        }
        b->refs++;
        s->queue[s->nqueue++] = value;
        s->type = AL_STATIC;
      } else {
        s->type = AL_UNDETERMINED;
      }
      break;
    case AL_LOOPING: s->looping = value != 0; break;
    case AL_SAMPLE_OFFSET: s->pos = (double)value; break;
    case AL_SOURCE_RELATIVE: break;
    default: break;
  }
  UNLOCK();
}

void w2c_env_alSourcef(struct w2c_env* e, u32 id, u32 param, f32 value) {
  LOCK();
  al_source* s = src_get(id);
  if (!s) set_err(AL_INVALID_NAME);
  else if (param == AL_GAIN) s->gain = value < 0 ? 0 : value;
  else if (param == AL_PITCH) s->pitch = value <= 0 ? 1 : value;
  else if (param == AL_SEC_OFFSET && s->cur < s->nqueue) {
    al_buffer* b = buf_get(s->queue[s->cur]);
    if (b) s->pos = value * b->freq;
  }
  UNLOCK();
}

void w2c_env_alSourcefv(struct w2c_env* e, u32 id, u32 param, u32 values) {
  if (!values) return;
  float v[3];
  memcpy(v, hp(values), param == AL_POSITION ? 12 : 4);
  if (param != AL_POSITION) {
    w2c_env_alSourcef(e, id, param, v[0]);
    return;
  }
  LOCK();
  al_source* s = src_get(id);
  if (s) s->pan = v[0] > 1 ? 1 : v[0] < -1 ? -1 : v[0];
  UNLOCK();
}

void w2c_env_alGetSourcei(struct w2c_env* e, u32 id, u32 param, u32 out) {
  LOCK();
  al_source* s = src_get(id);
  if (!s) {
    set_err(AL_INVALID_NAME);
    UNLOCK();
    return;
  }
  int v = 0;
  switch (param) {
    case AL_SOURCE_STATE: v = s->state; break;
    case AL_BUFFERS_PROCESSED: v = s->state == AL_INITIAL ? 0 : s->processed; break;
    case AL_BUFFERS_QUEUED: v = s->nqueue; break;
    case AL_BUFFER: v = s->nqueue && s->cur < s->nqueue ? (int)s->queue[s->cur] : 0; break;
    case AL_LOOPING: v = s->looping; break;
    case AL_SOURCE_TYPE: v = s->type; break;
    case AL_SAMPLE_OFFSET: v = (int)s->pos; break;
    default: break;
  }
  if (out) hw32(out, (u32)v);
  UNLOCK();
}

static inline float cubic(float x0, float x1, float x2, float x3, float t) {
  return x1 + 0.5f * t * (x2 - x0 + t * (2 * x0 - 5 * x1 + 4 * x2 - x3 + t * (3 * (x1 - x2) + x3 - x0)));
}

void host_audio_mix(int16_t* out, int frames, int rate) {
  static float* acc;
  static int acc_frames;
  if (acc_frames < frames) {
    free(acc);
    acc = malloc(sizeof(float) * 2 * (size_t)frames);
    acc_frames = frames;
  }
  memset(acc, 0, sizeof(float) * 2 * (size_t)frames);
  LOCK();
  for (int si = 1; si < MAX_SRC; si++) {
    al_source* s = &g_src[si];
    if (!s->used || s->state != AL_PLAYING) continue;
    float lg = s->gain * g_master, rg = lg;
    if (s->pan != 0) {
      float a = (s->pan + 1) * 0.25f * 3.14159265f;
      lg *= cosf(a) * 1.41421356f;
      rg *= sinf(a) * 1.41421356f;
    }
    int f = 0;
    while (f < frames && s->state == AL_PLAYING) {
      if (s->cur >= s->nqueue) {
        if (s->looping && s->nqueue) {
          rewind_source(s);
          continue;
        }
        s->state = AL_STOPPED;
        break;
      }
      al_buffer* b = buf_get(s->queue[s->cur]);
      if (!b || !b->frames || !b->data) {
        s->cur++;
        s->processed++;
        s->pos = 0;
        continue;
      }
      double step = (double)b->freq / rate * s->pitch;
      u32 last = b->frames - 1;
      const int16_t* d = b->data;
      for (; f < frames && s->pos < b->frames; f++, s->pos += step) {
        u32 i1 = (u32)s->pos;
        u32 i0 = i1 ? i1 - 1 : 0, i2 = i1 < last ? i1 + 1 : last, i3 = i2 < last ? i2 + 1 : last;
        float t = (float)(s->pos - i1);
        float l, r;
        if (b->channels == 1) {
          l = r = cubic(d[i0], d[i1], d[i2], d[i3], t) * (1 / 32768.0f);
        } else {
          l = cubic(d[2 * i0], d[2 * i1], d[2 * i2], d[2 * i3], t) * (1 / 32768.0f);
          r = cubic(d[2 * i0 + 1], d[2 * i1 + 1], d[2 * i2 + 1], d[2 * i3 + 1], t) * (1 / 32768.0f);
        }
        acc[2 * f] += l * lg;
        acc[2 * f + 1] += r * rg;
      }
      if (s->pos >= b->frames) {
        s->pos -= b->frames;
        if (s->type == AL_STATIC && s->looping) {
          s->pos = 0;
          continue;
        }
        s->cur++;
        s->processed++;
      }
    }
  }
  UNLOCK();
  host_midi_mix(acc, frames, rate);
  for (int i = 0; i < 2 * frames; i++) {
    float v = acc[i];
    v = v > 1 ? 1 : v < -1 ? -1 : v;
    out[i] = (int16_t)(v * 32767);
  }
}
