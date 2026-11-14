#include "host.h"

u32 w2c_env_invoke_i(struct w2c_env* env, u32 index) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return 0;
  }
  u32 r = ((u32 (*)(void*))f->func)(f->module_instance);
  host_eh_pop(&fr);
  return r;
}

u32 w2c_env_invoke_iffddiiii(struct w2c_env* env, u32 index, f32 a0, f32 a1, f64 a2, f64 a3, u32 a4, u32 a5, u32 a6, u32 a7) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return 0;
  }
  u32 r = ((u32 (*)(void*, f32, f32, f64, f64, u32, u32, u32, u32))f->func)(f->module_instance, a0, a1, a2, a3, a4, a5, a6, a7);
  host_eh_pop(&fr);
  return r;
}

u32 w2c_env_invoke_iffffffffii(struct w2c_env* env, u32 index, f32 a0, f32 a1, f32 a2, f32 a3, f32 a4, f32 a5, f32 a6, f32 a7, u32 a8, u32 a9) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return 0;
  }
  u32 r = ((u32 (*)(void*, f32, f32, f32, f32, f32, f32, f32, f32, u32, u32))f->func)(f->module_instance, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
  host_eh_pop(&fr);
  return r;
}

u32 w2c_env_invoke_ii(struct w2c_env* env, u32 index, u32 a0) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return 0;
  }
  u32 r = ((u32 (*)(void*, u32))f->func)(f->module_instance, a0);
  host_eh_pop(&fr);
  return r;
}

u32 w2c_env_invoke_iii(struct w2c_env* env, u32 index, u32 a0, u32 a1) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return 0;
  }
  u32 r = ((u32 (*)(void*, u32, u32))f->func)(f->module_instance, a0, a1);
  host_eh_pop(&fr);
  return r;
}

u32 w2c_env_invoke_iiidddiiii(struct w2c_env* env, u32 index, u32 a0, u32 a1, f64 a2, f64 a3, f64 a4, u32 a5, u32 a6, u32 a7, u32 a8) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return 0;
  }
  u32 r = ((u32 (*)(void*, u32, u32, f64, f64, f64, u32, u32, u32, u32))f->func)(f->module_instance, a0, a1, a2, a3, a4, a5, a6, a7, a8);
  host_eh_pop(&fr);
  return r;
}

u32 w2c_env_invoke_iiii(struct w2c_env* env, u32 index, u32 a0, u32 a1, u32 a2) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return 0;
  }
  u32 r = ((u32 (*)(void*, u32, u32, u32))f->func)(f->module_instance, a0, a1, a2);
  host_eh_pop(&fr);
  return r;
}

u32 w2c_env_invoke_iiiii(struct w2c_env* env, u32 index, u32 a0, u32 a1, u32 a2, u32 a3) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return 0;
  }
  u32 r = ((u32 (*)(void*, u32, u32, u32, u32))f->func)(f->module_instance, a0, a1, a2, a3);
  host_eh_pop(&fr);
  return r;
}

u32 w2c_env_invoke_iiiiii(struct w2c_env* env, u32 index, u32 a0, u32 a1, u32 a2, u32 a3, u32 a4) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return 0;
  }
  u32 r = ((u32 (*)(void*, u32, u32, u32, u32, u32))f->func)(f->module_instance, a0, a1, a2, a3, a4);
  host_eh_pop(&fr);
  return r;
}

u32 w2c_env_invoke_iiiiiii(struct w2c_env* env, u32 index, u32 a0, u32 a1, u32 a2, u32 a3, u32 a4, u32 a5) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return 0;
  }
  u32 r = ((u32 (*)(void*, u32, u32, u32, u32, u32, u32))f->func)(f->module_instance, a0, a1, a2, a3, a4, a5);
  host_eh_pop(&fr);
  return r;
}

u32 w2c_env_invoke_iiiiiiii(struct w2c_env* env, u32 index, u32 a0, u32 a1, u32 a2, u32 a3, u32 a4, u32 a5, u32 a6) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return 0;
  }
  u32 r = ((u32 (*)(void*, u32, u32, u32, u32, u32, u32, u32))f->func)(f->module_instance, a0, a1, a2, a3, a4, a5, a6);
  host_eh_pop(&fr);
  return r;
}

