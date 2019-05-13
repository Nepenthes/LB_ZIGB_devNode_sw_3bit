#include "dataManage.h"

#include "STC15Fxxxx.H"

#include "Relay.h"

#include "eeprom.h"
#include "delay.h"
#include "USART.h"

#include "stdio.h"
#include "string.h"

#include "Tips.h"

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_FANS)
 u8 SWITCH_TYPE = SWITCH_TYPE_FANS;
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
 u8 SWITCH_TYPE = SWITCH_TYPE_dIMMER;
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
 u8 SWITCH_TYPE = SWITCH_TYPE_SOCKETS;
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)
 u8 SWITCH_TYPE = SWITCH_TYPE_INFRARED;
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SCENARIO)
 u8 SWITCH_TYPE = SWITCH_TYPE_SCENARIO;
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_HEATER)
 u8 SWITCH_TYPE = SWITCH_TYPE_HEATER;
#else
 u8 SWITCH_TYPE = SWITCH_TYPE_CURTAIN;
#endif

u8 DEV_actReserve = 0x01;

//u8 CTRLEATHER_PORT[clusterNum_usr] = {0x1A, 0x1B, 0x1C};
u8 CTRLEATHER_PORT[clusterNum_usr] = {0, 0, 0};
u16 idata mutualCtrlDevList[clusterNum_usr][MUTUALCTRL_DEV_NUM_MAX - 1] = {0}; //���������豸�б�,<����ʹ��idata �� data �ڴ棬xdata�ڴ���ȱ��,�ᵼ�µ�һ��8 bit�ڴ��޹ʱ�����>

u16 dev_currentPanid = 0;

#if(DEBUG_LOGOUT_EN == 1)	
 u8 xdata log_buf[LOGBUFF_LEN] = {0};
#endif		

/********************�����ļ�����������******************/
unsigned char xdata MAC_ID[6] 		= {0}; 
unsigned char xdata MAC_ID_DST[6] 	= {1,1,1,1,1,1};  //Զ��MAC��ַĬ��ȫ��1��ȫ��0�Ļ�Ӱ�����������

#if(DATASAVE_INTLESS_ENABLEIF)
 u8 xdata loopInsert_relayStatusRealTime_record = 0; //�̵���״̬ʵʱ��¼�α�
#endif

//�豸����־
bit	deviceLock_flag	= false;

//zigb������ڱ�־
bit zigbNwk_exist_FLG = 0;

/*MAC����*/
void MAC_ID_Relaes(void){
	
	u8 code *id_ptr = ROMADDR_ROM_STC_ID;

	memcpy(MAC_ID, id_ptr - 5, 6); //�����ǰ��������ֻȡ����λ
	
#if(DEBUG_LOGOUT_EN == 1)	
	{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�

		memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
		
		sprintf(log_buf, "mac_reales:%02X %02X %02X ", (int)MAC_ID[0], (int)MAC_ID[1], (int)MAC_ID[2]);
		PrintString1_logOut(log_buf);
		sprintf(log_buf, "%02X %02X %02X.\n", (int)MAC_ID[3], (int)MAC_ID[4], (int)MAC_ID[5]);
		PrintString1_logOut(log_buf);
	}
#endif

//	memcpy(MAC_ID, id_ptr - 5, 6); 
//	memcpy(MAC_ID, MACID_test, 6); 
}

void portCtrlEachOther_Reales(void){

	EEPROM_read_n(EEPROM_ADDR_portCtrlEachOther, CTRLEATHER_PORT, 3);
}

void devLockInfo_Reales(void){

	u8 xdata deviceLock_IF = 0;

	EEPROM_read_n(EEPROM_ADDR_deviceLockFLAG, &deviceLock_IF, 1);
	
	(deviceLock_IF)?(deviceLock_flag = 1):(deviceLock_flag = 0);
}

/*��ȡ��ǰ�������Ͷ�Ӧ��Ч����λ*/
u8 switchTypeReserve_GET(void){

	u8 act_Reserve = 0x07;

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
	act_Reserve = 0x01;
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)
	act_Reserve = 0x01;
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
	act_Reserve = 0x07;
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_FANS)
	act_Reserve = 0x07;
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SCENARIO)
	act_Reserve = 0x07;
