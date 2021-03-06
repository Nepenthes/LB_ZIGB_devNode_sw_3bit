#ifndef __USRKIN_H_
#define __USRKIN_H_

#include "STC15Fxxxx.H"

#include "dataManage.h"
#include "dataTrans.h"

#define KEYPRESS_CONTINE_RESERVE_NUMMAX		10		//连按有效次数

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)	
 #define	timeDef_touchPressCfm		20		//短按消抖时间 <单位：ms>
 #define	timeDef_touchPressLongA		1500	//长按A时间 <单位：ms> ,dimmer触发连续调制
 #define	timeDef_touchPressLongB		20000	//长按B时间 <单位：ms>	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)
 #define	timeDef_touchPressCfm		20		//短按消抖时间 <单位：ms>
 #define	timeDef_touchPressLongA		3000	//长按A时间 <单位：ms>
 #define	timeDef_touchPressLongB		20000	//长按B时间 <单位：ms>	
#else
 #define	timeDef_touchPressCfm		20		//短按消抖时间 <单位：ms>
 #define	timeDef_touchPressLongA		3000	//长按A时间 <单位：ms>
 #define	timeDef_touchPressLongB		20000	//长按B时间 <单位：ms>	
#endif

#define	timeDef_touchPressContinue	350		//连按间隔定义时间 <单位：ms>

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)	
	sbit Usr_key 	= P2^4;
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)
	sbit Usr_key 	= P5^4;
#else
	sbit Dcode0		= P1^0;
	sbit Dcode1 	= P1^1;
	sbit Dcode2 	= P1^2;
	sbit Dcode3 	= P1^3;//暂缺保留
	sbit Dcode4 	= P1^4;
	sbit Dcode5 	= P1^3;

	sbit Usr_key 	= P0^2;

	sbit touchPad_1 = P1^6;
	sbit touchPad_2 = P1^7;
	sbit touchPad_3 = P1^5;
#endif

#define Dcode_FLG_ifAP			0x01
#define Dcode_FLG_ifLED			0x02
#define Dcode_FLG_ifMemory		0x04
#define Dcode_FLG_bitReserve	0x30			

#define Dcode_bitReserve(x)		((x & 0x30) >> 4)

typedef const void (*funKey_Callback)(void);

typedef enum{

	press_Null = 1,
	press_Short,
	press_ShortCnt, //带连按的短按
	press_LongA,
	press_LongB,
}keyCfrm_Type;

typedef struct{

	u8 param_combinationFunPreTrig_standBy_FLG:1; //预触发标志
	u8 param_combinationFunPreTrig_standBy_keyVal:7; //预触发按键键值缓存
}param_combinationFunPreTrig;

///*回调注册函数形参类型*///为减少代码冗余，此段弃用
//typedef enum{

//	kinObj_touch_1 = 0,
//	kinObj_touch_2,
//	kinObj_touch_3,
//	kinObj_button,
//}objKey;

//typedef enum{

//	method_pressShort = 0,
//	method_pressCnt,
//	method_pressLong_A,
//	method_pressLong_B,
//}trig_Method;

//typedef struct{

//	funKey_Callback press_Short; //短按
//	funKey_Callback press_Continue[KEYPRESS_CONTINE_RESERVE_NUMMAX];  //连按
//	funKey_Callback press_Long_A; //长按 A
//	funKey_Callback press_Long_B; //长按 B
//}fun_KeyTrigger;

void usrKin_pinInit(void);

bit UsrKEYScan_oneShoot(void);
u8 DcodeScan_oneShoot(void);

u8 touchPadScan_oneShoot(void);
void DcodeScan(void);
void UsrKEYScan(funKey_Callback funCB_Short, funKey_Callback funCB_LongA, funKey_Callback funCB_LongB);
void touchPad_Scan(void);

void fun_Test(void);
void usrKeyFun_relayOpreation(void);
void usrKeyFun_zigbNwkRejoin(void);
void fun_touchReset(void);
void fun_factoryRecoverOpreat(void);

void fun_Test_short(void);
void fun_Test_longA(void);

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_FANS)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SCENARIO)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_HEATER)
 void devHeater_actOpeartionExecute(enumDevHeater_ActMode opreatParam);
#else
#endif

#endif