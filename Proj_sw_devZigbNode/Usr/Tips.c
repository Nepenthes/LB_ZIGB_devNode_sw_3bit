#include "Tips.h"

#include "USART.h"

#include "string.h"
#include "stdio.h"

#include "driver_I2C_HXD019D.h"

#include "Relay.h"
#include "dataManage.h"
#include "timerAct.h"
#include "appTimer.h"
#include "usrKin.h"

#include "eeprom.h"
#include "delay.h"

color_Attr code color_Tab[TIPS_SWBKCOLOR_TYPENUM] = { //ѡɫ��

	{ 0,  0,  0}, {20, 10, 31}, {31,  0,  0}, //��0����1����2
	{31,  0, 10}, { 8,  0, 16}, {0,  31,  0}, //��3����4����5
	{16, 31,  0}, {31, 10,  0}, {0,   0, 31}, //��6����7����8
	{ 0, 10, 31}, //����9
};

color_Attr xdata relay1_Tips 	= {0};
color_Attr xdata relay2_Tips 	= {0};
color_Attr xdata relay3_Tips 	= {0};
color_Attr xdata zigbNwk_Tips 	= {0};

bkLightColorInsert_paramAttr xdata devBackgroundLight_param = {0}; //�����������������

color_Attr code  tips_relayUnused= {0, 0, 0}; //��Ч�̵�����������ɫ

bit	idata ifHorsingLight_running_FLAG = 1;	//��������б�־λ  Ĭ�Ͽ�

tips_Status devTips_status 		= status_Null; //ϵͳ״ָ̬ʾ
tips_nwkZigbStatus devTips_nwkZigb = nwkZigb_Normal; //zigbee����״ָ̬ʾ��

u16 xdata counter_tipsAct 		= 0; //tips ��ɫ�Ƶ�ɫ������
u8  xdata counter_ifTipsFree 	= TIPS_SWFREELOOP_TIME; //���������ͷż�ʱ����
bit idata countEN_ifTipsFree	= 0; //���������ͷż�ʱʹ��
u8  xdata tipsTimeCount_zigNwkOpen	= 0; //zigb ���翪��tipsʱ���ʱ����
u8	xdata tipsTimeCount_touchReset	= 0; //����IC��λtips��ʱ����
u8	xdata tipsTimeCount_factoryRecover = 0; //�ָ�����tips����ʱ����

sound_Attr xdata devTips_beep  	= {0, 0, 0};
enum_beeps xdata dev_statusBeeps= beepsMode_null; //״̬��״̬��������״ָ̬ʾ

void tipLED_pinInit(void){

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
	P5M1 &= ~(0x10);
	P5M0 |= 0x10;
	
	P2M1 &= ~(0x20);
	P2M0 |= 0x20;
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)
	P2M1 &= ~(0x0C);
	P2M0 |= 0x0C;
	
#else
	P2M1 &= ~0xF7;
	P2M0 |= 0xF7;
	
	P0M1 &= ~0x0B;
	P0M0 |= 0x0B;
	
	P5M1 &= ~0x30;
	P5M0 |= 0x30;
	
#endif
	
	devTips_status = status_Normal;
	
	tipsLED_colorSet(obj_Relay1, 0, 0, 0);
	tipsLED_colorSet(obj_Relay2, 0, 0, 0);
	tipsLED_colorSet(obj_Relay3, 0, 0, 0);
	tipsLED_colorSet(obj_zigbNwk,0, 0, 0);
	
//	tipsLED_colorSet(obj_Relay1, 0, 0, 0);
//	tipsLED_colorSet(obj_Relay2, 0, 0, 0);
//	tipsLED_colorSet(obj_Relay3, 0, 0, 0);
//	tipsLED_colorSet(obj_zigbNwk,0, 0, 0);
}

void pinBeep_Init(void){

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)
	P2M1 &= ~0x01;
	P2M0 |= 0x01;
	
#else
	P3M1 &= ~(0x04);
	P3M0 |= 0x04;
	
#endif
}

/*���ڲ�eeprom����tips������ɫ*/
void ledBKGColorSw_Reales(void){

//	EEPROM_read_n(EEPROM_ADDR_ledSWBackGround, &tipsInsert_swLedBKG_ON, 1);
//	EEPROM_read_n(EEPROM_ADDR_ledSWBackGround + 1, &tipsInsert_swLedBKG_OFF, 1);
//	
//	if(tipsInsert_swLedBKG_ON > TIPS_SWBKCOLOR_TYPENUM - 1)tipsInsert_swLedBKG_ON = TIPSBKCOLOR_DEFAULT_ON;
//	if(tipsInsert_swLedBKG_OFF > TIPS_SWBKCOLOR_TYPENUM - 1)tipsInsert_swLedBKG_OFF = TIPSBKCOLOR_DEFAULT_OFF;
//	
//	coverEEPROM_write_n(EEPROM_ADDR_ledSWBackGround, &tipsInsert_swLedBKG_ON, 1);
//	coverEEPROM_write_n(EEPROM_ADDR_ledSWBackGround + 1, &tipsInsert_swLedBKG_OFF, 1);
	
	EEPROM_read_n(EEPROM_ADDR_ledSWBackGround, (u8 *)&devBackgroundLight_param, sizeof(bkLightColorInsert_paramAttr)); //������ȡ
	
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_FANS) //����Ԥ����
	if(devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear0 > (TIPS_SWBKCOLOR_TYPENUM - 1))devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear0 = 0; //��
	if(devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear1 > (TIPS_SWBKCOLOR_TYPENUM - 1))devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear1 = 5; //��
	if(devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear2 > (TIPS_SWBKCOLOR_TYPENUM - 1))devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear2 = 8; //��
	if(devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear3 > (TIPS_SWBKCOLOR_TYPENUM - 1))devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear3 = 2; //��
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER) //����Ԥ����
	if(devBackgroundLight_param.dimmer_BKlight_Param.dimmer_BKlightColorInsert_press > 			 (TIPS_SWBKCOLOR_TYPENUM - 1))devBackgroundLight_param.dimmer_BKlight_Param.dimmer_BKlightColorInsert_press = 0; //��
	if(devBackgroundLight_param.dimmer_BKlight_Param.dimmer_BKlightColorInsert_brightness0 > 	 (TIPS_SWBKCOLOR_TYPENUM - 1))devBackgroundLight_param.dimmer_BKlight_Param.dimmer_BKlightColorInsert_brightness0 = 5; //��
	if(devBackgroundLight_param.dimmer_BKlight_Param.dimmer_BKlightColorInsert_brightness1to99 > (TIPS_SWBKCOLOR_TYPENUM - 1))devBackgroundLight_param.dimmer_BKlight_Param.dimmer_BKlightColorInsert_brightness1to99 = 8; //��
	if(devBackgroundLight_param.dimmer_BKlight_Param.dimmer_BKlightColorInsert_brightness100 > 	 (TIPS_SWBKCOLOR_TYPENUM - 1))devBackgroundLight_param.dimmer_BKlight_Param.dimmer_BKlightColorInsert_brightness100 = 2; //��
	
//#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS) //����Ԥ���������⣬��Ĭ��
//#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED) //����Ԥ���������⣬��Ĭ��
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SCENARIO) //����Ԥ����
	if(devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig > (TIPS_SWBKCOLOR_TYPENUM - 1))devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig = 8; //��
	if(devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig > (TIPS_SWBKCOLOR_TYPENUM - 1))devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig = 5; //��

