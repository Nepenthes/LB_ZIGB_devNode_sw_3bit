#include "timerAct.h"

#include "dataManage.h" 
#include "Relay.h"
#include "dataTrans.h"

#include "string.h"
#include "stdio.h"

#include "eeprom.h"

/****************本地文件变量定义区*************************/
stt_Time xdata	systemTime_current				= {0};	//系统时间
u8 		 xdata	sysTimeReales_counter			= PERIOD_SYSTIMEREALES;
u8 	 	 xdata	sysTimeZone_H					= 8;
u8 	 	 xdata	sysTimeZone_M					= 0;

u16		 idata  sysTimeKeep_counter				= 0;	//系统时间维持计数，一秒递增，用于系统时间查询周期到达之前维持系统时间运转

u8 		 idata	swTim_oneShoot_FLAG				= 0;	//普通开关定时一次性标志——八位标识八个定时器，本地控制只作用到前四个
bit		 idata	ifTim_sw_running_FLAG			= 0;	//普通开关定时运行标志位

bit		 idata	ifNightMode_sw_running_FLAG		= 0;	//普通开关夜间模式运行标志位

u8 		 idata	ifDelay_sw_running_FLAG			= 0;	//延时动作_是否运行标志位（bit 1延时开关运行使能标志，bit 0绿色模式(定时关闭运行使能)标志）
u16		 idata	delayCnt_onoff					= 0;	//延时动作计时计数
u8		 idata	delayPeriod_onoff				= 0;	//延时动作周期
u8		 idata	delayUp_act						= 0;	//延时动作具体动作
u16		 idata	delayCnt_closeLoop				= 0;	//绿色模式计时计数
u8		 idata	delayPeriod_closeLoop			= 0;	//绿色模式动作周期

static	timing_Dats xdata timDatsTemp_CalibrateTab[TIMEER_TABLENGTH] = {0};	/*定时起始时刻表缓存*///起始时刻及属性
static	timing_Dats xdata nightDatsTemp_CalibrateTab[2] = {0};	/*夜间模式起始时刻表缓存*///起始时刻及属性

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
	
		timDats_tab[loop].Week_Num	= (dats_Temp[loop * 3 + 0] & 0x7f) >> 0;	/*周占位数据*///低七位
		timDats_tab[loop].if_Timing = (dats_Temp[loop * 3 + 0] & 0x80) >> 7;	/*是否开启定时器数据*///高一位
		timDats_tab[loop].Status_Act= (dats_Temp[loop * 3 + 1] & 0xe0) >> 5;	/*定时响应状态数据*///高三位
		timDats_tab[loop].Hour		= (dats_Temp[loop * 3 + 1] & 0x1f) >> 0;	/*定时时刻_小时*///低五位
		timDats_tab[loop].Minute	= (dats_Temp[loop * 3 + 2] & 0xff) >> 0;	/*定时时刻_分*///全八位
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
	
		timDats_tab[loop].Week_Num	= (dats_Temp[loop * 3 + 0] & 0x7f) >> 0;	/*周占位数据*///低七位 ------------夜间定时数据时，周占位全为1时则表示全天
		timDats_tab[loop].if_Timing = (dats_Temp[loop * 3 + 0] & 0x80) >> 7;	/*是否开启定时器数据*///高一位
		timDats_tab[loop].Status_Act= (dats_Temp[loop * 3 + 1] & 0xe0) >> 5;	/*定时响应状态数据*///高三位
		timDats_tab[loop].Hour		= (dats_Temp[loop * 3 + 1] & 0x1f) >> 0;	/*定时时刻_小时*///低五位
		timDats_tab[loop].Minute	= (dats_Temp[loop * 3 + 2] & 0xff) >> 0;	/*定时时刻_分*///全八位
	}
}

void itrf_datsTimNight_read_eeprom(void){

	datsTimNight_read_eeprom(nightDatsTemp_CalibrateTab);
}

/*周占位判断*///判断当前周值是否在占位字节中
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
	
	/*缓存不足时，分次打印*/
	
//	sprintf(Log, 
//	"\n>>===时间戳===<<\n    20%d/%02d/%02d-W%01d\n        %02d:%02d:%02d\n", 
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
	"\n>>===时间戳===<<\n    20%02d/%02d/%02d-W%01d\n", 
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
	
	static idata timeUp_actionDone_flg = 0; //静态值, 同一分钟内定时器响应动作完成标志<避免重复响应>, bit0对应定时段0, bit7对应定时段7, 以此类推