#else
	switch(SWITCH_TYPE){
	
		case SWITCH_TYPE_HEATER:
		case SWITCH_TYPE_SWBIT1:{
		
			act_Reserve = 0x02;
		
		}break;
		
		case SWITCH_TYPE_SWBIT2:{
		
			act_Reserve = 0x05;
		
		}break;
		
		case SWITCH_TYPE_SWBIT3:
		case SWITCH_TYPE_CURTAIN:{
		
			act_Reserve = 0x07;
		
		}break;
	}
	
#endif
	
	return act_Reserve;
}

void statusSave_zigbNwk_nwkExistIF(bit nwkExistIF){
	
	u8 idata dataTemp = 0;

	zigbNwk_exist_FLG = nwkExistIF;
	
	(nwkExistIF)?(dataTemp = DATASAVE_MASK_ZIGBNWK_EXIST):(dataTemp = DATASAVE_MASK_ZIGBNWK_EXISTNOT);
	
	coverEEPROM_write_n(EEPROM_ADDR_zigbNwkExistIF, &dataTemp, 1);
	
#if(DEBUG_LOGOUT_EN == 1)	
	{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�

		memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
		
		sprintf(log_buf, "zigbNwk exsitFLG reales:%d.\n", (int)zigbNwk_exist_FLG);
		PrintString1_logOut(log_buf);
	}
#endif

}

bit statusGet_zigbNwk_nwkExistIF(void){

	u8 idata dataTemp = 0;
	
	EEPROM_read_n(EEPROM_ADDR_zigbNwkExistIF, &dataTemp, 1);
	
	if(dataTemp == DATASAVE_MASK_ZIGBNWK_EXIST)return 1;
	else return 0;
}

void zigbNwkExist_detectReales(void){

	zigbNwk_exist_FLG = statusGet_zigbNwk_nwkExistIF();
	
#if(DEBUG_LOGOUT_EN == 1)	
	{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�

		memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
		
		sprintf(log_buf, "zigbNwk exsitFLG reales:%d.\n", (int)zigbNwk_exist_FLG);
		PrintString1_logOut(log_buf);
	}
#endif
}

void mutualCtrlSysParam_checkAndStore(u8 mutualCtrlGroup_insert, u16 devAddr){

	u8 xdata 	loop = 0;
	bit idata 	devAddrExist_IF = 0;
	
	if(mutualCtrlGroup_insert > (clusterNum_usr - 1))return;
	
	for(loop = 0; loop < (MUTUALCTRL_DEV_NUM_MAX - 1); loop ++){
		
#if(DEBUG_LOGOUT_EN == 1)	
		{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�

			memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
			
			sprintf(log_buf, "addr_t:%04X, addr_s:%04X.\n", (int)devAddr, (int)mutualCtrlDevList[mutualCtrlGroup_insert][loop]);
			PrintString1_logOut(log_buf);
		}
#endif
	
		if(devAddr == mutualCtrlDevList[mutualCtrlGroup_insert][loop]){
		
			devAddrExist_IF = 1;
			break;
		}
	}
	
	if(devAddrExist_IF){
	
#if(DEBUG_LOGOUT_EN == 1)	
		{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�

			memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
			
			sprintf(log_buf, "devAddr:%04X exist in mutualGroup:%d.\n", (int)devAddr, (int)mutualCtrlGroup_insert);
			PrintString1_logOut(log_buf);
		}
#endif
	
	}else{
		
		memcpy((u8 *)&(mutualCtrlDevList[mutualCtrlGroup_insert][0]), (u8 *)&(mutualCtrlDevList[mutualCtrlGroup_insert][1]), sizeof(u16) * (MUTUALCTRL_DEV_NUM_MAX - 2));
		mutualCtrlDevList[mutualCtrlGroup_insert][MUTUALCTRL_DEV_NUM_MAX - 2] = devAddr;
		coverEEPROM_write_n(EEPROM_ADDR_mutualCtrlAddrs, (u8 *)mutualCtrlDevList, sizeof(u16) * clusterNum_usr * (MUTUALCTRL_DEV_NUM_MAX - 1));
	
#if(DEBUG_LOGOUT_EN == 1)	
		{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�

			memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
			
			sprintf(log_buf, "devAddr:%04X add to mutualGroup:%d.\n", (int)devAddr, (int)mutualCtrlGroup_insert);
			PrintString1_logOut(log_buf);
		}
		
//		{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�

//			memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
//			
//			sprintf(log_buf, "devAddrList[0]:%04X %04X.\n", (int)mutualCtrlDevList[mutualCtrlGroup_insert][0],
//															(int)mutualCtrlDevList[mutualCtrlGroup_insert][1]);
//			PrintString1_logOut(log_buf);
//		}
#endif
	}
}

