#include "appTimer.h"

#include "STC15Fxxxx.H"

#include "stdio.h"
#include "string.h"

#include "USART.h"

#include "Relay.h"
#include "timerAct.h"
#include "touchPad.h"
#include "dataTrans.h"
#include "Tips.h"
#include "usrKin.h"

#include "devlopeDebug.h"

//***************数据传输变量引用区***************************/
extern bit 				rxTout_count_EN;	
extern u8  				rxTout_count;	//串口接收超时计数
extern bit 				uartRX_toutFLG;
extern u8 				datsRcv_length;
extern uartTout_datsRcv xdata datsRcv_ZIGB;

extern u16 xdata 		zigbNwkAction_counter; //zigb网络重连专用动作时间计数

extern u16 xdata 		dtReqEx_counter; //扩展型数据发送间隔计时值

extern bit 				heartBeatCycle_FLG;	//心跳周期触发标志
extern u8 xdata			heartBeatCount;	//心跳计数
extern u8 xdata			heartBeatPeriod; //心跳周期
extern u8 xdata 		heartBeatHang_timeCnt;

extern u8 xdata 		colonyCtrlGet_queryCounter; 
extern u8 xdata 		colonyCtrlGetHang_timeCnt;

//***************按键输入变量引用区***************************/
extern bit		 		usrKeyCount_EN;
extern u16		 		usrKeyCount;

extern u16 xdata 		touchPadActCounter;
extern u16 xdata 		touchPadContinueCnt;

//***************Tips变量引用区***************************/
extern u16 xdata 		counter_tipsAct;

/*-----------------------------------------------------------------------------------------------*/
void appTimer0_Init(void){	//50us 中断@24.000M

	AUXR |= 0x80;		
	TMOD &= 0xF0;		
	TL0   = 0x50;		
	TH0   = 0xFB;	
	TF0   = 0;	
	ET0	  = 1;	//开中断
	PT0   = 1;	
	
	TR0   = 1;		
}

void appTimer4_Init(void){	//50us 中断@24.000M
	
	T4T3M 	|= 0x20;		
	T4L 	= 0x50;		
	T4H 	= 0xFB;		
	T4T3M 	|= 0x80;	

	IE2 	|= 0x40;
}

void timer0_Rountine (void) interrupt TIMER0_VECTOR{
	
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_FANS)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
	{
		
		u8 xdata freq_periodBeatHalf = freq_Param.periodBeat_cfm / 2;

		freq_Param.periodBeat_counter ++; //电源频率单周期节拍数更新
		
		if(freq_Param.pwm_actEN){		
		
			freq_Param.pwm_actCounter ++;
			
			if(freq_Param.pwm_actCounter <= status_Relay && freq_Param.pwm_actCounter < freq_periodBeatHalf){ //前半周
				
				PIN_PWM_OUT = 1;
				
			}else{
			
				freq_Param.pwm_actCounter = 0;
				freq_Param.pwm_actEN = 0;
				PIN_PWM_OUT = 0;
			}
			
//			if(freq_Param.pwm_actCounter <= freq_periodBeatHalf){
//			
//				if(freq_Param.pwm_actCounter < status_Relay){
//					
//					PIN_PWM_OUT = 1;
//					
//				}else{
//				
//					PIN_PWM_OUT = 0;
//				}
//				
////				PIN_PWM_OUT = 0;
//			
//			}else
//			if(freq_Param.pwm_actCounter > freq_periodBeatHalf && freq_Param.pwm_actCounter <= freq_Param.periodBeat_cfm){
//				
////				if((freq_Param.pwm_actCounter - freq_periodBeatHalf) < status_Relay){
////					
////					PIN_PWM_OUT = 1;
////					
////				}else{
////				
////					PIN_PWM_OUT = 0;
////				}
//				
//				PIN_PWM_OUT = 0;

//			}else{
//			
//				freq_Param.pwm_actCounter = 0;
//				freq_Param.pwm_actEN = 0;
//				PIN_PWM_OUT = 0;
//			}
			
		}else{
		
			PIN_PWM_OUT = 0;
		}
	}
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
#else
#endif
}

