#include "Relay.h"

#include "string.h"
#include "stdio.h"

#include "Tips.h"
#include "timerAct.h"
#include "appTimer.h"
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

bit				idata statusRelay_saveEn= 0; //开关值本地存储使能,灵活使用,重复存储

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
stt_Dimmer_attrFreq	xdata dimmer_freqParam		= {0};
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
stt_eleSocket_attrFreq xdata socket_eleDetParam = {0};
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SCENARIO)
stt_scenario_attrAct xdata scenario_ActParam = {0};
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_HEATER)
stt_heater_attrAct xdata heater_ActParam = {0};
#else
stt_Curtain_motorAttr xdata curtainAct_Param = {0, CURTAIN_ORBITAL_PERIOD_INITTIME, cTact_stop}; //当设备定义为窗帘时，对应动作属性，默认轨道时间0s
bit 				  idata specialFlg_curtainEachctrlEn = 1;	//特殊标识位，窗帘互控同步使能，用于在场景控制下禁止触发互控
#endif

/*继电器状态更新，硬件执行*/
void relay_statusReales(void){
	
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_FANS)
	switch(status_Relay){
	
		case 0:{
		
			PIN_RELAY_1 = 0;PIN_RELAY_2 = 0;PIN_RELAY_3 = 0;
		
		}break;
		
		case 1:{
		
			PIN_RELAY_1 = 0;PIN_RELAY_2 = 0;PIN_RELAY_3 = 1;
			
		}break;
			
		case 2:{
		
			PIN_RELAY_1 = 0;PIN_RELAY_2 = 1;PIN_RELAY_3 = 0;
			
		}break;
			
		case 3:
		default:{
		
			PIN_RELAY_1 = 1;PIN_RELAY_2 = 0;PIN_RELAY_3 = 0;
			
		}break;
	}
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
	(status_Relay)?(PIN_RELAY_1 = 1):(PIN_RELAY_1 = 0);
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SCENARIO)
	{
		
		bit scenarioTrigReserve_flg = 0;
		
		if((!scenario_ActParam.scenarioKeepTrig_timeCounter) || (scenario_ActParam.scenarioKeepTrig_timeCounter == COUNTER_DISENABLE_MASK_SPECIALVAL_U8)){ //不同场景触发强制间隔时间结束后才允许动作
		
			switch(status_Relay){
			
				case 1:{
				
					scenario_ActParam.scenarioKey_currentTrig = scenarioKey_current_S1;
					scenarioTrigReserve_flg = 1;
				
				}break;
					
				case 2:{
				
					scenario_ActParam.scenarioKey_currentTrig = scenarioKey_current_S2;
					scenarioTrigReserve_flg = 1;
				
				}break;
					
				case 4:{
				
					scenario_ActParam.scenarioKey_currentTrig = scenarioKey_current_S3;
					scenarioTrigReserve_flg = 1;
				
				}break;
					
				default:{}break;
			}
			
			if(scenarioTrigReserve_flg){
			
				scenario_ActParam.scenarioKeepTrig_timeCounter = SCENARIOTRIG_KEEPTIME_PERIOD;
				scenario_ActParam.scenarioDataSend_FLG = 1;
			}
		}
	}
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_HEATER)
	switch(status_Relay){

		case 0:{
		
			PIN_RELAY_1 = PIN_RELAY_2 = 0;
			heater_ActParam.relayActDelay_counter = HEATER_RELAY_SYNCHRONIZATION_DELAYTIME; //大小继电器滞后时间设定
			
		}break;
		
		case 1:{
		
			PIN_RELAY_1 = PIN_RELAY_2 = 1;
			heater_ActParam.relayActDelay_counter = HEATER_RELAY_SYNCHRONIZATION_DELAYTIME; //大小继电器滞后时间设定
		
		}break;
		
		default:{

			
		}break;
	}
	
