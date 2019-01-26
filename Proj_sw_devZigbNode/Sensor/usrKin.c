#include "usrKin.h"

#include "driver_I2C_HXD019D.h"

#include "Tips.h"
#include "dataTrans.h"
#include "dataManage.h"
#include "Relay.h"
#include "touchPad.h"

#include "delay.h"
#include "USART.h"

#include "stdio.h"
#include "string.h"

//***************Tips变量引用区***************************/
extern tips_Status devTips_status;

/**********************本地文件变量定义区**********************/
u8 idata val_DcodeCfm 			= 0;  //拨码值
bit		 ledBackground_method	= 1;  //背景灯颜色方案 //为1时：开-绿 关-蓝   为0时：开-蓝 关-绿

bit		 usrKeyCount_EN			= 0;  //用户按键计数
u16		 usrKeyCount			= 0;

u16	xdata touchPadActCounter	= 0;  //触摸盘按键计时
u16	xdata touchPadContinueCnt	= 0;  //触摸盘连按计时

u8	xdata touchKeepCnt_record	= 1;  //连按正在进行时计数变量，连按必定从一开始，否则不叫连按

u16 xdata combinationFunFLG_3S5S_cancel_counter  = 0;  //三短五短预触发标志_衔接时长取消计数，衔接时间过长时，将预触发标志取消

static param_combinationFunPreTrig param_combinationFunTrigger_3S1L = {0};
static param_combinationFunPreTrig param_combinationFunTrigger_3S5S = {0};

/*------------------------------------------------------------------------------------------------------------*/
///*按键回调函数缓存*///为减少代码冗余，此段弃用
//static fun_KeyTrigger xdata funTrig_keyTouch_1 = {0}; //连按可用 
//static fun_KeyTrigger xdata funTrig_keyTouch_2 = {0}; //连按可用
//static fun_KeyTrigger xdata funTrig_keyTouch_3 = {0}; //连按可用
//static fun_KeyTrigger xdata funTrig_keyButton  = {0}; //连按不可用

static void touchPad_functionTrigNormal(u8 statusPad, keyCfrm_Type statusCfm);
static void touchPad_functionTrigContinue(u8 statusPad, u8 loopCount);

///*按键触发回调函数注册*///为减少代码冗余，此函数弃用
//void funKeyTrigger_register(funKey_Callback funTrigger, objKey key, trig_Method mTrig, u8 pressCnt_num){ //回调函数，按键对象，触发方式，连按次数（大于2且 触发方式为连按 有效）

//	switch(key){
//	
//		case kinObj_touch_1:{
//		
//			switch(mTrig){
//			
//				case method_pressShort:	funTrig_keyTouch_1.press_Short = funTrigger;break;
//				case method_pressCnt:	if(pressCnt_num >= 2)funTrig_keyTouch_1.press_Continue[pressCnt_num] = funTrigger;break;
//				case method_pressLong_A:funTrig_keyTouch_1.press_Long_A = funTrigger;break;
//				case method_pressLong_B:funTrig_keyTouch_1.press_Long_B = funTrigger;break;
//					
//				default:break;
//			}
//		}break;
//			
//		case kinObj_touch_2:{
//			
//			switch(mTrig){
//			
//				case method_pressShort:	funTrig_keyTouch_2.press_Short = funTrigger;break;
//				case method_pressCnt:	if(pressCnt_num >= 2)funTrig_keyTouch_2.press_Continue[pressCnt_num] = funTrigger;break;
//				case method_pressLong_A:funTrig_keyTouch_2.press_Long_A = funTrigger;break;
//				case method_pressLong_B:funTrig_keyTouch_2.press_Long_B = funTrigger;break;
//					
//				default:break;
//			}
//		}break;
//			
//		case kinObj_touch_3:{
//			
//			switch(mTrig){
//			
//				case method_pressShort:	funTrig_keyButton.press_Short = funTrigger;break;
//				case method_pressLong_A:funTrig_keyButton.press_Long_A = funTrigger;break;
//				case method_pressLong_B:funTrig_keyButton.press_Long_B = funTrigger;break;
//				
//				case method_pressCnt: //连按不可用
//				default:break;
//			}
//		}break;
//			
//		case kinObj_button:{
//		
//			switch(mTrig){
//			
//				case method_pressShort:	funTrig_keyTouch_3.press_Short = funTrigger;break;
//				case method_pressCnt:	if(pressCnt_num >= 2)funTrig_keyTouch_3.press_Continue[pressCnt_num] = funTrigger;break;
//				case method_pressLong_A:funTrig_keyTouch_3.press_Long_A = funTrigger;break;
//				case method_pressLong_B:funTrig_keyTouch_3.press_Long_B = funTrigger;break;
//					
//				default:break;
//			}
//		}break;
//			
//		default:break;
//	}
//}

