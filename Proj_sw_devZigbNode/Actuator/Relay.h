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
	#define HEATER_RELAY_SYNCHRONIZATION_DELAYTIME	500	//С�����̵����ͺ�ͬ��ʱ�䣬��λ��ms
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SCENARIO)	
	#define SCENARIOTRIG_KEEPTIME_PERIOD			3 //��ͬ�ĳ���������ǿ�Ƽ��ʱ�䣬��λ��s
#else
	#define CURTAIN_ORBITAL_PERIOD_INITTIME			0 //�������ʱ���ʼֵ����λ��s	
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

	u8 push_IF:1; //����ʹ��
	u8 dats_Push; //��������-����λ��ʾ����λ������λ��ʾ����״̬
}relayStatus_PUSH;

typedef struct{ //������Ӧ������ݽṹ

	u8 act_counter;
	u8 act_period;
	
	enum{
	
		cTact_stop = 0,
		cTact_close,
		cTact_open,
	}act;

}stt_Curtain_motorAttr;

typedef struct{ //�����߼�ҵ�����ݽṹ

	u8 periodBeat_cfm; //�����ڶ�ʱ������ֵȷ��
	u8 periodBeat_counter; //�����ڶ�ʱ������ֵ����
	
	u16 pwm_actEN; //����ʹ�ܣ�ȷ�ϵ������
	u16 pwm_actCounter; //���Ƽ�ʱֵ
}stt_Dimmer_attrFreq;

typedef struct{ //�����߼�ҵ�����ݽṹ

	float eleParam_power; //ʵ�ʹ���ֵ
	float eleParamFun_powerPulseCount; //���ʼ���������ֵ
	float eleParamFun_powerFreqVal; //��������Ƶ�ʼ��㻺��ֵ
	
	float ele_Consum;
	
}stt_eleSocket_attrFreq;

typedef enum{ //��ˮ���߼�ҵ�����ݽṹ --��ö��

	heaterActMode_swClose = 0, //�ر�
	heaterActMode_swKeepOpen, //����
	heaterActMode_swCloseDelay30min, //��ʱ30min��ر�
	heaterActMode_swCloseDelay60min //��ʱ60min��ر�
}enumDevHeater_ActMode;

typedef struct{ //��ˮ���߼�ҵ�����ݽṹ
	
	enumDevHeater_ActMode heater_currentActMode; //��ǰ��������ģʽ״̬

	u16 touchAction_defineJustSwitch_IF:1; //�Ƿ񽫵�ǰ���ض���ֱ�Ӷ���Ϊ1λ����
	
	u16 relayActDelay_actEn:1; //�ͺ����̵���ʵ�ʶ���ʹ�ܣ���λ�򴥷������̵������е�ƽͬ������
	u16 relayActDelay_counter:14; //�����̵����첽������ʱ��ʱ������С�����̵����ͺ������̵������ж�����ʱ��ʱ
	
	u16 timerClose_counter; //�Զ��رռ�ʱ����
	
}stt_heater_attrAct;

typedef struct{ //���������߼�ҵ�����ݽṹ

	enum{
	
		scenarioKey_current_null = 0, //��ǰ�޳�����������
		scenarioKey_current_S1,	//��ǰ��������1�ѱ�����
		scenarioKey_current_S2,	//��ǰ��������2�ѱ�����
		scenarioKey_current_S3,	//��ǰ��������3�ѱ�����
		
	}scenarioKey_currentTrig;
	
	u8 scenarioKeepTrig_timeCounter; //���γ���������ʱ��ʱ����		��λ��s
	u8 scenarioNum_record[3]; //����������Ӧ�ĳ�����
	
	u8 scenarioDataSend_FLG:1; //�������ݷ��ͱ�־

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