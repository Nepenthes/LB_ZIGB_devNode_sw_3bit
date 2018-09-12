#ifndef __TIMERACT_H_
#define __TIMERACT_H_

#include "STC15Fxxxx.H"

#define timCount_ENABLE		0x01
#define timCount_DISABLE	0x00

typedef struct{

	u8 time_Year;
	u8 time_Month;
	u8 time_Week;
	u8 time_Day;
	u8 time_Hour;
	u8 time_Minute;
	u8 time_Second;
}stt_Time;

typedef struct{

	u8 		  Week_Num	:7;	//��ֵռλ
	u8		  if_Timing	:1;	//�Ƿ񿪶�ʱ
	u8		  Status_Act:3;	//��ʱʱ�̿�����Ҫ��Ӧ��״̬
	u8		  Hour		:5;	//ʱ�̡���Сʱ
	u8		  Minute;		//ʱ�̡�����
}timing_Dats;

extern u8 		xdata sysTimeReales_counter;
extern stt_Time xdata systemTime_current;
extern u8 		xdata sysTimeZone_H;
extern u8 		xdata sysTimeZone_M;
extern u16		idata sysTimeKeep_counter;

extern u8 		idata ifDelay_sw_running_FLAG;	//��ʱ����_�Ƿ����б�־λ��bit 1��ʱ��������ʹ�ܱ�־��bit 0��ʱ�ر�����ʹ�ܱ�־��
extern bit		idata ifNightMode_sw_running_FLAG;
extern u16		idata delayCnt_onoff;
extern u8		idata delayPeriod_onoff;
extern u8		idata delayUp_act;
extern u16		idata delayCnt_closeLoop;
extern u8		idata delayPeriod_closeLoop;

extern u8 		idata swTim_onShoot_FLAG;	
extern bit		idata ifTim_sw_running_FLAG;

void timeZone_Reales(void);
void thread_Timing(void);

#endif
