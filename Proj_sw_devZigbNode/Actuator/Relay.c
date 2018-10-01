#include "Relay.h"

#include "Tips.h"
#include "timerAct.h"
#include "dataTrans.h"
#include "dataManage.h"

#include "eeprom.h"

/**********************�����ļ�����������*****************************/
status_ifSave	xdata relayStatus_ifSave = statusSave_disable;	//���ؼ���ʹ�ܱ���
u8 				xdata status_Relay 		 = 0;

relay_Command	xdata swCommand_fromUsr	 = {0, actionNull};

u8				xdata EACHCTRL_realesFLG = 0; //���ض�������ʹ�ܱ�־�����룩��־<bit0��һλ���ػ��ظ���; bit1����λ���ػ��ظ���; bit2����λ���ػ��ظ���;>
bit					  EACHCTRL_reportFLG = 0; //���ش������������ϱ�״̬ʹ��

relayStatus_PUSH xdata devActionPush_IF = {0};

/*�̵���״̬���£�Ӳ��ִ��*/
void relay_statusReales(void){
	
	if(DEV_actReserve & 0x01)(status_Relay & 0x01)?(PIN_RELAY_1 = 1):(PIN_RELAY_1 = 0);
	if(DEV_actReserve & 0x02)(status_Relay & 0x02)?(PIN_RELAY_2 = 1):(PIN_RELAY_2 = 0);
	if(DEV_actReserve & 0x04)(status_Relay & 0x04)?(PIN_RELAY_3 = 1):(PIN_RELAY_3 = 0);

	tips_statusChangeToNormal();
}

/*���س�ʼ��*/
void relay_pinInit(void){
	
	u8 idata statusTemp = 0;

	//����
	P3M1	&= ~0x38;
	P3M0	|= 0x38;
	
	PIN_RELAY_1 = PIN_RELAY_2 = PIN_RELAY_3 = 0;
	
	if(relayStatus_ifSave == statusSave_enable){
		
		EEPROM_read_n(EEPROM_ADDR_relayStatus, &statusTemp, 1);
		status_Relay = statusTemp;
		relay_statusReales(); //Ӳ������
		
	}else{
	
		statusTemp = 0;
		coverEEPROM_write_n(EEPROM_ADDR_relayStatus, &statusTemp, 1);
		relay_statusReales(); //Ӳ������
	}
}

/*���ض���*/
void relay_Act(relay_Command dats){
	
	u8 statusTemp = 0;
	
	statusTemp = status_Relay; //��ǰ����ֵ�ݴ�
	
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
	if(		(statusTemp & 0x01) != (status_Relay & 0x01))devActionPush_IF.dats_Push |= 0x20; //����ֵ��װ<����λ>��һλ
	else if((statusTemp & 0x02) != (status_Relay & 0x02))devActionPush_IF.dats_Push |= 0x40; //����ֵ��װ<����λ>�ڶ�λ
	else if((statusTemp & 0x04) != (status_Relay & 0x04))devActionPush_IF.dats_Push |= 0x80; //����ֵ��װ<����λ>����λ
	
	if(status_Relay)delayCnt_closeLoop = 0; //����һ�������̸�����ɫģʽʱ�����ֵ
	
	if(relayStatus_ifSave == statusSave_enable){ //����״̬����
	
		statusTemp = status_Relay;
		coverEEPROM_write_n(EEPROM_ADDR_relayStatus, &statusTemp, 1);
	}
}

/*�̵������߳�*/
void thread_Relay(void){
	
	if(swCommand_fromUsr.actMethod != actionNull){ //������Ӧ
	
		relay_Act(swCommand_fromUsr);
		
		swCommand_fromUsr.actMethod = actionNull;
		swCommand_fromUsr.objRelay = 0;
	}
}