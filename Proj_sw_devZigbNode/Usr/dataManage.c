#include "dataManage.h"

#include "STC15Fxxxx.H"

#include "eeprom.h"
#include "delay.h"

#include "stdio.h"
#include "string.h"

#include "Tips.h"

u8 SWITCH_TYPE = SWITCH_TYPE_SWBIT3;
u8 DEV_actReserve = 0x01;

//u8 CTRLEATHER_PORT[clusterNum_usr] = {0x1A, 0x1B, 0x1C};
u8 CTRLEATHER_PORT[clusterNum_usr] = {0, 0, 0};

/********************�����ļ�����������******************/
unsigned char xdata MAC_ID[6] 		= {0}; 
unsigned char xdata MAC_ID_DST[6] 	= {1,1,1,1,1,1};  //Զ��MAC��ַĬ��ȫ��1��ȫ��0�Ļ�Ӱ�����������

//�豸����־
bit	deviceLock_flag	= false;

/*MAC����*/
void MAC_ID_Relaes(void){
	
	u8 code *id_ptr = ROMADDR_ROM_STC_ID;

	memcpy(MAC_ID, id_ptr - 6, 6); //˳����ǰ����ǰ����ֻȡ����λ

//	memcpy(MAC_ID, id_ptr - 6, 6); 
//	memcpy(MAC_ID, MACID_test, 6); 
}

void portCtrlEachOther_Reales(void){

	EEPROM_read_n(EEPROM_ADDR_portCtrlEachOther, CTRLEATHER_PORT, 3);
}

void devLockInfo_Reales(void){

	u8 xdata deviceLock_IF = 0;

	coverEEPROM_write_n(EEPROM_ADDR_deviceLockFLAG, &deviceLock_IF, 1);
	
	(deviceLock_IF)?(deviceLock_flag = 1):(deviceLock_flag = 0);
}

/*��ȡ��ǰ�������Ͷ�Ӧ��Ч����λ*/
u8 switchTypeReserve_GET(void){

	u8 act_Reserve = 0x07;

	if(SWITCH_TYPE == SWITCH_TYPE_SWBIT3){
		
		act_Reserve = 0x07;
		
	}else
	if(SWITCH_TYPE == SWITCH_TYPE_SWBIT2){
		
		act_Reserve = 0x03;
	
	}else
	if(SWITCH_TYPE == SWITCH_TYPE_SWBIT1){
	
		act_Reserve = 0x01;
	}
	
	return act_Reserve;
}

///*�����Ŷ�ӦEEPROM�洢�����Ų���*/
//u8 swScenarioNum_findFromEEPROM(bit vacancyFind_IF, u8 scenarioNum){ //�Ƿ�Ϊ��λ���� ����ָ�������Ų���

//	u8 loop = 0;
//	u8 xdata datsTemp[SW_SCENCRAIO_LEN] = {0};
//	
//	EEPROM_read_n(EEPROM_ADDR_swScenarioNum, datsTemp, SW_SCENCRAIO_LEN);
//	for(loop = 0; loop < SW_SCENCRAIO_LEN; loop ++){
//	
//		if(vacancyFind_IF){ //���ҿ�λ
//		
//			if(0 == datsTemp[loop] || 0xff == datsTemp[loop])break;
//		
//		}else{	//����ָ���������
//		
//			if(scenarioNum == datsTemp[loop])break;
//		}
//	}
//	
//	if(loop < SW_SCENCRAIO_LEN){
//	
//		return loop; //�����ɲ� ��������
//		
//	}else{
//	
//		return SW_SCENCRAIO_INSERTINVALID; //�������ɲ� ������Чֵ
//	}
//}

///*�����洢*/
//bit swScenario_oprateSave(u8 scenarioNum, u8 swAct){

