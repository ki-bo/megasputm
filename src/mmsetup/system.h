#pragma once

#ifdef __CALYPSI_CORE_45GS02__

#define __REGA "a"
#define __REGX "x"
#define __REGY "y"
#define __REGZ "z"
#define __KZ16 "Kzp16"

#else

#define __REGA "rax"
#define __REGX "rbx"
#define __REGY "rcx"
#define __REGZ "rdx"
#define __KZ16 "Krm"

#endif