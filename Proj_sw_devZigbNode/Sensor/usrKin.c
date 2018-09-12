#include "usrKin.h"

#include "Tips.h"
#include "dataTrans.h"
#include "dataManage.h"
#include "Relay.h"

#include "delay.h"
#include "USART.h"

#include "stdio.h"
#include "string.h"

/**********************本地文件变量定义区**********************/
u8 idata val_DcodeCfm 			= 0;  //拨码值
bit		 ledBackground_method	= 1;  //背景灯颜色方案 //为1时：开-绿 关-蓝   为0时：开-蓝 关-绿

bit		 usrKeyCount_EN			= 0;  //用户按键计数
u16		 usrKeyCount			= 0;

u16	xdata touchPadActCounter	= 0;  //触摸盘按键计时
u16	xdata touchPadContinueCnt	= 0;  //触摸盘连按计时

//***************Tips变量引用区***************************/
extern tips_Status devTips_status;

/*------------------------------------------------------------------------------------------------------------*/
//funKey_Callback xdata funKey[10] = {0};

static void touchPad_functionTrigNormal(u8 statusPad, keyCfrm_Type statusCfm);
static void touchPad_functionTrigContinue(u8 statusPad, u8 loopCount);

void usrKin_pinInit(void){

	P1M1 &= ~(0xE0);
	P1M0 &= ~(0xE0);

	P0M1 &= ~(0x04);
	P0M0 &= ~(0x04);
	
	if(!Dcode2)relayStatus_ifSave = statusSave_enable;
}

void usrZigbNwkOpen(void){

	ZigB_nwkOpen(1, ZIGBNWK_OPNETIME_DEFAULT); //功能触发
	tips_statusChangeToZigbNwkOpen(ZIGBNWK_OPNETIME_DEFAULT); //tips触发
}

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

bit UsrKEYScan_oneShoot(void){

	return Usr_key;
}

u8 touchPadScan_oneShoot(void){

	u8 valKey_Temp = 0;
	
	if(!touchPad_1)valKey_Temp |= 0x01;
	if(!touchPad_2)valKey_Temp |= 0x02;
	if(!touchPad_3)valKey_Temp |= 0x04;
	
	return valKey_Temp;
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
		
			switch(Dcode_bitReserve(val_Dcode_Local)){
			
				case 0:{
				
					SWITCH_TYPE = SWITCH_TYPE_SWBIT3;	
					
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
		}
	}
}