void usrZigbNwkOpen(void){
	
	if(devTips_nwkZigb != nwkZigb_outLine &&  //网关失联，通信异常，不允许开放
	   devRunning_Status != status_nwkREQ &&  //本身无网络不允许开放
	   devRunning_Status != status_nwkReconnect){  //本身无网络不允许开放
	
		ZigB_nwkOpen(1, ZIGBNWK_OPNETIME_DEFAULT); //功能触发
		tips_statusChangeToZigbNwkOpen(ZIGBNWK_OPNETIME_DEFAULT); //tips触发
#if(DEBUG_LOGOUT_EN == 1)
		{ //输出打印，谨记 用后注释，否则占用大量代码空间
			memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
			sprintf(log_buf, "touchPad special trig:nwkOpen:%02ds.\n", (int)ZIGBNWK_OPNETIME_DEFAULT);
			PrintString1_logOut(log_buf);
		}			
#endif	
	}
}

void devTypeComfirm_byDcode(u8 valDcode){
	
	valDcode = valDcode;

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_FANS)
	SWITCH_TYPE = SWITCH_TYPE_FANS;
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
	SWITCH_TYPE = SWITCH_TYPE_dIMMER;
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
	SWITCH_TYPE = SWITCH_TYPE_SOCKETS;
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SCENARIO)
	SWITCH_TYPE = SWITCH_TYPE_SCENARIO;
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)
	SWITCH_TYPE = SWITCH_TYPE_INFRARED;	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_HEATER)
	switch(valDcode){
	
		case 1:{
		
			heater_ActParam.touchAction_defineJustSwitch_IF = 1;
		
		}break;

		default:{
		
			heater_ActParam.touchAction_defineJustSwitch_IF = 0;
		
		}break;
	}
#else
	switch(valDcode){
	
		case 0:{
		
			SWITCH_TYPE = SWITCH_TYPE_CURTAIN;	
			
		}break;
			
		case 1:{
		
			SWITCH_TYPE = SWITCH_TYPE_SWBIT1;	

		}break;
			
		case 2:{
		
			SWITCH_TYPE = SWITCH_TYPE_SWBIT2;	

		}break;
			
		case 3:{
			
			SWITCH_TYPE = SWITCH_TYPE_SWBIT3;	

		}break;
			
		default:break;
	}
#endif
}

void usrKin_pinInit(void){
	
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
	P2M1 |= 0x10;
	P2M0 &= ~(0x10);

#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)
	P5M1 &= ~ 0x10;
	P5M0 &= ~(0x10);
	
#else
	P1M1 &= ~(0xE0);
	P1M0 &= ~(0xE0);

	P0M1 &= ~(0x04);
	P0M0 &= ~(0x04);
	
	{ //拨码预检测
	
		u8 touchVal_temp = DcodeScan_oneShoot();
		
		if(touchVal_temp & Dcode_FLG_ifMemory)relayStatus_ifSave = statusSave_enable; //提前检测
		devTypeComfirm_byDcode(Dcode_bitReserve(touchVal_temp));
	}
	
#endif
}

bit UsrKEYScan_oneShoot(void){

	return Usr_key;
}

void UsrKEYScan(funKey_Callback funCB_Short, funKey_Callback funCB_LongA, funKey_Callback funCB_LongB){
	
	code	u16	keyCfrmLoop_Short 	= 10,	//短按消抖时间,据大循环而定
				keyCfrmLoop_LongA 	= 3000,	//长按A时间,据大循环而定
				keyCfrmLoop_LongB 	= 12000,//长按B时间,据大循环而定
				keyCfrmLoop_MAX	 	= 60000;//计时封顶
	
	static	bit LongA_FLG = 0;
	static	bit LongB_FLG = 0;
	
	static	bit keyPress_FLG = 0;

	if(!UsrKEYScan_oneShoot()){		
		
		keyPress_FLG = 1;
		
//		tips_statusChangeToNormal();
	
		if(!usrKeyCount_EN) usrKeyCount_EN= 1;	//计时
		
		if((usrKeyCount >= keyCfrmLoop_LongA) && (usrKeyCount <= keyCfrmLoop_LongB) && !LongA_FLG){
		
			funCB_LongA();
			
			LongA_FLG = 1;
		}	
		
		if((usrKeyCount >= keyCfrmLoop_LongB) && (usrKeyCount <= keyCfrmLoop_MAX) && !LongB_FLG){
		
			funCB_LongB();
			
			LongB_FLG = 1;
		}
		
	}else{		
		
		usrKeyCount_EN = 0;
		
		if(keyPress_FLG){
		
			keyPress_FLG = 0;
			
			if(usrKeyCount < keyCfrmLoop_LongA && usrKeyCount > keyCfrmLoop_Short){
			
//				static bit tipsFLG = 0;
//				 
//				tipsFLG = !tipsFLG;
//				(tipsFLG)?(tipsLED_colorSet(obj_zigbNwk, 5, 0, 0)):(tipsLED_colorSet(obj_zigbNwk, 0, 5, 0));
				
				funCB_Short();
			}
			
			usrKeyCount = 0;
			LongA_FLG 	= 0;
			LongB_FLG 	= 0;
		}
	}
}

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS) //设备类型为插座时，没有触摸按键驱动和拨码驱动
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)  //设备类型为红外转发器时，没有触摸按键驱动和拨码驱动
#else //除上述开关类型外，有触摸按键驱动和拨码驱动
u8 DcodeScan_oneShoot(void){

	u8 val_Dcode = 0;
	
	if(!Dcode0)val_Dcode |= 1 << 0;
	else val_Dcode &= ~(1 << 0);
	
	if(!Dcode1)val_Dcode |= 1 << 1;
	else val_Dcode &= ~(1 << 1);
	
	if(!Dcode2)val_Dcode |= 1 << 2;
	else val_Dcode &= ~(1 << 2);
	
	if(!Dcode3)val_Dcode |= 1 << 3;
	else val_Dcode &= ~(1 << 3);
	
	if(!Dcode4)val_Dcode |= 1 << 4;
	else val_Dcode &= ~(1 << 4);
	
	if(!Dcode5)val_Dcode |= 1 << 5;
	else val_Dcode &= ~(1 << 5);
	
	return val_Dcode;
}

