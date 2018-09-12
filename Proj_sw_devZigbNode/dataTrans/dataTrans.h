#ifndef __DATATRANS_H_
#define __DATATRANS_H_

#include "STC15Fxxxx.H"
#include "USART.h"
#include "timerAct.h"

#define zigbPin_RESET				P23

#define ZIGB_FRAME_HEAD 			0xFE

#define ZIGB_UTCTIME_START			946684800UL //zigbeeʱ�����unix��Ԫ946684800<2000/01/01 00:00:00>��ʼ����

#define PERIOD_HEARTBEAT			6  //��������������  ��λ��s
#define PERIOD_SYSTIMEREALES		10 //ϵͳʱ���������  ��λ��s
#define ZIGBNWK_OPNETIME_DEFAULT	12	//Ĭ��zigb���翪��ʱ�� ��λ��s
#define DEVHOLD_TIME_DEFAULT		240 //�豸����Ĭ��ʱ�䣬ʱ�䵽���������� ��λ��s

#define ZIGB_FRAMEHEAD_CTRLLOCAL	0xAA
#define ZIGB_FRAMEHEAD_CTRLREMOTE	0xCC
#define ZIGB_FRAMEHEAD_HEARTBEAT	0xAB
#define ZIGB_FRAMEHEAD_HBOFFLINE	0xBB

#define ZIGB_NWKADDR_CORDINATER		0

//#define FRAME_TYPE_MtoZIGB_CMD			0xA1	/*��������*///�ֻ�������
//#define FRAME_TYPE_ZIGBtoM_RCVsuccess		0x1A	/*��������*///�������ֻ�
#define FRAME_TYPE_MtoS_CMD					0xA0	/*��������*///�ֻ�������
#define FRAME_TYPE_StoM_RCVsuccess			0x0A	/*��������*///�������ֻ�

#define FRAME_HEARTBEAT_cmdOdd				0x22	/*����*///����������
#define FRAME_HEARTBEAT_cmdEven				0x23	/*����*///ż��������

#define FRAME_MtoZIGBCMD_cmdControl			0x10	/*����*///����
#define FRAME_MtoZIGBCMD_cmdConfigSearch	0x39	/*����*///��������
#define FRAME_MtoZIGBCMD_cmdQuery			0x11	/*����*///���ò�ѯ
#define FRAME_MtoZIGBCMD_cmdInterface		0x15	/*����*///���ý���
#define FRAME_MtoZIGBCMD_cmdReset			0x16	/*����*///��λ
#define FRAME_MtoZIGBCMD_cmdDevLockON		0x17	/*����*///�豸����
#define FRAME_MtoZIGBCMD_cmdDevLockOFF		0x18	/*����*///�豸����
#define FRAME_MtoZIGBCMD_cmdswTimQuery		0x19	/*����*///��ͨ���ض�ʱ��ѯ
#define FRAME_MtoZIGBCMD_cmdConfigAP		0x50	/*����*///AP����
#define FRAME_MtoZIGBCMD_cmdBeepsON			0x1A	/*����*///����ʾ��
#define FRAME_MtoZIGBCMD_cmdBeepsOFF		0x1B	/*����*///����ʾ��
#define FRAME_MtoZIGBCMD_cmdftRecoverRQ		0x22	/*����*///��ѯ�Ƿ�֧�ָֻ�����
#define FRAME_MtoZIGBCMD_cmdRecoverFactory	0x1F	/*����*///�ָ�����
#define FRAME_MtoZIGBCMD_cmdCfg_swTim		0x14	/*����*///��ͨ���ض�ʱ����
#define FRAME_MtoZIGBCMD_cmdCfg_ctrlEachO	0x41	/*����*///��ͨ���ػ�������
#define FRAME_MtoZIGBCMD_cmdQue_ctrlEachO	0x42	/*����*///��ͨ���ػ��ز�ѯ
#define FRAME_MtoZIGBCMD_cmdCfg_ledBackSet	0x43	/*����*///��ͨ���ر���������
#define FRAME_MtoZIGBCMD_cmdQue_ledBackSet	0x44	/*����*///��ͨ���ر����Ʋ�ѯ
#define FRAME_MtoZIGBCMD_cmdCfg_scenarioSet	0x45	/*����*///��ͨ���س�������
#define FRAME_MtoZIGBCMD_cmdCfg_scenarioCtl	0x47	/*����*///��ͨ���س�������
#define FRAME_MtoZIGBCMD_cmdCfg_scenarioDel	0x48	/*����*///��ͨ���س���ɾ��

#define	cmdConfigTim_normalSwConfig			0xA0	/*����1*///��ͨ���ض�ʱ��ʶ-����2
#define cmdConfigTim_onoffDelaySwConfig		0xA1	/*����1*///��ʱ���ر�ʶ-����2
#define cmdConfigTim_closeLoopSwConfig		0xA2	/*����1*///ѭ���رձ�ʶ-����2
#define cmdConfigTim_nightModeSwConfig		0xA3	/*����1*///ҹ��ģʽ��ʶ-����2

