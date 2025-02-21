#pragma once

#include "karljr.h"
#include "jude.h"
#include "jude_widgets.h"

#define LISTBOXLINESMEM 0x00016000

void mmsetupWelcConfigChg(void);

void mmsetupWelcNextChg(void);
void mmsetupConfigCancelChg(void);

void mmsetupConfigItemChg(void);

void mmsetupSelectCancelChg(void);