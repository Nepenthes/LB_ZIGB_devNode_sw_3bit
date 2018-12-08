#include "Relay.h"

#include "string.h"
#include "stdio.h"

#include "Tips.h"
#include "timerAct.h"
#include "dataTrans.h"
#include "dataManage.h"

#include "eeprom.h"

/**********************本地文件变量定义区*****************************/
status_ifSave	xdata relayStatus_ifSave = statusSave_disable;	//开关记忆使能变量
u8 				xdata status_Relay 		 = 0;

stt_motorAttr 	xdata curtainAct_Param 	 = {0, 3, cTact_stop};	//当设备定义为窗帘时，对应动作属性

relay_Command	xdata swCommand_fromUsr	 = {0, actionNull};

u8				xdata EACHCTRL_realesFLG = 0; //互控动作更新使能标志（发码）标志<bit0：一位开关互控更新; bit1：二位开关互控更新; bit2：三位开关互控更新;>
bit					  EACHCTRL_reportFLG = 0; //互控触发后向网关上报状态使能

relayStatus_PUSH xdata devActionPush_IF = {0};

bit				idata statusRelay_saveEn= 0; //开关值本地存储使能,灵活使用,重复存储

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
stt_attrFreq	xdata freq_Param		= {0};
#endif

/*继电器状态更新，硬件执行*/
void relay_statusReales(void){
	
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_FANS)
	switch(status_Relay){
	
		case 0:{
		
			PIN_RELAY_1 = 0;PIN_RELAY_2 = 0;PIN_RELAY_3 = 0;
		
		}break;
		
		case 1:{
		
			PIN_RELAY_1 = 1;PIN_RELAY_2 = 0;PIN_RELAY_3 = 0;
			
		}break;
			
		case 2:{
		
			PIN_RELAY_1 = 1;PIN_RELAY_2 = 1;PIN_RELAY_3 = 0;
			
		}break;
			
		case 3:
		default:{
		
			PIN_RELAY_1 = 1;PIN_RELAY_2 = 1;PIN_RELAY_3 = 1;
			
		}break;
	}
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
#else
	switch(SWITCH_TYPE){
		
		case SWITCH_TYPE_CURTAIN:{
		
			switch(status_Relay){
			
				case 1:{
				
					PIN_RELAY_1 = 1;
					PIN_RELAY_2 = PIN_RELAY_3 = 0;
					curtainAct_Param.act = cTact_open;
					
				}break;
					
				case 4:{
				
					PIN_RELAY_3 = 1;
					PIN_RELAY_1 = PIN_RELAY_2 = 0;
					curtainAct_Param.act = cTact_close;
					
				}break;
					
				case 2:
				default:{
				
					PIN_RELAY_1 = PIN_RELAY_2 = PIN_RELAY_3 = 0;
					curtainAct_Param.act = cTact_stop;
					
				}break;
			}
		
		}break;
	
		case SWITCH_TYPE_SWBIT1:{ //继电器位置调整 2对1
		
			if(DEV_actReserve & 0x02)(status_Relay & 0x01)?(PIN_RELAY_1 = 1):(PIN_RELAY_1 = 0);
			
		}break;
		
		case SWITCH_TYPE_SWBIT2:{ //继电器位置调整 3对2
		
			if(DEV_actReserve & 0x01)(status_Relay & 0x01)?(PIN_RELAY_1 = 1):(PIN_RELAY_1 = 0);
			if(DEV_actReserve & 0x04)(status_Relay & 0x02)?(PIN_RELAY_2 = 1):(PIN_RELAY_2 = 0);
		
		}break;
		
		case SWITCH_TYPE_SWBIT3:{ //继电器位置保持
		
			if(DEV_actReserve & 0x01)(status_Relay & 0x01)?(PIN_RELAY_1 = 1):(PIN_RELAY_1 = 0);
			if(DEV_actReserve & 0x02)(status_Relay & 0x02)?(PIN_RELAY_2 = 1):(PIN_RELAY_2 = 0);
			if(DEV_actReserve & 0x04)(status_Relay & 0x04)?(PIN_RELAY_3 = 1):(PIN_RELAY_3 = 0);
		
		}break;
		
		default:break;
	}
#endif	
	
	tips_statusChangeToNormal();
}

