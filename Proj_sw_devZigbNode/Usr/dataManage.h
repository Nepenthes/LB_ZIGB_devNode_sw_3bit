#ifndef __DATAMANAGE_H_
#define __DATAMANAGE_H_

#include "STC15Fxxxx.H"

#define 	DEBUG_LOGOUT_EN		1 //log打印输出使能（失能后将归还大量代码空间）
#define 	LOGBUFF_LEN			64 //log打印缓存长度	

#define		clusterNum_usr		3 //自定义通讯簇数量（互控）

#define		SW_SCENCRAIO_LEN				31   //本地场景存储数量限制
#define		SW_SCENCRAIO_INSERTINVALID		0xFF //场景号索引无效值
#define		SW_SCENCRAIO_ACTINVALID			0xF0 //场景号对应开关响应状态位无效值

#define 	DEVICE_VERSION_NUM				 7 //设备版本号：L7
#define 	SWITCH_TYPE_SWBIT1	 			 (0x38 + 0x01) //设备类型，一位开关
#define 	SWITCH_TYPE_SWBIT2	 			 (0x38 + 0x02) //设备类型，二位开关
#define 	SWITCH_TYPE_SWBIT3	 			 (0x38 + 0x03) //设备类型，三位开关
#define		SWITCH_TYPE_CURTAIN				 (0x38 + 0x08) //设备类型，窗帘

#define 	SWITCH_TYPE_FANS				 (0x38 + 0x05) //设备类型，风扇
#define 	SWITCH_TYPE_SOCKETS				 (0x38 + 0x07) //设备类型，插座
#define 	SWITCH_TYPE_dIMMER				 (0x38 + 0x04) //设备类型，调光

#define 	SWITCH_TYPE_FORCEDEF			 0 //强制开关类型定义,硬件不同,写 0 时则是非强制定义

//#define 	ROMADDR_ROM_STC_ID		 		 0x3ff8		//STC单片机 全球ID地址
#define 	ROMADDR_ROM_STC_ID		 		 0x7ff8		//STC单片机 全球ID地址

#define 	EEPROM_ADDR_START	 			 0x0000		//正常参数起始扇区地址
#define		EEPROM_ADDR_STATUSRELAY			 0x0200		//继电器状态独立记录起始扇区地址

#define 	EEPROM_USE_OF_NUMBER 			 0x0080	

#define 	DATASAVE_INTLESS_ENABLEIF	 1	//是否将继电器状态进行独立实时记录<在开关状态记忆使能的情况下，开启此功能可有效避免触摸时闪烁>
#if(DATASAVE_INTLESS_ENABLEIF)
 #define 	RECORDPERIOD_OPREATION_LOOP	100	//继电器实时状态记录 单循环 存储扇区单元擦除周期
#endif
	
#define		BIRTHDAY_FLAG					 0xA1		//产品出生标记
	
#define		EEPROM_ADDR_BirthdayMark         0x0001		//01H - 01H 是否首次启动							01_Byte
#define  	EEPROM_ADDR_relayStatus          0x0002		//02H - 02H 开关状态存储							01_Byte
#define  	EEPROM_ADDR_timeZone_H           0x0003		//03H - 03H 时区——时								01_Byte
#define  	EEPROM_ADDR_timeZone_M           0x0004		//04H - 04H 时区——分								01_Byte
#define  	EEPROM_ADDR_deviceLockFLAG       0x0005		//05H - 05H 设备锁状态位							01_Byte
#define		EEPROM_ADDR_portCtrlEachOther	 0x0006		//06H - 08H 互控位绑定端口,依次为1、2、3位			03_Byte 
#define  	EEPROM_ADDR_swTimeTab          	 0x0010		//10H - 28H 8组普通定时数据，每组3字节				24_Byte	
#define  	EEPROM_ADDR_swDelayFLAG			 0x0030		//30H - 30H 开关延时标志位集合						01_Byte
#define 	EEPROM_ADDR_periodCloseLoop		 0x0031		//31H - 31H	循环关闭时间间隔						01_Byte
#define 	EEPROM_ADDR_TimeTabNightMode	 0x0032		//32H - 37H 夜间模式定时表							06_Byte
#define 	EEPROM_ADDR_curtainActPeriod	 0x0033		//33H - 33H 窗帘导轨周期时间						01_Byte
#define 	EEPROM_ADDR_ledSWBackGround		 0x0040		//40H - 41H	开关背景灯色索引						02_Byte
#define		EEPROM_ADDR_swScenarioNum		 0x0050		//50H - 6FH 场景编号								31_Byte
#define		EEPROM_ADDR_swScenarioAct	     0x0070		//70H - 8FH 场景响应动作							31_Byte
#define		EEPROM_ADDR_unDefine05           0x0000
#define		EEPROM_ADDR_unDefine06           0x0000
#define		EEPROM_ADDR_unDefine07           0x0000
#define		EEPROM_ADDR_unDefine08           0x0000
#define		EEPROM_ADDR_unDefine11           0x0000
#define		EEPROM_ADDR_unDefine12           0x0000
#define		EEPROM_ADDR_unDefine13           0x0000

