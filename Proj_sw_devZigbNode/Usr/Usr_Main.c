#include "STC15Fxxxx.H"

#include "dataTrans.h"
#include "appTimer.h"
#include "pars_Method.h"
#include "dataManage.h"
#include "Tips.h"
#include "usrKin.h"
#include "Relay.h"
#include "timerAct.h"
#include "touchPad.h"
#include "driver_I2C_HXD019D.h"
#include "DS18B20.h"
#include "devlopeDebug.h"

#include "USART.h"
#include "delay.h"

void bsp_Init(void){

	appTimer0_Init();
	appTimer4_Init();
	zigbUart_pinInit();
	uartObjZigb_Init();
	tipLED_pinInit();
	pinBeep_Init();
	usrKin_pinInit();
	relay_pinInit();

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)
	infrared_pinInit();
	ds18b20_pinInit();
#endif
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
	
//	while(1)tips_specified(0);
	
	devStatus_switch.statusChange_standBy = status_nwkReconnect;
	devStatus_switch.statusChange_IF = 1;
	
//	while(1)PIN_RELAY_1 = touchPad_1;
	
//	while(1){
//	
//		P32 = !P32;
//		delay_ms(1);
//	}
	
	while(1){
		
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)  //设备类型为插座时，没有触摸按键驱动和拨码驱动
		UsrKEYScan(usrKeyFun_relayOpreation, usrKeyFun_zigbNwkRejoin, fun_factoryRecoverOpreat);
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)  //设备类型为红外转发器时，没有触摸按键驱动和拨码驱动
		UsrKEYScan(fun_Test, usrKeyFun_zigbNwkRejoin, fun_factoryRecoverOpreat);
		thread_infraredSM(); //红外线程
#else
		touchPad_Scan();
		DcodeScan();
		UsrKEYScan(fun_touchReset, usrKeyFun_zigbNwkRejoin, fun_factoryRecoverOpreat);
#endif
		
		thread_Timing();
		
		thread_Relay();
		
		thread_dataTrans();
		
		thread_Tips();
		
		touchPad_processThread();
		
		thread_devlopeDebug();
	}
	
	return 0;
}