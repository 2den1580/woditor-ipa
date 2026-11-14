#include "wasm-rt.h"

#include "wasm-rt-exceptions.h"

#include <string.h>

#define MAX_EXCEPTION_SIZE 256

static WASM_RT_THREAD_LOCAL wasm_rt_tag_t g_active_exception_tag;
static WASM_RT_THREAD_LOCAL uint8_t g_active_exception[MAX_EXCEPTION_SIZE];
static WASM_RT_THREAD_LOCAL uint32_t g_active_exception_size;

static WASM_RT_THREAD_LOCAL wasm_rt_jmp_buf* g_unwind_target;

void wasm_rt_load_exception(const wasm_rt_tag_t tag,
                            uint32_t size,
                            const void* values) {
  if (size > MAX_EXCEPTION_SIZE) {
    wasm_rt_trap(WASM_RT_TRAP_EXHAUSTION);
  }

  g_active_exception_tag = tag;
  g_active_exception_size = size;

  if (size) {
    memcpy(g_active_exception, values, size);
  }
}

WASM_RT_NO_RETURN void wasm_rt_throw(void) {
  WASM_RT_LONGJMP(*g_unwind_target, WASM_RT_TRAP_UNCAUGHT_EXCEPTION);
}

WASM_RT_UNWIND_TARGET* wasm_rt_get_unwind_target(void) {
  return g_unwind_target;
}

void wasm_rt_set_unwind_target(WASM_RT_UNWIND_TARGET* target) {
  g_unwind_target = target;
}

wasm_rt_tag_t wasm_rt_exception_tag(void) {
  return g_active_exception_tag;
}

uint32_t wasm_rt_exception_size(void) {
  return g_active_exception_size;
}

void* wasm_rt_exception(void) {
  return g_active_exception;
}
