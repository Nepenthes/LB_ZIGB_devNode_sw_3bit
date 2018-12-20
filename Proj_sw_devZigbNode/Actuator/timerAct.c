#include "timerAct.h"

#include "dataManage.h" 
#include "Relay.h"
#include "dataTrans.h"

#include "string.h"
#include "stdio.h"

#include "eeprom.h"

/****************�����ļ�����������*************************/
stt_Time xdata	systemTime_current				= {0};	//ϵͳʱ��
u8 		 xdata	sysTimeReales_counter			= PERIOD_SYSTIMEREALES;
u8 	 	 xdata	sysTimeZone_H					= 8;
u8 	 	 xdata	sysTimeZone_M					= 0;

u16		 idata  sysTimeKeep_counter				= 0;	//ϵͳʱ��ά�ּ�����һ�����������ϵͳʱ���ѯ���ڵ���֮ǰά��ϵͳʱ����ת

u8 		 idata	swTim_oneShoot_FLAG				= 0;	//��ͨ���ض�ʱһ���Ա�־������λ��ʶ�˸���ʱ�������ؿ���ֻ���õ�ǰ�ĸ�
bit		 idata	ifTim_sw_running_FLAG			= 0;	//��ͨ���ض�ʱ���б�־λ

bit		 idata	ifNightMode_sw_running_FLAG		= 0;	//��ͨ����ҹ��ģʽ���б�־λ

u8 		 idata	ifDelay_sw_running_FLAG			= 0;	//��ʱ����_�Ƿ����б�־λ��bit 1��ʱ��������ʹ�ܱ�־��bit 0��ɫģʽ(��ʱ�ر�����ʹ��)��־��
u16		 idata	delayCnt_onoff					= 0;	//��ʱ������ʱ����
u8		 idata	delayPeriod_onoff				= 0;	//��ʱ��������
u8		 idata	delayUp_act						= 0;	//��ʱ�������嶯��
u16		 idata	delayCnt_closeLoop				= 0;	//��ɫģʽ��ʱ����
u8		 idata	delayPeriod_closeLoop			= 0;	//��ɫģʽ��������

static	timing_Dats xdata timDatsTemp_CalibrateTab[TIMEER_TABLENGTH] = {0};	/*��ʱ��ʼʱ�̱���*///��ʼʱ�̼�����
static	timing_Dats xdata nightDatsTemp_CalibrateTab[2] = {0};	/*ҹ��ģʽ��ʼʱ�̱���*///��ʼʱ�̼�����

/*-----------------------------------------------------------------------------------------------*/
void timeZone_Reales(void){

	EEPROM_read_n(EEPROM_ADDR_timeZone_H, &sysTimeZone_H, 1);
	EEPROM_read_n(EEPROM_ADDR_timeZone_M, &sysTimeZone_M, 1);
}

void datsTiming_read_eeprom(timing_Dats timDats_tab[TIMEER_TABLENGTH]){

	u8 dats_Temp[TIMEER_TABLENGTH * 3] = {0};
	u8 loop = 0;
	
	EEPROM_read_n(EEPROM_ADDR_swTimeTab, dats_Temp, TIMEER_TABLENGTH * 3);
	
	for(loop = 0; loop < TIMEER_TABLENGTH; loop ++){
	
		timDats_tab[loop].Week_Num	= (dats_Temp[loop * 3 + 0] & 0x7f) >> 0;	/*��ռλ����*///����λ
		timDats_tab[loop].if_Timing = (dats_Temp[loop * 3 + 0] & 0x80) >> 7;	/*�Ƿ�����ʱ������*///��һλ
		timDats_tab[loop].Status_Act= (dats_Temp[loop * 3 + 1] & 0xe0) >> 5;	/*��ʱ��Ӧ״̬����*///����λ
		timDats_tab[loop].Hour		= (dats_Temp[loop * 3 + 1] & 0x1f) >> 0;	/*��ʱʱ��_Сʱ*///����λ
		timDats_tab[loop].Minute	= (dats_Temp[loop * 3 + 2] & 0xff) >> 0;	/*��ʱʱ��_��*///ȫ��λ
	}
}

void itrf_datsTiming_read_eeprom(void){

	datsTiming_read_eeprom(timDatsTemp_CalibrateTab);
}

