#include "script.h"

#pragma clang section bss="zdata"

uint8_t global_game_objects[780];
uint8_t variables[256];

#pragma clang section text="code"

//void load_script(uint8_t room_number, uint16_t offset)