//	u8 datsTemp = 0;
//	u8 insert = 0;
//	
//	insert = swScenarioNum_findFromEEPROM(0, scenarioNum); //�鳡�����
//	if(SW_SCENCRAIO_INSERTINVALID != insert){ //������ſɲ鵽����Ķ�Ӧ��Ӧ״̬
//	
//		datsTemp = swAct;
//		coverEEPROM_write_n(EEPROM_ADDR_swScenarioAct + insert, &datsTemp, 1);
//		
//	}else{ //������Ų��ɲ鵽������
//		
//		insert = swScenarioNum_findFromEEPROM(1, 0); //���λ
//		if(SW_SCENCRAIO_INSERTINVALID != insert){ //�п�λ������
//		
//			datsTemp = swAct;
//			coverEEPROM_write_n(EEPROM_ADDR_swScenarioNum + insert, &datsTemp, 1);
//			datsTemp = scenarioNum;
//			coverEEPROM_write_n(EEPROM_ADDR_swScenarioAct + insert, &datsTemp, 1);
//		
//		}else{ //�޿�λ����ʧ��
//		
//			return 0;
//		}
//	}
//	
//	return 1;
//}

///*����ɾ��*/
//bit swScenario_oprateDele(u8 scenarioNum){

//	u8 datsTemp = 0;
//	u8 insert = 0;
//	
//	insert = swScenarioNum_findFromEEPROM(0, scenarioNum); //�鳡�����
//	
//	if(SW_SCENCRAIO_INSERTINVALID != insert){ //������ſɲ鵽��ִ��ɾ��
//	
//		coverEEPROM_write_n(EEPROM_ADDR_swScenarioNum + insert, &datsTemp, 1);
//		coverEEPROM_write_n(EEPROM_ADDR_swScenarioAct + insert, &datsTemp, 1);
//		
//		return 1;
//		
//	}else{
//	
//		return 0;
//	}
//}

///*������Ӧ��Ӧ������ѯ*/
//u8 swScenario_oprateCheck(u8 scenarioNum){

//	u8 datsTemp = 0;
//	u8 insert = 0;
//	
//	insert = swScenarioNum_findFromEEPROM(0, scenarioNum); //�鳡�����
//	
//	if(SW_SCENCRAIO_INSERTINVALID != insert){ //������ſɲ鵽��ִ��ɾ��
//		
//		EEPROM_read_n(EEPROM_ADDR_swScenarioAct, &datsTemp, 1); //��ȡ������Ӧ����������
//		return datsTemp;
//		
//	}else{
//	
//		return SW_SCENCRAIO_ACTINVALID; //��������Ч���ɲ� ������Ч��Ӧ����ֵ
//	}
//}

void Factory_recover(void){
	
	u8 xdata datsTemp[EEPROM_USE_OF_NUMBER] = {0};
	
	coverEEPROM_write_n(EEPROM_ADDR_START, datsTemp, EEPROM_USE_OF_NUMBER); //�״�����EEPROM����
	datsTemp[0] = BIRTHDAY_FLAG;
	coverEEPROM_write_n(EEPROM_ADDR_BirthdayMark, &datsTemp[0], 1);	//��������
	
	datsTemp[0] = TIPSBKCOLOR_DEFAULT_ON; //�����ʼ��
	coverEEPROM_write_n(EEPROM_ADDR_ledSWBackGround, &datsTemp[0], 1);
	datsTemp[0] = TIPSBKCOLOR_DEFAULT_OFF;
	coverEEPROM_write_n(EEPROM_ADDR_ledSWBackGround + 1, &datsTemp[0], 1);	
	
	datsTemp[0] = 0;
	coverEEPROM_write_n(EEPROM_ADDR_portCtrlEachOther, &datsTemp[0], clusterNum_usr);
	
	delayMs(10);
	
//	((void(code *)(void))0x0000)(); //����
}

void birthDay_Judge(void){

	u8 xdata datsTemp = 0;
	
	EEPROM_read_n(EEPROM_ADDR_BirthdayMark, &datsTemp, 1);
	if(datsTemp != BIRTHDAY_FLAG){
	
		Factory_recover();//�״�����EEPROM����
	}
}