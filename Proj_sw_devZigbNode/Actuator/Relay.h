#ifndef __RELAY_H_
#define __RELAY_H_

#include "STC15Fxxxx.H"

#include "dataManage.h"

#define actRelay_ON		1
#define actRelay_OFF	0

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
	#define PIN_RELAY_1		P33
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
	#define PIN_INT_IN		P33
	#define PIN_PWM_OUT		P35
	#define PIN_RELAY_2		P34
#else
	#define PIN_RELAY_1		P33
	#define PIN_RELAY_2		P34
	#define PIN_RELAY_3		P35
#endif

typedef enum{

	statusSave_enable = 1,
	statusSave_disable,
}status_ifSave;

typedef enum{

	relay_flip = 1,
	relay_OnOff,
	actionNull,
}rly_methodType;

typedef struct{

	u8 objRelay;
	rly_methodType actMethod;
}relay_Command;

typedef struct{

	u8 push_IF:1; //推送使能
	u8 dats_Push; //推送数据-高三位表示动作位，低三位表示开关状态
}relayStatus_PUSH;

typedef struct{ //窗帘对应电机数据结构

	u8 act_counter;
	u8 act_period;
	
	enum{
	
		cTact_stop = 0,
		cTact_close,
		cTact_open,
	}act;

}stt_Curtain_motorAttr;

typedef struct{

	u8 periodBeat_cfm; //单周期定时器节拍值确认
	u8 periodBeat_counter; //单周期定时器节拍值更新
	
	u16 pwm_actEN; //调制使能，确认调制起点
	u16 pwm_actCounter; //调制计时值
}stt_Dimmer_attrFreq;

typedef struct{

	float eleParam_power; //实际功率值
	float eleParamFun_powerPulseCount; //功率检测脉冲计数值
	float eleParamFun_powerFreqVal; //功率脉冲频率计算缓冲值
	
	float ele_Consum;
	
}stt_eleSocket_attrFreq;

extern status_ifSave xdata relayStatus_ifSave;
extern u8 xdata status_Relay;
extern stt_Curtain_motorAttr xdata curtainAct_Param;
extern relay_Command xdata swCommand_fromUsr;
extern u8 xdata EACHCTRL_realesFLG;
extern bit		EACHCTRL_reportFLG;
extern relayStatus_PUSH xdata devActionPush_IF;
extern bit				idata statusRelay_saveEn;

extern stt_Dimmer_attrFreq xdata dimmer_freqParam;
extern stt_eleSocket_attrFreq xdata socket_eleDetParam;

void relay_pinInit(void);
void relay_Act(relay_Command dats);
void thread_Relay(void);

#endif