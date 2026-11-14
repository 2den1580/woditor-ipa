#ifndef BENTEN_H_GENERATED_
#define BENTEN_H_GENERATED_

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

struct w2c_wbg;

typedef struct w2c_benten {
  struct w2c_wbg* w2c_wbg_instance;
  u32 w2c_g0;
  wasm_rt_memory_t w2c_memory;
  wasm_rt_funcref_table_t w2c_T0;
} w2c_benten;

void wasm2c_benten_instantiate(w2c_benten*, struct w2c_wbg*);
void wasm2c_benten_free(w2c_benten*);
wasm_rt_func_type_t wasm2c_benten_get_func_type(uint32_t param_count, uint32_t result_count, ...);

void w2c_wbg_0x5F_wbindgen_copy_to_typed_array(struct w2c_wbg*, u32, u32, u32);

void w2c_wbg_0x5F_wbindgen_object_drop_ref(struct w2c_wbg*, u32);

wasm_rt_memory_t* w2c_benten_memory(w2c_benten* instance);

void w2c_benten_bt_play(w2c_benten*, u32, u32, u32, u32, u32, u32);

void w2c_benten_bt_send_message(w2c_benten*, u32, u32, u32, u32);

void w2c_benten_bt_stop(w2c_benten*);

void w2c_benten_bt_set_speed(w2c_benten*, f64);

void w2c_benten_bt_process(w2c_benten*, u32, u32, u32, u32, u32, u32);

u32 w2c_benten_0x5F_wbindgen_malloc(w2c_benten*, u32, u32);

#ifdef __cplusplus
}
#endif

#endif
