#ifndef __DATAMANAGE_H_
#define __DATAMANAGE_H_

#include "STC15Fxxxx.H"

#define 	DEBUG_LOGOUT_EN					0 //log��ӡ���ʹ�ܣ�ʧ�ܺ󽫹黹��������ռ䣩
#define 	LOGBUFF_LEN						56 //log��ӡ���泤��	

#define		clusterNum_usr					3 //�Զ���ͨѶ�����������أ�
#define 	MUTUALCTRL_DEV_NUM_MAX			3 //���黥�����豸�������

#define 	DATASAVE_INTLESS_ENABLEIF	 	1	//�Ƿ񽫼̵���״̬���ж���ʵʱ��¼<�ڿ���״̬����ʹ�ܵ�����£������˹��ܿ���Ч���ⴥ��ʱ��˸>

#define		SW_SCENCRAIO_LEN				16   //���س����洢��������
#define		SW_SCENCRAIO_INSERTINVALID		0xFF //������������Чֵ
#define		SW_SCENCRAIO_ACTINVALID			0xF0 //�����Ŷ�Ӧ������Ӧ״̬λ��Чֵ 

#define 	DEVICE_VERSION_NUM				 7 //�豸�汾�ţ�L7
#define 	SWITCH_TYPE_SWBIT1	 			 (0x38 + 0x01) //�豸���ͣ�һλ����
#define 	SWITCH_TYPE_SWBIT2	 			 (0x38 + 0x02) //�豸���ͣ���λ����
#define 	SWITCH_TYPE_SWBIT3	 			 (0x38 + 0x03) //�豸���ͣ���λ����
#define		SWITCH_TYPE_CURTAIN				 (0x38 + 0x08) //�豸���ͣ�����

#define 	SWITCH_TYPE_dIMMER				 (0x38 + 0x04) //�豸���ͣ�����
#define 	SWITCH_TYPE_FANS				 (0x38 + 0x05) //�豸���ͣ�����
#define 	SWITCH_TYPE_INFRARED			 (0x38 + 0x06) //�豸���ͣ�����ת����
#define 	SWITCH_TYPE_SOCKETS				 (0x38 + 0x07) //�豸���ͣ�����
#define		SWITCH_TYPE_SCENARIO			 (0x38 + 0x09) //�豸���ͣ���������
#define 	SWITCH_TYPE_HEATER				 (0x38 + 0x1F) //�豸���ͣ���ˮ��

#define 	SWITCH_TYPE_FORCEDEF			 0 //ǿ�ƶ��忪������,Ӳ����ͬ,д 0 ʱ���Ƿ�ǿ�ƶ���(���ݲ��붨��)

/*��������Ϊ����ʱ����غ궨��*///���˵��
#define 	SOCKETS_SPECIFICATION_AMERICA	0x0A //����
#define 	SOCKETS_SPECIFICATION_BRITISH	0x0B //Ӣ��
#define 	SOCKETS_SPECIFICATION_GENERAL	0x0C //ͨ��
#define 	SOCKETS_SPECIFICATION_SAFRICA	0x0D //�Ϸ�
/*��������Ϊ����ʱ����غ궨��*///�����
#define	SWITCH_TYPE_SOCKETS_SPECIFICATION	SOCKETS_SPECIFICATION_BRITISH  //-------����������

/*���κ괦�� --��ͨ��λ*///����ǿ���豸���Ͷ��忪�� ����Ӧ�ĺ���ж��δ���
#if(SWITCH_TYPE_FORCEDEF == 0)
 #define 	CURTAIN_RELAY_UPSIDE_DOWN		 1 				//�������� ����/�̵��� �Ƿ���
 