#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_HEATER) //����Ԥ������ˮ��Ϊǿ�Ʊ���
	devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_open = 2; //��
	devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_close = 0; //��
	devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_closeDelay30 = 5; //��
	devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_closeDelay60 = 8; //��

#else
	switch(SWITCH_TYPE){
	
		case SWITCH_TYPE_CURTAIN:{
		
			if(devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press > (TIPS_SWBKCOLOR_TYPENUM - 1))devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press = 8; //��
			if(devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce > (TIPS_SWBKCOLOR_TYPENUM - 1))devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce = 5; //��
		
		}break;
		
		case SWITCH_TYPE_SWBIT1:
		case SWITCH_TYPE_SWBIT2:
		case SWITCH_TYPE_SWBIT3:{
			
			if(devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open > (TIPS_SWBKCOLOR_TYPENUM - 1))devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open = 8; //��
			if(devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close > (TIPS_SWBKCOLOR_TYPENUM - 1))devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close = 5; //��
			
		}break;
		
		default:{}break;
	}

#endif
}

/*����������beeps_Tips*/
void beeps_usrActive(u8 tons, u8 time, u8 loop){ //���� ʱ�� ����

	if(!ifNightMode_sw_running_FLAG){ //��ҹ��ģʽ������Ч
	
		devTips_beep.tips_Period = tons;
		devTips_beep.tips_time = time;
		devTips_beep.tips_loop = loop;
		dev_statusBeeps = beepsMode_standBy;
	}
}

void thread_tipsGetDark(u8 funSet){ //ռλ��ɫֵ

	if((funSet & 0x01) >> 0)relay1_Tips.colorGray_R = relay1_Tips.colorGray_G = relay1_Tips.colorGray_B = 0;
	if((funSet & 0x02) >> 1)relay2_Tips.colorGray_R = relay2_Tips.colorGray_G = relay2_Tips.colorGray_B = 0;
	if((funSet & 0x04) >> 2)relay3_Tips.colorGray_R = relay3_Tips.colorGray_G = relay3_Tips.colorGray_B = 0;
	if((funSet & 0x08) >> 3)zigbNwk_Tips.colorGray_R = zigbNwk_Tips.colorGray_G = zigbNwk_Tips.colorGray_B = 0;
}

/*led״ָ̬ʾģʽ�л�������ģʽ*/
void tips_statusChangeToNormal(void){

	counter_ifTipsFree = TIPS_SWFREELOOP_TIME;
	devTips_status = status_Normal;
	if(!countEN_ifTipsFree)countEN_ifTipsFree = 1; //�����ͷż�ʱʹ��
	if(tipsTimeCount_zigNwkOpen)devTips_nwkZigb = nwkZigb_nwkOpen;
}

/*led״ָ̬ʾģʽ�л���zigb���翪��*/
void tips_statusChangeToZigbNwkOpen(u8 timeopen){

	beeps_usrActive(3, 40, 2);
	tipsTimeCount_zigNwkOpen = timeopen;
	devTips_status = status_tipsNwkOpen;
	devTips_nwkZigb = nwkZigb_nwkOpen;
	thread_tipsGetDark(0x0F);
}

/*led״ָ̬ʾģʽ�л���zigb�����������*/
void tips_statusChangeToZigbNwkFind(void){

	beeps_usrActive(3, 40, 2);
	devTips_status = status_tipsNwkFind;
	thread_tipsGetDark(0x0F);
}

/*led״ָ̬ʾģʽ�л�������IC��λģʽ*/
void tips_statusChangeToTouchReset(u8 timeHold){

	tipsTimeCount_touchReset = timeHold;
	devTips_status = status_touchReset;
	thread_tipsGetDark(0x0F);
	beeps_usrActive(3, 40, 2);
}

/*led״ָ̬ʾģʽ�л����ָ�����ģʽ*/
void tips_statusChangeToFactoryRecover(u8 timeHold){

	tipsTimeCount_factoryRecover = timeHold;
	devTips_status = status_sysStandBy;
	thread_tipsGetDark(0x0F);
	beeps_usrActive(3, 255, 3);
}

