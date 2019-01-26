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

//***************Tips����������***************************/
extern tips_Status devTips_status;

/**********************�����ļ�����������**********************/
u8 idata val_DcodeCfm 			= 0;  //����ֵ
bit		 ledBackground_method	= 1;  //��������ɫ���� //Ϊ1ʱ����-�� ��-��   Ϊ0ʱ����-�� ��-��

bit		 usrKeyCount_EN			= 0;  //�û���������
u16		 usrKeyCount			= 0;

u16	xdata touchPadActCounter	= 0;  //�����̰�����ʱ
u16	xdata touchPadContinueCnt	= 0;  //������������ʱ

u8	xdata touchKeepCnt_record	= 1;  //�������ڽ���ʱ���������������ض���һ��ʼ�����򲻽�����

u16 xdata combinationFunFLG_3S5S_cancel_counter  = 0;  //�������Ԥ������־_�ν�ʱ��ȡ���������ν�ʱ�����ʱ����Ԥ������־ȡ��

static param_combinationFunPreTrig param_combinationFunTrigger_3S1L = {0};
static param_combinationFunPreTrig param_combinationFunTrigger_3S5S = {0};

/*------------------------------------------------------------------------------------------------------------*/
///*�����ص���������*///Ϊ���ٴ������࣬�˶�����
//static fun_KeyTrigger xdata funTrig_keyTouch_1 = {0}; //�������� 
//static fun_KeyTrigger xdata funTrig_keyTouch_2 = {0}; //��������
//static fun_KeyTrigger xdata funTrig_keyTouch_3 = {0}; //��������
//static fun_KeyTrigger xdata funTrig_keyButton  = {0}; //����������

static void touchPad_functionTrigNormal(u8 statusPad, keyCfrm_Type statusCfm);
static void touchPad_functionTrigContinue(u8 statusPad, u8 loopCount);

