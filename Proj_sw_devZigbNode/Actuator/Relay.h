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

typedef struct{

	u8 periodBeat_cfm; //�����ڶ�ʱ������ֵȷ��
	u8 periodBeat_counter; //�����ڶ�ʱ������ֵ����
	
	u16 pwm_actEN; //����ʹ�ܣ�ȷ�ϵ������
	u16 pwm_actCounter; //���Ƽ�ʱֵ
}stt_Dimmer_attrFreq;

typedef struct{

	float eleParam_power; //ʵ�ʹ���ֵ
	float eleParamFun_powerPulseCount; //���ʼ���������ֵ
	float eleParamFun_powerFreqVal; //��������Ƶ�ʼ��㻺��ֵ
	
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