void datsTimNight_read_eeprom(timing_Dats timDats_tab[2]){

	u8 dats_Temp[6] = {0};
	u8 loop = 0;
	
	EEPROM_read_n(EEPROM_ADDR_TimeTabNightMode, dats_Temp, 6);
	
	for(loop = 0; loop < 2; loop ++){
	
		timDats_tab[loop].Week_Num	= (dats_Temp[loop * 3 + 0] & 0x7f) >> 0;	/*��ռλ����*///����λ ------------ҹ�䶨ʱ����ʱ����ռλȫΪ1ʱ���ʾȫ��
		timDats_tab[loop].if_Timing = (dats_Temp[loop * 3 + 0] & 0x80) >> 7;	/*�Ƿ�����ʱ������*///��һλ
		timDats_tab[loop].Status_Act= (dats_Temp[loop * 3 + 1] & 0xe0) >> 5;	/*��ʱ��Ӧ״̬����*///����λ
		timDats_tab[loop].Hour		= (dats_Temp[loop * 3 + 1] & 0x1f) >> 0;	/*��ʱʱ��_Сʱ*///����λ
		timDats_tab[loop].Minute	= (dats_Temp[loop * 3 + 2] & 0xff) >> 0;	/*��ʱʱ��_��*///ȫ��λ
	}
}

void itrf_datsTimNight_read_eeprom(void){

	datsTimNight_read_eeprom(nightDatsTemp_CalibrateTab);
}

/*��ռλ�ж�*///�жϵ�ǰ��ֵ�Ƿ���ռλ�ֽ���
bit weekend_judge(u8 weekNum, u8 HoldNum){

	u8 loop;
	
	weekNum --;
	for(loop = 0; loop < 7; loop ++){
	
		if(HoldNum & (1 << loop)){
			
			if(loop == weekNum)return 1;
		}
	}
	
	return 0;
}

#if(DEBUG_LOGOUT_EN == 1)
void time_Logout(stt_Time code timeDats){

	u8 xdata Log[80] 	= {0};
	
	/*���治��ʱ���ִδ�ӡ*/
	
//	sprintf(Log, 
//	"\n>>===ʱ���===<<\n    20%d/%02d/%02d-W%01d\n        %02d:%02d:%02d\n", 
//			(int)timeDats.time_Year,
//			(int)timeDats.time_Month,
//			(int)timeDats.time_Day,
//			(int)timeDats.time_Week,
//			(int)timeDats.time_Hour,
//			(int)timeDats.time_Minute,
//			(int)timeDats.time_Second);
//			
////	LogDats(Log, strlen(Log));
//	PrintString1_logOut(Log);
	
	sprintf(Log, 
	"\n>>===ʱ���===<<\n    20%02d/%02d/%02d-W%01d\n", 
			(int)timeDats.time_Year,
			(int)timeDats.time_Month,
			(int)timeDats.time_Day,
			(int)timeDats.time_Week);
			
//	LogDats(Log, strlen(Log));
	PrintString1_logOut(Log);
			
	sprintf(Log, 
	"        %02d:%02d:%02d\n", 
			(int)timeDats.time_Hour,
			(int)timeDats.time_Minute,
			(int)timeDats.time_Second);
			
//	LogDats(Log, strlen(Log));
	PrintString1_logOut(Log);
			
	sprintf(Log, 
			"timeZone_H:%02d.\n", 
			(int)sysTimeZone_H);
			
	PrintString1_logOut(Log);
}
#endif