void mutualCtrlSysParam_dataReset(u8 opreatBit){

	u8 idata loop = 0;
	
	for(loop = 0; loop < clusterNum_usr; loop ++){
	
		if(opreatBit & (1 << loop))memset(mutualCtrlDevList[loop], 0xff, sizeof(u16) * (MUTUALCTRL_DEV_NUM_MAX - 1));
	}
	
	coverEEPROM_write_n(EEPROM_ADDR_mutualCtrlAddrs, (u8 *)mutualCtrlDevList, sizeof(u16) * clusterNum_usr * (MUTUALCTRL_DEV_NUM_MAX - 1));
}

void mutualCtrlSysParam_dataRecover(void){

	EEPROM_read_n(EEPROM_ADDR_mutualCtrlAddrs, (u8 *)mutualCtrlDevList, sizeof(u16) * clusterNum_usr * (MUTUALCTRL_DEV_NUM_MAX - 1));
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
	
	coverEEPROM_write_n(EEPROM_ADDR_START_USRDATA, datsTemp, EEPROM_USE_OF_NUMBER); //�״�����EEPROM����
	datsTemp[0] = BIRTHDAY_FLAG;
	coverEEPROM_write_n(EEPROM_ADDR_BirthdayMark, &datsTemp[0], 1);	//��������
	
	memset(datsTemp, 0xff, sizeof(bkLightColorInsert_paramAttr)); //����Ʋ��������� --������0xff��������ʹ����Ƴ�ʼ��ʱ�����ָ���Ĭ��ֵ
	coverEEPROM_write_n(EEPROM_ADDR_ledSWBackGround, datsTemp, sizeof(bkLightColorInsert_paramAttr));
	
	datsTemp[0] = 0;
	coverEEPROM_write_n(EEPROM_ADDR_deviceLockFLAG, &datsTemp[0], 1); //���½���
	
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_FANS)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SCENARIO)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_HEATER)
#else
	datsTemp[0] = CURTAIN_ORBITAL_PERIOD_INITTIME; //�������ʱ�� ����ʼֵ
	coverEEPROM_write_n(EEPROM_ADDR_curtainOrbitalPeriod, &datsTemp[0], 1);
	
#endif	

	datsTemp[0] = 0;
	coverEEPROM_write_n(EEPROM_ADDR_portCtrlEachOther, &datsTemp[0], clusterNum_usr);
	
	memset(CTRLEATHER_PORT, 0, clusterNum_usr); //���л������
	
	delayMs(10);
	
	((void(code *)(void))0x0000)(); //����
}