u32 w2c_env_invoke_iiiiiiiii(struct w2c_env* env, u32 index, u32 a0, u32 a1, u32 a2, u32 a3, u32 a4, u32 a5, u32 a6, u32 a7) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return 0;
  }
  u32 r = ((u32 (*)(void*, u32, u32, u32, u32, u32, u32, u32, u32))f->func)(f->module_instance, a0, a1, a2, a3, a4, a5, a6, a7);
  host_eh_pop(&fr);
  return r;
}

u32 w2c_env_invoke_iiiiiiiiii(struct w2c_env* env, u32 index, u32 a0, u32 a1, u32 a2, u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return 0;
  }
  u32 r = ((u32 (*)(void*, u32, u32, u32, u32, u32, u32, u32, u32, u32))f->func)(f->module_instance, a0, a1, a2, a3, a4, a5, a6, a7, a8);
  host_eh_pop(&fr);
  return r;
}

u32 w2c_env_invoke_iiiiiiiiiiii(struct w2c_env* env, u32 index, u32 a0, u32 a1, u32 a2, u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8, u32 a9, u32 a10) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return 0;
  }
  u32 r = ((u32 (*)(void*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32))f->func)(f->module_instance, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
  host_eh_pop(&fr);
  return r;
}

u32 w2c_env_invoke_iiiiij(struct w2c_env* env, u32 index, u32 a0, u32 a1, u32 a2, u32 a3, u64 a4) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return 0;
  }
  u32 r = ((u32 (*)(void*, u32, u32, u32, u32, u64))f->func)(f->module_instance, a0, a1, a2, a3, a4);
  host_eh_pop(&fr);
  return r;
}

u32 w2c_env_invoke_iiiij(struct w2c_env* env, u32 index, u32 a0, u32 a1, u32 a2, u64 a3) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return 0;
  }
  u32 r = ((u32 (*)(void*, u32, u32, u32, u64))f->func)(f->module_instance, a0, a1, a2, a3);
  host_eh_pop(&fr);
  return r;
}

u32 w2c_env_invoke_iiij(struct w2c_env* env, u32 index, u32 a0, u32 a1, u64 a2) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return 0;
  }
  u32 r = ((u32 (*)(void*, u32, u32, u64))f->func)(f->module_instance, a0, a1, a2);
  host_eh_pop(&fr);
  return r;
}

u32 w2c_env_invoke_iiijiiii(struct w2c_env* env, u32 index, u32 a0, u32 a1, u64 a2, u32 a3, u32 a4, u32 a5, u32 a6) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return 0;
  }
  u32 r = ((u32 (*)(void*, u32, u32, u64, u32, u32, u32, u32))f->func)(f->module_instance, a0, a1, a2, a3, a4, a5, a6);
  host_eh_pop(&fr);
  return r;
}

u32 w2c_env_invoke_iij(struct w2c_env* env, u32 index, u32 a0, u64 a1) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return 0;
  }
  u32 r = ((u32 (*)(void*, u32, u64))f->func)(f->module_instance, a0, a1);
  host_eh_pop(&fr);
  return r;
}

u32 w2c_env_invoke_iiji(struct w2c_env* env, u32 index, u32 a0, u64 a1, u32 a2) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return 0;
  }
  u32 r = ((u32 (*)(void*, u32, u64, u32))f->func)(f->module_instance, a0, a1, a2);
  host_eh_pop(&fr);
  return r;
}

u64 w2c_env_invoke_ji(struct w2c_env* env, u32 index, u32 a0) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return 0;
  }
  u64 r = ((u64 (*)(void*, u32))f->func)(f->module_instance, a0);
  host_eh_pop(&fr);
  return r;
}

void w2c_env_invoke_v(struct w2c_env* env, u32 index) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return;
  }
  ((void (*)(void*))f->func)(f->module_instance);
  host_eh_pop(&fr);
}

void w2c_env_invoke_vd(struct w2c_env* env, u32 index, f64 a0) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return;
  }
  ((void (*)(void*, f64))f->func)(f->module_instance, a0);
  host_eh_pop(&fr);
}

void w2c_env_invoke_vi(struct w2c_env* env, u32 index, u32 a0) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return;
  }
  ((void (*)(void*, u32))f->func)(f->module_instance, a0);
  host_eh_pop(&fr);
}

void w2c_env_invoke_vii(struct w2c_env* env, u32 index, u32 a0, u32 a1) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return;
  }
  ((void (*)(void*, u32, u32))f->func)(f->module_instance, a0, a1);
  host_eh_pop(&fr);
}

