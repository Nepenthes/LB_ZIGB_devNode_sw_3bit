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

	u8 		  Week_Num	:7;	//周值占位
	u8		  if_Timing	:1;	//是否开定时
	u8		  Status_Act:3;	//定时时刻开关需要响应的状态
	u8		  Hour		:5;	//时刻——小时
	u8		  Minute;		//时刻——分
}timing_Dats;

extern u8 		xdata sysTimeReales_counter;
extern stt_Time xdata systemTime_current;
extern u8 		xdata sysTimeZone_H;
extern u8 		xdata sysTimeZone_M;
extern u16		idata sysTimeKeep_counter;

extern u8 		idata ifDelay_sw_running_FLAG;	//延时动作_是否运行标志位（bit 1延时开关运行使能标志，bit 0定时关闭运行使能标志）
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
