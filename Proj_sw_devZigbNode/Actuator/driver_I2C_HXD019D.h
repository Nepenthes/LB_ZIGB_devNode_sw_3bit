#ifndef __DRIVER_IIC_HXD019D_H_
#define __DRIVER_IIC_HXD019D_H_

#include "STC15Fxxxx.H"

#include "dataManage.h"

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)

 #define hxd019d_pinBUSY		P34
 #define hxd019d_pinREST		P35
 #define hxd019d_pinSDA			P32
 #define hxd019d_pinSCL			P36
 
 #define hxd019d_resetLevel		0

 #define SetSDAInput()	{P3M1 |=  0x04;	P3M0 &= ~0x04;}
 #define SetSDAOutput()	{P3M1 &= ~0x04;	P3M0 |=  0x04;}
 #define SetSDAHigh()	hxd019d_pinSDA = 1
 #define SetSDALow()	hxd019d_pinSDA = 0
 #define GetDINStatus()	hxd019d_pinSDA

 #define SetSCLOutput()	{P3M1 &= ~0x40;	P3M0 |=  0x40;}
 #define SetSCLHigh()	hxd019d_pinSCL = 1
 #define SetSCLLow()	hxd019d_pinSCL = 0
 
 #define IR_opStatusLearnningSTBY_TOUT			2000	//����ת����ѧϰ����̬ ���γ�ʱʱ��
 #define IR_opStatusLearnningSTBY_TOUTLOOP		3		//����ת����ѧϰ����̬ ��ʱ�����޶�
 #define IR_opStatusLearnning_TOUT				20000	//����ת����ѧϰ̬ ��ʱʱ��
 #define IR_opStatusSigSendSTBY_TOUT			300		//����ת�������ƾ���̬ �ȴ�ʱ��
 #define IR_resetOpreatTimeKeep					200		//����ת����оƬ��λ�͵�ƽ����ʱ��
 
 #define IR_OPREATCMD_CONTROL					0x20	//�����·�ָ���ʼ����
 #define IR_OPREATCMD_LEARNNING					0x21	//�����·�ָ���ʼѧϰ
 #define IR_OPREATRES_CONTROL					0x87	//���������������Ƴɹ�������ʧ�ܲ����룩
 #define IR_OPREATRES_LEARNNING					0x86	//������������ѧϰ�ɹ���ѧϰʧ�ܲ����룩
 
 typedef enum{

	infraredSMStatus_null = 0, //null
	infraredSMStatus_free, //����̬
	infraredSMStatus_learnningSTBY, //�ź�ѧϰ����̬
	infraredSMStatus_learnning, //�ź�ѧϰ̬
	infraredSMStatus_sigSendSTBY, //�źŷ��;���̬
	infraredSMStatus_sigSend, //�źŷ���̬
	infraredSMStatus_opStop //������ֹ
}enumInfrared_status;
 
 extern u8 IR_currentOpreatRes;
 extern u8 IR_currentOpreatinsert;

 extern u16 xdata infraredAct_timeCounter;

 void infrared_pinInit(void);
 
 void thread_infraredSM(void);
 enumInfrared_status infraredStatus_GET(void);

 void infraredOpreatAct_Stop(void);
 void infraredOpreatAct_learnningStart(u8 opInsert);
 void infraredOpreatAct_remoteControlStart(u8 opInsert);

#endif

#endif