/*开关初始化*/
void relay_pinInit(void){
	
	u8 idata statusTemp = 0;
	
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
	//推挽
	P3M1	&= ~0x30;
	P3M0	|= 0x30;
	PIN_RELAY_2 = PIN_PWM_OUT = 0;
	
	P3M1	|= 0x08;   //P33过0中断检测脚
	P3M0	&= ~0x08;
    INT1 = 0;
    IT1 = 1; 
	PX1 = 0; //高优先级
    EX1 = 1;                    

#else
	//推挽
	P3M1	&= ~0x38;
	P3M0	|= 0x38;
	PIN_RELAY_1 = PIN_RELAY_2 = PIN_RELAY_3 = 0;
	
#endif
	
	if(relayStatus_ifSave == statusSave_enable){
	
#if(DATASAVE_INTLESS_ENABLEIF)
		swCommand_fromUsr.objRelay = devDataRecovery_relayStatus();
#else
		EEPROM_read_n(EEPROM_ADDR_relayStatus, &statusTemp, 1);
		swCommand_fromUsr.objRelay = statusTemp;
#endif
		swCommand_fromUsr.actMethod = relay_OnOff; //硬件加载
		
	}else{
	
		statusTemp = 0;
		coverEEPROM_write_n(EEPROM_ADDR_relayStatus, &statusTemp, 1);
		relay_statusReales(); //硬件加载
	}
}

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
void Ext_INT1 (void) interrupt INT1_VECTOR{
	
	freq_Param.periodBeat_cfm = freq_Param.periodBeat_counter;
	freq_Param.periodBeat_counter = 0;
	
	freq_Param.pwm_actEN = 1;
}
#endif

/*开关动作*/
void relay_Act(relay_Command dats){
	
	u8 statusTemp = 0;
	
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER) //任意类型加载，不自动存储
	status_Relay = dats.objRelay;
	relay_statusReales();
	
#else
	
	statusTemp = status_Relay; //当前开关值暂存
	
	if(!countEN_ifTipsFree)countEN_ifTipsFree = 1; //触摸释放计时使能
	
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
	
//	/*优先方式*/
//	if(		(statusTemp & 0x01) != (status_Relay & 0x01))devActionPush_IF.dats_Push |= 0x20; //更改值填装<高三位>第一位
//	else if((statusTemp & 0x02) != (status_Relay & 0x02))devActionPush_IF.dats_Push |= 0x40; //更改值填装<高三位>第二位
//	else if((statusTemp & 0x04) != (status_Relay & 0x04))devActionPush_IF.dats_Push |= 0x80; //更改值填装<高三位>第三位
	/*非优先方式*/
	if((statusTemp & 0x01) != (status_Relay & 0x01))devActionPush_IF.dats_Push |= 0x20; //更改值填装<高三位>第一位
	if((statusTemp & 0x02) != (status_Relay & 0x02))devActionPush_IF.dats_Push |= 0x40; //更改值填装<高三位>第二位
	if((statusTemp & 0x04) != (status_Relay & 0x04))devActionPush_IF.dats_Push |= 0x80; //更改值填装<高三位>第三位
	
	if(status_Relay)delayCnt_closeLoop = 0; //开关一旦打开立刻更新绿色模式时间计数值
	
	if(relayStatus_ifSave == statusSave_enable){ //开关状态记忆
	
 #if(DATASAVE_INTLESS_ENABLEIF)
		devParamDtaaSave_relayStatusRealTime(status_Relay);
 #else
		statusTemp = status_Relay;
		coverEEPROM_write_n(EEPROM_ADDR_relayStatus, &statusTemp, 1);
 #endif
	}
	
#endif
}

/*继电器主线程*/
void thread_Relay(void){
	
	if(swCommand_fromUsr.actMethod != actionNull){ //请求响应
	
		relay_Act(swCommand_fromUsr);
		
		swCommand_fromUsr.actMethod = actionNull;
		swCommand_fromUsr.objRelay = 0;
	}
	
	if(statusRelay_saveEn){
	
		u8 idata statusTemp = 0;
		
		statusRelay_saveEn = 0;
		
//#if(DEBUG_LOGOUT_EN == 1)
//		{ //输出打印，谨记 用后注释，否则占用大量代码空间
//			u8 xdata log_buf[64];
//			
//			sprintf(log_buf, ">>>statusVal save cmp.\n");
//			PrintString1_logOut(log_buf);
//		}			
//#endif
	
#if(DATASAVE_INTLESS_ENABLEIF)
		devParamDtaaSave_relayStatusRealTime(status_Relay);
#else
		statusTemp = status_Relay;
		coverEEPROM_write_n(EEPROM_ADDR_relayStatus, &statusTemp, 1);
#endif
	}
}