#include "touchPad.h"

static bit touchPad_resetIF = 0;

void touchPad_pinInit(void){

	;
}

void touchPad_funSet(bit funLevel){

	touchPad_resetIF = funLevel;
}

void touchPad_processThread(void){

	(touchPad_resetIF)?(PIN_TOUCHPAD = 1):(PIN_TOUCHPAD = 0);
}