/*���κ괦�� --����*/
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)

 //����Ӧ����ϵ���ж�
 #if(SWITCH_TYPE_SOCKETS_SPECIFICATION == SOCKETS_SPECIFICATION_AMERICA)
 
  #define 	COEFFICIENT_POW					4.411065F		//����ϵ�� --L7 ����
  #define	COEFFICIENT_COMPENSATION_POW	0.000001F		//����ϵ������ --L7 ����
 
 #elif(SWITCH_TYPE_SOCKETS_SPECIFICATION == SOCKETS_SPECIFICATION_BRITISH)
 
  #define 	COEFFICIENT_POW					3.396465F		//����ϵ�� --L7 Ӣ��
  #define	COEFFICIENT_COMPENSATION_POW	0.000001F		//����ϵ������ --L7 Ӣ��
 
 #elif(SWITCH_TYPE_SOCKETS_SPECIFICATION == SOCKETS_SPECIFICATION_GENERAL)
 
  #define 	COEFFICIENT_POW					3.780465F		//����ϵ�� --L7 ͨ�� (ͨ�õװ� -20181214) //test L7-HMP
  #define 	COEFFICIENT_COMPENSATION_POW	0.000013F		//����ϵ������ --L7 ͨ�� (ͨ�õװ� -20181214) //test L7-HMP
 
 #elif(SWITCH_TYPE_SOCKETS_SPECIFICATION == SOCKETS_SPECIFICATION_SAFRICA)
 
  #define 	COEFFICIENT_POW					4.287465F		//����ϵ�� --L7 �Ϸ� (�Ϸǵװ� -20181214) //test L7-HMA
  #define 	COEFFICIENT_COMPENSATION_POW	0.000013F		//����ϵ������ --L7 �Ϸ� (�Ϸǵװ� -20181214) //test L7-HMA
 
 #endif

 #if(DEBUG_LOGOUT_EN == 1)
  #undef	DEBUG_LOGOUT_EN
  #define	DEBUG_LOGOUT_EN	0
  #warning	�����豸��֧��log��ӡ����ΪӲ����ͻ���ѽ���Ӧ��ʧ��
 #endif
 
/*���κ괦�� --����*/
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
 #if(DATASAVE_INTLESS_ENABLEIF == 0)
  #undef	DATASAVE_INTLESS_ENABLEIF
  #define	DATASAVE_INTLESS_ENABLEIF	1
  #warning	�����豸��Ҫ���ж���ʽ�̵���״̬�洢������洢��ǰ����ʱ����ɵƹ���˸���ѽ���Ӧ��ʹ��
 #endif
 
/*���κ괦�� --����*/
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_FANS)
 #define	SWITCHFANS_SPECIAL_VERSION_IMPACT	0 //�͵�λ����ر��
 
/*���κ괦�� --����ת����*/
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)
 #if(DATASAVE_INTLESS_ENABLEIF == 1)
  #undef	DATASAVE_INTLESS_ENABLEIF
  #define	DATASAVE_INTLESS_ENABLEIF	0
  #warning	����ת�����豸����Ҫ���ж���ʽ�̵���״̬�洢������ʽ�̵���״̬�洢��ռ�ô���RAM���ѽ���Ӧ��ʧ��
 #endif
#endif

/*���ݴ洢���붨��*/
#define		DATASAVE_MASK_ZIGBNWK_EXIST		 0xF1
#define		DATASAVE_MASK_ZIGBNWK_EXISTNOT	 0xF2

/*�洢���ݺ궨��*///�洢������صĺ궨��
//#define 	ROMADDR_ROM_STC_ID		 		 0x3ff8		//STC��Ƭ�� ȫ��ID��ַ
#define 	ROMADDR_ROM_STC_ID		 		 0x7ff8		//STC��Ƭ�� ȫ��ID��ַ

#define		EEPROM_SECTOR_SIZE				 0x200		//EEPROM ��Ԫ�����ռ��С

#define 	EEPROM_ADDR_START_USRDATA		 0x0000										//����������ʼ������ַ
#define		EEPROM_ADDR_START_STATUSRELAY	 EEPROM_ADDR_START_USRDATA + (0x0200 * 1)	//�̵���״̬������¼��ʼ������ַ
#define		EEPROM_ADDR_START_IRDATA	 	 EEPROM_ADDR_START_USRDATA + (0x0200 * 2)	//IR�������ݴ洢��ʼ��ַ���˵�ַ����IR���ݣ�һ������һ��IR����

#define 	EEPROM_USE_OF_NUMBER 			 0x0080		//�洢��ַ��Χ����
	
#define		BIRTHDAY_FLAG					 0xA1		//��Ʒ�������
		