/*tips���߳�*///״̬��
void thread_Tips(void){
	
	if(ifNightMode_sw_running_FLAG){ //ҹ��ģʽ��������tipsģʽǿ���л�
	
		if(devTips_status == status_Normal || //����ϵͳ��tips����ҹ��ģʽӰ��
		   devTips_status == status_keyFree){
			
			devTips_status = status_Night;
		}
		
	}else{ //��ҹ��ģʽ�����������л�����
		
		if(devTips_status == status_Night)tips_statusChangeToNormal(); //��ǰ��Ϊҹ��ģʽ���л�����ģʽ
		
		if(devTips_status == status_keyFree){
		
			if(!countEN_ifTipsFree)devTips_status = status_Normal; //�ж��������е������
		}
	
		if( !counter_ifTipsFree &&  //ָ��ʱ��û��Ӳ��������led״ָ̬ʾ�л�������ģʽ
		    (devTips_status == status_Normal) &&  //����ģʽ�²ſ����У�����ģʽ����
			countEN_ifTipsFree && //�ҿ��м�ʱʹ��
			ifHorsingLight_running_FLAG ){ //��־λ��λ�ſ�����
		    
			thread_tipsGetDark(0x0F);
			devTips_status = status_keyFree;
		}
	}
	
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
	ifHorsingLight_running_FLAG = 0; //���������������
	
	switch(devTips_status){
	
		case status_Night:
		case status_Normal:{
			
			/*zigb����״ָ̬ʾ*/
			{
				static bit tips_Type = 0;
				static u8 tipsEn_Loop = 0;
				
				static u8 xdata socketTips_R = 0,
								socketTips_B = 0;
				
				(status_Relay)?(socketTips_R = 30):(socketTips_R = 0);
				
				if(!counter_tipsAct){
				
					tips_Type = !tips_Type;
					
					switch(devTips_nwkZigb){
					
						case nwkZigb_Normal:{
							
							socketTips_B = 5;
						
						}break;
						
						case nwkZigb_nwkOpen:{
						
							if(tipsEn_Loop < (3 * 2)){ //������3
							
								tipsEn_Loop ++;
								counter_tipsAct = 150;
								(tips_Type)?(socketTips_B = 5):(socketTips_B = 0);
								
							}else{
							
								tipsEn_Loop = 0;
								counter_tipsAct = 2000;
								socketTips_B = 0;
							}
							
						}break;
						
						case nwkZigb_outLine:{
						
							socketTips_B = 0;
						
						}break;
						
						case nwkZigb_reConfig:{ //���1000 ����
						
							counter_tipsAct = 1000;
							if(zigbNwk_exist_FLG)(tips_Type)?(socketTips_B = 5):(socketTips_B = 0); //���� --�����������¼������
							else socketTips_B = 0; //���� --����û���������¼������������
							
						}break;
						
						case nwkZigb_nwkREQ:{ //���100 ����
						
							counter_tipsAct = 100;
							(tips_Type)?(socketTips_B = 5):(socketTips_B = 0);
						
						}break;
						
						case nwkZigb_hold:{
						
							if(tipsEn_Loop < (5 * 2)){ //������5
							
								tipsEn_Loop ++;
								counter_tipsAct = 100;
								(tips_Type)?(socketTips_B = 5):(socketTips_B = 0);
								
							}else{
							
								tipsEn_Loop = 0;
								counter_tipsAct = 2000;
								socketTips_B = 0;
							}
							
						}break;
						
						default:{
						
							devTips_nwkZigb = nwkZigb_Normal;
						
						}break;
					}
				}
				
				tipsLED_colorSet(obj_Relay1, socketTips_R, 0, socketTips_B);
			}
			
		}break;
		
		default:{ //����û��ǰ̨tips
		
			tips_statusChangeToNormal();
			
		}break;
	}
	
	/*����tips����*/
	{
	
		if((devTips_nwkZigb == nwkZigb_nwkOpen) && !tipsTimeCount_zigNwkOpen)devTips_nwkZigb = nwkZigb_Normal; //���翪�ź�̨tips���ڻָ�
	}
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)
	ifHorsingLight_running_FLAG = 0; //���ⲻ���������
	
	switch(devTips_status){
	
		case status_Night:
		case status_Normal:{
			
			/*zigb����״ָ̬ʾ*/
			{
				static bit tips_Type = 0;
				static u8 tipsEn_Loop = 0;
				
				static u8 xdata socketTips_R = 0,
								socketTips_B = 0;
				
				(infraredStatus_GET() != infraredSMStatus_free)?(socketTips_R = 30):(socketTips_R = 0);
				
				if(!counter_tipsAct){
				
					tips_Type = !tips_Type;
					
					switch(devTips_nwkZigb){
					
						case nwkZigb_Normal:{
							
							socketTips_B = 5;
						
						}break;
						
						case nwkZigb_nwkOpen:{
						
							if(tipsEn_Loop < (3 * 2)){ //������3
							
								tipsEn_Loop ++;
								counter_tipsAct = 150;
								(tips_Type)?(socketTips_B = 5):(socketTips_B = 0);
								
							}else{
							
								tipsEn_Loop = 0;
								counter_tipsAct = 2000;
								socketTips_B = 0;
							}
							
						}break;
						
						case nwkZigb_outLine:{
						
							socketTips_B = 0;
						
						}break;
						
						case nwkZigb_reConfig:{ //���1000 ����
							
							counter_tipsAct = 1000;
							if(zigbNwk_exist_FLG)(tips_Type)?(socketTips_B = 5):(socketTips_B = 0); //���� --�����������¼������
							else socketTips_B = 0; //���� --����û���������¼������������
							
						}break;
						
						case nwkZigb_nwkREQ:{ //���100 ����
						
							counter_tipsAct = 100;
							(tips_Type)?(socketTips_B = 5):(socketTips_B = 0);
						
						}break;
						
						case nwkZigb_hold:{
						
							if(tipsEn_Loop < (5 * 2)){ //������5
							
								tipsEn_Loop ++;
								counter_tipsAct = 100;
								(tips_Type)?(socketTips_B = 5):(socketTips_B = 0);
								
							}else{
							
								tipsEn_Loop = 0;
								counter_tipsAct = 2000;
								socketTips_B = 0;
							}
							
						}break;
						
						default:{
						
							devTips_nwkZigb = nwkZigb_Normal;
						
						}break;
					}
				}
				
				tipsLED_colorSet(obj_Relay1, socketTips_R, 0, socketTips_B);
			}
			
		}break;
		
		default:{ //����û��ǰ̨tips
		
			tips_statusChangeToNormal();
			
		}break;
	}
	
	/*����tips����*/
	{
	
		if((devTips_nwkZigb == nwkZigb_nwkOpen) && !tipsTimeCount_zigNwkOpen)devTips_nwkZigb = nwkZigb_Normal; //���翪�ź�̨tips���ڻָ�
	}
	
