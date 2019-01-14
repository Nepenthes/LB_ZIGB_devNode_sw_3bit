#ifndef __TIPS_H_
#define __TIPS_H_

#include "STC15Fxxxx.H"

#include "dataManage.h"

#define COLORGRAY_MAX 	32 //32���Ҷ�

#define TIPS_SWFREELOOP_TIME	60 //���ش����ͷ�ʱ�����ж��� ��λ��s

#define TIPS_SWBKCOLOR_TYPENUM	10 //ɫֵ����Ŀ

#define TIPSBKCOLOR_DEFAULT_ON	8  //Ĭ�Ͽ��ش�������ɫ������
#define TIPSBKCOLOR_DEFAULT_OFF 5  //Ĭ�Ͽ��ش�������ɫ���ر�

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)
 #define PIN_BEEP			P20
#else
 #define PIN_BEEP			P32
#endif

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
 #define BEEP_OPEN_LEVEL		1 //������������ƽ
#else
 #define BEEP_OPEN_LEVEL		0 //������������ƽ
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
	
		u8 fans_BKlightColorInsert_gear0:4; //0��ɫ bit0 - bit3
		u8 fans_BKlightColorInsert_gear1:4; //1��ɫ bit4 - bit7
		u8 fans_BKlightColorInsert_gear2:4; //2��ɫ bit0 - bit3
		u8 fans_BKlightColorInsert_gear3:4; //3��ɫ bit4 - bit7
	}fans_BKlight_Param;
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
	struct{
	
		u8 dimmer_BKlightColorInsert_press:4; //����ʱ��ɫ
		u8 dimmer_BKlightColorInsert_brightness0:4; //����Ϊ0ʱɫ
		u8 dimmer_BKlightColorInsert_brightness1to99:4; //����Ϊ1-99ʱɫ
		u8 dimmer_BKlightColorInsert_brightness100:4; //����Ϊ100ʱɫ
	}dimmer_BKlight_Param;
	
//#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS) //�����⣬��Ĭ��
//#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED) //�����⣬��Ĭ��
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SCENARIO)
	struct{
	
		u8 scenario_BKlightColorInsert_trig; //�Ѵ�����Ӧ��λ����ɫ
		u8 scenario_BKlightColorInsert_notrig; //δ������Ӧ��λ����ɫ
	}scenario_BKlight_Param;
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_HEATER)
	struct{
	
		u8 heater_BKlightColorInsert_open:4; //��ɫ
		u8 heater_BKlightColorInsert_closeDelay30:4; //��ʱ30���ӹ�ɫ
		u8 heater_BKlightColorInsert_closeDelay60:4; //��ʱ60���ӹ�ɫ
		u8 heater_BKlightColorInsert_close:4; //��ɫ
	}heater_BKlight_Param;
	
#else
	union{
	
		struct{
		
			u8 sw3bit_BKlightColorInsert_open; //��ɫ
			u8 sw3bit_BKlightColorInsert_close; //��ɫ
		}sw3bit_BKlight_Param;
		
		struct{
		
			u8 cuertain_BKlightColorInsert_press; //����ɫ
			u8 cuertain_BKlightColorInsert_bounce; //����ɫ
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
	status_devHold, //�豸��λ������𣬵ȴ����ظ���
}tips_Status;

typedef enum{

	nwkZigb_Normal = 0, //����
	nwkZigb_outLine, //�����쳣�����Ź�
	nwkZigb_reConfig,//���ø���/��������
	nwkZigb_nwkREQ, //������������
	nwkZigb_nwkOpen, //������������
	nwkZigb_hold, //�������
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