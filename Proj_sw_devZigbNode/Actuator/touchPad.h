#ifndef __TOUCHPAD_H_
#define __TOUCHPAD_H_

#include "STC15Fxxxx.H"

#define PIN_TOUCHPAD 	P36

#define TOUCHRESETTIME_DEFAULT			10

#define TOUCHPAD_RESET_LEVEL_ENABLE		1
#define TOUCHPAD_RESET_LEVEL_DISABLE	0

extern u8 xdata touchPad_resetTimeCount;

void touchPad_pinInit(void);
void touchPad_processThread(void);
void touchPad_resetOpreat(u8 holdTime);

#endif