#else
	switch(devTips_status){
	
		case status_sysStandBy:{
		
			tips_sysStandBy();
			
		}break;
			
		case status_keyFree:{
		
			tips_sysButtonReales();
		
		}break;
		
		case status_Night:
		case status_Normal:{
			
			static u16 tipsNwk_Counter = 0;
			u16 code tipsNwk_Period = 500;
			u8 relayStatus_tipsTemp = status_Relay;
			
			if(SWITCH_TYPE == SWITCH_TYPE_SWBIT1){ //��̵���
			
				relayStatus_tipsTemp |= status_Relay & 0x01; //�Դ�1����
				relayStatus_tipsTemp = relayStatus_tipsTemp << 1; //�Դ�1����

			}else
			if(SWITCH_TYPE == SWITCH_TYPE_SWBIT2){ //��̵���

				relayStatus_tipsTemp |= status_Relay & 0x02; //�Դ�2����
				relayStatus_tipsTemp = relayStatus_tipsTemp << 1; //�Դ�2����
				relayStatus_tipsTemp |= status_Relay & 0x01; //�Դ�1����

			}else
			if(SWITCH_TYPE == SWITCH_TYPE_SWBIT3){ //��̵���

				relayStatus_tipsTemp = status_Relay; //ֱ�Ӽ��أ�������
				
			}else
			if(SWITCH_TYPE == SWITCH_TYPE_CURTAIN || SWITCH_TYPE == SWITCH_TYPE_FANS){ //��̵���
				
				relayStatus_tipsTemp = status_Relay;
				
			}else
			if(SWITCH_TYPE == SWITCH_TYPE_dIMMER){ //�津��
				
				relayStatus_tipsTemp = touchPadScan_oneShoot();
			}

			/*�̵���״ָ̬ʾ*/
			switch(SWITCH_TYPE){
				
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_FANS)				
				case SWITCH_TYPE_FANS:{
				
					switch(relayStatus_tipsTemp){ //��ռλָʾ
							
						case 0x01:{
						
							tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear1].colorGray_R, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear1].colorGray_G, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear1].colorGray_B);
							tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear1].colorGray_R, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear1].colorGray_G, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear1].colorGray_B);
							tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear1].colorGray_R, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear1].colorGray_G, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear1].colorGray_B);
						
						}break;
						
						case 0x02:{
						
							tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear2].colorGray_R, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear2].colorGray_G, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear2].colorGray_B);
							tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear2].colorGray_R, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear2].colorGray_G, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear2].colorGray_B);
							tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear2].colorGray_R, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear2].colorGray_G, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear2].colorGray_B);
							
						}break;
							
						case 0x03:{
						
							tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear3].colorGray_R, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear3].colorGray_G, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear3].colorGray_B);
							tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear3].colorGray_R, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear3].colorGray_G, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear3].colorGray_B);
							tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear3].colorGray_R, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear3].colorGray_G, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear3].colorGray_B);
							
						}break;
						
						case 0x00:
						default:{
						
							tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear0].colorGray_R, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear0].colorGray_G, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear0].colorGray_B);
							tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear0].colorGray_R, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear0].colorGray_G, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear0].colorGray_B);
							tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear0].colorGray_R, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear0].colorGray_G, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear0].colorGray_B);
							
						}break;
					}
					
				}break;
				
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)		
				case SWITCH_TYPE_dIMMER:{
					
					u8 idata TIPSBKCOLOR_USRDEF_PRESS 	= devBackgroundLight_param.dimmer_BKlight_Param.dimmer_BKlightColorInsert_press; //����ɫ����
					u8 idata TIPSBKCOLOR_USRDEF_UP 		= devBackgroundLight_param.dimmer_BKlight_Param.dimmer_BKlightColorInsert_brightness0; //����ɫ����
					
					if(!status_Relay)TIPSBKCOLOR_USRDEF_UP = devBackgroundLight_param.dimmer_BKlight_Param.dimmer_BKlightColorInsert_brightness0; //����Ϊ0ʱ����ɫ
					else if(status_Relay == 100)TIPSBKCOLOR_USRDEF_UP = devBackgroundLight_param.dimmer_BKlight_Param.dimmer_BKlightColorInsert_brightness100; //����Ϊ100ʱ����ɫ
					else TIPSBKCOLOR_USRDEF_UP = devBackgroundLight_param.dimmer_BKlight_Param.dimmer_BKlightColorInsert_brightness1to99; //����Ϊ1-99ʱ����ɫ
				
					switch(relayStatus_tipsTemp){ //��ռλָʾ
					
						case 0x01:{
						
							tipsLED_colorSet(obj_Relay1, color_Tab[TIPSBKCOLOR_USRDEF_PRESS].colorGray_R, color_Tab[TIPSBKCOLOR_USRDEF_PRESS].colorGray_G, color_Tab[TIPSBKCOLOR_USRDEF_PRESS].colorGray_B);
							tipsLED_colorSet(obj_Relay2, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_R, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_G, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_B);
							tipsLED_colorSet(obj_Relay3, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_R, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_G, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_B);
						
						}break;
						
						case 0x02:{
						
							tipsLED_colorSet(obj_Relay1, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_R, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_G, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_B);
							tipsLED_colorSet(obj_Relay2, color_Tab[TIPSBKCOLOR_USRDEF_PRESS].colorGray_R, color_Tab[TIPSBKCOLOR_USRDEF_PRESS].colorGray_G, color_Tab[TIPSBKCOLOR_USRDEF_PRESS].colorGray_B);
							tipsLED_colorSet(obj_Relay3, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_R, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_G, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_B);
						
						}break;
							
						case 0x04:{
						
							tipsLED_colorSet(obj_Relay1, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_R, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_G, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_B);
							tipsLED_colorSet(obj_Relay2, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_R, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_G, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_B);
							tipsLED_colorSet(obj_Relay3, color_Tab[TIPSBKCOLOR_USRDEF_PRESS].colorGray_R, color_Tab[TIPSBKCOLOR_USRDEF_PRESS].colorGray_G, color_Tab[TIPSBKCOLOR_USRDEF_PRESS].colorGray_B);
						
						}break;
						
						default:{
					
							tipsLED_colorSet(obj_Relay1, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_R, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_G, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_B);
							tipsLED_colorSet(obj_Relay2, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_R, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_G, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_B);
							tipsLED_colorSet(obj_Relay3, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_R, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_G, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_B);
							
						}break;
					}
				
				}break;
				
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SCENARIO)
				case SWITCH_TYPE_SCENARIO:{
				
					u16 code tipsCounter_scenarioKeepTrigForce = 500; //��������ǿ�Ƽ��ʱ��tips��˸�������
					static bit tipsFlash_flg = 0;
					
					if(!counter_tipsAct){
					
						counter_tipsAct = tipsCounter_scenarioKeepTrigForce;
						tipsFlash_flg = !tipsFlash_flg;
					}
				
					switch(scenario_ActParam.scenarioKey_currentTrig){
					
						case scenarioKey_current_S1:{
						
							tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_R, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_G, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_B);
							tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_R, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_G, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_B);
							
							if(scenario_ActParam.scenarioKeepTrig_timeCounter && scenario_ActParam.scenarioKeepTrig_timeCounter != COUNTER_DISENABLE_MASK_SPECIALVAL_U8){
							
								(tipsFlash_flg)?\
									(tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_R, 
																  color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_G, 
																  color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_B)):\
									(tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_R, 
																  color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_G, 
																  color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_B));
								
							}else{
							
								tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_R, 
															 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_G, 
															 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_B);
							}
						
						}break;
							
						case scenarioKey_current_S2:{
						
							tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_R, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_G, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_B);
							tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_R, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_G, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_B);
							
							if(scenario_ActParam.scenarioKeepTrig_timeCounter && scenario_ActParam.scenarioKeepTrig_timeCounter != COUNTER_DISENABLE_MASK_SPECIALVAL_U8){
							
								(tipsFlash_flg)?\
									(tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_R, 
																  color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_G, 
																  color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_B)):\
									(tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_R, 
																  color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_G, 
																  color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_B));
								
							}else{
							
								tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_R, 
															 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_G, 
															 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_B);
							}
						
						}break;
						
						case scenarioKey_current_S3:{
						
							tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_R, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_G, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_B);
							tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_R, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_G, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_B);
							
							if(scenario_ActParam.scenarioKeepTrig_timeCounter && scenario_ActParam.scenarioKeepTrig_timeCounter != COUNTER_DISENABLE_MASK_SPECIALVAL_U8){
							
								(tipsFlash_flg)?\
									(tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_R, 
																  color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_G, 
																  color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_B)):\
									(tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_R, 
																  color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_G, 
																  color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_B));
								
							}else{
							
								tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_R, 
															 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_G, 
															 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_B);
							}
						
						}break;
						
						default:{
						
							tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_R, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_G, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_B);
							tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_R, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_G, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_B);
							tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_R, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_G, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_B);
							
						}break;
					}
					
				}break;
				
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_HEATER)	
				case SWITCH_TYPE_HEATER:{
					
					tipsLED_colorSet(obj_Relay1, 0, 0, 0);
					tipsLED_colorSet(obj_Relay3, 0, 0, 0);
				
					switch(heater_ActParam.heater_currentActMode){
					
						case heaterActMode_swClose:{
						
							tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_close].colorGray_R, 
														 color_Tab[devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_close].colorGray_G, 
														 color_Tab[devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_close].colorGray_B);
						
						}break;
							
						case heaterActMode_swKeepOpen:{
						
							tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_open].colorGray_R, 
														 color_Tab[devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_open].colorGray_G, 
														 color_Tab[devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_open].colorGray_B);
							
						}break;
							
						case heaterActMode_swCloseDelay30min:{
						
							tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_closeDelay30].colorGray_R, 
														 color_Tab[devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_closeDelay30].colorGray_G, 
														 color_Tab[devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_closeDelay30].colorGray_B);
						
						}break;
							
						case heaterActMode_swCloseDelay60min:{
						
							tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_closeDelay60].colorGray_R, 
														 color_Tab[devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_closeDelay60].colorGray_G, 
														 color_Tab[devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_closeDelay60].colorGray_B);
						
						}break;
							
						default:{}break;
					}
						
				}break;
				
