#include "script.h"
#include "resource.h"

#pragma clang section bss="zdata"

uint8_t global_game_objects[780];
uint16_t variables[256];

#pragma clang section text="code_main" rodata="cdata_main" data="data_main" bss="zdata"




void script_run(uint8_t script_id)
{
  res_provide(RES_TYPE_SCRIPT | RES_LOCKED_MASK, script_id, 0);
  
}