///*���������ص�����ע��*///Ϊ���ٴ������࣬�˺�������
//void funKeyTrigger_register(funKey_Callback funTrigger, objKey key, trig_Method mTrig, u8 pressCnt_num){ //�ص��������������󣬴�����ʽ����������������2�� ������ʽΪ���� ��Ч��

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
//				case method_pressCnt: //����������
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
	
	if(devTips_nwkZigb != nwkZigb_outLine &&  //����ʧ����ͨ���쳣����������
	   devRunning_Status != status_nwkREQ &&  //���������粻������
	   devRunning_Status != status_nwkReconnect){  //���������粻������
	
		ZigB_nwkOpen(1, ZIGBNWK_OPNETIME_DEFAULT); //���ܴ���
		tips_statusChangeToZigbNwkOpen(ZIGBNWK_OPNETIME_DEFAULT); //tips����
#if(DEBUG_LOGOUT_EN == 1)
		{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�
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
	
	{ //����Ԥ���
	
		u8 touchVal_temp = DcodeScan_oneShoot();
		
		if(touchVal_temp & Dcode_FLG_ifMemory)relayStatus_ifSave = statusSave_enable; //��ǰ���
		devTypeComfirm_byDcode(Dcode_bitReserve(touchVal_temp));
	}
	
#endif
}

bit UsrKEYScan_oneShoot(void){

	return Usr_key;
}

void UsrKEYScan(funKey_Callback funCB_Short, funKey_Callback funCB_LongA, funKey_Callback funCB_LongB){
	
	code	u16	keyCfrmLoop_Short 	= 10,	//�̰�����ʱ��,�ݴ�ѭ������
				keyCfrmLoop_LongA 	= 3000,	//����Aʱ��,�ݴ�ѭ������
				keyCfrmLoop_LongB 	= 12000,//����Bʱ��,�ݴ�ѭ������
				keyCfrmLoop_MAX	 	= 60000;//��ʱ�ⶥ
	
	static	bit LongA_FLG = 0;
	static	bit LongB_FLG = 0;
	
	static	bit keyPress_FLG = 0;

	if(!UsrKEYScan_oneShoot()){		
		
		keyPress_FLG = 1;
		
//		tips_statusChangeToNormal();
	
		if(!usrKeyCount_EN) usrKeyCount_EN= 1;	//��ʱ
		
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

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS) //�豸����Ϊ����ʱ��û�д������������Ͳ�������
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)  //�豸����Ϊ����ת����ʱ��û�д������������Ͳ�������
#else //���������������⣬�д������������Ͳ�������
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
	const  u8 	comfirm_Period		= 200;	//����ʱ����������ȡ�������̵߳�������
		
		   u8 	val_Dcode_differ	= 0;
	
		   bit	val_CHG				= 0;
	
	val_DcodeCfm = DcodeScan_oneShoot();
	
	DEV_actReserve = switchTypeReserve_GET(); //��ǰ�������Ͷ�Ӧ��Ч����λˢ��
	
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
	
	code	u16	touchCfrmLoop_Short 	= timeDef_touchPressCfm,	//�̰�����ʱ��
				touchCfrmLoop_LongA 	= timeDef_touchPressLongA,	//����Aʱ��
				touchCfrmLoop_LongB 	= timeDef_touchPressLongB,	//����Bʱ��
				touchCfrmLoop_MAX	 	= 60000;//��ʱ�ⶥ
	
	static u8 pressContinueGet = 0;
	       u8 pressContinueCfm = 0;
	
	u16 conterTemp = 0; //���¼�ʱ��ֵ���㻺��
	
	if(!combinationFunFLG_3S5S_cancel_counter)memset(&param_combinationFunTrigger_3S5S, 0, sizeof(param_combinationFunPreTrig)); //<3��5��>������ϰ����ν�ʱ�䳬ʱ���ҵ�񣬳�ʱ�򽫶�Ӧ��־��λ���������
	
	if(touchPadScan_oneShoot()){
		
		if(!keyPress_FLG){
		
			keyPress_FLG = 1;
			touchPadActCounter = touchCfrmLoop_MAX;
			touchPadContinueCnt = timeDef_touchPressContinue;  //��������ж�ʱ��
			touchPad_temp = touchPadScan_oneShoot();
		}
		else{
			
			if(touchPad_temp == touchPadScan_oneShoot()){ //��ֵ�˶�
				
				conterTemp = touchCfrmLoop_MAX - touchPadActCounter;
//				{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�
//				
//					memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
//					sprintf(log_buf, "conut:%d.\n", (int)conterTemp);
//					PrintString1_logOut(log_buf);
//				}
				
//				{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�
//				
//					memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
//					sprintf(log_buf, "current kVal:%d.\n", (int)touchPad_temp);
//					PrintString1_logOut(log_buf);
//				}
			
				if(conterTemp > touchCfrmLoop_LongA && conterTemp <= touchCfrmLoop_LongB){
				
					normalBussiness_longA_touchKeepTrig(touchPad_temp); //�����Դ���ҵ��
					
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
				
				if((touchCfrmLoop_MAX - touchPadActCounter) < touchCfrmLoop_Short){ //�̰�����ʱ������ʱ������ֵ����������ֹ
				
					touchPadActCounter = touchCfrmLoop_MAX;
					touchPadContinueCnt = timeDef_touchPressContinue;  //��������ж�ʱ��
					touchPad_temp = touchPadScan_oneShoot();
				}
			}
		}
		
	}else{
		
		if(keyPress_FLG){
		
			conterTemp = touchCfrmLoop_MAX - touchPadActCounter;
			if(conterTemp > touchCfrmLoop_Short && conterTemp <= touchCfrmLoop_LongA){
			
				if(touchPadContinueCnt)pressContinueGet ++;
				if(pressContinueGet <= 1)touchPad_functionTrigNormal(touchPad_temp, press_Short); //�������̰���������ͬ������Ϊ���������һ�δ���ͬ��
				else touchPad_functionTrigNormal(touchPad_temp, press_ShortCnt);
			}
		}
	
		if(!touchPadContinueCnt && pressContinueGet){
		
			pressContinueCfm = pressContinueGet;
			pressContinueGet = 0;
			
			if(pressContinueCfm >= 2){
//#if(DEBUG_LOGOUT_EN == 1)
//				{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�
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
				
			statusRelay_saveEn = 1; //�洢ʹ�ܣ��������ⵯ�����д洢���������Ͳ������Զ��洢�����Խ��������洢
			EACHCTRL_realesFLG = 1; //��Ч���ش���
			
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
	
	shortPressCnt_IF = shortPressCnt_IF; //ȥ���� ^.^
	
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
	{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�
		
		memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
		sprintf(log_buf, ">>>relayStatus sby:%d.\n", (int)swCommand_fromUsr.objRelay);
		PrintString1_logOut(log_buf);
	}			
 #endif
	
	swCommand_fromUsr.actMethod = relay_OnOff;
	devActionPush_IF.push_IF = 1; //����ʹ��
	tipsBeep_IF = 1;
	if(tipsBeep_IF)beeps_usrActive(3, 50, 1); //�������ò�tips
	
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
	
	if(!shortPressCnt_IF)EACHCTRL_realesFLG = 1; //��Ч���ش������������Ŵ������أ�
	
 #if(DEBUG_LOGOUT_EN == 1)
	{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�
		
		memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
		sprintf(log_buf, ">>>relayStatus sby:%d.\n", (int)swCommand_fromUsr.objRelay);
		PrintString1_logOut(log_buf);
	}			
 #endif
	
	swCommand_fromUsr.actMethod = relay_OnOff;
	statusRelay_saveEn = 1; //�洢ʹ�ܣ��������Ͳ������Զ��洢�����Խ��������洢
	devActionPush_IF.push_IF = 1; //����ʹ��
	tipsBeep_IF = 1;
	if(tipsBeep_IF)beeps_usrActive(3, 50, 1); //�������ò�tips
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SCENARIO)
	
	shortPressCnt_IF = shortPressCnt_IF; //ȥ���� ^.^
	
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
	
	devActionPush_IF.push_IF = 1; //����ʹ��
	tipsBeep_IF = 1;
	if(tipsBeep_IF)beeps_usrActive(3, 50, 1); //�������ò�tips
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_HEATER)
	
	shortPressCnt_IF = shortPressCnt_IF; //ȥ���� ^.^
	
	switch(statusPad){
	
		case 2:{ //��һλ����һ�����м䰴������Ч
		
			if(heater_ActParam.touchAction_defineJustSwitch_IF){
			
				(heaterActMode_swClose == heater_ActParam.heater_currentActMode)?(heater_ActParam.heater_currentActMode = heaterActMode_swKeepOpen):(heater_ActParam.heater_currentActMode = heaterActMode_swClose); //һλ���أ��߼�ֱ�ӿ���
			
			}else{
			
				(heaterActMode_swCloseDelay60min == heater_ActParam.heater_currentActMode)?(heater_ActParam.heater_currentActMode = heaterActMode_swClose):(heater_ActParam.heater_currentActMode ++); //��ˮ�����أ��߼�ģʽѭ��
			}
			
			devHeater_actOpeartionExecute(heater_ActParam.heater_currentActMode);
			
			tipsBeep_IF = 1;
			devActionPush_IF.push_IF = 1; //����ʹ��
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
		
			switch(SWITCH_TYPE){ //��Զ�λ���أ����������ͬʱ����
			
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
				
					return; //�������ز�֧�ֶ�λ����ͬʱ���������ж�λ����ͬʱ�������ж�Ϊ�������������ִ��
				
				}break;
			}
		
		}break;
	}
	
	if(SWITCH_TYPE == SWITCH_TYPE_SWBIT1 || SWITCH_TYPE == SWITCH_TYPE_SWBIT2 || SWITCH_TYPE == SWITCH_TYPE_SWBIT3){
	
		swCommand_fromUsr.actMethod = relay_flip;
		
	}else{
	 
		swCommand_fromUsr.actMethod = relay_OnOff;
	}
	
	if(!shortPressCnt_IF){ //�������Ŵ�������
	
		if(SWITCH_TYPE == SWITCH_TYPE_SWBIT1 || SWITCH_TYPE == SWITCH_TYPE_SWBIT2 || SWITCH_TYPE == SWITCH_TYPE_SWBIT3)EACHCTRL_realesFLG = swCommand_fromUsr.objRelay; //��Ч���ش���(ֱ�Ӵ���������Ӧ������)
		else
		if(SWITCH_TYPE == SWITCH_TYPE_CURTAIN)EACHCTRL_realesFLG = 1; //��Ч���ش���
	}
	
	if(swCommand_fromUsr.objRelay)devActionPush_IF.push_IF = 1; //����ʹ��
	if(tipsBeep_IF)beeps_usrActive(3, 50, 1); //�������ò�tips
#endif
}

void touchPad_functionTrigNormal(u8 statusPad, keyCfrm_Type statusCfm){ //��ͨ��������

	switch(statusCfm){
	
		case press_Short:{
			
			bit idata tipsBeep_IF = 0;
			
#if(DEBUG_LOGOUT_EN == 1)				
			{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�
				memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
				sprintf(log_buf, "touchPad:%02X, shortPress.\n", (int)statusPad);
				PrintString1_logOut(log_buf);
			}
#endif	
			normalBussiness_shortTouchTrig(statusPad, 0); //��ͨ�̰�ҵ�񴥷�
			
		}break;
		
		case press_ShortCnt:{
			
#if(DEBUG_LOGOUT_EN == 1)				
			{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�
				memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
				sprintf(log_buf, "touchPad:%02X, cntPress.\n", (int)statusPad);
				PrintString1_logOut(log_buf);
			}
#endif	
			touchKeepCnt_record ++; //��������ʱ������������
			
			if(touchKeepCnt_record == 3){
			
				param_combinationFunTrigger_3S1L.param_combinationFunPreTrig_standBy_FLG = 1; //������϶���Ԥ����<3��1��>
				param_combinationFunTrigger_3S1L.param_combinationFunPreTrig_standBy_keyVal = statusPad; //������϶���Ԥ�����Աȼ�ֵ����<3��1��>
				
			}else{
			
				memset(&param_combinationFunTrigger_3S1L, 0, sizeof(param_combinationFunPreTrig)); //��־��λ���������<3��1��>
			} 
			
			normalBussiness_shortTouchTrig(statusPad, 1); //�����̰�ҵ�񴥷�
			
		}break;
		
		case press_LongA:{
			
			if(param_combinationFunTrigger_3S1L.param_combinationFunPreTrig_standBy_FLG && (statusPad == param_combinationFunTrigger_3S1L.param_combinationFunPreTrig_standBy_keyVal)){ //������ϰ�������ҵ�񴥷�<3��1��>
			
				memset(&param_combinationFunTrigger_3S1L, 0, sizeof(param_combinationFunPreTrig)); //��־��λ���������
				
				usrKeyFun_zigbNwkRejoin();
				
#if(DEBUG_LOGOUT_EN == 1)				
				{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�
					memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
					sprintf(log_buf, "combination fun<3S1L> trig!\n");
					PrintString1_logOut(log_buf);
				}
#endif	
			}else{  //��ֹ ������϶������� �� ���������������� ���������ص�
				
#if(DEBUG_LOGOUT_EN == 1)				
				{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�
					memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
					sprintf(log_buf, "touchPad:%02X, longPress_A.\n", (int)statusPad);
					PrintString1_logOut(log_buf);
				}
#endif	
				switch(statusPad){ //������������϶����������ʵ�ʳ�������
				
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
			{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�
				memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
				sprintf(log_buf, "touchPad:%02X, longPress_B.\n", (int)statusPad);
				PrintString1_logOut(log_buf);
			}
#endif	
			devStatusChangeTo_devHold(1); //�豸����������������B
		
			switch(statusPad){
			
				case 1:{}break;
					
				case 2:{}break;
					
				case 4:{}break;
					
				default:{}break;
			}
			
		}break;
			
		default:{}break;
	}
	
	{ //���⶯����ϼ���ر�־���������
	
		if(statusCfm != press_ShortCnt){

			touchKeepCnt_record = 1; //��������ʱ����������ԭ
			memset(&param_combinationFunTrigger_3S1L, 0, sizeof(param_combinationFunPreTrig)); //������϶���Ԥ������־��λ���������<3��1��>
			
			if(statusCfm != press_Short)memset(&param_combinationFunTrigger_3S5S, 0, sizeof(param_combinationFunPreTrig)); //�Ƕ̰����������̰���������϶�����־��λ���������<3��5��>
		}
	}
}

void touchPad_functionTrigContinue(u8 statusPad, u8 loopCount){	//������������
	
	if(SWITCH_TYPE == SWITCH_TYPE_SWBIT1 || SWITCH_TYPE == SWITCH_TYPE_SWBIT2 || SWITCH_TYPE == SWITCH_TYPE_SWBIT3)EACHCTRL_realesFLG = swCommand_fromUsr.objRelay; //��Ч���ش��������һ��������������ͬ��
	else
	if(SWITCH_TYPE == SWITCH_TYPE_CURTAIN)EACHCTRL_realesFLG = 1; //��Ч���ش��������һ��������������ͬ��
	devActionPush_IF.push_IF = 1; //���һ��������������ʹ��
	
#if(DEBUG_LOGOUT_EN == 1)				
	{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�
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
		
			param_combinationFunTrigger_3S5S.param_combinationFunPreTrig_standBy_FLG = 1; //������϶���Ԥ������־��λ<3��5��>
			param_combinationFunTrigger_3S5S.param_combinationFunPreTrig_standBy_keyVal = statusPad;  //������϶���Ԥ�����Աȼ�ֵ����<3��5��>
			combinationFunFLG_3S5S_cancel_counter = 3000;  //������϶���Ԥ�����ν�ʱ���ʱ��ʼ<3��5��>
			
		}break;
		
		case 5:{
		
			if(param_combinationFunTrigger_3S5S.param_combinationFunPreTrig_standBy_FLG && (statusPad == param_combinationFunTrigger_3S5S.param_combinationFunPreTrig_standBy_keyVal)){ //������϶�����Ӧҵ����Ӧ<3��5��>
			
				memset(&param_combinationFunTrigger_3S5S, 0, sizeof(param_combinationFunPreTrig)); //��־��λ���������
				
				usrZigbNwkOpen(); //���翪��
				
#if(DEBUG_LOGOUT_EN == 1)				
				{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�
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
		
			devHoldStop_makeInAdvance(); //�豸�������ֹͣ
		
		}break;
		
		default:{}break;
	}
	
	{ //������ϼ���ر�־���������
	
		touchKeepCnt_record = 1; //��������ʱ����������ԭ
		memset(&param_combinationFunTrigger_3S1L, 0, sizeof(param_combinationFunPreTrig)); //������϶���Ԥ������־��λ���������<3��1��>
		if(loopCount != 3){ //��3��
		
			memset(&param_combinationFunTrigger_3S5S, 0, sizeof(param_combinationFunPreTrig)); //��־��λ���������<3��5��>
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
	
	if(countEN_ifTipsFree)countEN_ifTipsFree = 0; //�����ͷż�ʱʧ��

	devStatus_switch.statusChange_standBy = status_nwkREQ;
	devStatus_switch.statusChange_IF = 1;
	
	statusSave_zigbNwk_nwkExistIF(0); //������ڱ��ش洢�ж�ֵ����Ϊ������
	
	tips_statusChangeToZigbNwkFind(); //tips����
}