#else			
				case SWITCH_TYPE_CURTAIN:{
				
					switch(relayStatus_tipsTemp){ //��ռλָʾ
					
						case 0x01:{
						
							tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_R, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_G, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_B);
							tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_R, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_G, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_B);
							tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press].colorGray_R, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press].colorGray_G, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press].colorGray_B);
						
						}break;
							
						case 0x04:{
						
							tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press].colorGray_R, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press].colorGray_G, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press].colorGray_B);
							tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_R, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_G, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_B);
							tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_R, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_G, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_B);
						
						}break;
						
						default:{
						
							tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_R, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_G, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_B);
							tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press].colorGray_R, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press].colorGray_G, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press].colorGray_B);
							tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_R, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_G, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_B);
						
						}break;
					}
				
				}break;
				
				case SWITCH_TYPE_SWBIT1:
				case SWITCH_TYPE_SWBIT2:
				case SWITCH_TYPE_SWBIT3:
				default:{ //ռλָʾ
					
					(DEV_actReserve & 0x01)?\
						((relayStatus_tipsTemp & 0x01)?\
							(tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open].colorGray_R, 
														  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open].colorGray_G, 
														  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open].colorGray_B)):\
							(tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close].colorGray_R, 
														  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close].colorGray_G, 
														  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close].colorGray_B))):\
						(tipsLED_colorSet(obj_Relay1, tips_relayUnused.colorGray_R, tips_relayUnused.colorGray_G, tips_relayUnused.colorGray_B));
					
					(DEV_actReserve & 0x02)?\
						((relayStatus_tipsTemp & 0x02)?\
							(tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open].colorGray_R, 
														  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open].colorGray_G, 
														  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open].colorGray_B)):\
							(tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close].colorGray_R, 
														  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close].colorGray_G, 
														  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close].colorGray_B))):\
						(tipsLED_colorSet(obj_Relay2, tips_relayUnused.colorGray_R, tips_relayUnused.colorGray_G, tips_relayUnused.colorGray_B));	
					
					(DEV_actReserve & 0x04)?\
						((relayStatus_tipsTemp & 0x04)?\
							(tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open].colorGray_R, 
														  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open].colorGray_G, 
														  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open].colorGray_B)):\
							(tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close].colorGray_R, 
														  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close].colorGray_G, 
														  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close].colorGray_B))):\
						(tipsLED_colorSet(obj_Relay3, tips_relayUnused.colorGray_R, tips_relayUnused.colorGray_G, tips_relayUnused.colorGray_B));	
					
				}break;
				
#endif
			}
			
			/*����tips����*/
			{
			
				if((devTips_nwkZigb == nwkZigb_nwkOpen) && !tipsTimeCount_zigNwkOpen)devTips_nwkZigb = nwkZigb_Normal; //���翪�ź�̨tips���ڻָ�
			}
			
			/*zigb����״ָ̬ʾ*/
			{
				static bit tips_Type = 0;
				
				if(tipsNwk_Counter < tipsNwk_Period)tipsNwk_Counter ++;
				else{
				
					tips_Type = !tips_Type;
					tipsNwk_Counter = 0;
					
					switch(devTips_nwkZigb){
					
						case nwkZigb_Normal:{
						
							(tips_Type)?(tipsLED_colorSet(obj_zigbNwk, 0, 30, 0)):(tipsLED_colorSet(obj_zigbNwk, 0, 30, 0)); //����
						
						}break;
						
						case nwkZigb_nwkOpen:{
						
							(tips_Type)?(tipsLED_colorSet(obj_zigbNwk, 0, 30, 0)):(tipsLED_colorSet(obj_zigbNwk, 0, 0, 0)); //����
							
						}break;
						
						case nwkZigb_outLine:{
						
							(tips_Type)?(tipsLED_colorSet(obj_zigbNwk, 30, 0, 0)):(tipsLED_colorSet(obj_zigbNwk, 30, 0, 0)); //����
						
						}break;
						
						case nwkZigb_reConfig:{
							
							if(zigbNwk_exist_FLG){ //�������
							
								if(devZigbNwk_startUp_delayCounter)(tips_Type)?(tipsLED_colorSet(obj_zigbNwk, 30, 0, 0)):(tipsLED_colorSet(obj_zigbNwk, 0, 0, 0)); //����
								else tipsLED_colorSet(obj_zigbNwk, 30, 10, 0); //���� --�������¼��������
							
							}else{ //���粻����
								
								tipsLED_colorSet(obj_zigbNwk, 0, 0, 0); //���� --����û�������¼
							}
							
						}break;
						
						case nwkZigb_nwkREQ:{
						
							(tips_Type)?(tipsLED_colorSet(obj_zigbNwk, 30, 10, 0)):(tipsLED_colorSet(obj_zigbNwk, 0, 0, 0)); //����
						
						}break;
						
						case nwkZigb_hold:{
						
							(tips_Type)?(tipsLED_colorSet(obj_zigbNwk, 30, 0, 0)):(tipsLED_colorSet(obj_zigbNwk, 0, 0, 0)); //����
							
						}break;
						
						default:{
						
							devTips_nwkZigb = nwkZigb_Normal;
						
						}break;
					}
				}
			}
			
		}break;
		
		case status_tipsNwkOpen:{
			
			if(tipsTimeCount_zigNwkOpen)tips_specified(1);
			else{

				tips_statusChangeToNormal();
#if(DEBUG_LOGOUT_EN == 1)
				{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�
					memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
					sprintf(log_buf, "zigbNwk Close.\n");
					PrintString1_logOut(log_buf);
				}			
#endif	
			}
			
		}break;
		
		case status_tipsNwkFind:{
		
			tips_specified(0);
		
		}break;
		
		case status_devHold:{
		
			tips_warning();
			
		}break;
		
		case status_touchReset:{
		
			if(tipsTimeCount_touchReset)tips_sysTouchReset();
			else{
			
				tips_statusChangeToNormal();
#if(DEBUG_LOGOUT_EN == 1)
				{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�
					memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
					sprintf(log_buf, "touch reset complete.\n");
					PrintString1_logOut(log_buf);
				}			
#endif	
			}
			
		}break;
			
		default:{
		
			devTips_status = status_Normal;
			
		}break;
	}
#endif
}

void tipsLED_colorSet(tipsLED_Obj obj, u8 gray_R, u8 gray_G, u8 gray_B){

	if((devTips_status == status_Normal) ||
	   (devTips_status == status_Night)){
		   
		if(devTips_status == status_Night){	//ҹ��ģʽ���������ȵ���
			
			u8 code bright_coef = 4;
		
			gray_R /= bright_coef;
			gray_G /= bright_coef;
			gray_B /= bright_coef;
		}
	
		switch(obj){
		
			case obj_Relay1:{
			
				relay1_Tips.colorGray_R = gray_R;
				relay1_Tips.colorGray_G = gray_G;
				relay1_Tips.colorGray_B = gray_B;
				
			}break;
			
			case obj_Relay2:{
			
				relay2_Tips.colorGray_R = gray_R;
				relay2_Tips.colorGray_G = gray_G;
				relay2_Tips.colorGray_B = gray_B;
				
			}break;
			
			case obj_Relay3:{
			
				relay3_Tips.colorGray_R = gray_R;
				relay3_Tips.colorGray_G = gray_G;
				relay3_Tips.colorGray_B = gray_B;
				
			}break;
					
			case obj_zigbNwk:{
			
				zigbNwk_Tips.colorGray_R = gray_R;
				zigbNwk_Tips.colorGray_G = gray_G;
				zigbNwk_Tips.colorGray_B = gray_B;
				
			}break;
			
			default:break;
		}
	}
}

void tips_warning(void){

	static u8 xdata tipsStep = 0;
	
	switch(tipsStep){
	
		case 0:{
		
			counter_tipsAct = 1000;
			tipsStep = 1;
		
		}break;
		
		case 1:{
		
			if(counter_tipsAct){
				
				if(counter_tipsAct % 199 > 99){
				
					zigbNwk_Tips.colorGray_R = \
					relay3_Tips.colorGray_R = \
					relay2_Tips.colorGray_R = \
					relay1_Tips.colorGray_R = 31;
				
				}else{
				
					zigbNwk_Tips.colorGray_R = \
					relay3_Tips.colorGray_R = \
					relay2_Tips.colorGray_R = \
					relay1_Tips.colorGray_R = 0;
				}
			
			}else{
			
				counter_tipsAct = 600;
				thread_tipsGetDark(0x0F);
				tipsStep = 2;
			}
		
		}break;
		
		
		case 2:{
		
			if(!counter_tipsAct)tipsStep = 0;
			
		}break;
		
		default:{
		
			tipsStep = 0;
		
		}break;
	}
}