void DcodeScan(void){

	static u8 	val_Dcode_Local 	= 0,
				comfirm_Cnt			= 0;
	const  u8 	comfirm_Period		= 200;	//消抖时间因数——取决于主线程调度周期
		
		   u8 	val_Dcode_differ	= 0;
	
		   bit	val_CHG				= 0;
	
	val_DcodeCfm = DcodeScan_oneShoot();
	
	DEV_actReserve = switchTypeReserve_GET(); //当前开关类型对应有效操作位刷新
	
	if(val_Dcode_Local != val_DcodeCfm){
	
		if(comfirm_Cnt < comfirm_Period)comfirm_Cnt ++;
		else{
		
			comfirm_Cnt = 0;
			val_CHG		= 1;
		}
	}
	
	if(val_CHG){
		
		val_CHG				= 0;
	
		val_Dcode_differ 	= val_Dcode_Local ^ val_DcodeCfm;
		val_Dcode_Local		= val_DcodeCfm;
		
		beeps_usrActive(3, 40, 2);
		tips_statusChangeToNormal();
		
		if(val_Dcode_differ & Dcode_FLG_ifAP){
		
			if(val_Dcode_Local & Dcode_FLG_ifAP){
			

			}else{
			

			}
		}
		
		if(val_Dcode_differ & Dcode_FLG_ifLED){
		
			if(val_Dcode_Local & Dcode_FLG_ifLED){

				
			}else{
			

			}
		}
		
		if(val_Dcode_differ & Dcode_FLG_ifMemory){
		
			if(val_Dcode_Local & Dcode_FLG_ifMemory){

				relayStatus_ifSave = statusSave_enable;
				
			}else{
			
				relayStatus_ifSave = statusSave_disable;
			}
		}
		
		if(val_Dcode_differ & Dcode_FLG_bitReserve){
		
			devTypeComfirm_byDcode(Dcode_bitReserve(val_Dcode_Local));
		}
	}
}

static void normalBussiness_longA_touchKeepTrig(u8 statusPad){

	static u8 xdata trigCount_Loop = 0;
		   u8 code trigCount_Period = 30;
	
	if(trigCount_Loop < trigCount_Period)trigCount_Loop ++;
	else{
	
		trigCount_Loop = 0;
		
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)		
		switch(statusPad){
			
			case 1:{
				
				(status_Relay < 100)?(swCommand_fromUsr.objRelay = ++status_Relay):(swCommand_fromUsr.objRelay = 100);
			
			}break;
			
			case 4:{
		
				if(status_Relay > 0)swCommand_fromUsr.objRelay = --status_Relay;
				
			}break;
				
			default:{}break;
		}
		
		swCommand_fromUsr.actMethod = relay_OnOff;
		
//#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_FANS)		
//#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)		
#else
		statusPad = statusPad;
#endif
	}
}

u8 touchPadScan_oneShoot(void){

	u8 valKey_Temp = 0;
	
	if(!touchPad_1)valKey_Temp |= 0x01;
	if(!touchPad_2)valKey_Temp |= 0x02;
	if(!touchPad_3)valKey_Temp |= 0x04;
	
	return valKey_Temp;
}

