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

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_HEATER)
	#define HEATER_RELAY_SYNCHRONIZATION_DELAYTIME	500	//小电流继电器滞后同步时间，单位：ms
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SCENARIO)	
	#define SCENARIOTRIG_KEEPTIME_PERIOD			3 //不同的场景触发，强制间隔时间，单位：s
#else
	#define CURTAIN_ORBITAL_PERIOD_INITTIME			0 //窗帘轨道时间初始值，单位：s	
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

typedef struct{ //调光逻辑业务数据结构

	u8 periodBeat_cfm; //单周期定时器节拍值确认
	u8 periodBeat_counter; //单周期定时器节拍值更新
	
	u16 pwm_actEN; //调制使能，确认调制起点
	u16 pwm_actCounter; //调制计时值
}stt_Dimmer_attrFreq;

typedef struct{ //插座逻辑业务数据结构

	float eleParam_power; //实际功率值
	float eleParamFun_powerPulseCount; //功率检测脉冲计数值
	float eleParamFun_powerFreqVal; //功率脉冲频率计算缓冲值
	
	float ele_Consum;
	
}stt_eleSocket_attrFreq;

typedef enum{ //热水器逻辑业务数据结构 --子枚举

	heaterActMode_swClose = 0, //关闭
	heaterActMode_swKeepOpen, //常开
	heaterActMode_swCloseDelay30min, //延时30min后关闭
	heaterActMode_swCloseDelay60min //延时60min后关闭
}enumDevHeater_ActMode;

typedef struct{ //热水器逻辑业务数据结构
	
	enumDevHeater_ActMode heater_currentActMode; //当前开关所在模式状态

	u16 touchAction_defineJustSwitch_IF:1; //是否将当前开关动作直接定义为1位开关
	
	u16 relayActDelay_actEn:1; //滞后动作继电器实际动作使能，置位则触发两个继电器进行电平同步动作
	u16 relayActDelay_counter:14; //两个继电器异步动作延时计时变量，小电流继电器滞后大电流继电器进行动作延时计时
	
	u16 timerClose_counter; //自动关闭计时变量
	
}stt_heater_attrAct;

typedef struct{ //场景开关逻辑业务数据结构

	enum{
	
		scenarioKey_current_null = 0, //当前无场景按键触发
		scenarioKey_current_S1,	//当前场景按键1已被触发
		scenarioKey_current_S2,	//当前场景按键2已被触发
		scenarioKey_current_S3,	//当前场景按键3已被触发
		
	}scenarioKey_currentTrig;
	
	u8 scenarioKeepTrig_timeCounter; //单次场景触发耗时计时变量		单位：s
	u8 scenarioNum_record[3]; //三个按键对应的场景号
	
	u8 scenarioDataSend_FLG:1; //场景数据发送标志

}stt_scenario_attrAct;

extern status_ifSave xdata relayStatus_ifSave;
extern u8 xdata status_Relay;
extern stt_Curtain_motorAttr xdata curtainAct_Param;
extern bit idata specialFlg_curtainEachctrlEn;
extern relay_Command xdata swCommand_fromUsr;
extern u8 xdata EACHCTRL_realesFLG;
extern bit		EACHCTRL_reportFLG;
extern relayStatus_PUSH xdata devActionPush_IF;
extern bit				idata statusRelay_saveEn;

extern stt_Dimmer_attrFreq xdata dimmer_freqParam;
extern stt_eleSocket_attrFreq xdata socket_eleDetParam;
extern float xdata pinFP_stdby_powerCNT;
extern float xdata pinFP_powerStdby;
extern stt_scenario_attrAct xdata scenario_ActParam;
extern stt_heater_attrAct xdata heater_ActParam;

void relay_pinInit(void);
void relay_Act(relay_Command dats);
void thread_Relay(void);

#endif