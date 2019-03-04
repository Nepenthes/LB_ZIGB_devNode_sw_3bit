#include "Relay.h"

#include "string.h"
#include "stdio.h"

#include "Tips.h"
#include "timerAct.h"
#include "appTimer.h"
#include "dataTrans.h"
#include "dataManage.h"

#include "eeprom.h"

/**********************�����ļ�����������*****************************/
status_ifSave	xdata relayStatus_ifSave = statusSave_disable;	//���ؼ���ʹ�ܱ���
u8 				xdata status_Relay 		 = 0;

relay_Command	xdata swCommand_fromUsr	 = {0, actionNull};
  
u8				xdata EACHCTRL_realesFLG = 0; //���ض�������ʹ�ܱ�־�����룩��־<bit0��һλ���ػ��ظ���; bit1����λ���ػ��ظ���; bit2����λ���ػ��ظ���;>
bit					  EACHCTRL_reportFLG = 0; //���ش������������ϱ�״̬ʹ��

relayStatus_PUSH xdata devActionPush_IF = {0};

bit				idata statusRelay_saveEn= 0; //����ֵ���ش洢ʹ��,���ʹ��,�ظ��洢

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
stt_Dimmer_attrFreq	xdata dimmer_freqParam		= {0};
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
stt_eleSocket_attrFreq xdata socket_eleDetParam = {0};
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SCENARIO)
stt_scenario_attrAct xdata scenario_ActParam = {0};
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_HEATER)
stt_heater_attrAct xdata heater_ActParam = {0};
#else
stt_Curtain_motorAttr xdata curtainAct_Param = {0, CURTAIN_ORBITAL_PERIOD_INITTIME, cTact_stop}; //���豸����Ϊ����ʱ����Ӧ�������ԣ�Ĭ�Ϲ��ʱ��0s
bit 				  idata specialFlg_curtainEachctrlEn = 1;	//�����ʶλ����������ͬ��ʹ�ܣ������ڳ��������½�ֹ��������
#endif

/*�̵���״̬���£�Ӳ��ִ��*/
void relay_statusReales(void){
	
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_FANS)
	switch(status_Relay){
	
		case 0:{
		
			PIN_RELAY_1 = 0;PIN_RELAY_2 = 0;PIN_RELAY_3 = 0;
		
		}break;
		
		case 1:{
		
			PIN_RELAY_1 = 0;PIN_RELAY_2 = 0;PIN_RELAY_3 = 1;
			
		}break;
			
		case 2:{
		
			PIN_RELAY_1 = 0;PIN_RELAY_2 = 1;PIN_RELAY_3 = 0;
			
		}break;
			
		case 3:
		default:{
		
			PIN_RELAY_1 = 1;PIN_RELAY_2 = 0;PIN_RELAY_3 = 0;
			
		}break;
	}
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
	(status_Relay)?(PIN_RELAY_1 = 1):(PIN_RELAY_1 = 0);
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SCENARIO)
	{
		
		bit scenarioTrigReserve_flg = 0;
		
		if((!scenario_ActParam.scenarioKeepTrig_timeCounter) || (scenario_ActParam.scenarioKeepTrig_timeCounter == COUNTER_DISENABLE_MASK_SPECIALVAL_U8)){ //��ͬ��������ǿ�Ƽ��ʱ��������������
		
			switch(status_Relay){
			
				case 1:{
				
					scenario_ActParam.scenarioKey_currentTrig = scenarioKey_current_S1;
					scenarioTrigReserve_flg = 1;
				
				}break;
					
				case 2:{
				
					scenario_ActParam.scenarioKey_currentTrig = scenarioKey_current_S2;
					scenarioTrigReserve_flg = 1;
				
				}break;
					
				case 4:{
				
					scenario_ActParam.scenarioKey_currentTrig = scenarioKey_current_S3;
					scenarioTrigReserve_flg = 1;
				
				}break;
					
				default:{}break;
			}
			
			if(scenarioTrigReserve_flg){
			
				scenario_ActParam.scenarioKeepTrig_timeCounter = SCENARIOTRIG_KEEPTIME_PERIOD;
				scenario_ActParam.scenarioDataSend_FLG = 1;
			}
		}
	}
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_HEATER)
	switch(status_Relay){

		case 0:{
		
			PIN_RELAY_1 = PIN_RELAY_2 = 0;
			heater_ActParam.relayActDelay_counter = HEATER_RELAY_SYNCHRONIZATION_DELAYTIME; //��С�̵����ͺ�ʱ���趨
			
		}break;
		
		case 1:{
		
			PIN_RELAY_1 = PIN_RELAY_2 = 1;
			heater_ActParam.relayActDelay_counter = HEATER_RELAY_SYNCHRONIZATION_DELAYTIME; //��С�̵����ͺ�ʱ���趨
		
		}break;
		
		default:{

			
		}break;
	}
	