void touchPad_Scan(void){

	static u8 touchPad_temp = 0;
	static bit keyPress_FLG = 0;
	
	static bit funTrigFLG_LongA = 0;
	static bit funTrigFLG_LongB = 0;
	
	code	u16	touchCfrmLoop_Short 	= timeDef_touchPressCfm,	//短按消抖时间
				touchCfrmLoop_LongA 	= timeDef_touchPressLongA,	//长按A时间
				touchCfrmLoop_LongB 	= timeDef_touchPressLongB,	//长按B时间
				touchCfrmLoop_MAX	 	= 60000;//计时封顶
	
	static u8 pressContinueGet = 0;
	       u8 pressContinueCfm = 0;
	
	u16 conterTemp = 0; //按下计时差值计算缓存
	
	if(!combinationFunFLG_3S5S_cancel_counter)memset(&param_combinationFunTrigger_3S5S, 0, sizeof(param_combinationFunPreTrig)); //<3短5短>特殊组合按键衔接时间超时检测业务，超时则将对应标志复位、参数清空
	
	if(touchPadScan_oneShoot()){
		
		if(!keyPress_FLG){
		
			keyPress_FLG = 1;
			touchPadActCounter = touchCfrmLoop_MAX;
			touchPadContinueCnt = timeDef_touchPressContinue;  //连按间隔判断时间
			touchPad_temp = touchPadScan_oneShoot();
		}
		else{
			
			if(touchPad_temp == touchPadScan_oneShoot()){ //等值核对
				
				conterTemp = touchCfrmLoop_MAX - touchPadActCounter;
//				{ //输出打印，谨记 用后注释，否则占用大量代码空间
//				
//					memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
//					sprintf(log_buf, "conut:%d.\n", (int)conterTemp);
//					PrintString1_logOut(log_buf);
//				}
				
//				{ //输出打印，谨记 用后注释，否则占用大量代码空间
//				
//					memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
//					sprintf(log_buf, "current kVal:%d.\n", (int)touchPad_temp);
//					PrintString1_logOut(log_buf);
//				}
			
				if(conterTemp > touchCfrmLoop_LongA && conterTemp <= touchCfrmLoop_LongB){
				
					normalBussiness_longA_touchKeepTrig(touchPad_temp); //持续性触发业务
					
					if(!funTrigFLG_LongA){
					
						funTrigFLG_LongA = 1;
						touchPad_functionTrigNormal(touchPad_temp, press_LongA);
					}
				}
				if(conterTemp > touchCfrmLoop_LongB && conterTemp <= touchCfrmLoop_MAX){
				
					if(!funTrigFLG_LongB){
					
						funTrigFLG_LongB = 1;
						touchPad_functionTrigNormal(touchPad_temp, press_LongB);
					}
				}
				
			}else{
				
				if((touchCfrmLoop_MAX - touchPadActCounter) < touchCfrmLoop_Short){ //短按消抖时间内随时可做键值变更，否则禁止
				
					touchPadActCounter = touchCfrmLoop_MAX;
					touchPadContinueCnt = timeDef_touchPressContinue;  //连按间隔判断时间
					touchPad_temp = touchPadScan_oneShoot();
				}
			}
		}
		
	}else{
		
		if(keyPress_FLG){
		
			conterTemp = touchCfrmLoop_MAX - touchPadActCounter;
			if(conterTemp > touchCfrmLoop_Short && conterTemp <= touchCfrmLoop_LongA){
			
				if(touchPadContinueCnt)pressContinueGet ++;
				if(pressContinueGet <= 1)touchPad_functionTrigNormal(touchPad_temp, press_Short); //非连按短按触发互控同步，若为连按则最后一次触发同步
				else touchPad_functionTrigNormal(touchPad_temp, press_ShortCnt);
			}
		}
	
		if(!touchPadContinueCnt && pressContinueGet){
		
			pressContinueCfm = pressContinueGet;
			pressContinueGet = 0;
			
			if(pressContinueCfm >= 2){
//#if(DEBUG_LOGOUT_EN == 1)
//				{ //输出打印，谨记 用后注释，否则占用大量代码空间
//				
//					memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
//					sprintf(log_buf, "conut:%d.\n", (int)pressContinueCfm);
//					PrintString1_logOut(log_buf);
//				}			
//#endif
				touchPad_functionTrigContinue(touchPad_temp, pressContinueCfm);
				pressContinueCfm = 0;
			}
			
			touchPad_temp = 0;
		}

		if(funTrigFLG_LongA){
		
			funTrigFLG_LongA = 0;
			
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
				
			statusRelay_saveEn = 1; //存储使能，连续调光弹起后进行存储，调光类型不进行自动存储，所以进行主动存储
			EACHCTRL_realesFLG = 1; //有效互控触发
			
//#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_FANS)
//#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
//#else
#endif
		}
		
		if(funTrigFLG_LongB){funTrigFLG_LongB = 0;}
			
		touchPadActCounter = 0;
		keyPress_FLG = 0;
	}
}	

