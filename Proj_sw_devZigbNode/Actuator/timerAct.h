#ifndef __TIMERACT_H_
#define __TIMERACT_H_

#include "STC15Fxxxx.H"

#define timCount_ENABLE		0x01
#define timCount_DISABLE	0x00

#define TIMEER_TABLENGTH	8 //��ʱʱ������

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

extern bit		idata ifNightMode_sw_running_FLAG;

extern u8 		idata ifDelay_sw_running_FLAG;	//��ʱ����_�Ƿ����б�־λ��bit 1��ʱ��������ʹ�ܱ�־��bit 0��ʱ�ر�����ʹ�ܱ�־��
extern u16		idata delayCnt_onoff;
extern u8		idata delayPeriod_onoff;
extern u8		idata delayUp_act;
extern u16		idata delayCnt_closeLoop;
extern u8		idata delayPeriod_closeLoop;

extern u8 		idata swTim_oneShoot_FLAG;	
extern bit		idata ifTim_sw_running_FLAG;

void timeZone_Reales(void);
void thread_Timing(void);

void timerActionDone_FLG_RESET(void);

void datsTimNight_read_eeprom(timing_Dats timDats_tab[2]);
void datsTiming_read_eeprom(timing_Dats timDats_tab[TIMEER_TABLENGTH]);
void itrf_datsTiming_read_eeprom(void);
void itrf_datsTimNight_read_eeprom(void);

#endif