/*=======================↓↓↓↓↓定时询访机制专用数据结构↓↓↓↓↓=============================*/

typedef struct agingDataSet_bitHold{ //数据结构_时效占位;	使用指针强转时注意，agingCmd_swOpreat对应单字节最低位bit0, 依此类推<tips,根据平台大小端不同区分bit0左对齐还是右对齐>

	u8 agingCmd_swOpreat:1; //时效_开关操作 -bit0
	u8 agingCmd_devLock:1; //时效_设备锁设置 -bit1
	u8 agingCmd_delaySetOpreat:1; //时效_延时设置 -bit2
	u8 agingCmd_greenModeSetOpreat:1; //时效_绿色模式设置 -bit3
	u8 agingCmd_timerSetOpreat:1; //时效_定时设置 -bit4
	u8 agingCmd_nightModeSetOpreat:1; //时效_夜间模式设置 -bit5
	u8 agingCmd_bkLightSetOpreat:1; //时效_背光灯设置 -bit6
	u8 agingCmd_devResetOpreat:1; //时效_开关复位恢复出厂操作 -bit7
	
	u8 agingCmd_horsingLight:1; //时效_跑马灯设置 -bit0
	u8 agingCmd_switchBitBindSetOpreat:3; //时效_开关位互控组设置_针对三个开关位进行设置 -bit1...bit3
	u8 agingCmd_curtainOpPeriodSetOpreat:1; //时效_针对窗帘导轨运行周期时间设置 -bit4
	u8 statusRef_bitReserve:3; //时效_bit保留 -bit5...bit7
	
	u8 agingCmd_byteReserve[4];	//5字节占位保留
	
}stt_agingDataSet_bitHold; //standard_length = 6Bytes

typedef struct swDevStatus_reference{ //数据结构_设备运行状态

	u8 statusRef_swStatus:3; //状态_设备开关状态 -bit0...bit2
	u8 statusRef_reserve:2; //状态_reserve -bit3...bit4
	u8 statusRef_swPush:3; //状态_推送占位 -bit5...bit7
	
	u8 statusRef_timer:1; //状态_定时器运行 -bit0
	u8 statusRef_devLock:1; //状态_设备锁 -bit1
	u8 statusRef_delay:1; //状态_延时运行 -bit2
	u8 statusRef_greenMode:1; //状态_绿色模式运行 -bit3
	u8 statusRef_nightMode:1; //状态_夜间模式运行 -bit4
	u8 statusRef_horsingLight:1; //状态_跑马灯运行 -bit5
	u8 statusRef_bitReserve:2; //状态_reserve -bit6...bit7
	
	u8 statusRef_byteReserve[2];   //状态_reserve -bytes2...3
	
}stt_swDevStatusReference_bitHold; //standard_length = 4Bytes

typedef struct dataPonit{ //数据结构_数据点

	stt_agingDataSet_bitHold 			devAgingOpreat_agingReference; //时效操作占位, 6Bytes
	stt_swDevStatusReference_bitHold	devStatus_Reference; //设备状态占位, 4Bytes 			
	u8 						 			devData_timer[24]; //定时器数据 8段, 24Bytes
	u8									devData_delayer; //延时数据, 1Bytes
	u8									devData_delayUpStatus; //延时到达时，开关响应状态, 1Bytes
	u8									devData_greenMode; //绿色模式数据, 1Bytes
	u8									devData_nightMode[6]; //夜间模式数据, 6Bytes
	u8									devData_bkLight[2]; //背光灯颜色数据, 2Bytes
	u8									devData_devReset; //开关复位数据, 1Bytes
	u8									devData_switchBitBind[3]; //开关位互控绑定数据, 3Bytes
	
}stt_devOpreatDataPonit; //standard_length = 49Bytes

/*=======================↑↑↑↑↑定时询访机制专用数据结构↑↑↑↑↑=============================*/
#if(DEBUG_LOGOUT_EN == 1)	
 extern u8 xdata log_buf[LOGBUFF_LEN];
#endif		

extern u8 	SWITCH_TYPE;
extern u8 	DEV_actReserve;
extern u8 	CTRLEATHER_PORT[3];
extern u16 	dev_currentPanid;

extern unsigned char xdata MAC_ID[6];
extern unsigned char xdata MAC_ID_DST[6];

extern bit deviceLock_flag;

u8 switchTypeReserve_GET(void);
void MAC_ID_Relaes(void);
void portCtrlEachOther_Reales(void);

bit swScenario_oprateSave(u8 scenarioNum, u8 swAct);
bit swScenario_oprateDele(u8 scenarioNum);
u8 swScenario_oprateCheck(u8 scenarioNum);

void Factory_recover(void);
void birthDay_Judge(void);

void devParamDtaaSave_relayStatusRealTime(u8 currentRelayStatus);
u8 devDataRecovery_relayStatus(void);

#endif