static void normalBussiness_shortTouchTrig(u8 statusPad, bit shortPressCnt_IF){
	
	bit idata tipsBeep_IF = 0;
	
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_FANS)
	
	shortPressCnt_IF = shortPressCnt_IF; //去警告 ^.^
	
	switch(statusPad){
		
		case 1:{
			
			(status_Relay < 3)?(swCommand_fromUsr.objRelay = ++status_Relay):(swCommand_fromUsr.objRelay = 3);
		
		}break;
		
		case 2:{
		
			(status_Relay)?(swCommand_fromUsr.objRelay = 0):(swCommand_fromUsr.objRelay = 3);
				
		}break;
		
		case 4:{
	
			if(status_Relay > 0)swCommand_fromUsr.objRelay = --status_Relay;
			
		}break;
			
		default:{}break;
	}
	
 #if(DEBUG_LOGOUT_EN == 1)
	{ //输出打印，谨记 用后注释，否则占用大量代码空间
		
		memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
		sprintf(log_buf, ">>>relayStatus sby:%d.\n", (int)swCommand_fromUsr.objRelay);
		PrintString1_logOut(log_buf);
	}			
 #endif
	
	swCommand_fromUsr.actMethod = relay_OnOff;
	devActionPush_IF.push_IF = 1; //推送使能
	tipsBeep_IF = 1;
	if(tipsBeep_IF)beeps_usrActive(3, 50, 1); //触摸可用才tips
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
	switch(statusPad){
		
		case 1:{
			
			(status_Relay < 100)?(swCommand_fromUsr.objRelay = status_Relay + 5):(swCommand_fromUsr.objRelay = 100);
		
		}break;
		
		case 2:{
		
			(status_Relay)?(swCommand_fromUsr.objRelay = 0):(swCommand_fromUsr.objRelay = 100);
				
		}break;
		
		case 4:{
	
			if(status_Relay > 0){
			
				(status_Relay >= 5)?(swCommand_fromUsr.objRelay = status_Relay - 5):(swCommand_fromUsr.objRelay = 0);
			}
			
		}break;
			
		default:{}break;
	}
	
	if(!shortPressCnt_IF)EACHCTRL_realesFLG = 1; //有效互控触发（非连按才触发互控）
	
 #if(DEBUG_LOGOUT_EN == 1)
	{ //输出打印，谨记 用后注释，否则占用大量代码空间
		
		memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
		sprintf(log_buf, ">>>relayStatus sby:%d.\n", (int)swCommand_fromUsr.objRelay);
		PrintString1_logOut(log_buf);
	}			
 #endif
	
	swCommand_fromUsr.actMethod = relay_OnOff;
	statusRelay_saveEn = 1; //存储使能，调光类型不进行自动存储，所以进行主动存储
	devActionPush_IF.push_IF = 1; //推送使能
	tipsBeep_IF = 1;
	if(tipsBeep_IF)beeps_usrActive(3, 50, 1); //触摸可用才tips
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SCENARIO)
	
	shortPressCnt_IF = shortPressCnt_IF; //去警告 ^.^
	
	switch(statusPad){
	
		case 1:
		case 2:
		case 4:{
		
			swCommand_fromUsr.objRelay = statusPad;
			swCommand_fromUsr.actMethod = relay_OnOff;
			tipsBeep_IF = 1;
		
		}break;
			
		default:{};
	}
	
	devActionPush_IF.push_IF = 1; //推送使能
	tipsBeep_IF = 1;
	if(tipsBeep_IF)beeps_usrActive(3, 50, 1); //触摸可用才tips
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_HEATER)
	
	shortPressCnt_IF = shortPressCnt_IF; //去警告 ^.^
	
	switch(statusPad){
	
		case 2:{ //和一位开关一样，中间按键才有效
		
			if(heater_ActParam.touchAction_defineJustSwitch_IF){
			
				(heaterActMode_swClose == heater_ActParam.heater_currentActMode)?(heater_ActParam.heater_currentActMode = heaterActMode_swKeepOpen):(heater_ActParam.heater_currentActMode = heaterActMode_swClose); //一位开关，逻辑直接开关
			
			}else{
			
				(heaterActMode_swCloseDelay60min == heater_ActParam.heater_currentActMode)?(heater_ActParam.heater_currentActMode = heaterActMode_swClose):(heater_ActParam.heater_currentActMode ++); //热水器开关，逻辑模式循环
			}
			
			devHeater_actOpeartionExecute(heater_ActParam.heater_currentActMode);
			
			tipsBeep_IF = 1;
			devActionPush_IF.push_IF = 1; //推送使能
			if(tipsBeep_IF)beeps_usrActive(3, 50, 1);
		
		}break;
		
		default:{}break;
	}
	
