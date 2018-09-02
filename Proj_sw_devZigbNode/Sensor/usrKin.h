#ifndef __USRKIN_H_
#define __USRKIN_H_

#include "STC15Fxxxx.H"

#define	timeDef_touchPressCfm		20		//短按消抖时间 <单位：ms>
#define	timeDef_touchPressLongA		3000	//长按A时间 <单位：ms>
#define	timeDef_touchPressLongB		10000	//长按B时间 <单位：ms>

#define	timeDef_touchPressContinue	350		//连按间隔定义时间 <单位：ms>

sbit Dcode0		= P1^0;
sbit Dcode1 	= P1^1;
sbit Dcode2 	= P1^2;
sbit Dcode3 	= P1^3;//暂缺保留
sbit Dcode4 	= P1^3;
sbit Dcode5 	= P1^4;

sbit Usr_key 	= P0^2;

sbit touchPad_1 = P1^6;
sbit touchPad_2 = P1^7;
sbit touchPad_3 = P1^5;

#define Dcode_FLG_ifAP			0x01
#define Dcode_FLG_ifLED			0x02
#define Dcode_FLG_ifMemory		0x04
#define Dcode_FLG_bitReserve	0x30			

#define Dcode_bitReserve(x)		((x & 0x30) >> 4)

typedef void *funKey_Callback(void);

typedef enum{

	press_Null = 1,
	press_Short,
	press_ShortCnt, //带连按的短按
	press_LongA,
	press_LongB,
}keyCfrm_Type;

void usrKin_pinInit(void);

bit UsrKEYScan_oneShoot(void);
u8 DcodeScan_oneShoot(void);

void DcodeScan(void);
void UsrKEYScan(funKey_Callback funCB_Short, funKey_Callback funCB_LongA, funKey_Callback funCB_LongB);
void touchPad_Scan(void);

void fun_Test(void);
void usrKeyFun_zigbNwkRejoin(void);

#endif