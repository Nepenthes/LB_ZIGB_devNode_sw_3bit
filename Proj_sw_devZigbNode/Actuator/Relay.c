#include "Relay.h"

#include "Tips.h"
#include "timerAct.h"
#include "dataTrans.h"
#include "dataManage.h"

#include "eeprom.h"

/**********************本地文件变量定义区*****************************/
status_ifSave	xdata relayStatus_ifSave = statusSave_disable;	//开关记忆使能变量
u8 				xdata status_Relay 		 = 0;

relay_Command	xdata swCommand_fromUsr	 = {0, actionNull};

u8				xdata EACHCTRL_realesFLG = 0; //互控动作更新使能标志（发码）标志<bit0：一位开关互控更新; bit1：二位开关互控更新; bit2：三位开关互控更新;>
bit					  EACHCTRL_reportFLG = 0; //互控触发后向网关上报状态使能

relayStatus_PUSH xdata devActionPush_IF = {0};

/*继电器状态更新，硬件执行*/
void relay_statusReales(void){
	
	if(DEV_actReserve & 0x01)(status_Relay & 0x01)?(PIN_RELAY_1 = 1):(PIN_RELAY_1 = 0);
	if(DEV_actReserve & 0x02)(status_Relay & 0x02)?(PIN_RELAY_2 = 1):(PIN_RELAY_2 = 0);
	if(DEV_actReserve & 0x04)(status_Relay & 0x04)?(PIN_RELAY_3 = 1):(PIN_RELAY_3 = 0);

	tips_statusChangeToNormal();
}

/*开关初始化*/
void relay_pinInit(void){
	
	u8 idata statusTemp = 0;

	//推挽
	P3M1	&= ~0x38;
	P3M0	|= 0x38;
	
	PIN_RELAY_1 = PIN_RELAY_2 = PIN_RELAY_3 = 0;
	
	if(relayStatus_ifSave == statusSave_enable){
		
		EEPROM_read_n(EEPROM_ADDR_relayStatus, &statusTemp, 1);
		status_Relay = statusTemp;
		relay_statusReales(); //硬件加载
		
	}else{
	
		statusTemp = 0;
		coverEEPROM_write_n(EEPROM_ADDR_relayStatus, &statusTemp, 1);
		relay_statusReales(); //硬件加载
	}
}

/*开关动作*/
void relay_Act(relay_Command dats){
	
	u8 statusTemp = 0;
	
	statusTemp = status_Relay; //当前开关值暂存
	
	switch(dats.actMethod){
	
		case relay_flip:{ 
			
			if(dats.objRelay & 0x01)status_Relay ^= 1 << 0;
			if(dats.objRelay & 0x02)status_Relay ^= 1 << 1;
			if(dats.objRelay & 0x04)status_Relay ^= 1 << 2;
				
		}break;
		
		case relay_OnOff:{
			
			(dats.objRelay & 0x01)?(status_Relay |= 1 << 0):(status_Relay &= ~(1 << 0));
			(dats.objRelay & 0x02)?(status_Relay |= 1 << 1):(status_Relay &= ~(1 << 1));
			(dats.objRelay & 0x04)?(status_Relay |= 1 << 2):(status_Relay &= ~(1 << 2));
			
		}break;
		
		default:break;
		
	}relay_statusReales(); //硬件加载
	
	devActionPush_IF.dats_Push = 0;
	devActionPush_IF.dats_Push |= (status_Relay & 0x07); //当前开关值位填装<低三位>
	if(		(statusTemp & 0x01) != (status_Relay & 0x01))devActionPush_IF.dats_Push |= 0x20; //更改值填装<高三位>第一位
	else if((statusTemp & 0x02) != (status_Relay & 0x02))devActionPush_IF.dats_Push |= 0x40; //更改值填装<高三位>第二位
	else if((statusTemp & 0x04) != (status_Relay & 0x04))devActionPush_IF.dats_Push |= 0x80; //更改值填装<高三位>第三位
	
	if(status_Relay)delayCnt_closeLoop = 0; //开关一旦打开立刻更新绿色模式时间计数值
	
	if(relayStatus_ifSave == statusSave_enable){ //开关状态记忆
	
		statusTemp = status_Relay;
		coverEEPROM_write_n(EEPROM_ADDR_relayStatus, &statusTemp, 1);
	}
}

/*继电器主线程*/
void thread_Relay(void){
	
	if(swCommand_fromUsr.actMethod != actionNull){ //请求响应
	
		relay_Act(swCommand_fromUsr);
		
		swCommand_fromUsr.actMethod = actionNull;
		swCommand_fromUsr.objRelay = 0;
	}
}