#else
	switch(statusPad){
		
		case 1:{
			
			if(SWITCH_TYPE == SWITCH_TYPE_SWBIT1){
			
				swCommand_fromUsr.objRelay = 0;
			}
			else if(SWITCH_TYPE == SWITCH_TYPE_CURTAIN){
			
				swCommand_fromUsr.objRelay = 4;
			}
			else if(SWITCH_TYPE == SWITCH_TYPE_FANS){
			
				if(status_Relay < 3)swCommand_fromUsr.objRelay ++;
			}
			else{
			
				swCommand_fromUsr.objRelay = statusPad;
			}
			
			if(DEV_actReserve & 0x01)tipsBeep_IF = 1;
		
		}break;
		
		case 2:{
		
			if(SWITCH_TYPE == SWITCH_TYPE_SWBIT1){
			
				swCommand_fromUsr.objRelay = 1;
			}
			else if(SWITCH_TYPE == SWITCH_TYPE_SWBIT2){
			
				swCommand_fromUsr.objRelay = 0;
			}
			else if(SWITCH_TYPE == SWITCH_TYPE_CURTAIN){
			
				swCommand_fromUsr.objRelay = 2;
			}
			else if(SWITCH_TYPE == SWITCH_TYPE_FANS){
			
				(status_Relay)?(swCommand_fromUsr.objRelay = 0):(swCommand_fromUsr.objRelay = 3);
			}
			else{
			
				swCommand_fromUsr.objRelay = statusPad;
			}
			
			if(DEV_actReserve & 0x02)tipsBeep_IF = 1;
		
		}break;
		
		case 4:{
	
			if(SWITCH_TYPE == SWITCH_TYPE_SWBIT2){
				
				swCommand_fromUsr.objRelay = 2;
			}
			else if(SWITCH_TYPE == SWITCH_TYPE_CURTAIN){
			
				swCommand_fromUsr.objRelay = 1;
			}
			else if(SWITCH_TYPE == SWITCH_TYPE_FANS){
			
				if(status_Relay > 0)swCommand_fromUsr.objRelay --;
			}
			else{
			
				swCommand_fromUsr.objRelay = statusPad;
			}
			
			if(DEV_actReserve & 0x04)tipsBeep_IF = 1;
			
		}break;
			
		default:{
		
			switch(SWITCH_TYPE){ //针对多位开关，多个按键可同时触发
			
				case SWITCH_TYPE_SWBIT1:{
				
					if(statusPad & 0x02)swCommand_fromUsr.objRelay |= 0x01;
					if(DEV_actReserve & 0x02)tipsBeep_IF = 1;
					
				}break;
					
				case SWITCH_TYPE_SWBIT2:{
				
					if(statusPad & 0x01)swCommand_fromUsr.objRelay |= 0x01;
					if(statusPad & 0x04)swCommand_fromUsr.objRelay |= 0x02;
					
					if(DEV_actReserve & 0x05)tipsBeep_IF = 1;
					
				}break;
					
				case SWITCH_TYPE_SWBIT3:{
				
					if(statusPad & 0x01)swCommand_fromUsr.objRelay |= 0x01;
					if(statusPad & 0x02)swCommand_fromUsr.objRelay |= 0x02;
					if(statusPad & 0x04)swCommand_fromUsr.objRelay |= 0x04;
					
					if(DEV_actReserve & 0x07)tipsBeep_IF = 1;
					
				}break;
					
				default:{
				
					return; //其他开关不支持多位按键同时触发，若有多位按键同时触发则判定为误操作，动作不执行
				
				}break;
			}
		
		}break;
	}
	
	if(SWITCH_TYPE == SWITCH_TYPE_SWBIT1 || SWITCH_TYPE == SWITCH_TYPE_SWBIT2 || SWITCH_TYPE == SWITCH_TYPE_SWBIT3){
	
		swCommand_fromUsr.actMethod = relay_flip;
		
	}else{
	 
		swCommand_fromUsr.actMethod = relay_OnOff;
	}
	
	if(!shortPressCnt_IF){ //非连按才触发互控
	
		if(SWITCH_TYPE == SWITCH_TYPE_SWBIT1 || SWITCH_TYPE == SWITCH_TYPE_SWBIT2 || SWITCH_TYPE == SWITCH_TYPE_SWBIT3)EACHCTRL_realesFLG = swCommand_fromUsr.objRelay; //有效互控触发(直接触发触摸对应键互控)
		else
		if(SWITCH_TYPE == SWITCH_TYPE_CURTAIN)EACHCTRL_realesFLG = 1; //有效互控触发
	}
	
	if(swCommand_fromUsr.objRelay)devActionPush_IF.push_IF = 1; //推送使能
	if(tipsBeep_IF)beeps_usrActive(3, 50, 1); //触摸可用才tips
#endif
}