#define		EEPROM_ADDR_BirthdayMark         0x0001		//01H - 01H �Ƿ��״�����							01_Byte -(�洢��ַ��Χ��0x0000 - EEPROM_USE_OF_NUMBER)
#define  	EEPROM_ADDR_relayStatus          0x0002		//02H - 02H ����״̬�洢							01_Byte -(�洢��ַ��Χ��0x0000 - EEPROM_USE_OF_NUMBER)
#define  	EEPROM_ADDR_timeZone_H           0x0003		//03H - 03H ʱ������ʱ								01_Byte -(�洢��ַ��Χ��0x0000 - EEPROM_USE_OF_NUMBER)
#define  	EEPROM_ADDR_timeZone_M           0x0004		//04H - 04H ʱ��������								01_Byte -(�洢��ַ��Χ��0x0000 - EEPROM_USE_OF_NUMBER)
#define  	EEPROM_ADDR_deviceLockFLAG       0x0005		//05H - 05H �豸��״̬λ							01_Byte -(�洢��ַ��Χ��0x0000 - EEPROM_USE_OF_NUMBER)
#define		EEPROM_ADDR_portCtrlEachOther	 0x0006		//06H - 08H ����λ�󶨶˿�,����Ϊ1��2��3λ			03_Byte -(�洢��ַ��Χ��0x0000 - EEPROM_USE_OF_NUMBER)
#define		EEPROM_ADDR_curtainOrbitalPeriod 0x0009		//09H - 09H ����������ڶ�Ӧʱ��					01_Byte -(�洢��ַ��Χ��0x0000 - EEPROM_USE_OF_NUMBER)
#define		EEPROM_ADDR_curtainOrbitalCnter	 0x000A		//0AH - 0AH ����������ڶ�Ӧλ�ü�ʱֵ				01_Byte -(�洢��ַ��Χ��0x0000 - EEPROM_USE_OF_NUMBER)
#define  	EEPROM_ADDR_swDelayFLAG			 0x000B		//0BH - 0BH ������ʱ��־λ����						01_Byte -(�洢��ַ��Χ��0x0000 - EEPROM_USE_OF_NUMBER)
#define 	EEPROM_ADDR_periodCloseLoop		 0x000C		//0CH - 0CH	ѭ���ر�ʱ����						01_Byte -(�洢��ַ��Χ��0x0000 - EEPROM_USE_OF_NUMBER)
#define		EEPROM_ADDR_zigbNwkExistIF		 0x000D		//0DH - 0DH	zigb�����Ƿ���ڱ��ؼ�¼				01_Byte -(�洢��ַ��Χ��0x0000 - EEPROM_USE_OF_NUMBER)
#define  	EEPROM_ADDR_swTimeTab          	 0x0010		//10H - 28H 8����ͨ��ʱ���ݣ�ÿ��3�ֽ�				24_Byte	-(�洢��ַ��Χ��0x0000 - EEPROM_USE_OF_NUMBER)
#define 	EEPROM_ADDR_TimeTabNightMode	 0x0032		//32H - 37H ҹ��ģʽ��ʱ��							06_Byte -(�洢��ַ��Χ��0x0000 - EEPROM_USE_OF_NUMBER)
#define 	EEPROM_ADDR_ledSWBackGround		 0x0038		//38H - 39H	���ر�����ɫ����						02_Byte -(�洢��ַ��Χ��0x0000 - EEPROM_USE_OF_NUMBER)
#define		EEPROM_ADDR_swScenarioNum		 0x0040		//40H - 4FH �������								31_Byte -(�洢��ַ��Χ��0x0000 - EEPROM_USE_OF_NUMBER)
#define		EEPROM_ADDR_swScenarioAct	     0x0050		//50H - 5FH ������Ӧ����							31_Byte -(�洢��ַ��Χ��0x0000 - EEPROM_USE_OF_NUMBER)
#define		EEPROM_ADDR_mutualCtrlAddrs		 0x0060		//60h - 6FH	�������ڵ�ַ�б�					   <15_byte -(�洢��ַ��Χ��0x0000 - EEPROM_USE_OF_NUMBER) 

