#ifndef __DATAMANAGE_H_
#define __DATAMANAGE_H_

#include "STC15Fxxxx.H"

#define 	DEBUG_LOGOUT_EN		1 //log��ӡ���ʹ�ܣ�ʧ�ܺ󽫹黹��������ռ䣩
#define 	LOGBUFF_LEN			64 //log��ӡ���泤��	

#define		clusterNum_usr		3 //�Զ���ͨѶ�����������أ�

#define		SW_SCENCRAIO_LEN				31   //���س����洢��������
#define		SW_SCENCRAIO_INSERTINVALID		0xFF //������������Чֵ
#define		SW_SCENCRAIO_ACTINVALID			0xF0 //�����Ŷ�Ӧ������Ӧ״̬λ��Чֵ

#define 	DEVICE_VERSION_NUM				 7 //�豸�汾�ţ�L7
#define 	SWITCH_TYPE_SWBIT1	 			 (0x38 + 0x01) //�豸���ͣ�һλ����
#define 	SWITCH_TYPE_SWBIT2	 			 (0x38 + 0x02) //�豸���ͣ���λ����
#define 	SWITCH_TYPE_SWBIT3	 			 (0x38 + 0x03) //�豸���ͣ���λ����
#define		SWITCH_TYPE_CURTAIN				 (0x38 + 0x08) //�豸���ͣ�����

#define 	SWITCH_TYPE_FANS				 (0x38 + 0x05) //�豸���ͣ�����
#define 	SWITCH_TYPE_SOCKETS				 (0x38 + 0x07) //�豸���ͣ�����
#define 	SWITCH_TYPE_dIMMER				 (0x38 + 0x04) //�豸���ͣ�����

#define 	SWITCH_TYPE_FORCEDEF			 0 //ǿ�ƿ������Ͷ���,Ӳ����ͬ,д 0 ʱ���Ƿ�ǿ�ƶ���

//#define 	ROMADDR_ROM_STC_ID		 		 0x3ff8		//STC��Ƭ�� ȫ��ID��ַ
#define 	ROMADDR_ROM_STC_ID		 		 0x7ff8		//STC��Ƭ�� ȫ��ID��ַ

#define 	EEPROM_ADDR_START	 			 0x0000		//����������ʼ������ַ
#define		EEPROM_ADDR_STATUSRELAY			 0x0200		//�̵���״̬������¼��ʼ������ַ

#define 	EEPROM_USE_OF_NUMBER 			 0x0080	

#define 	DATASAVE_INTLESS_ENABLEIF	 1	//�Ƿ񽫼̵���״̬���ж���ʵʱ��¼<�ڿ���״̬����ʹ�ܵ�����£������˹��ܿ���Ч���ⴥ��ʱ��˸>
#if(DATASAVE_INTLESS_ENABLEIF)
 #define 	RECORDPERIOD_OPREATION_LOOP	100	//�̵���ʵʱ״̬��¼ ��ѭ�� �洢������Ԫ��������
#endif
	
#define		BIRTHDAY_FLAG					 0xA1		//��Ʒ�������
	
#define		EEPROM_ADDR_BirthdayMark         0x0001		//01H - 01H �Ƿ��״�����							01_Byte
#define  	EEPROM_ADDR_relayStatus          0x0002		//02H - 02H ����״̬�洢							01_Byte
#define  	EEPROM_ADDR_timeZone_H           0x0003		//03H - 03H ʱ������ʱ								01_Byte
#define  	EEPROM_ADDR_timeZone_M           0x0004		//04H - 04H ʱ��������								01_Byte
#define  	EEPROM_ADDR_deviceLockFLAG       0x0005		//05H - 05H �豸��״̬λ							01_Byte
#define		EEPROM_ADDR_portCtrlEachOther	 0x0006		//06H - 08H ����λ�󶨶˿�,����Ϊ1��2��3λ			03_Byte 
#define  	EEPROM_ADDR_swTimeTab          	 0x0010		//10H - 28H 8����ͨ��ʱ���ݣ�ÿ��3�ֽ�				24_Byte	
#define  	EEPROM_ADDR_swDelayFLAG			 0x0030		//30H - 30H ������ʱ��־λ����						01_Byte
#define 	EEPROM_ADDR_periodCloseLoop		 0x0031		//31H - 31H	ѭ���ر�ʱ����						01_Byte
#define 	EEPROM_ADDR_TimeTabNightMode	 0x0032		//32H - 37H ҹ��ģʽ��ʱ��							06_Byte
#define 	EEPROM_ADDR_curtainActPeriod	 0x0033		//33H - 33H ������������ʱ��						01_Byte
#define 	EEPROM_ADDR_ledSWBackGround		 0x0040		//40H - 41H	���ر�����ɫ����						02_Byte
#define		EEPROM_ADDR_swScenarioNum		 0x0050		//50H - 6FH �������								31_Byte
#define		EEPROM_ADDR_swScenarioAct	     0x0070		//70H - 8FH ������Ӧ����							31_Byte
#define		EEPROM_ADDR_unDefine05           0x0000
#define		EEPROM_ADDR_unDefine06           0x0000
#define		EEPROM_ADDR_unDefine07           0x0000
#define		EEPROM_ADDR_unDefine08           0x0000
#define		EEPROM_ADDR_unDefine11           0x0000
#define		EEPROM_ADDR_unDefine12           0x0000
#define		EEPROM_ADDR_unDefine13           0x0000

