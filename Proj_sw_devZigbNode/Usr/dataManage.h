#ifndef __DATAMANAGE_H_
#define __DATAMANAGE_H_

#include "STC15Fxxxx.H"

#define 	DEBUG_LOGOUT_EN		1 //log��ӡ���ʹ�ܣ�ʧ�ܺ󽫹黹��������ռ䣩

#define		clusterNum_usr		3 //�Զ���ͨѶ�����������أ�

#define		SW_SCENCRAIO_LEN				31   //���س����洢��������
#define		SW_SCENCRAIO_INSERTINVALID		0xFF //������������Чֵ
#define		SW_SCENCRAIO_ACTINVALID			0xF0 //�����Ŷ�Ӧ������Ӧ״̬λ��Чֵ

#define 	SWITCH_TYPE_SWBIT1	 			 (0x01 + 0x38) //�豸���ͣ�һλ����
#define 	SWITCH_TYPE_SWBIT2	 			 (0x02 + 0x38) //�豸���ͣ���λ����
#define 	SWITCH_TYPE_SWBIT3	 			 (0x03 + 0x38) //�豸���ͣ���λ����

//#define 	ROMADDR_ROM_STC_ID		 		 0x3ff8		//STC��Ƭ�� ȫ��ID��ַ
#define 	ROMADDR_ROM_STC_ID		 		 0x7ff8		//STC��Ƭ�� ȫ��ID��ַ

#define 	EEPROM_ADDR_START	 			 0x0000		//��ʼ��ַ

#define 	EEPROM_USE_OF_NUMBER 			 0x0080	
	
#define		BIRTHDAY_FLAG					 0xA1		//��Ʒ�������
	
#define		EEPROM_ADDR_BirthdayMark         0x0001		//01H - 01H �Ƿ��״�����							01_Byte
#define  	EEPROM_ADDR_relayStatus          0x0002		//02H - 02H ����״̬�洢							01_Byte
#define  	EEPROM_ADDR_timeZone_H           0x0003		//03H - 03H ʱ������ʱ								01_Byte
#define  	EEPROM_ADDR_timeZone_M           0x0004		//04H - 04H ʱ��������								01_Byte
#define  	EEPROM_ADDR_deviceLockFLAG       0x0005		//05H - 05H �豸��״̬λ							01_Byte
#define		EEPROM_ADDR_portCtrlEachOther	 0x0006		//06H - 08H ����λ�󶨶˿�,����Ϊ1��2��3λ			03_Byte 
#define  	EEPROM_ADDR_swTimeTab          	 0x0010		//10H - 1CH 4����ͨ��ʱ���ݣ�ÿ��3�ֽ�				12_Byte	
#define  	EEPROM_ADDR_swDelayFLAG			 0x0020		//20H - 20H ������ʱ��־λ����						01_Byte
#define 	EEPROM_ADDR_periodCloseLoop		 0x0021		//21H - 21H	ѭ���ر�ʱ����						01_Byte
#define 	EEPROM_ADDR_TimeTabNightMode	 0x0022		//22H - 27H ҹ��ģʽ��ʱ��							06_Byte
#define 	EEPROM_ADDR_ledSWBackGround		 0x0030		//30H - 31H	���ر�����ɫ����						02_Byte
#define		EEPROM_ADDR_swScenarioNum		 0x0040		//40H - 5FH �������								31_Byte
#define		EEPROM_ADDR_swScenarioAct	     0x0060		//60H - 7FH ������Ӧ����							31_Byte
#define		EEPROM_ADDR_unDefine05           0x0000
#define		EEPROM_ADDR_unDefine06           0x0000
#define		EEPROM_ADDR_unDefine07           0x0000
#define		EEPROM_ADDR_unDefine08           0x0000
#define		EEPROM_ADDR_unDefine11           0x0000
#define		EEPROM_ADDR_unDefine12           0x0000
#define		EEPROM_ADDR_unDefine13           0x0000

extern u8 SWITCH_TYPE;
extern u8 DEV_actReserve;
extern u8 CTRLEATHER_PORT[3];

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

#endif