void birthDay_Judge(void){

	u8 xdata datsTemp = 0;
	
	EEPROM_read_n(EEPROM_ADDR_BirthdayMark, &datsTemp, 1);
	if(datsTemp != BIRTHDAY_FLAG){
	
		Factory_recover(); //�״�����EEPROM����
	}
}

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED) //�������ݴ洢��һ������һ������
void infrared_eeprom_dataSave(u8 insertNum, u8 dats[], u8 datsLen){
	
	//һ������Ϊ512 Bytes��һ������ָ��Ϊ232 Bytes����һ���������Դ���������ָ��

	u16 code EEPROM_SECTOR_HALFSIZE = EEPROM_SECTOR_SIZE / 2;
	
	u8	xdata dataSaveTemp[EEPROM_SECTOR_SIZE] = {0}; //���ݴ洢����
	u16 xdata EEadress_Insert = EEPROM_ADDR_START_IRDATA + ((u16)(insertNum / 2) * EEPROM_SECTOR_SIZE); //�洢��ַ��������
	
	EEPROM_read_n(EEadress_Insert, dataSaveTemp, EEPROM_SECTOR_SIZE); //�����������ݶ�������
	EEPROM_SectorErase(EEadress_Insert); //�����ȡ�������������
	
	if(insertNum % 2){ //��������Ӧ�������
	
		memset(&dataSaveTemp[EEPROM_SECTOR_HALFSIZE], 0, EEPROM_SECTOR_HALFSIZE); //���ݴ洢���������
		memcpy(&dataSaveTemp[EEPROM_SECTOR_HALFSIZE], dats, datsLen);
		
	}else{ //ż������Ӧ����ǰ��
	
		memset(&dataSaveTemp[0], 0, EEPROM_SECTOR_HALFSIZE); //���ݴ洢������ǰ��
		memcpy(&dataSaveTemp[0], dats, datsLen);
	}
	
	EEPROM_write_n(EEadress_Insert, dataSaveTemp, EEPROM_SECTOR_SIZE);
}

void infrared_eeprom_dataRead(u8 insertNum, u8 dats[], u8 datsLen){
	
	//һ������Ϊ512 Bytes��һ������ָ��Ϊ232 Bytes����һ���������Դ���������ָ��
	
	u16 code EEPROM_SECTOR_HALFSIZE = EEPROM_SECTOR_SIZE / 2;

	u16 xdata EEadress_Insert = EEPROM_ADDR_START_IRDATA + ((u16)(insertNum / 2) * EEPROM_SECTOR_SIZE); //�洢��ַ��������
	
	if(insertNum % 2){ //��������Ӧ�������
	
		EEadress_Insert += EEPROM_SECTOR_HALFSIZE; //�洢�����������������
		
	}else{ //ż������Ӧ����ǰ��
	
		EEadress_Insert = EEadress_Insert; //�洢��������������ǰ��
	}
	
	EEPROM_read_n(EEadress_Insert, dats, datsLen);
}
#else
 #if(DATASAVE_INTLESS_ENABLEIF) //�̵���״̬EEPROM�����洢��غ�������
void devParamDtaaSave_relayStatusRealTime(u8 currentRelayStatus){
	
	u8 xdata dataRead_temp[RECORDPERIOD_OPREATION_LOOP] = {0};
	
	if(loopInsert_relayStatusRealTime_record >= RECORDPERIOD_OPREATION_LOOP){
	
		loopInsert_relayStatusRealTime_record = 0;
		EEPROM_SectorErase(EEPROM_ADDR_START_STATUSRELAY); //������
	}
	
	dataRead_temp[loopInsert_relayStatusRealTime_record ++] = currentRelayStatus;
	
	EEPROM_write_n(EEPROM_ADDR_START_STATUSRELAY, dataRead_temp, loopInsert_relayStatusRealTime_record);
}

u8 devDataRecovery_relayStatus(void){

	u8 xdata dataRead_temp[RECORDPERIOD_OPREATION_LOOP] = {0};
	u8 xdata loop = 0;
	u8 xdata res = 0;
	
	EEPROM_read_n(EEPROM_ADDR_START_STATUSRELAY, dataRead_temp, RECORDPERIOD_OPREATION_LOOP);
	
	for(loop = 0; loop < RECORDPERIOD_OPREATION_LOOP; loop ++){
	
		if(dataRead_temp[loop] == 0xff){
		
			(!loop)?(res = 0):(res = dataRead_temp[loop - 1]);
  #if(DEBUG_LOGOUT_EN == 1)	
			{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�

				memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
				
				sprintf(log_buf, "insert catch: %d, val:%02X.\n", (int)loop, (int)res);
				PrintString1_logOut(log_buf);
			}
  #endif
			break;
		}
	}
	
	EEPROM_SectorErase(EEPROM_ADDR_START_STATUSRELAY); //������
	
	return res;
}
 #endif

#endif