/*=======================������������ʱѯ�û���ר�����ݽṹ����������=============================*/

typedef struct agingDataSet_bitHold{ //���ݽṹ_ʱЧռλ;	ʹ��ָ��ǿתʱע�⣬agingCmd_swOpreat��Ӧ���ֽ����λbit0, ��������<tips,����ƽ̨��С�˲�ͬ����bit0����뻹���Ҷ���>

	u8 agingCmd_swOpreat:1; //ʱЧ_���ز��� -bit0
	u8 agingCmd_devLock:1; //ʱЧ_�豸������ -bit1
	u8 agingCmd_delaySetOpreat:1; //ʱЧ_��ʱ���� -bit2
	u8 agingCmd_greenModeSetOpreat:1; //ʱЧ_��ɫģʽ���� -bit3
	u8 agingCmd_timerSetOpreat:1; //ʱЧ_��ʱ���� -bit4
	u8 agingCmd_nightModeSetOpreat:1; //ʱЧ_ҹ��ģʽ���� -bit5
	u8 agingCmd_bkLightSetOpreat:1; //ʱЧ_��������� -bit6
	u8 agingCmd_devResetOpreat:1; //ʱЧ_���ظ�λ�ָ��������� -bit7
	
	u8 agingCmd_horsingLight:1; //ʱЧ_��������� -bit0
	u8 agingCmd_switchBitBindSetOpreat:3; //ʱЧ_����λ����������_�����������λ�������� -bit1...bit3
	u8 agingCmd_curtainOpPeriodSetOpreat:1; //ʱЧ_��Դ���������������ʱ������ -bit4
	u8 statusRef_bitReserve:3; //ʱЧ_bit���� -bit5...bit7
	
	u8 agingCmd_byteReserve[4];	//5�ֽ�ռλ����
	
}stt_agingDataSet_bitHold; //standard_length = 6Bytes

typedef struct swDevStatus_reference{ //���ݽṹ_�豸����״̬

	u8 statusRef_swStatus:3; //״̬_�豸����״̬ -bit0...bit2
	u8 statusRef_reserve:2; //״̬_reserve -bit3...bit4
	u8 statusRef_swPush:3; //״̬_����ռλ -bit5...bit7
	
	u8 statusRef_timer:1; //״̬_��ʱ������ -bit0
	u8 statusRef_devLock:1; //״̬_�豸�� -bit1
	u8 statusRef_delay:1; //״̬_��ʱ���� -bit2
	u8 statusRef_greenMode:1; //״̬_��ɫģʽ���� -bit3
	u8 statusRef_nightMode:1; //״̬_ҹ��ģʽ���� -bit4
	u8 statusRef_horsingLight:1; //״̬_��������� -bit5
	u8 statusRef_bitReserve:2; //״̬_reserve -bit6...bit7
	
	u8 statusRef_byteReserve[2];   //״̬_reserve -bytes2...3
	
}stt_swDevStatusReference_bitHold; //standard_length = 4Bytes

typedef struct dataPonit{ //���ݽṹ_���ݵ�

	stt_agingDataSet_bitHold 			devAgingOpreat_agingReference; //ʱЧ����ռλ, 6Bytes
	stt_swDevStatusReference_bitHold	devStatus_Reference; //�豸״̬ռλ, 4Bytes 			
	u8 						 			devData_timer[24]; //��ʱ������ 8��, 24Bytes
	u8									devData_delayer; //��ʱ����, 1Bytes
	u8									devData_delayUpStatus; //��ʱ����ʱ��������Ӧ״̬, 1Bytes
	u8									devData_greenMode; //��ɫģʽ����, 1Bytes
	u8									devData_nightMode[6]; //ҹ��ģʽ����, 6Bytes
	u8									devData_bkLight[2]; //�������ɫ����, 2Bytes
	u8									devData_devReset; //���ظ�λ����, 1Bytes
	u8									devData_switchBitBind[3]; //����λ���ذ�����, 3Bytes
	
}stt_devOpreatDataPonit; //standard_length = 49Bytes

/*=======================������������ʱѯ�û���ר�����ݽṹ����������=============================*/
#if(DEBUG_LOGOUT_EN == 1)	
 extern u8 xdata log_buf[LOGBUFF_LEN];
#endif		

extern u8 	SWITCH_TYPE;
extern u8 	DEV_actReserve;
extern u8 	CTRLEATHER_PORT[3];
extern u16 	dev_currentPanid;

extern unsigned char xdata MAC_ID[6];
extern unsigned char xdata MAC_ID_DST[6];

extern bit deviceLock_flag;

u8 switchTypeReserve_GET(void);
void MAC_ID_Relaes(void);
void portCtrlEachOther_Reales(void);

bit swScenario_oprateSave(u8 scenarioNum, u8 swAct);
bit swScenario_oprateDele(u8 scenarioNum);
u8 swScenario_oprateCheck(u8 scenarioNum);

void Factory_recover(void);
void birthDay_Judge(void);

void devParamDtaaSave_relayStatusRealTime(u8 currentRelayStatus);
u8 devDataRecovery_relayStatus(void);

#endif