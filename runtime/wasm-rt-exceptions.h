#ifndef WASM_RT_EXCEPTIONS_H_
#define WASM_RT_EXCEPTIONS_H_

#include "wasm-rt.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef const void* wasm_rt_tag_t;

void wasm_rt_load_exception(const wasm_rt_tag_t tag,
                            uint32_t size,
                            const void* values);

WASM_RT_NO_RETURN void wasm_rt_throw(void);

#define WASM_RT_UNWIND_TARGET wasm_rt_jmp_buf

WASM_RT_UNWIND_TARGET* wasm_rt_get_unwind_target(void);

void wasm_rt_set_unwind_target(WASM_RT_UNWIND_TARGET* target);

wasm_rt_tag_t wasm_rt_exception_tag(void);

uint32_t wasm_rt_exception_size(void);

void* wasm_rt_exception(void);

#ifdef __cplusplus
}
#endif

#endif