void timer4_Rountine (void) interrupt TIMER4_VECTOR{
	
	u16 code period_1second = 20000;
	static u16 counter_1second = 0; 
	
	u8 code period_1ms 		= 20;
	static u8 counter_1ms 	= 0; 
	
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_FANS)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
#else
	u16 code period_100ms 	= 2000;
	static u16 counter_100ms = 0; 
	u8 code period10_100ms 	= 10;
	static u8 counter10_100ms = 0; 
#endif
	
	u8 code period_tipsColor = COLORGRAY_MAX * 3;
	static u8 counter_tipsColor = 0; 
	static color_Attr xdata cnt_relay1_Tips = {0};
	static color_Attr xdata cnt_relay2_Tips = {0};
	static color_Attr xdata cnt_relay3_Tips = {0};
	static color_Attr xdata cnt_zigbNwk_Tips = {0};
	
	static u8 xdata period_beep = 3;		//beep专用
	static u8 xdata	count_beep 	= 0;
	 
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_FANS)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
#else
	//****************100ms特殊**********************************************/
	if(counter_100ms < period_100ms)counter_100ms ++;
	else{
		
		counter_100ms = 0;
		counter10_100ms ++;
	
		/*窗帘逻辑业务，按照轨道时间动作*/
		if(SWITCH_TYPE == SWITCH_TYPE_CURTAIN){
		
			switch(curtainAct_Param.act){
			
				case cTact_open:{
				
					if(curtainAct_Param.act_counter < curtainAct_Param.act_period){
					
						if(counter10_100ms >= period10_100ms)curtainAct_Param.act_counter ++;
						
					}else{
					
						curtainAct_Param.act = cTact_stop;
					}
					
				}break;
					
				case cTact_close:{
				
					if(curtainAct_Param.act_counter > 0){
					
						if(counter10_100ms >= period10_100ms)curtainAct_Param.act_counter --;
						
					}else{
					
						curtainAct_Param.act = cTact_stop;
					}
				
				}break;
					
				case cTact_stop:{
				
					if(status_Relay != 2){
					
						swCommand_fromUsr.objRelay = 2;
						swCommand_fromUsr.actMethod = relay_OnOff;
						devActionPush_IF.push_IF = 1; //推送使能
					}
					
				}break;
					
				default:{}break;
			}
		}
		
		if(counter10_100ms >= period10_100ms)counter10_100ms = 0;
	}
