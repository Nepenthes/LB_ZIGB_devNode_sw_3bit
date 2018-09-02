#ifndef __DATAMANAGE_H_
#define __DATAMANAGE_H_

#include "STC15Fxxxx.H"

#define 	DEBUG_LOGOUT_EN		1 //log打印输出使能（失能后将归还大量代码空间）

#define		clusterNum_usr		3 //自定义通讯簇数量（互控）

#define		SW_SCENCRAIO_LEN				31   //本地场景存储数量限制
#define		SW_SCENCRAIO_INSERTINVALID		0xFF //场景号索引无效值
#define		SW_SCENCRAIO_ACTINVALID			0xF0 //场景号对应开关响应状态位无效值

#define 	SWITCH_TYPE_SWBIT1	 			 (0x01 + 0x38) //设备类型，一位开关
#define 	SWITCH_TYPE_SWBIT2	 			 (0x02 + 0x38) //设备类型；二位开关
#define 	SWITCH_TYPE_SWBIT3	 			 (0x03 + 0x38) //设备类型，三位开关

//#define 	ROMADDR_ROM_STC_ID		 		 0x3ff8		//STC单片机 全球ID地址
#define 	ROMADDR_ROM_STC_ID		 		 0x7ff8		//STC单片机 全球ID地址

#define 	EEPROM_ADDR_START	 			 0x0000		//起始地址

#define 	EEPROM_USE_OF_NUMBER 			 0x0080	
	
#define		BIRTHDAY_FLAG					 0xA1		//产品出生标记
	
#define		EEPROM_ADDR_BirthdayMark         0x0001		//01H - 01H 是否首次启动							01_Byte
#define  	EEPROM_ADDR_relayStatus          0x0002		//02H - 02H 开关状态存储							01_Byte
#define  	EEPROM_ADDR_timeZone_H           0x0003		//03H - 03H 时区——时								01_Byte
#define  	EEPROM_ADDR_timeZone_M           0x0004		//04H - 04H 时区——分								01_Byte
#define  	EEPROM_ADDR_deviceLockFLAG       0x0005		//05H - 05H 设备锁状态位							01_Byte
#define		EEPROM_ADDR_portCtrlEachOther	 0x0006		//06H - 08H 互控位绑定端口,依次为1、2、3位			03_Byte 
#define  	EEPROM_ADDR_swTimeTab          	 0x0010		//10H - 1CH 4组普通定时数据，每组3字节				12_Byte	
#define  	EEPROM_ADDR_swDelayFLAG			 0x0020		//20H - 20H 开关延时标志位集合						01_Byte
#define 	EEPROM_ADDR_periodCloseLoop		 0x0021		//21H - 21H	循环关闭时间间隔						01_Byte
#define 	EEPROM_ADDR_TimeTabNightMode	 0x0022		//22H - 27H 夜间模式定时表							06_Byte
#define 	EEPROM_ADDR_ledSWBackGround		 0x0030		//30H - 31H	开关背景灯色索引						02_Byte
#define		EEPROM_ADDR_swScenarioNum		 0x0040		//40H - 5FH 场景编号								31_Byte
#define		EEPROM_ADDR_swScenarioAct	     0x0060		//60H - 7FH 场景响应动作							31_Byte
#define		EEPROM_ADDR_unDefine05           0x0000
#define		EEPROM_ADDR_unDefine06           0x0000
#define		EEPROM_ADDR_unDefine07           0x0000
#define		EEPROM_ADDR_unDefine08           0x0000
#define		EEPROM_ADDR_unDefine11           0x0000
#define		EEPROM_ADDR_unDefine12           0x0000
#define		EEPROM_ADDR_unDefine13           0x0000

extern u8 SWITCH_TYPE;
extern u8 DEV_actReserve;
extern u8 CTRLEATHER_PORT[3];

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

#endif