void w2c_env_invoke_viiddd(struct w2c_env* env, u32 index, u32 a0, u32 a1, f64 a2, f64 a3, f64 a4) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return;
  }
  ((void (*)(void*, u32, u32, f64, f64, f64))f->func)(f->module_instance, a0, a1, a2, a3, a4);
  host_eh_pop(&fr);
}

void w2c_env_invoke_viii(struct w2c_env* env, u32 index, u32 a0, u32 a1, u32 a2) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return;
  }
  ((void (*)(void*, u32, u32, u32))f->func)(f->module_instance, a0, a1, a2);
  host_eh_pop(&fr);
}

void w2c_env_invoke_viiidd(struct w2c_env* env, u32 index, u32 a0, u32 a1, u32 a2, f64 a3, f64 a4) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return;
  }
  ((void (*)(void*, u32, u32, u32, f64, f64))f->func)(f->module_instance, a0, a1, a2, a3, a4);
  host_eh_pop(&fr);
}

void w2c_env_invoke_viiii(struct w2c_env* env, u32 index, u32 a0, u32 a1, u32 a2, u32 a3) {
  if (host_error_dialog_hook(a0, a1, a2)) return;
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return;
  }
  ((void (*)(void*, u32, u32, u32, u32))f->func)(f->module_instance, a0, a1, a2, a3);
  host_eh_pop(&fr);
}

void w2c_env_invoke_viiiii(struct w2c_env* env, u32 index, u32 a0, u32 a1, u32 a2, u32 a3, u32 a4) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return;
  }
  ((void (*)(void*, u32, u32, u32, u32, u32))f->func)(f->module_instance, a0, a1, a2, a3, a4);
  host_eh_pop(&fr);
}

void w2c_env_invoke_viiiiii(struct w2c_env* env, u32 index, u32 a0, u32 a1, u32 a2, u32 a3, u32 a4, u32 a5) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return;
  }
  ((void (*)(void*, u32, u32, u32, u32, u32, u32))f->func)(f->module_instance, a0, a1, a2, a3, a4, a5);
  host_eh_pop(&fr);
}

void w2c_env_invoke_viiiiiii(struct w2c_env* env, u32 index, u32 a0, u32 a1, u32 a2, u32 a3, u32 a4, u32 a5, u32 a6) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return;
  }
  ((void (*)(void*, u32, u32, u32, u32, u32, u32, u32))f->func)(f->module_instance, a0, a1, a2, a3, a4, a5, a6);
  host_eh_pop(&fr);
}

void w2c_env_invoke_viiiiiiii(struct w2c_env* env, u32 index, u32 a0, u32 a1, u32 a2, u32 a3, u32 a4, u32 a5, u32 a6, u32 a7) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return;
  }
  ((void (*)(void*, u32, u32, u32, u32, u32, u32, u32, u32))f->func)(f->module_instance, a0, a1, a2, a3, a4, a5, a6, a7);
  host_eh_pop(&fr);
}

void w2c_env_invoke_viiiiiiiii(struct w2c_env* env, u32 index, u32 a0, u32 a1, u32 a2, u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return;
  }
  ((void (*)(void*, u32, u32, u32, u32, u32, u32, u32, u32, u32))f->func)(f->module_instance, a0, a1, a2, a3, a4, a5, a6, a7, a8);
  host_eh_pop(&fr);
}

void w2c_env_invoke_viiiiiiiiii(struct w2c_env* env, u32 index, u32 a0, u32 a1, u32 a2, u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8, u32 a9) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return;
  }
  ((void (*)(void*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32))f->func)(f->module_instance, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
  host_eh_pop(&fr);
}

void w2c_env_invoke_viiiiiiiiiiiii(struct w2c_env* env, u32 index, u32 a0, u32 a1, u32 a2, u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8, u32 a9, u32 a10, u32 a11, u32 a12) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return;
  }
  ((void (*)(void*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32))f->func)(f->module_instance, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
  host_eh_pop(&fr);
}

void w2c_env_invoke_viiiiiiiiiiiiiii(struct w2c_env* env, u32 index, u32 a0, u32 a1, u32 a2, u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8, u32 a9, u32 a10, u32 a11, u32 a12, u32 a13, u32 a14) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return;
  }
  ((void (*)(void*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32))f->func)(f->module_instance, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14);
  host_eh_pop(&fr);
}

