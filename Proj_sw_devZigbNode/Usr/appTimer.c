#include "appTimer.h"

#include "STC15Fxxxx.H"

#include "stdio.h"
#include "string.h"

#include "USART.h"

#include "Relay.h"
#include "timerAct.h"
#include "dataTrans.h"
#include "Tips.h"

#include "devlopeDebug.h"

//***************���ݴ������������***************************/
extern bit 				rxTout_count_EN;	
extern u8  				rxTout_count;	//���ڽ��ճ�ʱ����
extern bit 				uartRX_toutFLG;
extern u8 				datsRcv_length;
extern uartTout_datsRcv xdata datsRcv_ZIGB;

extern u16 xdata 		zigbNwkAction_counter; //zigb��������ר�ö���ʱ�����

extern bit 				heartBeatCycle_FLG;	//�������ڴ�����־
extern u8 xdata			heartBeatCount;	//��������
extern u8 xdata			heartBeatPeriod; //��������
extern u8 xdata 		heartBeatHang_timeCnt;

extern u8 xdata 		colonyCtrlGet_queryCounter; 
extern u8 xdata 		colonyCtrlGetHang_timeCnt;

//***************�����������������***************************/
extern bit		 		usrKeyCount_EN;
extern u16		 		usrKeyCount;

extern u16 xdata 		touchPadActCounter;
extern u16 xdata 		touchPadContinueCnt;

//***************Tips����������***************************/
extern u16 xdata 		counter_tipsAct;

/*-----------------------------------------------------------------------------------------------*/
void appTimer0_Init(void){	//50us �ж�@24.000M

	AUXR |= 0x80;		
	TMOD &= 0xF0;		
	TL0   = 0x50;		
	TH0   = 0xFB;	
	TF0   = 0;	
	ET0	  = 1;	//���ж�
	PT0   = 0;	//�����ȼ��ж�
	
	TR0   = 1;		
}