#else
	switch(SWITCH_TYPE){
		
		case SWITCH_TYPE_CURTAIN:{
		
			switch(status_Relay){
			
				case 1:{
				
					PIN_RELAY_2 = 1;
					PIN_RELAY_1 = PIN_RELAY_3 = 0;
					curtainAct_Param.act = cTact_open;
					
				}break;
					
				case 4:{
				
					PIN_RELAY_1 = 1;
					PIN_RELAY_2 = PIN_RELAY_3 = 0;
					curtainAct_Param.act = cTact_close;
					
				}break;
					
				case 2:
				default:{
				
					PIN_RELAY_1 = PIN_RELAY_2 = PIN_RELAY_3 = 0;
					curtainAct_Param.act = cTact_stop;
					
					if(curtainAct_Param.act != cTact_stop)coverEEPROM_write_n(EEPROM_ADDR_curtainOrbitalCnter, &(curtainAct_Param.act_counter), 1); //每次窗帘运动停止时，记录当前位置对应的轨道周期计时值
					
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
	
	statusRelay_saveEn = 1; //重复主动记忆当前启动值，防止开关未操作情况下第三次上电启动时记忆值丢失
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_FANS)
	//推挽
	P3M1	&= ~0x38;
	P3M0	|= 0x38;
	PIN_RELAY_1 = PIN_RELAY_2 = PIN_RELAY_3 = 0;
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
	//推挽
	P3M1	&= ~0x08;
	P3M0	|= 0x08;
	PIN_RELAY_1 = 0;
	
	//高阻入
	P3M1	|= 0xC0;
	P3M0	&= ~0xC0;
	INT_CLKO |=  (1 << 4); //外部中断2使能
	INT_CLKO |=  (1 << 5); //外部中断3使能

#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SCENARIO)
	//场景开关没有继电器
	//键位绑定场景号掉电记忆恢复
	EEPROM_read_n(EEPROM_ADDR_swTypeForceScenario_scencarioNumKeyBind, scenario_ActParam.scenarioNum_record, 3);

#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_HEATER)
	//推挽
	P3M1	&= ~0x38;
	P3M0	|= 0x38;
	PIN_RELAY_1 = PIN_RELAY_2 = PIN_RELAY_3 = 0;
	
#elif(SWITCH_TYPE_FORCEDEF == 0)
	//推挽
	P3M1	&= ~0x38;
	P3M0	|= 0x38;
	PIN_RELAY_1 = PIN_RELAY_2 = PIN_RELAY_3 = 0;
	
	//窗帘轨道周期及其对应位置计时值恢复
	EEPROM_read_n(EEPROM_ADDR_curtainOrbitalPeriod, &(curtainAct_Param.act_period), 1);
	if(curtainAct_Param.act_period == 0xff)curtainAct_Param.act_period = CURTAIN_ORBITAL_PERIOD_INITTIME; //值限定	
	EEPROM_read_n(EEPROM_ADDR_curtainOrbitalCnter, &(curtainAct_Param.act_counter), 1);
	if(curtainAct_Param.act_counter == 0xff)curtainAct_Param.act_counter = 0; //值限定	
	
 #if(DEBUG_LOGOUT_EN == 1)
	{ //输出打印，谨记 用后注释，否则占用大量代码空间
		
		memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
		sprintf(log_buf, ">>>curtain param recover orbitalPeriod:%d, placeCounter:%d\n", (int)curtainAct_Param.act_period, (int)curtainAct_Param.act_counter);
		PrintString1_logOut(log_buf);
	}			
 #endif
#else
	
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
	
	dimmer_freqParam.periodBeat_cfm = dimmer_freqParam.periodBeat_counter;
	dimmer_freqParam.periodBeat_counter = 0;
	
	dimmer_freqParam.pwm_actEN = 1;
}
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
void Ext_INT2 (void) interrupt INT2_VECTOR{ //中断2
	
	socket_eleDetParam.eleParamFun_powerPulseCount += 1.0F;
}

void Ext_INT3 (void) interrupt INT3_VECTOR{ //中断3
	
	
}
#else
#endif

/*开关动作*/
void relay_Act(relay_Command dats){
	
	u8 statusTemp = 0;
	
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER) //任意类型加载，不要自动存储记忆，因为长按按键连续更改亮度值都进行记忆的话话造成灯光亮度突变，长按在结束时进行记忆即可
	status_Relay = dats.objRelay;
	relay_statusReales();
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)
	dats = dats;
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SCENARIO)
	status_Relay = dats.objRelay;
	relay_statusReales();
	
#else
	
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
	
//	/*优先方式*/
//	if(		(statusTemp & 0x01) != (status_Relay & 0x01))devActionPush_IF.dats_Push |= 0x20; //更改值填装<高三位>第一位
//	else if((statusTemp & 0x02) != (status_Relay & 0x02))devActionPush_IF.dats_Push |= 0x40; //更改值填装<高三位>第二位
//	else if((statusTemp & 0x04) != (status_Relay & 0x04))devActionPush_IF.dats_Push |= 0x80; //更改值填装<高三位>第三位
	/*非优先方式*/
	if((statusTemp & 0x01) != (status_Relay & 0x01))devActionPush_IF.dats_Push |= 0x20; //更改值填装<高三位>第一位
	if((statusTemp & 0x02) != (status_Relay & 0x02))devActionPush_IF.dats_Push |= 0x40; //更改值填装<高三位>第二位
	if((statusTemp & 0x04) != (status_Relay & 0x04))devActionPush_IF.dats_Push |= 0x80; //更改值填装<高三位>第三位
	
	if(status_Relay)delayCnt_closeLoop = 0; //开关一旦打开立刻更新绿色模式时间计数值
	
	if(relayStatus_ifSave == statusSave_enable){ /*每次更改开关值时都进行存储记忆*///开关状态存储自动被动记忆
	
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
	
	if(statusRelay_saveEn){ /*主动记忆使能判断*///开关状态存储主动记忆
	
		u8 idata statusTemp = 0;
		
		statusRelay_saveEn = 0;
		
//#if(DEBUG_LOGOUT_EN == 1)
//		{ //输出打印，谨记 用后注释，否则占用大量代码空间
//			
//			memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
//			sprintf(log_buf, ">>>statusVal save cmp.\n");
//			PrintString1_logOut(log_buf);
//		}			
//#endif
	
		if(relayStatus_ifSave == statusSave_enable){ 
	
#if(DATASAVE_INTLESS_ENABLEIF)
			devParamDtaaSave_relayStatusRealTime(status_Relay);
#else
			statusTemp = status_Relay;
			coverEEPROM_write_n(EEPROM_ADDR_relayStatus, &statusTemp, 1);
#endif
		}
	}
	
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_HEATER)
	if(heater_ActParam.relayActDelay_actEn){ //同步触发标志位响应
	
		heater_ActParam.relayActDelay_actEn = 0;
		PIN_RELAY_3 = PIN_RELAY_1; //热水器继电器电平同步动作触发
	}
	
	if(heater_ActParam.heater_currentActMode == heaterActMode_swClose){ //补偿响应，避免有时候指示灯响应了但继电器没响应
	
		if((status_Relay & (1 << 0)) != 0){
		
			swCommand_fromUsr.objRelay = 0;
			swCommand_fromUsr.actMethod = relay_OnOff; //开关动作
		}
	}
	
#else
#endif
}