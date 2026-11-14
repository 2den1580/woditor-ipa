#pragma once
#include <stdint.h>

enum {
  ASM_UNKNOWN = 0,
  ASM_CANVAS_WIDTH,
  ASM_CHECK_FAILED_0,
  ASM_CHECK_FAILED_1,
  ASM_BEFORE_GAME_START,
  ASM_SLEEP_100,
};

typedef struct {
  uint32_t code;
  int kind;
} host_asm_entry;

extern const host_asm_entry host_asm_table[];
