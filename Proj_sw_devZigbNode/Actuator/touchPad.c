#include "touchPad.h"

u8 xdata touchPad_resetTimeCount = 0;

void touchPad_pinInit(void){

	;
}

void touchPad_processThread(void){

	(touchPad_resetTimeCount)?(PIN_TOUCHPAD = TOUCHPAD_RESET_LEVEL_ENABLE):(PIN_TOUCHPAD = TOUCHPAD_RESET_LEVEL_DISABLE);
}

void touchPad_resetOpreat(u8 holdTime){

	touchPad_resetTimeCount = holdTime;
}