//EEPROM��ַ���ö�
#define		EEPROM_ADDR_swTypeForceScenario_scencarioNumKeyBind	0x0070 //70H - 72H ��������ǿ��Ϊ��������ʱ	 	�����󶨳���������				03_Byte -(�洢��ַ��Χ��0x0000 - EEPROM_USE_OF_NUMBER)
#define		EEPROM_ADDR_swTypeForceInfrared_timeUpActNum		0x0070 //70H - 77H ��������ǿ��Ϊ����ת����ʱ	��ʱ���ʱ��Ӧ���͵ĺ���ָ���	08_Byte -(�洢��ַ��Χ��0x0000 - EEPROM_USE_OF_NUMBER)
//--------------------------------------------------------------���������������޶�ֵС�� EEPROM_USE_OF_NUMBER//
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
	u8 agingCmd_infrareOpreat:1;	//ʱЧ_��Ժ���ת�������� -bit5
	u8 agingCmd_scenarioSwOpreat:1;	//ʱЧ_��Գ������ز��� -bit6
	u8 agingCmd_timeZoneReset:1; //ʱЧ_ʱ��У׼���� -bit7
	
	u8 agingCmd_byteReserve[4];	//4�ֽ�ռλ����
	
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
	
	union devClassfication{ //���ݴӴ˴���ʼ����
	
		struct funParam_curtain{ //����
		
			u8 orbital_Period; //�������ʱ��
			
		}curtain_param;
		
		struct funParam_socket{ //����
		
			u8 data_elePower[4]; //��������
			u8 data_eleConsum[3]; //��������
			u8 data_corTime; //��ǰ������ӦСʱ��
			
			u8 dataDebug_powerFreq[4]; //debug����-powerƵ��
			
		}socket_param;
		
		struct funParam_infrared{
		
			u8 opreatAct; //����ָ��
			u8 opreatInsert; //��Ӧ����ң�����
			u8 currentTemperature_integerPrt; //�¶�����-��������
			u8 currentTemperature_decimalPrt; //�¶�����-С������
			u8 currentOpreatReserveNum; //��ǰ������������ж϶�����ֹ�����źŲ���ʱ�ط�
			
			u8 irTimeAct_timeUpNum[8]; //�˶κ��ⶨʱ��Ӧ��ָ���
			
		}infrared_param;
		
		struct funParam_scenarioSw{
		
			u8 scenarioOpreatCmd; //��������
			u8 scenarioKeyBind[3]; //��Ӧ�����󶨵ĳ�����
			
		}scenarioSw_param;
		
	}union_devParam;
	
//	u8	devData_byteReserve[63];
	
}stt_devOpreatDataPonit; //standard_length = 49Bytes + class_extension

/*=======================������������ʱѯ�û���ר�����ݽṹ����������=============================*/

#if(DATASAVE_INTLESS_ENABLEIF)
 #define 	RECORDPERIOD_OPREATION_LOOP		100	//�̵���ʵʱ״̬��¼ ��ѭ�� �洢������Ԫ��������
#endif

#if(DEBUG_LOGOUT_EN == 1)	
 extern u8 xdata log_buf[LOGBUFF_LEN];
#endif		

extern u8 	SWITCH_TYPE;
extern u8 	DEV_actReserve;
extern u8 	CTRLEATHER_PORT[3];
extern u16 	dev_currentPanid;

extern u16 idata mutualCtrlDevList[clusterNum_usr][MUTUALCTRL_DEV_NUM_MAX - 1];

extern unsigned char xdata MAC_ID[6];
extern unsigned char xdata MAC_ID_DST[6];

extern bit deviceLock_flag;
extern bit zigbNwk_exist_FLG;

u8 switchTypeReserve_GET(void);
void MAC_ID_Relaes(void);
void portCtrlEachOther_Reales(void);
void devLockInfo_Reales(void);

bit swScenario_oprateSave(u8 scenarioNum, u8 swAct);
bit swScenario_oprateDele(u8 scenarioNum);
u8 swScenario_oprateCheck(u8 scenarioNum);

void Factory_recover(void);
void birthDay_Judge(void);

void statusSave_zigbNwk_nwkExistIF(bit nwkExistIF);
bit statusGet_zigbNwk_nwkExistIF(void);
void zigbNwkExist_detectReales(void);

void mutualCtrlSysParam_checkAndStore(u8 mutualCtrlGroup_insert, u16 devAddr);
void mutualCtrlSysParam_dataReset(u8 opreatBit);
void mutualCtrlSysParam_dataRecover(void);

void infrared_eeprom_dataSave(u8 insertNum, u8 dats[], u8 datsLen);
void infrared_eeprom_dataRead(u8 insertNum, u8 dats[], u8 datsLen);

void devParamDtaaSave_relayStatusRealTime(u8 currentRelayStatus);
u8 devDataRecovery_relayStatus(void);

#endif