#if(DEBUG_LOGOUT_EN == 1)	
	{ //调试log代码-当前时间输出
		
		u16 code log_period = 10000;
		static u16 xdata log_Count = 0;
		
		if(log_Count < log_period)log_Count ++;
		else{
		
			log_Count = 0;
			
			time_Logout(systemTime_current);
		}
	}
#endif

	/*延时业务及自动循环业务掉电存储数据读取*///开机读一次更新即可
	{
		
		static bit read_FLG = 0;
		
		if(!read_FLG){
		
			read_FLG = 1;
			
			EEPROM_read_n(EEPROM_ADDR_swDelayFLAG, &ifDelay_sw_running_FLAG, 1);
			EEPROM_read_n(EEPROM_ADDR_periodCloseLoop, &delayPeriod_closeLoop, 1);
	
			datsTiming_read_eeprom(timDatsTemp_CalibrateTab);  //普通开关定时表更新<<<
			datsTimNight_read_eeprom(nightDatsTemp_CalibrateTab); //夜间模式定时表更新<<<
		}
	}
	
	/*系统时间维持更新*///zigb查询sysTime周期之外 本地自行每秒更新
	{
		
		systemTime_current.time_Minute = sysTimeKeep_counter / 60;
		systemTime_current.time_Second = sysTimeKeep_counter % 60;
		
		if(sysTimeKeep_counter < 3600){
			
		}else{
		
			sysTimeKeep_counter = 0;
			(systemTime_current.time_Hour >= 24)?(systemTime_current.time_Hour = 0):(systemTime_current.time_Hour ++);
			(systemTime_current.time_Week >   7)?(systemTime_current.time_Week = 1):(systemTime_current.time_Week ++);
		}
		
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS) //插座电量每小时定点清零
		
		if(!(systemTime_current.time_Minute) && !(systemTime_current.time_Second))socket_eleDetParam.ele_Consum = 0.0F;