#else
	switch(SWITCH_TYPE){
		
		case SWITCH_TYPE_CURTAIN:{
		
			switch(status_Relay){
			
				case 1:{
				
					PIN_RELAY_2 = 1;
					PIN_RELAY_1 = PIN_RELAY_3 = 0;
					curtainAct_Param.act = cTact_open;
					
				}break;
					
				case 4:{
				
					PIN_RELAY_1 = 1;
					PIN_RELAY_2 = PIN_RELAY_3 = 0;
					curtainAct_Param.act = cTact_close;
					
				}break;
					
				case 2:
				default:{
				
					PIN_RELAY_1 = PIN_RELAY_2 = PIN_RELAY_3 = 0;
					curtainAct_Param.act = cTact_stop;
					
					if(curtainAct_Param.act != cTact_stop)coverEEPROM_write_n(EEPROM_ADDR_curtainOrbitalCnter, &(curtainAct_Param.act_counter), 1); //ÿ�δ����˶�ֹͣʱ����¼��ǰλ�ö�Ӧ�Ĺ�����ڼ�ʱֵ
					
				}break;
			}
		
		}break;
	
		case SWITCH_TYPE_SWBIT1:{ //�̵���λ�õ��� 2��1
		
			if(DEV_actReserve & 0x02)(status_Relay & 0x01)?(PIN_RELAY_1 = 1):(PIN_RELAY_1 = 0);
			
		}break;
		
		case SWITCH_TYPE_SWBIT2:{ //�̵���λ�õ��� 3��2
		
			if(DEV_actReserve & 0x01)(status_Relay & 0x01)?(PIN_RELAY_1 = 1):(PIN_RELAY_1 = 0);
			if(DEV_actReserve & 0x04)(status_Relay & 0x02)?(PIN_RELAY_2 = 1):(PIN_RELAY_2 = 0);
		
		}break;
		
		case SWITCH_TYPE_SWBIT3:{ //�̵���λ�ñ���
		
			if(DEV_actReserve & 0x01)(status_Relay & 0x01)?(PIN_RELAY_1 = 1):(PIN_RELAY_1 = 0);
			if(DEV_actReserve & 0x02)(status_Relay & 0x02)?(PIN_RELAY_2 = 1):(PIN_RELAY_2 = 0);
			if(DEV_actReserve & 0x04)(status_Relay & 0x04)?(PIN_RELAY_3 = 1):(PIN_RELAY_3 = 0);
		
		}break;
		
		default:break;
	}
#endif	
	
	tips_statusChangeToNormal();
}

/*���س�ʼ��*/
void relay_pinInit(void){
	
	u8 idata statusTemp = 0;
	
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
	//����
	P3M1	&= ~0x30;
	P3M0	|= 0x30;
	PIN_RELAY_2 = PIN_PWM_OUT = 0;
	
	P3M1	|= 0x08;   //P33��0�жϼ���
	P3M0	&= ~0x08;
    INT1 = 0;
    IT1 = 1; 
	PX1 = 0; //�����ȼ�
    EX1 = 1;           
	
	statusRelay_saveEn = 1; //�ظ��������䵱ǰ����ֵ����ֹ����δ��������µ������ϵ�����ʱ����ֵ��ʧ
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_FANS)
	//����
	P3M1	&= ~0x38;
	P3M0	|= 0x38;
	PIN_RELAY_1 = PIN_RELAY_2 = PIN_RELAY_3 = 0;
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
	//����
	P3M1	&= ~0x08;
	P3M0	|= 0x08;
	PIN_RELAY_1 = 0;
	
	//������
	P3M1	|= 0xC0;
	P3M0	&= ~0xC0;
	INT_CLKO |=  (1 << 4); //�ⲿ�ж�2ʹ��
	INT_CLKO |=  (1 << 5); //�ⲿ�ж�3ʹ��

