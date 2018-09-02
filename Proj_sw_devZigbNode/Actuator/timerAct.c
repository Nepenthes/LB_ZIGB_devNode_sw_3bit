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
char 	 xdata	sysTimeZone_H					= 8;
char 	 xdata	sysTimeZone_M					= 0;

u16		 idata  sysTimeKeep_counter				= 0;	//系统时间维持计数，一秒递增，用于系统时间查询周期到达之前维持系统时间运转

u8 		 idata	swTim_onShoot_FLAG				= 0;	//普通开关定时一次性标志——低四位标识四个定时器
bit		 idata	ifTim_sw_running_FLAG			= 0;	//普通开关定时运行标志位

bit		 idata	ifNightMode_sw_running_FLAG		= 0;	//普通开关夜间模式运行标志位

u8 		 idata	ifDelay_sw_running_FLAG			= 0;	//延时动作_是否运行标志位（bit 1延时开关运行使能标志，bit 0定时关闭运行使能标志）
u16		 idata	delayCnt_onoff					= 0;	//延时动作计时计数
u8		 idata	delayPeriod_onoff				= 0;	//延时动作周期
u8		 idata	delayUp_act						= 0;	//延时动作具体动作
u16		 idata	delayCnt_closeLoop				= 0;	//绿色模式计时计数
u8		 idata	delayPeriod_closeLoop			= 0;	//绿色模式动作周期

/*-----------------------------------------------------------------------------------------------*/
void timeZone_Reales(void){

	EEPROM_read_n(EEPROM_ADDR_timeZone_H, &sysTimeZone_H, 1);
	EEPROM_read_n(EEPROM_ADDR_timeZone_M, &sysTimeZone_M, 1);
}

void datsTiming_read_eeprom(timing_Dats timDats_tab[4]){

	u8 dats_Temp[12] = {0};
	u8 loop = 0;
	
	EEPROM_read_n(EEPROM_ADDR_swTimeTab, dats_Temp, 12);
	
	for(loop = 0; loop < 4; loop ++){
	
		timDats_tab[loop].Week_Num	= (dats_Temp[loop * 3 + 0] & 0x7f) >> 0;	/*周占位数据*///低七位
		timDats_tab[loop].if_Timing = (dats_Temp[loop * 3 + 0] & 0x80) >> 7;	/*是否开启定时器数据*///高一位
		timDats_tab[loop].Status_Act= (dats_Temp[loop * 3 + 1] & 0xe0) >> 5;	/*定时响应状态数据*///高三位
		timDats_tab[loop].Hour		= (dats_Temp[loop * 3 + 1] & 0x1f) >> 0;	/*定时时刻_小时*///低五位
		timDats_tab[loop].Minute	= (dats_Temp[loop * 3 + 2] & 0xff) >> 0;	/*定时时刻_分*///全八位
	}
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
	"\n>>===时间戳===<<\n    20%d/%02d/%02d-W%01d\n", 
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
}
#endif

