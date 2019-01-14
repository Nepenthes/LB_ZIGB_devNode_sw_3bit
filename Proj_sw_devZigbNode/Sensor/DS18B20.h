#ifndef __DRIVER_DS18B20_H_
#define __DRIVER_DS18B20_H_

#include "STC15Fxxxx.H"

#include "dataManage.h"

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED) //����ת��������ds18b20Ӳ��

 #define DSPORT							P24
 
 #define DS18B20_TEMPRATRUE_DTPERIOD	5		//ds18b20�ȶ���ȡ����	

 void ds18b20_pinInit(void);
 u16 Ds18b20ReadTemp(void);
 
 extern u16 temperatureCurrent_VAL;
 
 extern u8 couter_ds18b20Temperature_dtPeriod;

#endif

#endif