#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SCENARIO)
	//��������û�м̵���
	//��λ�󶨳����ŵ������ָ�
	EEPROM_read_n(EEPROM_ADDR_swTypeForceScenario_scencarioNumKeyBind, scenario_ActParam.scenarioNum_record, 3);

#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_HEATER)
	//����
	P3M1	&= ~0x38;
	P3M0	|= 0x38;
	PIN_RELAY_1 = PIN_RELAY_2 = PIN_RELAY_3 = 0;
	
#elif(SWITCH_TYPE_FORCEDEF == 0)
	//����
	P3M1	&= ~0x38;
	P3M0	|= 0x38;
	PIN_RELAY_1 = PIN_RELAY_2 = PIN_RELAY_3 = 0;
	
	//����������ڼ����Ӧλ�ü�ʱֵ�ָ�
	EEPROM_read_n(EEPROM_ADDR_curtainOrbitalPeriod, &(curtainAct_Param.act_period), 1);
	if(curtainAct_Param.act_period == 0xff)curtainAct_Param.act_period = CURTAIN_ORBITAL_PERIOD_INITTIME; //ֵ�޶�	
	EEPROM_read_n(EEPROM_ADDR_curtainOrbitalCnter, &(curtainAct_Param.act_counter), 1);
	if(curtainAct_Param.act_counter == 0xff)curtainAct_Param.act_counter = 0; //ֵ�޶�	
	
 #if(DEBUG_LOGOUT_EN == 1)
	{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�
		
		memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
		sprintf(log_buf, ">>>curtain param recover orbitalPeriod:%d, placeCounter:%d\n", (int)curtainAct_Param.act_period, (int)curtainAct_Param.act_counter);
		PrintString1_logOut(log_buf);
	}			
 #endif
#else
	
#endif
	
	if(relayStatus_ifSave == statusSave_enable){
	
#if(DATASAVE_INTLESS_ENABLEIF)
		swCommand_fromUsr.objRelay = devDataRecovery_relayStatus();
#else
		EEPROM_read_n(EEPROM_ADDR_relayStatus, &statusTemp, 1);
		swCommand_fromUsr.objRelay = statusTemp;
#endif
		swCommand_fromUsr.actMethod = relay_OnOff; //Ӳ������
		
	}else{
	
		statusTemp = 0;
		coverEEPROM_write_n(EEPROM_ADDR_relayStatus, &statusTemp, 1);
		relay_statusReales(); //Ӳ������
	}
}

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
void Ext_INT1 (void) interrupt INT1_VECTOR{
	
	dimmer_freqParam.periodBeat_cfm = dimmer_freqParam.periodBeat_counter;
	dimmer_freqParam.periodBeat_counter = 0;
	
	dimmer_freqParam.pwm_actEN = 1;
}
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
void Ext_INT2 (void) interrupt INT2_VECTOR{ //�ж�2
	
	socket_eleDetParam.eleParamFun_powerPulseCount += 1.0F;
}

void Ext_INT3 (void) interrupt INT3_VECTOR{ //�ж�3
	
	
}
#else
#endif

/*���ض���*/
void relay_Act(relay_Command dats){
	
	u8 statusTemp = 0;
	
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER) //�������ͼ��أ���Ҫ�Զ��洢���䣬��Ϊ��������������������ֵ�����м���Ļ�����ɵƹ�����ͻ�䣬�����ڽ���ʱ���м��伴��
	status_Relay = dats.objRelay;
	relay_statusReales();
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)
	dats = dats;
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SCENARIO)
	status_Relay = dats.objRelay;
	relay_statusReales();
	