void w2c_env_invoke_viiiiiiiiiiiiiiii(struct w2c_env* env, u32 index, u32 a0, u32 a1, u32 a2, u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8, u32 a9, u32 a10, u32 a11, u32 a12, u32 a13, u32 a14, u32 a15) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return;
  }
  ((void (*)(void*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32))f->func)(f->module_instance, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15);
  host_eh_pop(&fr);
}

void w2c_env_invoke_viiiiiiiiiiiiiiiiii(struct w2c_env* env, u32 index, u32 a0, u32 a1, u32 a2, u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8, u32 a9, u32 a10, u32 a11, u32 a12, u32 a13, u32 a14, u32 a15, u32 a16, u32 a17) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return;
  }
  ((void (*)(void*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32))f->func)(f->module_instance, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17);
  host_eh_pop(&fr);
}

void w2c_env_invoke_viiiiiiiiiiiiiiiiiii(struct w2c_env* env, u32 index, u32 a0, u32 a1, u32 a2, u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8, u32 a9, u32 a10, u32 a11, u32 a12, u32 a13, u32 a14, u32 a15, u32 a16, u32 a17, u32 a18) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return;
  }
  ((void (*)(void*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32))f->func)(f->module_instance, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18);
  host_eh_pop(&fr);
}

void w2c_env_invoke_viiiiiiiiiiiiiiiiiiiii(struct w2c_env* env, u32 index, u32 a0, u32 a1, u32 a2, u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8, u32 a9, u32 a10, u32 a11, u32 a12, u32 a13, u32 a14, u32 a15, u32 a16, u32 a17, u32 a18, u32 a19, u32 a20) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return;
  }
  ((void (*)(void*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32))f->func)(f->module_instance, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20);
  host_eh_pop(&fr);
}

void w2c_env_invoke_viiiiiiiiiiiiiiiiiiiiiiii(struct w2c_env* env, u32 index, u32 a0, u32 a1, u32 a2, u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8, u32 a9, u32 a10, u32 a11, u32 a12, u32 a13, u32 a14, u32 a15, u32 a16, u32 a17, u32 a18, u32 a19, u32 a20, u32 a21, u32 a22, u32 a23) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return;
  }
  ((void (*)(void*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32))f->func)(f->module_instance, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23);
  host_eh_pop(&fr);
}

void w2c_env_invoke_viiiiiiiiiiiiiiiiiiiiiiiiii(struct w2c_env* env, u32 index, u32 a0, u32 a1, u32 a2, u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8, u32 a9, u32 a10, u32 a11, u32 a12, u32 a13, u32 a14, u32 a15, u32 a16, u32 a17, u32 a18, u32 a19, u32 a20, u32 a21, u32 a22, u32 a23, u32 a24, u32 a25) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return;
  }
  ((void (*)(void*, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32))f->func)(f->module_instance, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25);
  host_eh_pop(&fr);
}

void w2c_env_invoke_viiiiijjii(struct w2c_env* env, u32 index, u32 a0, u32 a1, u32 a2, u32 a3, u32 a4, u64 a5, u64 a6, u32 a7, u32 a8) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return;
  }
  ((void (*)(void*, u32, u32, u32, u32, u32, u64, u64, u32, u32))f->func)(f->module_instance, a0, a1, a2, a3, a4, a5, a6, a7, a8);
  host_eh_pop(&fr);
}

void w2c_env_invoke_viiij(struct w2c_env* env, u32 index, u32 a0, u32 a1, u32 a2, u64 a3) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return;
  }
  ((void (*)(void*, u32, u32, u32, u64))f->func)(f->module_instance, a0, a1, a2, a3);
  host_eh_pop(&fr);
}

void w2c_env_invoke_vijj(struct w2c_env* env, u32 index, u32 a0, u64 a1, u64 a2) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return;
  }
  ((void (*)(void*, u32, u64, u64))f->func)(f->module_instance, a0, a1, a2);
  host_eh_pop(&fr);
}

void w2c_env_invoke_vji(struct w2c_env* env, u32 index, u64 a0, u32 a1) {
  const wasm_rt_funcref_t* f = host_table_entry(index);
  host_eh_frame fr;
  u32 sp = host_stack_save();
  host_eh_push(&fr);
  if (setjmp(fr.jb)) {
    host_eh_caught(&fr, sp);
    return;
  }
  ((void (*)(void*, u64, u32))f->func)(f->module_instance, a0, a1);
  host_eh_pop(&fr);
}