void touchPad_functionTrigNormal(u8 statusPad, keyCfrm_Type statusCfm){ //普通触摸触发

	switch(statusCfm){
	
		case press_Short:{
			
			bit idata tipsBeep_IF = 0;
			
#if(DEBUG_LOGOUT_EN == 1)				
			{ //输出打印，谨记 用后注释，否则占用大量代码空间
				memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
				sprintf(log_buf, "touchPad:%02X, shortPress.\n", (int)statusPad);
				PrintString1_logOut(log_buf);
			}
#endif	
			normalBussiness_shortTouchTrig(statusPad, 0); //普通短按业务触发
			
		}break;
		
		case press_ShortCnt:{
			
#if(DEBUG_LOGOUT_EN == 1)				
			{ //输出打印，谨记 用后注释，否则占用大量代码空间
				memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
				sprintf(log_buf, "touchPad:%02X, cntPress.\n", (int)statusPad);
				PrintString1_logOut(log_buf);
			}
#endif	
			touchKeepCnt_record ++; //连按进行时计数变量更新
			
			if(touchKeepCnt_record == 3){
			
				param_combinationFunTrigger_3S1L.param_combinationFunPreTrig_standBy_FLG = 1; //特殊组合动作预触发<3短1长>
				param_combinationFunTrigger_3S1L.param_combinationFunPreTrig_standBy_keyVal = statusPad; //特殊组合动作预触发对比键值更新<3短1长>
				
			}else{
			
				memset(&param_combinationFunTrigger_3S1L, 0, sizeof(param_combinationFunPreTrig)); //标志复位、参数清空<3短1长>
			} 
			
			normalBussiness_shortTouchTrig(statusPad, 1); //连按短按业务触发
			
		}break;
		
		case press_LongA:{
			
			if(param_combinationFunTrigger_3S1L.param_combinationFunPreTrig_standBy_FLG && (statusPad == param_combinationFunTrigger_3S1L.param_combinationFunPreTrig_standBy_keyVal)){ //特殊组合按键动作业务触发<3短1长>
			
				memset(&param_combinationFunTrigger_3S1L, 0, sizeof(param_combinationFunPreTrig)); //标志复位、参数清空
				
				usrKeyFun_zigbNwkRejoin();
				
#if(DEBUG_LOGOUT_EN == 1)				
				{ //输出打印，谨记 用后注释，否则占用大量代码空间
					memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
					sprintf(log_buf, "combination fun<3S1L> trig!\n");
					PrintString1_logOut(log_buf);
				}
#endif	
			}else{  //禁止 特殊组合动作触发 与 结束动作（长按） 产生触发重叠
				
#if(DEBUG_LOGOUT_EN == 1)				
				{ //输出打印，谨记 用后注释，否则占用大量代码空间
					memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
					sprintf(log_buf, "touchPad:%02X, longPress_A.\n", (int)statusPad);
					PrintString1_logOut(log_buf);
				}
#endif	
				switch(statusPad){ //若不是特殊组合动作，则进行实际长按触发
				
					case 1:{
						
					
					}break;
						
					case 2:{
					
					
					}break;
						
					case 4:{
						
					
					}break;
						
					default:{}break;
				}
			}
		
		}break;
			
		case press_LongB:{
			
#if(DEBUG_LOGOUT_EN == 1)				
			{ //输出打印，谨记 用后注释，否则占用大量代码空间
				memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
				sprintf(log_buf, "touchPad:%02X, longPress_B.\n", (int)statusPad);
				PrintString1_logOut(log_buf);
			}
#endif	
			devStatusChangeTo_devHold(1); //设备网络挂起，任意键长按B
		
			switch(statusPad){
			
				case 1:{}break;
					
				case 2:{}break;
					
				case 4:{}break;
					
				default:{}break;
			}
			
		}break;
			
		default:{}break;
	}
	
	{ //特殊动作组合键相关标志及变量清空
	
		if(statusCfm != press_ShortCnt){

			touchKeepCnt_record = 1; //连按进行时计数变量复原
			memset(&param_combinationFunTrigger_3S1L, 0, sizeof(param_combinationFunPreTrig)); //特殊组合动作预触发标志复位、参数清空<3短1长>
			
			if(statusCfm != press_Short)memset(&param_combinationFunTrigger_3S5S, 0, sizeof(param_combinationFunPreTrig)); //非短按及非连续短按，特殊组合动作标志复位、参数清空<3短5短>
		}
	}
}

