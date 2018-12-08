#include "Relay.h"

#include "string.h"
#include "stdio.h"

#include "Tips.h"
#include "timerAct.h"
#include "dataTrans.h"
#include "dataManage.h"

#include "eeprom.h"

/**********************�����ļ�����������*****************************/
status_ifSave	xdata relayStatus_ifSave = statusSave_disable;	//���ؼ���ʹ�ܱ���
u8 				xdata status_Relay 		 = 0;

stt_motorAttr 	xdata curtainAct_Param 	 = {0, 3, cTact_stop};	//���豸����Ϊ����ʱ����Ӧ��������

relay_Command	xdata swCommand_fromUsr	 = {0, actionNull};

u8				xdata EACHCTRL_realesFLG = 0; //���ض�������ʹ�ܱ�־�����룩��־<bit0��һλ���ػ��ظ���; bit1����λ���ػ��ظ���; bit2����λ���ػ��ظ���;>
bit					  EACHCTRL_reportFLG = 0; //���ش������������ϱ�״̬ʹ��

relayStatus_PUSH xdata devActionPush_IF = {0};

bit				idata statusRelay_saveEn= 0; //����ֵ���ش洢ʹ��,���ʹ��,�ظ��洢

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
stt_attrFreq	xdata freq_Param		= {0};
#endif

/*�̵���״̬���£�Ӳ��ִ��*/
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
	
		case SWITCH_TYPE_SWBIT1:{ //�̵���λ�õ��� 2��1
		
			if(DEV_actReserve & 0x02)(status_Relay & 0x01)?(PIN_RELAY_1 = 1):(PIN_RELAY_1 = 0);
			
		}break;
		
		case SWITCH_TYPE_SWBIT2:{ //�̵���λ�õ��� 3��2
		
			if(DEV_actReserve & 0x01)(status_Relay & 0x01)?(PIN_RELAY_1 = 1):(PIN_RELAY_1 = 0);
			if(DEV_actReserve & 0x04)(status_Relay & 0x02)?(PIN_RELAY_2 = 1):(PIN_RELAY_2 = 0);
		
		}break;
		
		case SWITCH_TYPE_SWBIT3:{ //�̵���λ�ñ���
		
			if(DEV_actReserve & 0x01)(status_Relay & 0x01)?(PIN_RELAY_1 = 1):(PIN_RELAY_1 = 0);
			if(DEV_actReserve & 0x02)(status_Relay & 0x02)?(PIN_RELAY_2 = 1):(PIN_RELAY_2 = 0);
			if(DEV_actReserve & 0x04)(status_Relay & 0x04)?(PIN_RELAY_3 = 1):(PIN_RELAY_3 = 0);
		
		}break;
		
		default:break;
	}
#endif	
	
	tips_statusChangeToNormal();
}

/*���س�ʼ��*/
void relay_pinInit(void){
	
	u8 idata statusTemp = 0;
	
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
	//����
	P3M1	&= ~0x30;
	P3M0	|= 0x30;
	PIN_RELAY_2 = PIN_PWM_OUT = 0;
	
	P3M1	|= 0x08;   //P33��0�жϼ���
	P3M0	&= ~0x08;
    INT1 = 0;
    IT1 = 1; 
	PX1 = 0; //�����ȼ�
    EX1 = 1;                    

#else
	//����
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
		swCommand_fromUsr.actMethod = relay_OnOff; //Ӳ������
		
	}else{
	
		statusTemp = 0;
		coverEEPROM_write_n(EEPROM_ADDR_relayStatus, &statusTemp, 1);
		relay_statusReales(); //Ӳ������
	}
}

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
void Ext_INT1 (void) interrupt INT1_VECTOR{
	
	freq_Param.periodBeat_cfm = freq_Param.periodBeat_counter;
	freq_Param.periodBeat_counter = 0;
	
	freq_Param.pwm_actEN = 1;
}
#endif

/*���ض���*/
void relay_Act(relay_Command dats){
	
	u8 statusTemp = 0;
	
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER) //�������ͼ��أ����Զ��洢
	status_Relay = dats.objRelay;
	relay_statusReales();
	
#else
	
	statusTemp = status_Relay; //��ǰ����ֵ�ݴ�
	
	if(!countEN_ifTipsFree)countEN_ifTipsFree = 1; //�����ͷż�ʱʹ��
	
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
		
	}relay_statusReales(); //Ӳ������
	
	devActionPush_IF.dats_Push = 0;
	devActionPush_IF.dats_Push |= (status_Relay & 0x07); //��ǰ����ֵλ��װ<����λ>
	
//	/*���ȷ�ʽ*/
//	if(		(statusTemp & 0x01) != (status_Relay & 0x01))devActionPush_IF.dats_Push |= 0x20; //����ֵ��װ<����λ>��һλ
//	else if((statusTemp & 0x02) != (status_Relay & 0x02))devActionPush_IF.dats_Push |= 0x40; //����ֵ��װ<����λ>�ڶ�λ
//	else if((statusTemp & 0x04) != (status_Relay & 0x04))devActionPush_IF.dats_Push |= 0x80; //����ֵ��װ<����λ>����λ
	/*�����ȷ�ʽ*/
	if((statusTemp & 0x01) != (status_Relay & 0x01))devActionPush_IF.dats_Push |= 0x20; //����ֵ��װ<����λ>��һλ
	if((statusTemp & 0x02) != (status_Relay & 0x02))devActionPush_IF.dats_Push |= 0x40; //����ֵ��װ<����λ>�ڶ�λ
	if((statusTemp & 0x04) != (status_Relay & 0x04))devActionPush_IF.dats_Push |= 0x80; //����ֵ��װ<����λ>����λ
	
	if(status_Relay)delayCnt_closeLoop = 0; //����һ�������̸�����ɫģʽʱ�����ֵ
	
	if(relayStatus_ifSave == statusSave_enable){ //����״̬����
	
 #if(DATASAVE_INTLESS_ENABLEIF)
		devParamDtaaSave_relayStatusRealTime(status_Relay);
 #else
		statusTemp = status_Relay;
		coverEEPROM_write_n(EEPROM_ADDR_relayStatus, &statusTemp, 1);
 #endif
	}
	
#endif
}

/*�̵������߳�*/
void thread_Relay(void){
	
	if(swCommand_fromUsr.actMethod != actionNull){ //������Ӧ
	
		relay_Act(swCommand_fromUsr);
		
		swCommand_fromUsr.actMethod = actionNull;
		swCommand_fromUsr.objRelay = 0;
	}
	
	if(statusRelay_saveEn){
	
		u8 idata statusTemp = 0;
		
		statusRelay_saveEn = 0;
		
//#if(DEBUG_LOGOUT_EN == 1)
//		{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�
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