void tips_breath(void){

	static u8 	localTips_Count = COLORGRAY_MAX - 1; //���������л���ʼֵ
	static bit 	count_FLG = 1;
	static u8 	tipsStep = 0;

	u8 code speed = 3;
	u8 code step_period = 3;
	
//	if(!localTips_Count && !count_FLG)(tipsStep >= step_period)?(tipsStep = 0):(tipsStep ++); //�����ʼ���� �л�����һ����
	if(!localTips_Count)count_FLG = 1;
	else 
	if(localTips_Count >= (COLORGRAY_MAX - 1)){
		
		count_FLG = 0;
		
		localTips_Count = COLORGRAY_MAX - 2; //�������뵱ǰ״̬
		(tipsStep >= step_period)?(tipsStep = 0):(tipsStep ++);  //�������� �л�����һ����
	}
	
	if(!counter_tipsAct){
	
		(!count_FLG)?(counter_tipsAct = speed * (localTips_Count --)):(counter_tipsAct = speed * (localTips_Count ++)); //���µ�����ʱ��
	}	
	
	switch(tipsStep){
	
		case 0:{
		
			zigbNwk_Tips.colorGray_G = 31 - localTips_Count;
			
		}break;
		
		case 2:{
		
			relay3_Tips.colorGray_R = 31 - localTips_Count;
			
		}break;
		
		default:break;
	}
}

void tips_fadeOut(void){

	static u8 	localTips_Count = 0; //��������л���ʼֵ
	static bit 	count_FLG = 1;
	static u8 	tipsStep = 0;
	static u8 	pwmType_A = 0,
				pwmType_B = 0,
				pwmType_C = 0,
				pwmType_D = 0;
	
	u8 code speed = 3;
	u8 code step_period = 1;
	
	if(!localTips_Count && !count_FLG)(tipsStep >= step_period)?(tipsStep = 0):(tipsStep ++); //�����ʼ���� �л�����һ����
	if(!localTips_Count)count_FLG = 1;
	else 
	if(localTips_Count > (COLORGRAY_MAX * 4)){
	
		count_FLG = 0;
		
//		localTips_Count = COLORGRAY_MAX - 2; //�������뵱ǰ״̬
//		(tipsStep >= step_period)?(tipsStep = 0):(tipsStep ++);  //�������� �л�����һ����
	}
	
	if(!counter_tipsAct){
	
		(!count_FLG)?(counter_tipsAct = speed * ((localTips_Count --) % COLORGRAY_MAX)):(counter_tipsAct = speed * ((localTips_Count ++) % COLORGRAY_MAX)); //���µ�����ʱ��
		
		if(localTips_Count >= 00 && localTips_Count< 32)pwmType_A = localTips_Count - 0;
		if(localTips_Count >= 32 && localTips_Count< 64)pwmType_B = localTips_Count - 32;
		if(localTips_Count >= 64 && localTips_Count< 96)pwmType_C = localTips_Count - 64;
		if(localTips_Count >= 96 && localTips_Count< 128)pwmType_D = localTips_Count - 96;
	}
	
	switch(tipsStep){
	
		case 0:{
			
			if(count_FLG){ 
				
				zigbNwk_Tips.colorGray_B = pwmType_A;
				relay3_Tips.colorGray_B = pwmType_B;
				relay2_Tips.colorGray_B = pwmType_C;
				relay1_Tips.colorGray_B = pwmType_D;
				
				zigbNwk_Tips.colorGray_R = 31 - pwmType_A;
				relay3_Tips.colorGray_R = 31 - pwmType_B;
				relay2_Tips.colorGray_R = 31 - pwmType_C;
				relay1_Tips.colorGray_R = 31 - pwmType_D;
				
			}else{ 
				
				zigbNwk_Tips.colorGray_R = 31 - pwmType_D;
				relay3_Tips.colorGray_R = 31 - pwmType_C;
				relay2_Tips.colorGray_R = 31 - pwmType_B;
				relay1_Tips.colorGray_R = 31 - pwmType_A;
				
				zigbNwk_Tips.colorGray_B = pwmType_D;
				relay3_Tips.colorGray_B = pwmType_C;
				relay2_Tips.colorGray_B = pwmType_B;
				relay1_Tips.colorGray_B = pwmType_A;
			}
			
		}break;
			
		case 1:{
			
			if(count_FLG){ 
				
				zigbNwk_Tips.colorGray_G = pwmType_A / 4;
				relay3_Tips.colorGray_G = pwmType_B / 4;
				relay2_Tips.colorGray_G = pwmType_C / 4;
				relay1_Tips.colorGray_G = pwmType_D / 4;
				
				zigbNwk_Tips.colorGray_R = 31 - pwmType_A;
				relay3_Tips.colorGray_R = 31 - pwmType_B;
				relay2_Tips.colorGray_R = 31 - pwmType_C;
				relay1_Tips.colorGray_R = 31 - pwmType_D;
				
			}else{ 
				
				zigbNwk_Tips.colorGray_R = 31 - pwmType_D;
				relay3_Tips.colorGray_R = 31 - pwmType_C;
				relay2_Tips.colorGray_R = 31 - pwmType_B;
				relay1_Tips.colorGray_R = 31 - pwmType_A;
				
				zigbNwk_Tips.colorGray_G = pwmType_D / 4;
				relay3_Tips.colorGray_G = pwmType_C / 4;
				relay2_Tips.colorGray_G = pwmType_B / 4;
				relay1_Tips.colorGray_G = pwmType_A / 4;
			}
			
		}break;
			
		default:break;
	}
}

