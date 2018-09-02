#ifndef __RELAY_H_
#define __RELAY_H_

#include "STC15Fxxxx.H"

#define actRelay_ON		1
#define actRelay_OFF	0

#define PIN_RELAY_1		P33
#define PIN_RELAY_2		P34
#define PIN_RELAY_3		P35

typedef enum{

	statusSave_enable = 1,
	statusSave_disable,
}status_ifSave;

typedef enum{

	relay_flip = 1,
	relay_OnOff,
	actionNull,
}rly_methodType;

typedef struct{

	u8 objRelay;
	rly_methodType actMethod;
}relay_Command;

extern status_ifSave xdata relayStatus_ifSave;
extern u8 xdata status_Relay;
extern relay_Command xdata swCommand_fromUsr;
extern u8 EACHCTRL_realesFLG;

void relay_pinInit(void);
void relay_Act(relay_Command dats);
void thread_Relay(void);

#endif