#else
	
	statusTemp = status_Relay; //��ǰ����ֵ�ݴ�
	
	switch(dats.actMethod){
	
		case relay_flip:{ 
			
			if(dats.objRelay & 0x01)status_Relay ^= 1 << 0;
			if(dats.objRelay & 0x02)status_Relay ^= 1 << 1;
			if(dats.objRelay & 0x04)status_Relay ^= 1 << 2;
				
		}break;
		
		case relay_OnOff:{
			
			(dats.objRelay & 0x01)?(status_Relay |= 1 << 0):(status_Relay &= ~(1 << 0));
			(dats.objRelay & 0x02)?(status_Relay |= 1 << 1):(status_Relay &= ~(1 << 1));
			(dats.objRelay & 0x04)?(status_Relay |= 1 << 2):(status_Relay &= ~(1 << 2));
			
		}break;
		
		default:break;
		
	}relay_statusReales(); //Ӳ������
	
	devActionPush_IF.dats_Push = 0;
	devActionPush_IF.dats_Push |= (status_Relay & 0x07); //��ǰ����ֵλ��װ<����λ>
	
//	/*���ȷ�ʽ*/
//	if(		(statusTemp & 0x01) != (status_Relay & 0x01))devActionPush_IF.dats_Push |= 0x20; //����ֵ��װ<����λ>��һλ
//	else if((statusTemp & 0x02) != (status_Relay & 0x02))devActionPush_IF.dats_Push |= 0x40; //����ֵ��װ<����λ>�ڶ�λ
//	else if((statusTemp & 0x04) != (status_Relay & 0x04))devActionPush_IF.dats_Push |= 0x80; //����ֵ��װ<����λ>����λ
	/*�����ȷ�ʽ*/
	if((statusTemp & 0x01) != (status_Relay & 0x01))devActionPush_IF.dats_Push |= 0x20; //����ֵ��װ<����λ>��һλ
	if((statusTemp & 0x02) != (status_Relay & 0x02))devActionPush_IF.dats_Push |= 0x40; //����ֵ��װ<����λ>�ڶ�λ
	if((statusTemp & 0x04) != (status_Relay & 0x04))devActionPush_IF.dats_Push |= 0x80; //����ֵ��װ<����λ>����λ
	
	if(status_Relay)delayCnt_closeLoop = 0; //����һ�������̸�����ɫģʽʱ�����ֵ
	
	if(relayStatus_ifSave == statusSave_enable){ /*ÿ�θ��Ŀ���ֵʱ�����д洢����*///����״̬�洢�Զ���������
	
 #if(DATASAVE_INTLESS_ENABLEIF)
		devParamDtaaSave_relayStatusRealTime(status_Relay);
 #else
		statusTemp = status_Relay;
		coverEEPROM_write_n(EEPROM_ADDR_relayStatus, &statusTemp, 1);
 #endif
	}
	
#endif
}

/*�̵������߳�*/
void thread_Relay(void){
	
	if(swCommand_fromUsr.actMethod != actionNull){ //������Ӧ
	
		relay_Act(swCommand_fromUsr);
		
		swCommand_fromUsr.actMethod = actionNull;
		swCommand_fromUsr.objRelay = 0;
	}
	
	if(statusRelay_saveEn){ /*��������ʹ���ж�*///����״̬�洢��������
	
		u8 idata statusTemp = 0;
		
		statusRelay_saveEn = 0;
		
//#if(DEBUG_LOGOUT_EN == 1)
//		{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�
//			
//			memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
//			sprintf(log_buf, ">>>statusVal save cmp.\n");
//			PrintString1_logOut(log_buf);
//		}			
//#endif
	
		if(relayStatus_ifSave == statusSave_enable){ 
	
#if(DATASAVE_INTLESS_ENABLEIF)
			devParamDtaaSave_relayStatusRealTime(status_Relay);
#else
			statusTemp = status_Relay;
			coverEEPROM_write_n(EEPROM_ADDR_relayStatus, &statusTemp, 1);
#endif
		}
	}
	
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_HEATER)
	if(heater_ActParam.relayActDelay_actEn){ //ͬ��������־λ��Ӧ
	
		heater_ActParam.relayActDelay_actEn = 0;
		PIN_RELAY_3 = PIN_RELAY_1; //��ˮ���̵�����ƽͬ����������
	}
	
	if(heater_ActParam.heater_currentActMode == heaterActMode_swClose){ //������Ӧ��������ʱ��ָʾ����Ӧ�˵��̵���û��Ӧ
	
		if((status_Relay & (1 << 0)) != 0){
		
			swCommand_fromUsr.objRelay = 0;
			swCommand_fromUsr.actMethod = relay_OnOff; //���ض���
		}
	}
	
#else
#endif
}