void tips_specified(u8 tips_Type){ //tips���

	static u8 	localTips_Count = 0; //��������л���ʼֵ
	static bit 	count_FLG = 1;
	static u8 	tipsStep = 0;
	static u8 	pwmType_A = 0,
				pwmType_B = 0,
				pwmType_C = 0,
				pwmType_D = 0;
	
	u8 code speed_Mol = 5,
	        speed_Den = 4;
	u8 code step_period = 0;
	
	if(!localTips_Count && !count_FLG){
	
		if(tipsStep < step_period)tipsStep ++; //�����ʼ���� �л�����һ����
		else{
		
			tipsStep = 0;
			
			pwmType_A = pwmType_B = pwmType_C = pwmType_D = 0;
		}
	}
	if(!localTips_Count)count_FLG = 1;
	else 
	if(localTips_Count > 80){
	
		count_FLG = 0;
		
//		localTips_Count = COLORGRAY_MAX - 2; //�������뵱ǰ״̬
//		(tipsStep >= step_period)?(tipsStep = 0):(tipsStep ++);  //�������� �л�����һ����
	}
	
	if(!counter_tipsAct){
	
		(!count_FLG)?(counter_tipsAct = ((localTips_Count --) % COLORGRAY_MAX) / speed_Mol * speed_Den):(counter_tipsAct = ((localTips_Count ++) % COLORGRAY_MAX) / speed_Mol * speed_Den); //���µ�����ʱ��
		
		if(localTips_Count >= 00 && localTips_Count < 32)pwmType_A = localTips_Count - 0; 
		if(localTips_Count >= 16 && localTips_Count < 48)pwmType_B = localTips_Count - 16;
		if(localTips_Count >= 32 && localTips_Count < 64)pwmType_C = localTips_Count - 32;
		if(localTips_Count >= 48 && localTips_Count < 80)pwmType_D = localTips_Count - 48;
	}
	
	switch(tips_Type){
	
		case 0:{
		
			switch(tipsStep){
			
				case 0:{
					
					if(count_FLG){ 
						
						if(localTips_Count < 46){
						
							if(localTips_Count % 15 > 10){
							
								zigbNwk_Tips.colorGray_R = 31;
								zigbNwk_Tips.colorGray_G = 10;
								
							}else{
							
								zigbNwk_Tips.colorGray_R = 0;
								zigbNwk_Tips.colorGray_G = 0;
							}
						
						}else{
						
							if(localTips_Count > 45){zigbNwk_Tips.colorGray_R = 31; zigbNwk_Tips.colorGray_G = 10;}
							if(localTips_Count > 50){relay3_Tips.colorGray_R = 31; relay3_Tips.colorGray_G = 10;}
							if(localTips_Count > 60){relay2_Tips.colorGray_R = 31; relay2_Tips.colorGray_G = 10;}
							if(localTips_Count > 70){relay1_Tips.colorGray_R = 31; relay1_Tips.colorGray_G = 10;}
						}
						
					}else{ 
						
						zigbNwk_Tips.colorGray_R = pwmType_D;
						relay3_Tips.colorGray_R = pwmType_C;
						relay2_Tips.colorGray_R = pwmType_B;
						relay1_Tips.colorGray_R = pwmType_A;
						
						zigbNwk_Tips.colorGray_G = pwmType_D / 3;
						relay3_Tips.colorGray_G = pwmType_C / 3;
						relay2_Tips.colorGray_G = pwmType_B / 3;
						relay1_Tips.colorGray_G = pwmType_A / 3;
					}
					
				}break;
					
				default:break;
			}
			
		}break;
		
		case 1:{
		
			switch(tipsStep){
			
				case 0:{
					
					if(count_FLG){ 
						
						if(localTips_Count < 46){
						
//							if(localTips_Count % 15 > 10)zigbNwk_Tips.colorGray_B = 31;else zigbNwk_Tips.colorGray_B = 0;
							if(localTips_Count % 15 > 10)zigbNwk_Tips.colorGray_G = 31;else zigbNwk_Tips.colorGray_G = 0;
						
						}else{
						
//							if(localTips_Count > 45)relay1_Tips.colorGray_B = 31;
//							if(localTips_Count > 50)relay2_Tips.colorGray_B = 31;
//							if(localTips_Count > 60)relay3_Tips.colorGray_B = 31;
//							if(localTips_Count > 70)zigbNwk_Tips.colorGray_B = 31;
							
							if(localTips_Count > 45)relay1_Tips.colorGray_G = 31;
							if(localTips_Count > 50)relay2_Tips.colorGray_G = 31;
							if(localTips_Count > 60)relay3_Tips.colorGray_G = 31;
							if(localTips_Count > 70)zigbNwk_Tips.colorGray_G = 31;
						}
						
					}else{ 
						
//						relay1_Tips.colorGray_B = pwmType_D;
//						relay2_Tips.colorGray_B = pwmType_C;
//						relay3_Tips.colorGray_B = pwmType_B;
//						zigbNwk_Tips.colorGray_B = pwmType_A;
						
						relay1_Tips.colorGray_G = pwmType_D;
						relay2_Tips.colorGray_G = pwmType_C;
						relay3_Tips.colorGray_G = pwmType_B;
						zigbNwk_Tips.colorGray_G = pwmType_A;
					}
					
				}break;
					
				default:break;
			}
			
		}break;
		
		default:break;
	}
}

void tips_sysTouchReset(void){
	
	static bit tipsStep = 0;

	if(counter_tipsAct){
	
		if(tipsStep){
		
			zigbNwk_Tips.colorGray_R = 0;
			relay1_Tips.colorGray_R = 0;
			relay2_Tips.colorGray_R = 0;
			relay3_Tips.colorGray_R = 0;
			
		}else{
		
			zigbNwk_Tips.colorGray_R = 31;
			relay1_Tips.colorGray_R = 31;
			relay2_Tips.colorGray_R = 31;
			relay3_Tips.colorGray_R = 31;
		}
		
	}else{
	
		counter_tipsAct = 400;
		tipsStep = !tipsStep;
	}
}

void tips_sysStandBy(void){

	zigbNwk_Tips.colorGray_R = 31;
	relay1_Tips.colorGray_R = 31;
	relay2_Tips.colorGray_R = 31;
	relay3_Tips.colorGray_R = 31;
}