void touchPad_functionTrigContinue(u8 statusPad, u8 loopCount){	//触摸连按触发
	
	if(SWITCH_TYPE == SWITCH_TYPE_SWBIT1 || SWITCH_TYPE == SWITCH_TYPE_SWBIT2 || SWITCH_TYPE == SWITCH_TYPE_SWBIT3)EACHCTRL_realesFLG = swCommand_fromUsr.objRelay; //有效互控触发，最后一次连按触发互控同步
	else
	if(SWITCH_TYPE == SWITCH_TYPE_CURTAIN)EACHCTRL_realesFLG = 1; //有效互控触发，最后一次连按触发互控同步
	devActionPush_IF.push_IF = 1; //最后一次连按触发推送使能
	
#if(DEBUG_LOGOUT_EN == 1)				
	{ //输出打印，谨记 用后注释，否则占用大量代码空间
		memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
		sprintf(log_buf, "touchPad:%02X, %02Xtime pressOver.\n", (int)statusPad, (int)loopCount);
		PrintString1_logOut(log_buf);
	}
#endif	
	
	switch(loopCount){
	
		case 2:{
		
			switch(statusPad){
			
				case 1:{
				
					
				}break;
					
				case 2:{
				
					
				}break;
					
				case 4:{

					
				}break;
					
				default:{}break;
			}
			
		}break;
		
		case 3:{
		
			param_combinationFunTrigger_3S5S.param_combinationFunPreTrig_standBy_FLG = 1; //特殊组合动作预触发标志置位<3短5短>
			param_combinationFunTrigger_3S5S.param_combinationFunPreTrig_standBy_keyVal = statusPad;  //特殊组合动作预触发对比键值更新<3短5短>
			combinationFunFLG_3S5S_cancel_counter = 3000;  //特殊组合动作预触发衔接时间计时开始<3短5短>
			
		}break;
		
		case 5:{
		
			if(param_combinationFunTrigger_3S5S.param_combinationFunPreTrig_standBy_FLG && (statusPad == param_combinationFunTrigger_3S5S.param_combinationFunPreTrig_standBy_keyVal)){ //特殊组合动作对应业务响应<3短5短>
			
				memset(&param_combinationFunTrigger_3S5S, 0, sizeof(param_combinationFunPreTrig)); //标志复位、参数清空
				
				usrZigbNwkOpen(); //网络开放
				
#if(DEBUG_LOGOUT_EN == 1)				
				{ //输出打印，谨记 用后注释，否则占用大量代码空间
					memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
					sprintf(log_buf, "combination fun<3S5S> trig!\n");
					PrintString1_logOut(log_buf);
				}
#endif
			}
			
		}break;
		
		case 6:{
		
		
		}break;
		
		case 10:{
		
			devHoldStop_makeInAdvance(); //设备网络挂起停止
		
		}break;
		
		default:{}break;
	}
	
	{ //特殊组合键相关标志及变量清空
	
		touchKeepCnt_record = 1; //连按进行时计数变量复原
		memset(&param_combinationFunTrigger_3S1L, 0, sizeof(param_combinationFunPreTrig)); //特殊组合动作预触发标志复位、参数清空<3短1长>
		if(loopCount != 3){ //非3短
		
			memset(&param_combinationFunTrigger_3S5S, 0, sizeof(param_combinationFunPreTrig)); //标志复位、参数清空<3短5短>
		}
	}
}

#endif

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_FANS)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)
void fun_Test(void){

	
}

void fun_Test_short(void){

//	static idata bit k_flg = 0;
//	
//	k_flg = !k_flg;
//	
//	infraredOpreatAct_Stop();
//	(k_flg)?(infraredOpreatAct_learnningStart(3)):(infraredOpreatAct_remoteControlStart(3));
	
	infraredOpreatAct_Stop();
	infraredOpreatAct_remoteControlStart(3);
}

void fun_Test_longA(void){

	infraredOpreatAct_Stop();
	infraredOpreatAct_learnningStart(3);
}

#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SCENARIO)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_HEATER)
void devHeater_actOpeartionExecute(enumDevHeater_ActMode opreatParam){

	switch(opreatParam){
	
		case heaterActMode_swClose:{
		
			swCommand_fromUsr.objRelay = 0;
			swCommand_fromUsr.actMethod = relay_OnOff;
		
		}break;
			
		case heaterActMode_swKeepOpen:{
		
			swCommand_fromUsr.objRelay = 1;
			swCommand_fromUsr.actMethod = relay_OnOff;
			
		}break;
			
		case heaterActMode_swCloseDelay30min:{
		
			swCommand_fromUsr.objRelay = 1;
			swCommand_fromUsr.actMethod = relay_OnOff;
			heater_ActParam.timerClose_counter = 60 * 30;
		
		}break;
			
		case heaterActMode_swCloseDelay60min:{
		
			swCommand_fromUsr.objRelay = 1;
			swCommand_fromUsr.actMethod = relay_OnOff;
			heater_ActParam.timerClose_counter = 60 * 60;
		
		}break;
			
		default:{}break;
	}
}

#else
void fun_Test(void){

	
}

#endif

void fun_touchReset(void){

	touchPad_resetOpreat(TOUCHRESETTIME_DEFAULT);
	tips_statusChangeToTouchReset(TOUCHRESETTIME_DEFAULT);
}

void usrKeyFun_relayOpreation(void){

	swCommand_fromUsr.objRelay = 1;
	swCommand_fromUsr.actMethod = relay_flip;
	
	devActionPush_IF.dats_Push = 0;
	devActionPush_IF.push_IF = 1;
	
	beeps_usrActive(3, 50, 1);
}

void usrKeyFun_zigbNwkRejoin(void){
	
	if(countEN_ifTipsFree)countEN_ifTipsFree = 0; //触摸释放计时失能

	devStatus_switch.statusChange_standBy = status_nwkREQ;
	devStatus_switch.statusChange_IF = 1;
	
	statusSave_zigbNwk_nwkExistIF(0); //网络存在本地存储判断值更新为不存在
	
	tips_statusChangeToZigbNwkFind(); //tips更新
}