void UsrKEYScan(funKey_Callback funCB_Short, funKey_Callback funCB_LongA, funKey_Callback funCB_LongB){
	
	code	u16	keyCfrmLoop_Short 	= 10,	//短按消抖时间,据大循环而定
				keyCfrmLoop_LongA 	= 3000,	//长按A时间,据大循环而定
				keyCfrmLoop_LongB 	= 10000,//长按B时间,据大循环而定
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
	
	if(touchPadScan_oneShoot()){
		
		if(!keyPress_FLG){
		
			keyPress_FLG = 1;
			touchPadActCounter = touchCfrmLoop_MAX;
			touchPadContinueCnt = timeDef_touchPressContinue;  //连按间隔判断时间
			touchPad_temp = touchPadScan_oneShoot();
		}
		else{
			
			if(touchPad_temp == touchPadScan_oneShoot()){
				
				conterTemp = touchCfrmLoop_MAX - touchPadActCounter;
//				{ //输出打印，谨记 用后注释，否则占用大量代码空间
//					u8 xdata log_buf[64];
//					
//					sprintf(log_buf, "conut:%d.\n", (int)conterTemp);
//					PrintString1_logOut(log_buf);
//				}
			
				if(conterTemp > touchCfrmLoop_LongA && conterTemp <= touchCfrmLoop_LongB){
				
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
			}
		}
	}else{
		
		if(keyPress_FLG){
		
			conterTemp = touchCfrmLoop_MAX - touchPadActCounter;
			if(conterTemp > touchCfrmLoop_Short && conterTemp <= touchCfrmLoop_LongA){
			
				if(touchPadContinueCnt)pressContinueGet ++;
				if(pressContinueGet <= 1)touchPad_functionTrigNormal(touchPad_temp, press_Short); //非连按短按触发互控同步，若为连按则最后一次触发同步
				else touchPad_functionTrigNormal(touchPad_temp, press_ShortCnt);
				beeps_usrActive(3, 50, 3);
			}
		}
	
		if(!touchPadContinueCnt && pressContinueGet){
		
			pressContinueCfm = pressContinueGet;
			pressContinueGet = 0;
			
			if(pressContinueCfm >= 2){
#if(DEBUG_LOGOUT_EN == 1)
//				{ //输出打印，谨记 用后注释，否则占用大量代码空间
//					u8 xdata log_buf[64];
//					
//					sprintf(log_buf, "conut:%d.\n", (int)pressContinueCfm);
//					PrintString1_logOut(log_buf);
//				}			
#endif
				touchPad_functionTrigContinue(touchPad_temp, pressContinueCfm);
				pressContinueCfm = 0;
			}
			
			touchPad_temp = 0;
		}

		funTrigFLG_LongA = 0;
		funTrigFLG_LongB = 0;
			
		touchPadActCounter = 0;
		keyPress_FLG = 0;
	}
}

void touchPad_functionTrigNormal(u8 statusPad, keyCfrm_Type statusCfm){ //普通触摸触发

	switch(statusCfm){
	
		case press_Short:{
			
#if(DEBUG_LOGOUT_EN == 1)				
			{ //输出打印，谨记 用后注释，否则占用大量代码空间
				u8 xdata log_buf[64];
				
				sprintf(log_buf, "touchPad:%02X, shortPress.\n", (int)statusPad);
				PrintString1_logOut(log_buf);
			}
#endif	
			switch(statusPad){
				
				case 1:
				case 2:
				case 4:{
					
					swCommand_fromUsr.actMethod = relay_flip;
					swCommand_fromUsr.objRelay = statusPad;
					EACHCTRL_realesFLG = statusPad; //互控
					devActionPush_IF.push_IF = 1; //推送使能
					
				}break;
					
				default:{}break;
			}
			
		}break;
		
		case press_ShortCnt:{
			
#if(DEBUG_LOGOUT_EN == 1)				
			{ //输出打印，谨记 用后注释，否则占用大量代码空间
				u8 xdata log_buf[64];
				
				sprintf(log_buf, "touchPad:%02X, cntPress.\n", (int)statusPad);
				PrintString1_logOut(log_buf);
			}
#endif	
			
			switch(statusPad){
				
				case 1:
				case 2:
				case 4:{
					
					swCommand_fromUsr.actMethod = relay_flip;
					swCommand_fromUsr.objRelay = statusPad;
					
				}break;
					
				default:{}break;
			}
			
		}break;
		
		case press_LongA:{
			
#if(DEBUG_LOGOUT_EN == 1)				
			{ //输出打印，谨记 用后注释，否则占用大量代码空间
				u8 xdata log_buf[64];
				
				sprintf(log_buf, "touchPad:%02X, longPress_A.\n", (int)statusPad);
				PrintString1_logOut(log_buf);
			}
#endif	
		
			switch(statusPad){
			
				case 1:{
					
				
				}break;
					
				case 2:{}break;
					
				case 4:{
				
					devStatusChangeTo_devHold(1); //设备网络挂起
				
				}break;
					
				default:{}break;
			}
			
		}break;
			
		case press_LongB:{
			
#if(DEBUG_LOGOUT_EN == 1)				
			{ //输出打印，谨记 用后注释，否则占用大量代码空间
				u8 xdata log_buf[64];
				
				sprintf(log_buf, "touchPad:%02X, longPress_B.\n", (int)statusPad);
				PrintString1_logOut(log_buf);
			}
#endif	
		
			switch(statusPad){
			
				case 1:{}break;
					
				case 2:{}break;
					
				case 4:{}break;
					
				default:{}break;
			}
			
		}break;
			
		default:{}break;
	}
}

void touchPad_functionTrigContinue(u8 statusPad, u8 loopCount){	//触摸连按触发
	
	EACHCTRL_realesFLG = statusPad; //最后一次连按触发互控同步
	devActionPush_IF.push_IF = 1; //最后一次连按触发推送使能
	
#if(DEBUG_LOGOUT_EN == 1)				
	{ //输出打印，谨记 用后注释，否则占用大量代码空间
		u8 xdata log_buf[64];
		
		sprintf(log_buf, "touchPad:%02X, %02Xtime pressOver.\n", (int)statusPad, (int)loopCount);
		PrintString1_logOut(log_buf);
	}
#endif	

	switch(statusPad){
	
		case 1:{
		
			switch(loopCount){
			
				case 3:{
				
				}break;
				
				case 4:{
				
					usrZigbNwkOpen(); //网络开放
					
				}break;
					
				default:{}break;
			}
			
		}break;
			
		case 2:{
		
			switch(loopCount){
			
				case 3:{}break;
					
				default:{}break;
			}
			
		}break;
			
		case 4:{
		
			switch(loopCount){
			
				case 3:{}break;
					
				case 4:{
				
					devHoldStop_makeInAdvance(); //设备网络挂起停止
				
				}break;
					
				default:{}break;
			}
			
		}break;
			
		default:{}break;
	}
}

void fun_Test(void){

	;
}

void usrKeyFun_zigbNwkRejoin(void){

	devStatus_switch.statusChange_standBy = status_nwkREQ;
	devStatus_switch.statusChange_IF = 1;
	
	tips_statusChangeToZigbNwkFind(); //tips更新
}
