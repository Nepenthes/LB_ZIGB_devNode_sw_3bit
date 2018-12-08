#ifndef __TIPS_H_
#define __TIPS_H_

#include "STC15Fxxxx.H"

#define PIN_BEEP	P32

#define COLORGRAY_MAX 	32 //32级灰度

#define TIPS_SWFREELOOP_TIME	60 //开关触摸释放时长空闲定义 单位：s

#define TIPS_SWBKCOLOR_TYPENUM	10 //色值表数目

#define TIPSBKCOLOR_DEFAULT_ON	8  //默认开关触摸背景色：开启
#define TIPSBKCOLOR_DEFAULT_OFF 5  //默认开关触摸背景色：关闭

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

typedef struct{

	u8 colorGray_R:5;
	u8 colorGray_G:5;
	u8 colorGray_B:5;
}color_Attr;

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

extern u8 tipsInsert_swLedBKG_ON;
extern u8 tipsInsert_swLedBKG_OFF;
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