void thread_Timing(void){

	u8 loop = 0;
	
	timing_Dats xdata timDatsTemp_CalibrateTab[4] = {0};	/*定时起始时刻表缓存*///起始时刻及属性
	timing_Dats xdata nightDatsTemp_CalibrateTab[2] = {0};	/*夜间模式起始时刻表缓存*///起始时刻及属性

#if(DEBUG_LOGOUT_EN == 1)	
	{ //调试log代码-当前时间输出
		
		u16 code log_period = 3000;
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
	}
	
	/*夜间模式定时*///两段数据=======================================================================================================<<<
	datsTimNight_read_eeprom(nightDatsTemp_CalibrateTab);
	/*判断是否为夜间模式*/
	if((nightDatsTemp_CalibrateTab[0].Week_Num & 0x7F) == 0x7F){ //全天判断，如果第一段周占位全满则为全天
	
		ifNightMode_sw_running_FLAG = 1;
		
	}else{
		
		if(nightDatsTemp_CalibrateTab[0].if_Timing){ //使能判断
		
			if(((u16)systemTime_current.time_Hour * 60 + (u16)systemTime_current.time_Minute) >  ((u16)nightDatsTemp_CalibrateTab[0].Hour * 60 + (u16)nightDatsTemp_CalibrateTab[0].Minute) && //定时区间判断
			   ((u16)systemTime_current.time_Hour * 60 + (u16)systemTime_current.time_Minute) <= ((u16)nightDatsTemp_CalibrateTab[1].Hour * 60 + (u16)nightDatsTemp_CalibrateTab[1].Minute)){
			   
				ifNightMode_sw_running_FLAG = 1;
				   
			}else{
			
				ifNightMode_sw_running_FLAG = 0;
			}
			
		}else{
		
			ifNightMode_sw_running_FLAG = 0;
		}
	}
	
	/*普通开关定时*///四段数据=======================================================================================================<<<
	datsTiming_read_eeprom(timDatsTemp_CalibrateTab);	/*普通开关*///时刻表读取
	/*判断是否所有普通开关定时都为关*/
	if((timDatsTemp_CalibrateTab[0].if_Timing == 0) &&	//全关，置标志位
	   (timDatsTemp_CalibrateTab[1].if_Timing == 0) &&
	   (timDatsTemp_CalibrateTab[2].if_Timing == 0) &&
	   (timDatsTemp_CalibrateTab[3].if_Timing == 0)
	  ){
	  
		ifTim_sw_running_FLAG = 0; 
		  
	}else{	//非全关，置标志位，并执行定时逻辑
		
		ifTim_sw_running_FLAG = 1; 
	
		for(loop = 0; loop < 4; loop ++){
			
			if(weekend_judge(systemTime_current.time_Week, timDatsTemp_CalibrateTab[loop].Week_Num)){	//周占位比对，成功才进行下一步
			
				if(timCount_ENABLE == timDatsTemp_CalibrateTab[loop].if_Timing){	//是否开启定时
					
//					{ //调试log代码-当前有效定时信息输出
//						
//						u8 xdata log_dats[80] = {0};
//						u8 code log_period = 200;
//						static u8 log_Count = 0;
//						
//						if(log_Count < log_period)log_Count ++;
//						else{
//						
//							log_Count = 0;
//							
//							sprintf(log_dats, 
//									"有效定时：%d号, 定_时:%d, 定_分:%d \n", 
//									(int)loop, 
//									(int)timDatsTemp_CalibrateTab[loop].Hour, 
//									(int)timDatsTemp_CalibrateTab[loop].Minute);
//							/*log调试打印*///普通定时定时信息
//							uartObjWIFI_Send_String(log_dats, strlen(log_dats));
//						}
//					}
					
					if(((u16)systemTime_current.time_Hour * 60 + (u16)systemTime_current.time_Minute) ==  \
					   ((u16)timDatsTemp_CalibrateTab[loop].Hour * 60 + (u16)timDatsTemp_CalibrateTab[loop].Minute) && //时刻比对
					   ((u16)systemTime_current.time_Second <= 10)){	 //时刻比对时间限在前10秒
						   
//						uartObjWIFI_Send_String("time_UP!!!", 11);
						
						//一次性定时判断
						if(swTim_onShoot_FLAG & (1 << loop)){	//是否为一次性定时，是则清空本段定时信息
							
							u8 code dats_Temp = 0;
							
							swTim_onShoot_FLAG &= ~(1 << loop);
							coverEEPROM_write_n(EEPROM_ADDR_swTimeTab + loop * 3, &dats_Temp, 1); //定时信息清空
						}
					   
						//普通开关动作响应
						swCommand_fromUsr.actMethod = relay_OnOff; //开关动作
						swCommand_fromUsr.objRelay = timDatsTemp_CalibrateTab[loop].Status_Act;
						
					}else
					if(((u16)systemTime_current.time_Hour * 60 + (u16)systemTime_current.time_Minute) >	//当前时间大于定时时间，直接清除一次性标志
					   ((u16)timDatsTemp_CalibrateTab[loop].Hour * 60 + (u16)timDatsTemp_CalibrateTab[loop].Minute)){
						   
						//一次性定时判断
						if(swTim_onShoot_FLAG & (1 << loop)){	//是否为一次性定时，是则清空本段定时信息
							
							u8 code dats_Temp = 0;
							
							swTim_onShoot_FLAG &= ~(1 << loop);
							coverEEPROM_write_n(EEPROM_ADDR_swTimeTab + loop * 3, &dats_Temp, 1); //定时信息清空
						}
					}
				}
			}
		}
	}
}