void tips_sysButtonReales(void){

//	u8 code timUnit_period = 0;
//	static u8 timUnit_count = 0;
	
	static u8 localTips_Count = 0;
	u8 code localTips_Period = 128;
	static u8 tipsStep = 0;
	static u8 pwmType_A = 0,
		      pwmType_B = 0,
			  pwmType_C = 0,
			  pwmType_D = 0;
	
	if(!counter_tipsAct){
		
		counter_tipsAct = 12; //���µ�����ʱ��(@50us�жϣ��ж�Ƶ��Ϊ������������)
	
		if(localTips_Count > localTips_Period){
		
			localTips_Count = 0;
			(tipsStep > 7)?(tipsStep = 0):(tipsStep ++);
			pwmType_A = pwmType_B = pwmType_C = pwmType_D = 0;
		}
		else{
		
			localTips_Count ++;
			
			if(localTips_Count >= 00 && localTips_Count< 32)pwmType_A = localTips_Count - 0;
			if(localTips_Count >= 32 && localTips_Count< 64)pwmType_B = localTips_Count - 32;
			if(localTips_Count >= 64 && localTips_Count< 96)pwmType_C = localTips_Count - 64;
			if(localTips_Count >= 96 && localTips_Count< 128)pwmType_D = localTips_Count - 96;
		}
	}
	
	switch(tipsStep){
		
//		case 0:{
//		
//			zigbNwk_Tips.colorGray_R	= pwmType_A / 3;
//			relay3_Tips.colorGray_R 	= pwmType_B / 3;
//			relay2_Tips.colorGray_R 	= pwmType_C / 3;
//			relay1_Tips.colorGray_R 	= pwmType_D / 3;
//			
//			zigbNwk_Tips.colorGray_G	= pwmType_A / 5;
//			relay3_Tips.colorGray_G 	= pwmType_B / 5;
//			relay2_Tips.colorGray_G 	= pwmType_C / 5;
//			relay1_Tips.colorGray_G 	= pwmType_D / 5;
//			
//			zigbNwk_Tips.colorGray_B	= pwmType_A / 2;
//			relay3_Tips.colorGray_B 	= pwmType_B / 2;
//			relay2_Tips.colorGray_B 	= pwmType_C / 2;
//			relay1_Tips.colorGray_B 	= pwmType_D / 2;
//		
//		}break;
//		
//		case 1:{
//		
//			zigbNwk_Tips.colorGray_R	= 5 + (pwmType_A - 5);
//			relay3_Tips.colorGray_R 	= 5 + (pwmType_B - 5);
//			relay2_Tips.colorGray_R 	= 5 + (pwmType_C - 5);
//			relay1_Tips.colorGray_R 	= 5 + (pwmType_D - 5);
//			
//			zigbNwk_Tips.colorGray_G	= 3 - pwmType_A / 5;
//			relay3_Tips.colorGray_G 	= 3 - pwmType_B / 5;
//			relay2_Tips.colorGray_G 	= 3 - pwmType_C / 5;
//			relay1_Tips.colorGray_G 	= 3 - pwmType_D / 5;
//			
//			zigbNwk_Tips.colorGray_B	= 7 - pwmType_A / 2;
//			relay3_Tips.colorGray_B 	= 7 - pwmType_B / 2;
//			relay2_Tips.colorGray_B 	= 7 - pwmType_C / 2;
//			relay1_Tips.colorGray_B 	= 7 - pwmType_D / 2;
//		
//		}break;
//		
//		case 2:{
//			
//			zigbNwk_Tips.colorGray_G	= pwmType_A / 5;
//			relay3_Tips.colorGray_G 	= pwmType_B / 5;
//			relay2_Tips.colorGray_G 	= pwmType_C / 5;
//			relay1_Tips.colorGray_G 	= pwmType_D / 5;
//		
//		}break;
//			
//		case 3:{
//		
//			zigbNwk_Tips.colorGray_G	= 3 - pwmType_A / 5;
//			relay3_Tips.colorGray_G 	= 3 - pwmType_B / 5;
//			relay2_Tips.colorGray_G 	= 3 - pwmType_C / 5;
//			relay1_Tips.colorGray_G 	= 3 - pwmType_D / 5;
//			
//			zigbNwk_Tips.colorGray_B	= pwmType_A / 5;
//			relay3_Tips.colorGray_B 	= pwmType_B / 5;
//			relay2_Tips.colorGray_B 	= pwmType_C /5;
//			relay1_Tips.colorGray_B 	= pwmType_D / 5;
//		
//		}break;
//			
//		case 4:{
//			
//			zigbNwk_Tips.colorGray_R	= 15 - pwmType_A;
//			relay3_Tips.colorGray_R 	= 15 - pwmType_B;
//			relay2_Tips.colorGray_R 	= 15 - pwmType_C;
//			relay1_Tips.colorGray_R 	= 15 - pwmType_D;

//			zigbNwk_Tips.colorGray_B	= 3 + (pwmType_A - 3);
//			relay3_Tips.colorGray_B 	= 3 + (pwmType_B - 3);
//			relay2_Tips.colorGray_B 	= 3 + (pwmType_C - 3);
//			relay1_Tips.colorGray_B 	= 3 + (pwmType_D - 3);
//			
//		}break;
//			
//		case 5:{

//			zigbNwk_Tips.colorGray_B	= 15 - pwmType_A;
//			relay3_Tips.colorGray_B 	= 15 - pwmType_B;
//			relay2_Tips.colorGray_B 	= 15 - pwmType_C;
//			relay1_Tips.colorGray_B 	= 15 - pwmType_D;
//			
//			zigbNwk_Tips.colorGray_G	= pwmType_A;
//			relay3_Tips.colorGray_G 	= pwmType_B;
//			relay2_Tips.colorGray_G 	= pwmType_C;
//			relay1_Tips.colorGray_G 	= pwmType_D;
//			
//		}break;
//			
//		case 6:{
//		
//			zigbNwk_Tips.colorGray_B	= pwmType_A;
//			relay3_Tips.colorGray_B 	= pwmType_B;
//			relay2_Tips.colorGray_B 	= pwmType_C;
//			relay1_Tips.colorGray_B 	= pwmType_D;
//			
//			zigbNwk_Tips.colorGray_G	= 15 - pwmType_A + 3;
//			relay3_Tips.colorGray_G 	= 15 - pwmType_B + 3;
//			relay2_Tips.colorGray_G 	= 15 - pwmType_C + 3;
//			relay1_Tips.colorGray_G 	= 15 - pwmType_D + 3;
//		
//		}break;
//		
//		case 7:{
//		
//			zigbNwk_Tips.colorGray_B	= 15 - pwmType_A;
//			relay3_Tips.colorGray_B 	= 15 - pwmType_B;
//			relay2_Tips.colorGray_B 	= 15 - pwmType_C;
//			relay1_Tips.colorGray_B 	= 15 - pwmType_D;
//			
//			zigbNwk_Tips.colorGray_G	= 3 - pwmType_A / 5;
//			relay3_Tips.colorGray_G 	= 3 - pwmType_A / 5;
//			relay2_Tips.colorGray_G 	= 3 - pwmType_A / 5;
//			relay1_Tips.colorGray_G 	= 3 - pwmType_A / 5;
//			
//		}break;
	
		case 0:{ /*��*///����
			
			zigbNwk_Tips.colorGray_G = pwmType_A / 5;
			relay3_Tips.colorGray_G = pwmType_B / 5;
			relay2_Tips.colorGray_G = pwmType_C / 5;
			relay1_Tips.colorGray_G = pwmType_D / 5;
			
		}break;
		
		case 1:{ /*��*///����
			
			zigbNwk_Tips.colorGray_R = pwmType_A;
			relay3_Tips.colorGray_R = pwmType_B;
			relay2_Tips.colorGray_R = pwmType_C;
			relay1_Tips.colorGray_R = pwmType_D;
			
		}break;
		
		case 2:{ /*��*///����
			
			zigbNwk_Tips.colorGray_G = 6 - pwmType_A / 5;
			relay3_Tips.colorGray_G = 6 - pwmType_B / 5;
			relay2_Tips.colorGray_G = 6 - pwmType_C / 5;
			relay1_Tips.colorGray_G = 6 - pwmType_D / 5;
			
		}break;
		
		case 3:{ /*��*///����
			
			zigbNwk_Tips.colorGray_B = pwmType_A / 2;
			relay3_Tips.colorGray_B = pwmType_B / 2;
			relay2_Tips.colorGray_B = pwmType_C / 2;
			relay1_Tips.colorGray_B = pwmType_D / 2;
			
		}break;
		
		case 4:{ /*��*///����
		
			zigbNwk_Tips.colorGray_R = 31 - pwmType_A;
			relay3_Tips.colorGray_R = 31 - pwmType_B;
			relay2_Tips.colorGray_R = 31 - pwmType_C;
			relay1_Tips.colorGray_R = 31 - pwmType_D;
			
		}break;
		
		case 5:{ /*��*///�������
		
			zigbNwk_Tips.colorGray_G = pwmType_A / 3;
			relay3_Tips.colorGray_G = pwmType_B / 3;
			relay2_Tips.colorGray_G = pwmType_C / 3;
			relay1_Tips.colorGray_G = pwmType_D / 3;
			
			zigbNwk_Tips.colorGray_R = pwmType_A / 2;
			relay3_Tips.colorGray_R = pwmType_B / 2;
			relay2_Tips.colorGray_R = pwmType_C / 2;
			relay1_Tips.colorGray_R = pwmType_D / 2;
			
		}break;
		
		case 6:{ /*dark*///ȫ��
		
			zigbNwk_Tips.colorGray_B = 15 - pwmType_A / 2;
			relay3_Tips.colorGray_B = 15 - pwmType_B / 2;
			relay2_Tips.colorGray_B = 15 - pwmType_C / 2;
			relay1_Tips.colorGray_B = 15 - pwmType_D / 2;
			
			zigbNwk_Tips.colorGray_G = 10 - pwmType_A / 3;
			relay3_Tips.colorGray_G = 10 - pwmType_B / 3;
			relay2_Tips.colorGray_G = 10 - pwmType_C / 3;
			relay1_Tips.colorGray_G = 10 - pwmType_D / 3;
			
			zigbNwk_Tips.colorGray_R = 15 - pwmType_A / 2;
			relay3_Tips.colorGray_R = 15 - pwmType_B / 2;
			relay2_Tips.colorGray_R = 15 - pwmType_C / 2;
			relay1_Tips.colorGray_R = 15 - pwmType_D / 2;
			
		}break;
		
		default:{}break;
	}
}