#define ZIGB_SYSCMD_NWKOPEN	0x68  //zigbϵͳ����ָ���������
#define ZIGB_SYSCMD_TIMESET	0x69  //zigbϵͳ����ָ�ʱ���趨
#define ZIGB_SYSCMD_DEVHOLD	0x6A  //zigbϵͳ����ָ��豸����

#define ZIGB_BAUND	 115200UL   //���ڲ�����->ZIGBģ��ͨѶ

#define rxBuff_WIFI	 RX1_Buffer

#define NORMALDATS_DEFAULT_LENGTH	80 //Ĭ�����ݷ��ͻ��泤��

#define ZIGB_ENDPOINT_CTRLSECENARIO		12 //������Ⱥ����ר�ö˿�
#define ZIGB_ENDPOINT_CTRLNORMAL		13 //��������ת��ר�ö˿�
#define ZIGB_ENDPOINT_CTRLSYSZIGB		14 //zigbϵͳ����ר�ö˿�

#define zigbDatsDefault_GroupID		13
#define zigbDatsDefault_ClustID		13
#define zigbDatsDefault_TransID		13
#define zigbDatsDefault_Option		0
#define zigbDatsDefault_Radius		7

typedef enum{

	status_NULL = 0,
	status_passiveDataRcv, //��������
	status_nwkREQ, //������������������
	status_nwkReconnect, //������������
	status_dataTransRequestDatsSend, //�������ݷ�������
	status_devNwkHold, //�豸�������
}threadRunning_Status;

typedef enum{

	step_standBy = 0,
	step_nwkActive,
	step_hwReset,
	step_complete
}status_Reconnect;

typedef struct{

	u8 zigbNwkSystemNote_IF:1; //�Ƿ���Ҫ���й㲥֪ͨ�������豸����
	u8 devHoldTime_counter; //���𵹼�ʱʱ���趨
}attr_devNwkHold;

typedef struct{

	u8 statusChange_IF:1; //״̬�л�ʹ��
	threadRunning_Status statusChange_standBy; //��Ҫ�л�����״̬
}stt_statusChange;

typedef struct{

	u8 command;
	u8 dats[32];
	u8 datsLen;
}frame_zigbSysCtrl;

#define DATBASE_LENGTH	72
typedef struct{

	u8 dats[DATBASE_LENGTH];
	u8 datsLen;
}datsAttr_datsBase;

typedef struct{

	datsAttr_datsBase datsTrans;
	u8 	portPoint;
	u16	nwkAddr;
}datsAttr_datsTrans;

typedef struct{

	u8 	endpoint;
	u16 devID;
}datsAttr_clusterREG;

typedef struct{

	u8 rcvDats[COM_RX1_Lenth];
	u8 rcvDatsLen;
}uartTout_datsRcv;

typedef struct ZigB_Init_datsAttr{

	u8 	 zigbInit_reqCMD[2];	//����ָ��
	u8 	 zigbInit_reqDAT[64];	//��������
	u8	 reqDAT_num;			//�������ݳ���
	u8 	 zigbInit_REPLY[64];	//��Ӧ����
	u8 	 REPLY_num;				//��Ӧ�����ܳ���
	u16  timeTab_waitAnsr;		//�ȴ���Ӧʱ��
}datsAttr_ZigbInit;

extern attr_devNwkHold	xdata devNwkHoldTime_Param;
extern stt_statusChange xdata devStatus_switch;
extern u8 xdata devNwkHoldTime_counter;

void zigbUart_pinInit(void);
void uartObjZigb_Init(void);
void rxBuff_Zigb_Clr(void);
void uartObjZigb_Send_Byte(u8 dat);
void uartObjZigb_Send_String(char *s,unsigned char ucLength);

void thread_dataTrans(void);
bit zigb_clusterSet(u16 deviveID, u8 endPoint);
bit ZigB_NwkJoin(u16 PANID, u8 CHANNELS);
bit ZigB_nwkOpen(bit openIF, u8 openTime);
bit ZigB_inspectionSelf(void);

bit zigb_VALIDA_INPUT(u8 REQ_CMD[2],		//ָ��
					  u8 REQ_DATS[],		//����
					  u8 REQdatsLen,		//���ݳ���
					  u8 ANSR_frame[],		//��Ӧ֡
					  u8 ANSRdatsLen,		//��Ӧ֡����
					  u8 times,u16 timeDelay);	//ѭ�����������εȴ�ʱ��
						  
void ZigB_datsTX(u16  DstAddr,
				 u8  SrcPoint,
				 u8  DstPoint,
				 u8  ClustID,
				 u8  dats[],
				 u8  datsLen);

void thread_dataTrans(void);

void devStatusChangeTo_devHold(bit zigbNwkSysNote_IF);
void devHoldStop_makeInAdvance(void);

#endif