void timer0_Rountine (void) interrupt TIMER0_VECTOR{
	
	u16 code period_1second = 20000;
	static u16 counter_1second = 0; 
	
	u8 code period_1ms 		= 20;
	static u8 counter_1ms 	= 0; 
	
	u8 code period_tipsColor = COLORGRAY_MAX * 3;
	static u8 counter_tipsColor = 0; 
	static color_Attr xdata cnt_relay1_Tips = {0};
	static color_Attr xdata cnt_relay2_Tips = {0};
	static color_Attr xdata cnt_relay3_Tips = {0};
	static color_Attr xdata cnt_zigbNwk_Tips = {0};
	
	static u8 xdata period_beep = 3;		//beepר��
	static u8 xdata	count_beep 	= 0;
	
	//****************1msר��**********************************************/
	if(counter_1ms < period_1ms)counter_1ms ++;
	else{
	
		counter_1ms = 0;
		
		/*zigbר�ö���ʱ�����*/
		if(zigbNwkAction_counter)zigbNwkAction_counter --;
		
		/*�û���������ר��ʱ�����*/
		if(usrKeyCount_EN)usrKeyCount ++;
		else usrKeyCount = 0;
		
		/*������������ר��ʱ�����*/
		if(touchPadActCounter)touchPadActCounter --;
		
		/*������������ר��ʱ�����*/
		if(touchPadContinueCnt)touchPadContinueCnt --;
		
		/*Tips����ר��ʱ�����*/
		if(counter_tipsAct)counter_tipsAct --;
	}
	
	//****************1sר��**********************************************/
	if(counter_1second < period_1second)counter_1second ++;
	else{
	
		counter_1second = 0;
		
		/*��������ʱ����ҵ��*/
		if(!heartBeatCycle_FLG){
		
			if(heartBeatCount < heartBeatPeriod)heartBeatCount ++;
			else{
			
				heartBeatCount = 0;
				heartBeatCycle_FLG = 1;
			}
		}
		
		/*��ʱ��ʱҵ�񣬵��㶯��*/
		if(ifDelay_sw_running_FLAG & (1 << 1)){
		
			if(delayCnt_onoff < ((u16)delayPeriod_onoff * 60))delayCnt_onoff ++;
			else{
			
				delayPeriod_onoff = delayCnt_onoff = 0; 
				
				ifDelay_sw_running_FLAG &= ~(1 << 1);	//һ���Ա�־���
				
				swCommand_fromUsr.actMethod = relay_OnOff; //���ض���
				swCommand_fromUsr.objRelay = delayUp_act;
				devActionPush_IF.push_IF = 1; //����ʹ��
				dev_agingCmd_sndInitative.agingCmd_delaySetOpreat = 1; //��Ӧ�����ϴ�ʱЧռλ��һ
			}
		}
		
		/*��ɫģʽ��ʱҵ��ѭ���ر�*/
		if((ifDelay_sw_running_FLAG & (1 << 0)) && status_Relay){
		
			if(delayCnt_closeLoop < ((u16)delayPeriod_closeLoop * 60))delayCnt_closeLoop ++;
			else{
				
				delayCnt_closeLoop = 0;
			
				swCommand_fromUsr.actMethod = relay_OnOff; //���ض���
				swCommand_fromUsr.objRelay = 0;
				devActionPush_IF.push_IF = 1; //����ʹ��
				dev_agingCmd_sndInitative.agingCmd_greenModeSetOpreat = 1; //��Ӧ�����ϴ�ʱЧռλ��һ
			}
		}
		
		/*��������ѯ������ʱֵ����*///��������<<
		if(heartBeatHang_timeCnt)heartBeatHang_timeCnt --;
		
		/*���������ʱֵ����*///��������<<
		if(colonyCtrlGetHang_timeCnt)colonyCtrlGetHang_timeCnt --;
		
		/*ϵͳʱ�䱾��ά�ּ���ֵ����*/
		sysTimeKeep_counter ++;
		
		/*tips�����ͷż�ʱ����ҵ��*/
		if(counter_ifTipsFree)counter_ifTipsFree --;
		
		/*ϵͳʱ�������¸��¼�ʱ����ҵ��*/
		if(sysTimeReales_counter)sysTimeReales_counter --;
		
		/*zigb���翪�ŵ���ʱ*/
		if(timeCount_zigNwkOpen)timeCount_zigNwkOpen --;
		
		/*�豸�������ʱ�䵹��ʱ*///��������<<
		if(devNwkHoldTime_Param.devHoldTime_counter)devNwkHoldTime_Param.devHoldTime_counter --;
		
		/*��Ⱥ�ܿ�״̬��������ѯ���ڼ�ʱ*/
		if(colonyCtrlGet_queryCounter)colonyCtrlGet_queryCounter --;
	}

	//***************���ڽ��ճ�ʱʱ������*******************************//
	if(rxTout_count_EN){ //���ճ�ʱ��ʱʹ���ж�
	
		if(rxTout_count < TimeOutSet1)rxTout_count ++;
		else{
			
			if(!uartRX_toutFLG && datsRcv_length >= 5){ //��ʱʱ���жϼ���С֡���ж�
			
				uartRX_toutFLG = 1;
			
				memset(datsRcv_ZIGB.rcvDats, 0xff, sizeof(char) * COM_RX1_Lenth);
				memcpy(datsRcv_ZIGB.rcvDats, RX1_Buffer, COM_RX1_Lenth);
				datsRcv_ZIGB.rcvDatsLen = datsRcv_length;
				
				/*>>>usr_debug<<<*/
				if(datsRcv_length != (datsRcv_ZIGB.rcvDats[1] + 5)){  //���֡���жϣ��Ƿ񳬳�
				
					//usr_debug������װ
					frameDebug_data.frameIllegal_FLG = 1;
					frameDebug_data.frame_aLength = RX1_Buffer[1];
					frameDebug_data.frame_rLength = datsRcv_length;
					
				}else{
				

				}
			}
			rxTout_count_EN = 0;
		}
	}
	
	//*******************beep��ʱ����ҵ��**************************/
	if(count_beep < period_beep)count_beep ++;
	else{
		
		static u16 xdata 	tips_Period = 20 * 50 / 2;
		static u16 xdata 	tips_Count 	= 0;
		static u8 xdata 	tips_Loop 	= 2 * 4;
		static bit 			beeps_en 	= 1;
	
		count_beep = 0;

		switch(dev_statusBeeps){ //״̬��
			
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
	
	//***************tips_Led ˢ��ҵ��*******************************//
	if(counter_tipsColor > period_tipsColor){	//�Ҷ�ֵֵ����
	
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
	else{ //pwmִ��
	
		counter_tipsColor ++;
		
		if((counter_tipsColor > 0) && (counter_tipsColor <= (COLORGRAY_MAX * 1))){
			
			if(cnt_relay1_Tips.colorGray_R){cnt_relay1_Tips.colorGray_R --; PIN_TIPS_RELAY1_R = 0;}
			else PIN_TIPS_RELAY1_R = 1;
			if(cnt_relay2_Tips.colorGray_R){cnt_relay2_Tips.colorGray_R --; PIN_TIPS_RELAY2_R = 0;}
			else PIN_TIPS_RELAY2_R = 1;
			if(cnt_relay3_Tips.colorGray_R){cnt_relay3_Tips.colorGray_R --; PIN_TIPS_RELAY3_R = 0;}
			else PIN_TIPS_RELAY3_R = 1;
			if(cnt_zigbNwk_Tips.colorGray_R){cnt_zigbNwk_Tips.colorGray_R --; PIN_TIPS_ZIGBNWK_R = 0;}
			else PIN_TIPS_ZIGBNWK_R = 1;
			
		}else
		if((counter_tipsColor > (COLORGRAY_MAX * 1)) && (counter_tipsColor <= (COLORGRAY_MAX * 2))){
		
			if(cnt_relay1_Tips.colorGray_G){cnt_relay1_Tips.colorGray_G --; PIN_TIPS_RELAY1_G = 0;}
			else PIN_TIPS_RELAY1_G = 1;
			if(cnt_relay2_Tips.colorGray_G){cnt_relay2_Tips.colorGray_G --; PIN_TIPS_RELAY2_G = 0;}
			else PIN_TIPS_RELAY2_G = 1;
			if(cnt_relay3_Tips.colorGray_G){cnt_relay3_Tips.colorGray_G --; PIN_TIPS_RELAY3_G = 0;}
			else PIN_TIPS_RELAY3_G = 1;
			if(cnt_zigbNwk_Tips.colorGray_G){cnt_zigbNwk_Tips.colorGray_G --; PIN_TIPS_ZIGBNWK_G = 0;}
			else PIN_TIPS_ZIGBNWK_G = 1;
			
		}else
		if((counter_tipsColor > (COLORGRAY_MAX * 2)) && (counter_tipsColor <= (COLORGRAY_MAX * 3))){
		
			if(cnt_relay1_Tips.colorGray_B){cnt_relay1_Tips.colorGray_B --; PIN_TIPS_RELAY1_B = 0;}
			else PIN_TIPS_RELAY1_B = 1;
			if(cnt_relay2_Tips.colorGray_B){cnt_relay2_Tips.colorGray_B --; PIN_TIPS_RELAY2_B = 0;}
			else PIN_TIPS_RELAY2_B = 1;
			if(cnt_relay3_Tips.colorGray_B){cnt_relay3_Tips.colorGray_B --; PIN_TIPS_RELAY3_B = 0;}
			else PIN_TIPS_RELAY3_B = 1;
			if(cnt_zigbNwk_Tips.colorGray_B){cnt_zigbNwk_Tips.colorGray_B --; PIN_TIPS_ZIGBNWK_B = 0;}
			else PIN_TIPS_ZIGBNWK_B = 1;
		}
	}
}