#endif
	}
	
	/*判断是否为夜间模式*/
	if((nightDatsTemp_CalibrateTab[0].Week_Num & 0x7F) == 0x7F){ //全天判断，如果第一段周占位全满则为全天
	
		ifNightMode_sw_running_FLAG = 1;
		
	}else{
		
		bit idata timeTab_reserveFLG = 0;
		u16 xdata minutesTemp_CalibrateTab_A 	= ((u16)nightDatsTemp_CalibrateTab[0].Hour * 60 + (u16)nightDatsTemp_CalibrateTab[0].Minute),
				  minutesTemp_CalibrateTab_B 	= ((u16)nightDatsTemp_CalibrateTab[1].Hour * 60 + (u16)nightDatsTemp_CalibrateTab[1].Minute),
				  minutesTemp_CalibrateTab_cur	= ((u16)systemTime_current.time_Hour * 60 + (u16)systemTime_current.time_Minute);
		
		(minutesTemp_CalibrateTab_A < minutesTemp_CalibrateTab_B)?(timeTab_reserveFLG = 0):(timeTab_reserveFLG = 1); //时间表是否反序定义
		
		if(nightDatsTemp_CalibrateTab[0].if_Timing){ //使能判断
			
			if(!timeTab_reserveFLG){ //时段反序判断 -顺序
			
				((minutesTemp_CalibrateTab_cur >=  	minutesTemp_CalibrateTab_A)&&\
			     (minutesTemp_CalibrateTab_cur <	minutesTemp_CalibrateTab_B))?\
					(ifNightMode_sw_running_FLAG = 1):(ifNightMode_sw_running_FLAG = 0);
			
			}else{ //时段反序判断 -反序
			
				((minutesTemp_CalibrateTab_cur >=  	minutesTemp_CalibrateTab_A)||\
			     (minutesTemp_CalibrateTab_cur < 	minutesTemp_CalibrateTab_B))?\
					(ifNightMode_sw_running_FLAG = 1):(ifNightMode_sw_running_FLAG = 0);
			}
			
		}else{
		
			ifNightMode_sw_running_FLAG = 0;
		}
	}
	
	for(loop = 0; loop < TIMEER_TABLENGTH; loop ++){
	
		if(timDatsTemp_CalibrateTab[loop].if_Timing){ //判断是否有定时段开启
		
			ifTim_sw_running_FLAG = 1; //只要有一个定时段开启则定时运行标志置位
			break;
			
		}else{
		
			ifTim_sw_running_FLAG = 0;
		}
	}
	
	/*判断是否所有普通开关定时都为关*/
	if(ifTim_sw_running_FLAG){	//非全关，置标志位，并执行定时逻辑
	
		for(loop = 0; loop < TIMEER_TABLENGTH; loop ++){
			
			if(weekend_judge(systemTime_current.time_Week, timDatsTemp_CalibrateTab[loop].Week_Num)){	//周占位比对，成功才进行下一步
			
				if(timCount_ENABLE == timDatsTemp_CalibrateTab[loop].if_Timing){	//是否开启定时
//#if(DEBUG_LOGOUT_EN == 1)					
//					{ //调试log代码-当前有效定时信息输出
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
//							/*log调试打印*///普通定时定时信息
//							PrintString1_logOut(log_buf);
//						}
//					}
//#endif
					if(((u16)systemTime_current.time_Hour * 60 + (u16)systemTime_current.time_Minute) !=  \
					   ((u16)timDatsTemp_CalibrateTab[loop].Hour * 60 + (u16)timDatsTemp_CalibrateTab[loop].Minute) && //时刻比对,不对则动作完成标志复位
					   (timeUp_actionDone_flg & (1 << loop))){
					   
						timeUp_actionDone_flg &= ~(1 << loop);
					}				
					
					if(((u16)systemTime_current.time_Hour * 60 + (u16)systemTime_current.time_Minute) ==  \
					   ((u16)timDatsTemp_CalibrateTab[loop].Hour * 60 + (u16)timDatsTemp_CalibrateTab[loop].Minute) && //时刻比对,整分钟都是响应期
					   !(timeUp_actionDone_flg & (1 << loop))){	 //时刻比对时间
#if(DEBUG_LOGOUT_EN == 1)							   
						{ //输出打印，谨记 用后注释，否则占用大量代码空间
							memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
							sprintf(log_buf, ">>>>>>>>timer-%02d_UP!!!.\n", (int)loop);
							PrintString1_logOut(log_buf);
						}	
#endif						   
						timeUp_actionDone_flg |= (1 << loop); //动作完成标志置位
						
						//一次性定时判断
						if(swTim_oneShoot_FLAG & (1 << loop)){	//是否为一次性定时，是则清空本段定时信息
							
							u8 code dats_Temp = 0;
							
							swTim_oneShoot_FLAG &= ~(1 << loop);
							coverEEPROM_write_n(EEPROM_ADDR_swTimeTab + loop * 3, &dats_Temp, 1); //定时信息清空
							itrf_datsTiming_read_eeprom(); //运行缓存更新
						}
					   
						//普通开关动作响应
						swCommand_fromUsr.actMethod = relay_OnOff; //开关动作
						swCommand_fromUsr.objRelay = timDatsTemp_CalibrateTab[loop].Status_Act;
						
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER) //调光值 1 改为 100
	if(swCommand_fromUsr.objRelay == 1)swCommand_fromUsr.objRelay = 100;
#else
#endif
						devActionPush_IF.push_IF = 1; //推送使能
						dev_agingCmd_sndInitative.agingCmd_timerSetOpreat = 1; //对应主动上传时效占位置一
						
					}else
					if(((u16)systemTime_current.time_Hour * 60 + (u16)systemTime_current.time_Minute) >	//当前时间大于定时时间，直接清除一次性标志
					   ((u16)timDatsTemp_CalibrateTab[loop].Hour * 60 + (u16)timDatsTemp_CalibrateTab[loop].Minute)){
						   
						//一次性定时判断
						if(swTim_oneShoot_FLAG & (1 << loop)){	//是否为一次性定时，是则清空本段定时信息
							
							u8 code dats_Temp = 0;
							
							swTim_oneShoot_FLAG &= ~(1 << loop);
							coverEEPROM_write_n(EEPROM_ADDR_swTimeTab + loop * 3, &dats_Temp, 1); //定时信息清空，只清空第一字节属性信息，后两字节时间信息保留
						}
					}
				}
			}
		}
	}
}