#endif
	
	//****************1ms专用**********************************************/
	if(counter_1ms < period_1ms)counter_1ms ++;
	else{
	
		counter_1ms = 0;
		
		/*zigb专用动作时间计数*/
		if(zigbNwkAction_counter)zigbNwkAction_counter --;
		
		/*用户按键动作专用时间计数*/
		if(usrKeyCount_EN)usrKeyCount ++;
		else usrKeyCount = 0;
		
		/*触摸按键动作专用时间计数*/
		if(touchPadActCounter)touchPadActCounter --;
		
		/*触摸按键连按专用时间计数*/
		if(touchPadContinueCnt)touchPadContinueCnt --;
		
		/*Tips动作专用时间计数*/
		if(counter_tipsAct)counter_tipsAct --;
		
		/*扩展性(持续性)数据发送动作间隔时间计数*/
		if(dtReqEx_counter)dtReqEx_counter --;
	}
	
	//****************1s专用**********************************************/
	if(counter_1second < period_1second)counter_1second ++;
	else{
	
		counter_1second = 0;
		
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_FANS)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
//		/*电源频率单周期节拍数统计*/
//		/*>>>usr_debug<<<*/
//		//usr_debug数据填装
//		dev_debugInfoLog.debugInfoData.dimmerInfo.soureFreq = freq_Param.periodBeat_cfm;
//		//usr_debug打印类型填装填装
//		dev_debugInfoLog.debugInfoType = infoType_dimmerFreq;
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
#else
#endif
		/*心跳包计时计数业务*/
		if(!heartBeatCycle_FLG){
		
			if(heartBeatCount < heartBeatPeriod)heartBeatCount ++;
			else{
			
				heartBeatCount = 0;
				heartBeatCycle_FLG = 1;
			}
		}
		
		/*延时计时业务，到点动作*/
		if(ifDelay_sw_running_FLAG & (1 << 1)){
		
			if(delayCnt_onoff < ((u16)delayPeriod_onoff * 60))delayCnt_onoff ++;
			else{
			
				delayPeriod_onoff = delayCnt_onoff = 0; 
				
				ifDelay_sw_running_FLAG &= ~(1 << 1);	//一次性标志清除
				
				swCommand_fromUsr.actMethod = relay_OnOff; //开关动作
				swCommand_fromUsr.objRelay = delayUp_act;
				devActionPush_IF.push_IF = 1; //推送使能
				dev_agingCmd_sndInitative.agingCmd_delaySetOpreat = 1; //对应主动上传时效占位置一
				
				/*>>>usr_debug<<<*/
				//usr_debug数据填装
				dev_debugInfoLog.debugInfoData.delayActInfo.delayAct_Up = 1;
				//usr_debug打印类型填装填装
				dev_debugInfoLog.debugInfoType = infoType_delayUp;
			}
		}
		
		/*绿色模式计时业务，循环关闭*/
		if((ifDelay_sw_running_FLAG & (1 << 0)) && status_Relay){
		
			if(delayCnt_closeLoop < ((u16)delayPeriod_closeLoop * 60))delayCnt_closeLoop ++;
			else{
				
				delayCnt_closeLoop = 0;
			
				swCommand_fromUsr.actMethod = relay_OnOff; //开关动作
				swCommand_fromUsr.objRelay = 0;
				devActionPush_IF.push_IF = 1; //推送使能
				dev_agingCmd_sndInitative.agingCmd_greenModeSetOpreat = 1; //对应主动上传时效占位置一
			}
		}
		
		/*场景周期询查挂起计时值更新*///挂起作用<<
		if(heartBeatHang_timeCnt)heartBeatHang_timeCnt --;
		
		/*心跳挂起计时值更新*///挂起作用<<
		if(colonyCtrlGetHang_timeCnt)colonyCtrlGetHang_timeCnt --;
		
		/*系统时间本地维持计数值更新*/
		sysTimeKeep_counter ++;
		
		/*tips空闲释放计时计数业务*/
		if(counter_ifTipsFree && countEN_ifTipsFree)counter_ifTipsFree --;
		
		/*系统时间周期新更新计时计数业务*/
		if(sysTimeReales_counter)sysTimeReales_counter --;
		
		/*zigb网络开放倒计时*/
		if(tipsTimeCount_zigNwkOpen)tipsTimeCount_zigNwkOpen --;
		
		/*设备网络挂起时间倒计时*///挂起作用<<
		if(devNwkHoldTime_Param.devHoldTime_counter)devNwkHoldTime_Param.devHoldTime_counter --;
		
		/*集群受控状态周期性轮询周期计时*/
		if(colonyCtrlGet_queryCounter)colonyCtrlGet_queryCounter --;
		
		/*触摸IC复位时间倒计时*/
		if(touchPad_resetTimeCount)touchPad_resetTimeCount --;
		
		/*触摸IC复位Tips倒计时*/
		if(tipsTimeCount_touchReset)tipsTimeCount_touchReset --;
		
		/*恢复预置动作倒计时*/
		if(factoryRecover_HoldTimeCount)factoryRecover_HoldTimeCount --;
		
		/*恢复出厂Tips倒计时*/
		if(tipsTimeCount_factoryRecover)tipsTimeCount_factoryRecover --;
		
		/*协调器失联/丢失 确认倒计时*/
		if(timeCounter_coordinatorLost_detecting)timeCounter_coordinatorLost_detecting --;
	}

	//***************串口接收超时时长计数*******************************//
	if(rxTout_count_EN){ //接收超时计时使能判断
	
		if(rxTout_count < TimeOutSet1)rxTout_count ++;
		else{
			
			if(!uartRX_toutFLG && datsRcv_length >= 5){ //超时时间判断及最小帧长判断
			
				uartRX_toutFLG = 1;
			
				memset(datsRcv_ZIGB.rcvDats, 0xff, sizeof(char) * COM_RX1_Lenth);
				memcpy(datsRcv_ZIGB.rcvDats, RX1_Buffer, COM_RX1_Lenth);
				datsRcv_ZIGB.rcvDatsLen = datsRcv_length;
				
				/*>>>usr_debug<<<*/
				if(datsRcv_length != (datsRcv_ZIGB.rcvDats[1] + 5)){  //标的帧长判断，是否超长
				
					//usr_debug数据填装
					dev_debugInfoLog.debugInfoData.frameInfo.frameIllegal_FLG = 1;
					dev_debugInfoLog.debugInfoData.frameInfo.frame_aLength = RX1_Buffer[1];
					dev_debugInfoLog.debugInfoData.frameInfo.frame_rLength = datsRcv_length;
					//usr_debug打印类型填装填装
					dev_debugInfoLog.debugInfoType = infoType_frameUart;
					
				}else{
				

				}
			}
			rxTout_count_EN = 0;
		}
	}
	
	//*******************beep计时计数业务**************************/
	if(count_beep < period_beep)count_beep ++;
	else{
		
		static u16 xdata 	tips_Period = 20 * 50 / 2;
		static u16 xdata 	tips_Count 	= 0;
		static u8 xdata 	tips_Loop 	= 2 * 4;
		static bit 			beeps_en 	= 1;
	
		count_beep = 0;

		switch(dev_statusBeeps){ //状态机
			
			case beepsMode_standBy:{
				
				period_beep = devTips_beep.tips_Period;
				tips_Period = 20 * devTips_beep.tips_time / period_beep;
				tips_Loop 	= 2 * devTips_beep.tips_loop;
				tips_Count 	= 0;
				beeps_en 	= 1;
				dev_statusBeeps = beepsWorking;
	
			}break;
			
			case beepsWorking:{
			
				if(tips_Loop){
				
					if(tips_Count < tips_Period){
					
						tips_Count ++;
						(beeps_en)?(PIN_BEEP = !PIN_BEEP):(PIN_BEEP = 1);
						
					}else{
					
						tips_Count = 0;
						beeps_en = !beeps_en;
						tips_Loop --;
					}
					
				}else{
				
					dev_statusBeeps = beepsComplete;
				}
			
			}break;
			
			case beepsComplete:{
			
				tips_Count = 0;
				beeps_en = 1;
				PIN_BEEP = 1;
				dev_statusBeeps = beepsMode_null;
				
			}break;
		
			default:{
			
				PIN_BEEP = 1;
				
			}break;
		}
	}
	
	//***************tips_Led 刷新业务*******************************//
	if(counter_tipsColor > period_tipsColor){	//灰度值值加载
	
		counter_tipsColor = 0;
		
		cnt_relay1_Tips.colorGray_R = relay1_Tips.colorGray_R;
		cnt_relay1_Tips.colorGray_G = relay1_Tips.colorGray_G;
		cnt_relay1_Tips.colorGray_B = relay1_Tips.colorGray_B;
		
		cnt_relay2_Tips.colorGray_R = relay2_Tips.colorGray_R;
		cnt_relay2_Tips.colorGray_G = relay2_Tips.colorGray_G;
		cnt_relay2_Tips.colorGray_B = relay2_Tips.colorGray_B;
		
		cnt_relay3_Tips.colorGray_R = relay3_Tips.colorGray_R;
		cnt_relay3_Tips.colorGray_G = relay3_Tips.colorGray_G;
		cnt_relay3_Tips.colorGray_B = relay3_Tips.colorGray_B;
		
		cnt_zigbNwk_Tips.colorGray_R = zigbNwk_Tips.colorGray_R;
		cnt_zigbNwk_Tips.colorGray_G = zigbNwk_Tips.colorGray_G;
		cnt_zigbNwk_Tips.colorGray_B = zigbNwk_Tips.colorGray_B;
	}
	else{ //pwm执行
	
		counter_tipsColor ++;
		
		if((counter_tipsColor > 0) && (counter_tipsColor <= (COLORGRAY_MAX * 1))){
			
			 //指示可用核准
			if(cnt_relay1_Tips.colorGray_R && (DEV_actReserve & 0x01)){cnt_relay1_Tips.colorGray_R --; PIN_TIPS_RELAY1_R = 0;}
			else PIN_TIPS_RELAY1_R = 1;
			if(cnt_relay2_Tips.colorGray_R && (DEV_actReserve & 0x02)){cnt_relay2_Tips.colorGray_R --; PIN_TIPS_RELAY2_R = 0;}
			else PIN_TIPS_RELAY2_R = 1;
			if(cnt_relay3_Tips.colorGray_R && (DEV_actReserve & 0x04)){cnt_relay3_Tips.colorGray_R --; PIN_TIPS_RELAY3_R = 0;}
			else PIN_TIPS_RELAY3_R = 1;
			if(cnt_zigbNwk_Tips.colorGray_R){cnt_zigbNwk_Tips.colorGray_R --; PIN_TIPS_ZIGBNWK_R = 0;}
			else PIN_TIPS_ZIGBNWK_R = 1;
			
		}else
		if((counter_tipsColor > (COLORGRAY_MAX * 1)) && (counter_tipsColor <= (COLORGRAY_MAX * 2))){
		
			 //指示可用核准
			if(cnt_relay1_Tips.colorGray_G && (DEV_actReserve & 0x01)){cnt_relay1_Tips.colorGray_G --; PIN_TIPS_RELAY1_G = 0;}
			else PIN_TIPS_RELAY1_G = 1;
			if(cnt_relay2_Tips.colorGray_G && (DEV_actReserve & 0x02)){cnt_relay2_Tips.colorGray_G --; PIN_TIPS_RELAY2_G = 0;}
			else PIN_TIPS_RELAY2_G = 1;
			if(cnt_relay3_Tips.colorGray_G && (DEV_actReserve & 0x04)){cnt_relay3_Tips.colorGray_G --; PIN_TIPS_RELAY3_G = 0;}
			else PIN_TIPS_RELAY3_G = 1;
			if(cnt_zigbNwk_Tips.colorGray_G){cnt_zigbNwk_Tips.colorGray_G --; PIN_TIPS_ZIGBNWK_G = 0;}
			else PIN_TIPS_ZIGBNWK_G = 1;
			
		}else
		if((counter_tipsColor > (COLORGRAY_MAX * 2)) && (counter_tipsColor <= (COLORGRAY_MAX * 3))){
		
			 //指示可用核准
			if(cnt_relay1_Tips.colorGray_B && (DEV_actReserve & 0x01)){cnt_relay1_Tips.colorGray_B --; PIN_TIPS_RELAY1_B = 0;}
			else PIN_TIPS_RELAY1_B = 1;
			if(cnt_relay2_Tips.colorGray_B && (DEV_actReserve & 0x02)){cnt_relay2_Tips.colorGray_B --; PIN_TIPS_RELAY2_B = 0;}
			else PIN_TIPS_RELAY2_B = 1;
			if(cnt_relay3_Tips.colorGray_B && (DEV_actReserve & 0x04)){cnt_relay3_Tips.colorGray_B --; PIN_TIPS_RELAY3_B = 0;}
			else PIN_TIPS_RELAY3_B = 1;
			if(cnt_zigbNwk_Tips.colorGray_B){cnt_zigbNwk_Tips.colorGray_B --; PIN_TIPS_ZIGBNWK_B = 0;}
			else PIN_TIPS_ZIGBNWK_B = 1;
		}
	}
}