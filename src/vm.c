#include "vm.h"

#define NUM_SCRIPT_SLOTS 8

#pragma clang section bss="zdata"

enum {
  PROC_STATE_FREE,
  PROC_STATE_RUNNING,
  PROC_STATE_WAITING,
  PROC_STATE_DEAD
};

uint8_t proc_state[NUM_SCRIPT_SLOTS];
uint8_t proc_res_slot[NUM_SCRIPT_SLOTS];
uint16_t proc_pc[NUM_SCRIPT_SLOTS];

#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="bss_init"

void vm_init()
{
  for (int i = 0; i < NUM_SCRIPT_SLOTS; i++)
  {
    proc_state[i] = PROC_STATE_FREE;
  }
}

