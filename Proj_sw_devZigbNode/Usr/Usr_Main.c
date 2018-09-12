#include "STC15Fxxxx.H"

#include "dataTrans.h"
#include "appTimer.h"
#include "pars_Method.h"
#include "dataManage.h"
#include "Tips.h"
#include "usrKin.h"
#include "Relay.h"
#include "timerAct.h"

#include "USART.h"
#include "delay.h"

extern bit devZigbOnline_IF;

void bsp_Init(void){

	appTimer0_Init();
	zigbUart_pinInit();
	uartObjZigb_Init();
	tipLED_pinInit();
	usrKin_pinInit();
	relay_pinInit();
}

void bsp_datsReales(void){

	MAC_ID_Relaes();
	portCtrlEachOther_Reales();
	ledBKGColorSw_Reales();
	timeZone_Reales();
}

int main(void){
	
	bsp_Init();
	bsp_datsReales();
	birthDay_Judge();
	
//	while(1)tips_warning();
	
	devStatus_switch.statusChange_standBy = status_nwkReconnect;
	devStatus_switch.statusChange_IF = 1;
	
//	while(1)PIN_RELAY_1 = touchPad_1;
	
	while(1){
		
		touchPad_Scan();
		UsrKEYScan(usrKeyFun_zigbNwkRejoin, fun_Test, fun_Test);
		DcodeScan();
		
		thread_Timing();
		
		thread_Relay();
		
		thread_dataTrans();
		
		thread_Tips();
	}
	
	return 0;
}