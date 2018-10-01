#ifndef __TOUCHPAD_H_
#define __TOUCHPAD_H_

#include "STC15Fxxxx.H"

#define PIN_TOUCHPAD 	P36

void touchPad_pinInit(void);
void touchPad_funSet(bit funLevel);
void touchPad_processThread(void);

#endif