void thread_Timing(void){

	u8 loop = 0;
	
	static idata timeUp_actionDone_flg = 0; //��ֵ̬, ͬһ�����ڶ�ʱ����Ӧ������ɱ�־<�����ظ���Ӧ>, bit0��Ӧ��ʱ��0, bit7��Ӧ��ʱ��7, �Դ�����

#if(DEBUG_LOGOUT_EN == 1)	
	{ //����log����-��ǰʱ�����
		
		u16 code log_period = 10000;
		static u16 xdata log_Count = 0;
		
		if(log_Count < log_period)log_Count ++;
		else{
		
			log_Count = 0;
			
			time_Logout(systemTime_current);
		}
	}
#endif

	/*��ʱҵ���Զ�ѭ��ҵ�����洢���ݶ�ȡ*///������һ�θ��¼���
	{
		
		static bit read_FLG = 0;
		
		if(!read_FLG){
		
			read_FLG = 1;
			
			EEPROM_read_n(EEPROM_ADDR_swDelayFLAG, &ifDelay_sw_running_FLAG, 1);
			EEPROM_read_n(EEPROM_ADDR_periodCloseLoop, &delayPeriod_closeLoop, 1);
	
			datsTiming_read_eeprom(timDatsTemp_CalibrateTab);  //��ͨ���ض�ʱ�����<<<
			datsTimNight_read_eeprom(nightDatsTemp_CalibrateTab); //ҹ��ģʽ��ʱ�����<<<
		}
	}
	
	/*ϵͳʱ��ά�ָ���*///zigb��ѯsysTime����֮�� ��������ÿ�����
	{
		
		systemTime_current.time_Minute = sysTimeKeep_counter / 60;
		systemTime_current.time_Second = sysTimeKeep_counter % 60;
		
		if(sysTimeKeep_counter < 3600){
			
		}else{
		
			sysTimeKeep_counter = 0;
			(systemTime_current.time_Hour >= 24)?(systemTime_current.time_Hour = 0):(systemTime_current.time_Hour ++);
			(systemTime_current.time_Week >   7)?(systemTime_current.time_Week = 1):(systemTime_current.time_Week ++);
		}
		
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS) //��������ÿСʱ��������
		
		if(!(systemTime_current.time_Minute) && !(systemTime_current.time_Second))socket_eleDetParam.ele_Consum = 0.0F;
#endif
	}
	
	/*�ж��Ƿ�Ϊҹ��ģʽ*/
	if((nightDatsTemp_CalibrateTab[0].Week_Num & 0x7F) == 0x7F){ //ȫ���жϣ������һ����ռλȫ����Ϊȫ��
	
		ifNightMode_sw_running_FLAG = 1;
		
	}else{
		
		bit idata timeTab_reserveFLG = 0;
		u16 xdata minutesTemp_CalibrateTab_A 	= ((u16)nightDatsTemp_CalibrateTab[0].Hour * 60 + (u16)nightDatsTemp_CalibrateTab[0].Minute),
				  minutesTemp_CalibrateTab_B 	= ((u16)nightDatsTemp_CalibrateTab[1].Hour * 60 + (u16)nightDatsTemp_CalibrateTab[1].Minute),
				  minutesTemp_CalibrateTab_cur	= ((u16)systemTime_current.time_Hour * 60 + (u16)systemTime_current.time_Minute);
		
		(minutesTemp_CalibrateTab_A < minutesTemp_CalibrateTab_B)?(timeTab_reserveFLG = 0):(timeTab_reserveFLG = 1); //ʱ����Ƿ�����
		
		if(nightDatsTemp_CalibrateTab[0].if_Timing){ //ʹ���ж�
			
			if(!timeTab_reserveFLG){ //ʱ�η����ж� -˳��
			
				((minutesTemp_CalibrateTab_cur >=  	minutesTemp_CalibrateTab_A)&&\
			     (minutesTemp_CalibrateTab_cur <	minutesTemp_CalibrateTab_B))?\
					(ifNightMode_sw_running_FLAG = 1):(ifNightMode_sw_running_FLAG = 0);
			
			}else{ //ʱ�η����ж� -����
			
				((minutesTemp_CalibrateTab_cur >=  	minutesTemp_CalibrateTab_A)||\
			     (minutesTemp_CalibrateTab_cur < 	minutesTemp_CalibrateTab_B))?\
					(ifNightMode_sw_running_FLAG = 1):(ifNightMode_sw_running_FLAG = 0);
			}
			
		}else{
		
			ifNightMode_sw_running_FLAG = 0;
		}
	}
	
	for(loop = 0; loop < TIMEER_TABLENGTH; loop ++){
	
		if(timDatsTemp_CalibrateTab[loop].if_Timing){ //�ж��Ƿ��ж�ʱ�ο���
		
			ifTim_sw_running_FLAG = 1; //ֻҪ��һ����ʱ�ο�����ʱ���б�־��λ
			break;
			
		}else{
		
			ifTim_sw_running_FLAG = 0;
		}
	}
	
	/*�ж��Ƿ�������ͨ���ض�ʱ��Ϊ��*/
	if(ifTim_sw_running_FLAG){	//��ȫ�أ��ñ�־λ����ִ�ж�ʱ�߼�
	
		for(loop = 0; loop < TIMEER_TABLENGTH; loop ++){
			
			if(weekend_judge(systemTime_current.time_Week, timDatsTemp_CalibrateTab[loop].Week_Num)){	//��ռλ�ȶԣ��ɹ��Ž�����һ��
			
				if(timCount_ENABLE == timDatsTemp_CalibrateTab[loop].if_Timing){	//�Ƿ�����ʱ
//#if(DEBUG_LOGOUT_EN == 1)					
//					{ //����log����-��ǰ��Ч��ʱ��Ϣ���
//						
//						u8 xdata log_buf[80] = {0};
//						u16 code log_period = 3000;
//						static u16 log_Count = 0;
//						
//						if(log_Count < log_period)log_Count ++;
//						else{
//						
//							log_Count = 0;
//							
//							sprintf(log_buf, 
//									"timer_%d is running, up time: %02dhour-%02dminute.\n", 
//									(int)loop, 
//									(int)timDatsTemp_CalibrateTab[loop].Hour, 
//									(int)timDatsTemp_CalibrateTab[loop].Minute);
//							/*log���Դ�ӡ*///��ͨ��ʱ��ʱ��Ϣ
//							PrintString1_logOut(log_buf);
//						}
//					}
//#endif
					if(((u16)systemTime_current.time_Hour * 60 + (u16)systemTime_current.time_Minute) !=  \
					   ((u16)timDatsTemp_CalibrateTab[loop].Hour * 60 + (u16)timDatsTemp_CalibrateTab[loop].Minute) && //ʱ�̱ȶ�,����������ɱ�־��λ
					   (timeUp_actionDone_flg & (1 << loop))){
					   
						timeUp_actionDone_flg &= ~(1 << loop);
					}				
					
					if(((u16)systemTime_current.time_Hour * 60 + (u16)systemTime_current.time_Minute) ==  \
					   ((u16)timDatsTemp_CalibrateTab[loop].Hour * 60 + (u16)timDatsTemp_CalibrateTab[loop].Minute) && //ʱ�̱ȶ�,�����Ӷ�����Ӧ��
					   !(timeUp_actionDone_flg & (1 << loop))){	 //ʱ�̱ȶ�ʱ��
#if(DEBUG_LOGOUT_EN == 1)							   
						{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�
							memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
							sprintf(log_buf, ">>>>>>>>timer-%02d_UP!!!.\n", (int)loop);
							PrintString1_logOut(log_buf);
						}	
#endif						   
						timeUp_actionDone_flg |= (1 << loop); //������ɱ�־��λ
						
						//һ���Զ�ʱ�ж�
						if(swTim_oneShoot_FLAG & (1 << loop)){	//�Ƿ�Ϊһ���Զ�ʱ��������ձ��ζ�ʱ��Ϣ
							
							u8 code dats_Temp = 0;
							
							swTim_oneShoot_FLAG &= ~(1 << loop);
							coverEEPROM_write_n(EEPROM_ADDR_swTimeTab + loop * 3, &dats_Temp, 1); //��ʱ��Ϣ���
							itrf_datsTiming_read_eeprom(); //���л������
						}
					   
						//��ͨ���ض�����Ӧ
						swCommand_fromUsr.actMethod = relay_OnOff; //���ض���
						swCommand_fromUsr.objRelay = timDatsTemp_CalibrateTab[loop].Status_Act;
						
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER) //����ֵ 1 ��Ϊ 100
	if(swCommand_fromUsr.objRelay == 1)swCommand_fromUsr.objRelay = 100;
#else
#endif
						devActionPush_IF.push_IF = 1; //����ʹ��
						dev_agingCmd_sndInitative.agingCmd_timerSetOpreat = 1; //��Ӧ�����ϴ�ʱЧռλ��һ
						
					}else
					if(((u16)systemTime_current.time_Hour * 60 + (u16)systemTime_current.time_Minute) >	//��ǰʱ����ڶ�ʱʱ�䣬ֱ�����һ���Ա�־
					   ((u16)timDatsTemp_CalibrateTab[loop].Hour * 60 + (u16)timDatsTemp_CalibrateTab[loop].Minute)){
						   
						//һ���Զ�ʱ�ж�
						if(swTim_oneShoot_FLAG & (1 << loop)){	//�Ƿ�Ϊһ���Զ�ʱ��������ձ��ζ�ʱ��Ϣ
							
							u8 code dats_Temp = 0;
							
							swTim_oneShoot_FLAG &= ~(1 << loop);
							coverEEPROM_write_n(EEPROM_ADDR_swTimeTab + loop * 3, &dats_Temp, 1); //��ʱ��Ϣ��գ�ֻ��յ�һ�ֽ�������Ϣ�������ֽ�ʱ����Ϣ����
						}
					}
				}
			}
		}
	}
}
