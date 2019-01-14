#ifndef __TIPS_H_
#define __TIPS_H_

#include "STC15Fxxxx.H"

#include "dataManage.h"

#define COLORGRAY_MAX 	32 //32级灰度

#define TIPS_SWFREELOOP_TIME	60 //开关触摸释放时长空闲定义 单位：s

#define TIPS_SWBKCOLOR_TYPENUM	10 //色值表数目

#define TIPSBKCOLOR_DEFAULT_ON	8  //默认开关触摸背景色：开启
#define TIPSBKCOLOR_DEFAULT_OFF 5  //默认开关触摸背景色：关闭

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)
 #define PIN_BEEP			P20
#else
 #define PIN_BEEP			P32
#endif

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
 #define BEEP_OPEN_LEVEL		1 //蜂鸣器开启电平
#else
 #define BEEP_OPEN_LEVEL		0 //蜂鸣器开启电平
#endif

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)	
	#define PIN_TIPS_RELAY1_R 	P25
	#define PIN_TIPS_RELAY1_B 	P54
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)
	#define PIN_TIPS_RELAY1_R 	P23
	#define PIN_TIPS_RELAY1_B 	P22
	
#else
	#define PIN_TIPS_RELAY1_R 	P20
	#define PIN_TIPS_RELAY2_R 	P26
	#define PIN_TIPS_RELAY3_R 	P01
	#define PIN_TIPS_ZIGBNWK_R 	P55

	#define PIN_TIPS_RELAY1_G 	P21
	#define PIN_TIPS_RELAY2_G 	P25
	#define PIN_TIPS_RELAY3_G 	P00
	#define PIN_TIPS_ZIGBNWK_G 	P54

	#define PIN_TIPS_RELAY1_B 	P22
	#define PIN_TIPS_RELAY2_B 	P24
	#define PIN_TIPS_RELAY3_B 	P27
	#define PIN_TIPS_ZIGBNWK_B 	P03
	
#endif

typedef struct{

	u8 colorGray_R:5;
	u8 colorGray_G:5;
	u8 colorGray_B:5;
}color_Attr;

typedef union{

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_FANS)
	struct{
	
		u8 fans_BKlightColorInsert_gear0:4; //0档色 bit0 - bit3
		u8 fans_BKlightColorInsert_gear1:4; //1档色 bit4 - bit7
		u8 fans_BKlightColorInsert_gear2:4; //2档色 bit0 - bit3
		u8 fans_BKlightColorInsert_gear3:4; //3档色 bit4 - bit7
	}fans_BKlight_Param;
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
	struct{
	
		u8 dimmer_BKlightColorInsert_press:4; //按下时颜色
		u8 dimmer_BKlightColorInsert_brightness0:4; //亮度为0时色
		u8 dimmer_BKlightColorInsert_brightness1to99:4; //亮度为1-99时色
		u8 dimmer_BKlightColorInsert_brightness100:4; //亮度为100时色
	}dimmer_BKlight_Param;
	
//#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS) //无特殊，随默认
//#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED) //无特殊，随默认
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SCENARIO)
	struct{
	
		u8 scenario_BKlightColorInsert_trig; //已触发对应键位背景色
		u8 scenario_BKlightColorInsert_notrig; //未触发对应键位背景色
	}scenario_BKlight_Param;
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_HEATER)
	struct{
	
		u8 heater_BKlightColorInsert_open:4; //开色
		u8 heater_BKlightColorInsert_closeDelay30:4; //延时30分钟关色
		u8 heater_BKlightColorInsert_closeDelay60:4; //延时60分钟关色
		u8 heater_BKlightColorInsert_close:4; //关色
	}heater_BKlight_Param;
	
#else
	union{
	
		struct{
		
			u8 sw3bit_BKlightColorInsert_open; //开色
			u8 sw3bit_BKlightColorInsert_close; //关色
		}sw3bit_BKlight_Param;
		
		struct{
		
			u8 cuertain_BKlightColorInsert_press; //按下色
			u8 cuertain_BKlightColorInsert_bounce; //弹起色
		}cuertain_BKlight_Param;
	}sw3bitIcurtain_BKlight_Param;

#endif
	
}bkLightColorInsert_paramAttr;

typedef enum{

	obj_Relay1 = 0,
	obj_Relay2,
	obj_Relay3,
	obj_zigbNwk,
}tipsLED_Obj;

typedef enum{

	status_Null = 0,
	status_sysStandBy,
	status_keyFree,
	status_Normal,
	status_Night,
	status_tipsNwkOpen,
	status_tipsNwkFind,
	status_touchReset,
	status_devHold, //设备复位网络挂起，等待网关更换
}tips_Status;

typedef enum{

	nwkZigb_Normal = 0, //正常
	nwkZigb_outLine, //网络异常，或遗孤
	nwkZigb_reConfig,//配置更新/重新入网
	nwkZigb_nwkREQ, //主动请求网络
	nwkZigb_nwkOpen, //主动开放网络
	nwkZigb_hold, //网络挂起
}tips_nwkZigbStatus;

typedef struct{

	u8 tips_Period:3;
	u8 tips_time;
	u8 tips_loop:5;
}sound_Attr;

typedef enum beepsMode{

	beepsMode_null = 0,
	beepsMode_standBy,
	beepsWorking,
	beepsComplete,
}enum_beeps;

extern color_Attr xdata relay1_Tips;
extern color_Attr xdata relay2_Tips;
extern color_Attr xdata relay3_Tips;
extern color_Attr xdata zigbNwk_Tips;

extern tips_Status devTips_status;

extern bkLightColorInsert_paramAttr xdata devBackgroundLight_param;
extern color_Attr code color_Tab[10];

extern bit	idata ifHorsingLight_running_FLAG;

extern u8 xdata counter_ifTipsFree;
extern bit idata countEN_ifTipsFree;
extern u8 xdata tipsTimeCount_zigNwkOpen;
extern u8 xdata tipsTimeCount_touchReset;
extern u8 xdata tipsTimeCount_factoryRecover;

extern enum_beeps xdata dev_statusBeeps;
extern sound_Attr xdata devTips_beep;

extern tips_Status devTips_status;
extern tips_nwkZigbStatus devTips_nwkZigb;

void tips_statusChangeToNormal(void);
void tips_statusChangeToZigbNwkOpen(u8 timeopen);
void tips_statusChangeToZigbNwkFind(void);
void tips_statusChangeToTouchReset(u8 timeHold);
void tips_statusChangeToFactoryRecover(u8 timeHold);

void beeps_usrActive(u8 tons, u8 time, u8 loop);

void tipLED_pinInit(void);
void pinBeep_Init(void);
void ledBKGColorSw_Reales(void);
void tipsLED_colorSet(tipsLED_Obj obj, u8 gray_R, u8 gray_G, u8 gray_B);

void thread_tipsGetDark(u8 funSet);

void tips_warning(void);
void tips_sysTouchReset(void);
void tips_fadeOut(void);
void tips_specified(u8 tips_Type);
void tips_breath(void);
void tips_sysButtonReales(void);
void tips_sysTouchReset(void);
void tips_sysStandBy(void);

void thread_Tips(void);

#endif