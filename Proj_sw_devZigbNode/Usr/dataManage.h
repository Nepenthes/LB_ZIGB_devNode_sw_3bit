#ifndef __DATAMANAGE_H_
#define __DATAMANAGE_H_

#include "STC15Fxxxx.H"

#define 	DEBUG_LOGOUT_EN					0 //log打印输出使能（失能后将归还大量代码空间）
#define 	LOGBUFF_LEN						56 //log打印缓存长度	

#define		clusterNum_usr					3 //自定义通讯簇数量（互控）
#define 	MUTUALCTRL_DEV_NUM_MAX			3 //单组互控内设备最大数量

#define 	DATASAVE_INTLESS_ENABLEIF	 	1	//是否将继电器状态进行独立实时记录<在开关状态记忆使能的情况下，开启此功能可有效避免触摸时闪烁>

#define		SW_SCENCRAIO_LEN				16   //本地场景存储数量限制
#define		SW_SCENCRAIO_INSERTINVALID		0xFF //场景号索引无效值
#define		SW_SCENCRAIO_ACTINVALID			0xF0 //场景号对应开关响应状态位无效值 

#define 	DEVICE_VERSION_NUM				 7 //设备版本号：L7
#define 	SWITCH_TYPE_SWBIT1	 			 (0x38 + 0x01) //设备类型，一位开关
#define 	SWITCH_TYPE_SWBIT2	 			 (0x38 + 0x02) //设备类型，二位开关
#define 	SWITCH_TYPE_SWBIT3	 			 (0x38 + 0x03) //设备类型，三位开关
#define		SWITCH_TYPE_CURTAIN				 (0x38 + 0x08) //设备类型，窗帘

#define 	SWITCH_TYPE_dIMMER				 (0x38 + 0x04) //设备类型，调光
#define 	SWITCH_TYPE_FANS				 (0x38 + 0x05) //设备类型，风扇
#define 	SWITCH_TYPE_INFRARED			 (0x38 + 0x06) //设备类型，红外转发器
#define 	SWITCH_TYPE_SOCKETS				 (0x38 + 0x07) //设备类型，插座
#define		SWITCH_TYPE_SCENARIO			 (0x38 + 0x09) //设备类型，场景开关
#define 	SWITCH_TYPE_HEATER				 (0x38 + 0x1F) //设备类型，热水器

#define 	SWITCH_TYPE_FORCEDEF			 0 //强制定义开关类型,硬件不同,写 0 时则是非强制定义(根据拨码定义)

/*开关类型为插座时，相关宏定义*///规格说明
#define 	SOCKETS_SPECIFICATION_AMERICA	0x0A //美规
#define 	SOCKETS_SPECIFICATION_BRITISH	0x0B //英规
#define 	SOCKETS_SPECIFICATION_GENERAL	0x0C //通用
#define 	SOCKETS_SPECIFICATION_SAFRICA	0x0D //南非
/*开关类型为插座时，相关宏定义*///规格定义
#define	SWITCH_TYPE_SOCKETS_SPECIFICATION	SOCKETS_SPECIFICATION_BRITISH  //-------定义插座规格

/*二次宏处理 --普通三位*///根据强制设备类型定义开关 将对应的宏进行二次处理
#if(SWITCH_TYPE_FORCEDEF == 0)
 #define 	CURTAIN_RELAY_UPSIDE_DOWN		 1 				//窗帘开关 按键/继电器 是否倒置
 
/*二次宏处理 --插座*/
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)

 //规格对应测量系数判断
 #if(SWITCH_TYPE_SOCKETS_SPECIFICATION == SOCKETS_SPECIFICATION_AMERICA)
 
  #define 	COEFFICIENT_POW					4.411065F		//功率系数 --L7 美规
  #define	COEFFICIENT_COMPENSATION_POW	0.000001F		//功率系数补偿 --L7 美规
 
 #elif(SWITCH_TYPE_SOCKETS_SPECIFICATION == SOCKETS_SPECIFICATION_BRITISH)
 
  #define 	COEFFICIENT_POW					3.396465F		//功率系数 --L7 英规
  #define	COEFFICIENT_COMPENSATION_POW	0.000001F		//功率系数补偿 --L7 英规
 
 #elif(SWITCH_TYPE_SOCKETS_SPECIFICATION == SOCKETS_SPECIFICATION_GENERAL)
 
  #define 	COEFFICIENT_POW					3.780465F		//功率系数 --L7 通用 (通用底板 -20181214) //test L7-HMP
  #define 	COEFFICIENT_COMPENSATION_POW	0.000013F		//功率系数补偿 --L7 通用 (通用底板 -20181214) //test L7-HMP
 
 #elif(SWITCH_TYPE_SOCKETS_SPECIFICATION == SOCKETS_SPECIFICATION_SAFRICA)
 
  #define 	COEFFICIENT_POW					4.287465F		//功率系数 --L7 南非 (南非底板 -20181214) //test L7-HMA
  #define 	COEFFICIENT_COMPENSATION_POW	0.000013F		//功率系数补偿 --L7 南非 (南非底板 -20181214) //test L7-HMA
 
 #endif

 #if(DEBUG_LOGOUT_EN == 1)
  #undef	DEBUG_LOGOUT_EN
  #define	DEBUG_LOGOUT_EN	0
  #warning	插座设备不支持log打印，因为硬件冲突，已将对应宏失能
 #endif
 
/*二次宏处理 --调光*/
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
 #if(DATASAVE_INTLESS_ENABLEIF == 0)
  #undef	DATASAVE_INTLESS_ENABLEIF
  #define	DATASAVE_INTLESS_ENABLEIF	1
  #warning	调光设备需要进行独立式继电器状态存储，否则存储当前亮度时会造成灯光闪烁，已将对应宏使能
 #endif
 
/*二次宏处理 --风扇*/
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_FANS)
 #define	SWITCHFANS_SPECIAL_VERSION_IMPACT	0 //低档位冲击特别版
 
/*二次宏处理 --红外转发器*/
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)
 #if(DATASAVE_INTLESS_ENABLEIF == 1)
  #undef	DATASAVE_INTLESS_ENABLEIF
  #define	DATASAVE_INTLESS_ENABLEIF	0
  #warning	红外转发器设备不需要进行独立式继电器状态存储，独立式继电器状态存储将占用大量RAM，已将对应宏失能
 #endif
#endif

/*数据存储掩码定义*/
#define		DATASAVE_MASK_ZIGBNWK_EXIST		 0xF1
#define		DATASAVE_MASK_ZIGBNWK_EXISTNOT	 0xF2

/*存储数据宏定义*///存储数据相关的宏定义
//#define 	ROMADDR_ROM_STC_ID		 		 0x3ff8		//STC单片机 全球ID地址
#define 	ROMADDR_ROM_STC_ID		 		 0x7ff8		//STC单片机 全球ID地址

#define		EEPROM_SECTOR_SIZE				 0x200		//EEPROM 单元扇区空间大小

#define 	EEPROM_ADDR_START_USRDATA		 0x0000										//正常参数起始扇区地址
#define		EEPROM_ADDR_START_STATUSRELAY	 EEPROM_ADDR_START_USRDATA + (0x0200 * 1)	//继电器状态独立记录起始扇区地址
#define		EEPROM_ADDR_START_IRDATA	 	 EEPROM_ADDR_START_USRDATA + (0x0200 * 2)	//IR红外数据存储起始地址，此地址后都是IR数据，一个扇区一条IR数据

#define 	EEPROM_USE_OF_NUMBER 			 0x0080		//存储地址范围上限
	
#define		BIRTHDAY_FLAG					 0xA1		//产品出生标记
		
#define		EEPROM_ADDR_BirthdayMark         0x0001		//01H - 01H 是否首次启动							01_Byte -(存储地址范围：0x0000 - EEPROM_USE_OF_NUMBER)
#define  	EEPROM_ADDR_relayStatus          0x0002		//02H - 02H 开关状态存储							01_Byte -(存储地址范围：0x0000 - EEPROM_USE_OF_NUMBER)
#define  	EEPROM_ADDR_timeZone_H           0x0003		//03H - 03H 时区——时								01_Byte -(存储地址范围：0x0000 - EEPROM_USE_OF_NUMBER)
#define  	EEPROM_ADDR_timeZone_M           0x0004		//04H - 04H 时区——分								01_Byte -(存储地址范围：0x0000 - EEPROM_USE_OF_NUMBER)
#define  	EEPROM_ADDR_deviceLockFLAG       0x0005		//05H - 05H 设备锁状态位							01_Byte -(存储地址范围：0x0000 - EEPROM_USE_OF_NUMBER)
#define		EEPROM_ADDR_portCtrlEachOther	 0x0006		//06H - 08H 互控位绑定端口,依次为1、2、3位			03_Byte -(存储地址范围：0x0000 - EEPROM_USE_OF_NUMBER)
#define		EEPROM_ADDR_curtainOrbitalPeriod 0x0009		//09H - 09H 窗帘轨道周期对应时间					01_Byte -(存储地址范围：0x0000 - EEPROM_USE_OF_NUMBER)
#define		EEPROM_ADDR_curtainOrbitalCnter	 0x000A		//0AH - 0AH 窗帘轨道周期对应位置计时值				01_Byte -(存储地址范围：0x0000 - EEPROM_USE_OF_NUMBER)
#define  	EEPROM_ADDR_swDelayFLAG			 0x000B		//0BH - 0BH 开关延时标志位集合						01_Byte -(存储地址范围：0x0000 - EEPROM_USE_OF_NUMBER)
#define 	EEPROM_ADDR_periodCloseLoop		 0x000C		//0CH - 0CH	循环关闭时间间隔						01_Byte -(存储地址范围：0x0000 - EEPROM_USE_OF_NUMBER)
#define		EEPROM_ADDR_zigbNwkExistIF		 0x000D		//0DH - 0DH	zigb网络是否存在本地记录				01_Byte -(存储地址范围：0x0000 - EEPROM_USE_OF_NUMBER)
#define  	EEPROM_ADDR_swTimeTab          	 0x0010		//10H - 28H 8组普通定时数据，每组3字节				24_Byte	-(存储地址范围：0x0000 - EEPROM_USE_OF_NUMBER)
#define 	EEPROM_ADDR_TimeTabNightMode	 0x0032		//32H - 37H 夜间模式定时表							06_Byte -(存储地址范围：0x0000 - EEPROM_USE_OF_NUMBER)
#define 	EEPROM_ADDR_ledSWBackGround		 0x0038		//38H - 39H	开关背景灯色索引						02_Byte -(存储地址范围：0x0000 - EEPROM_USE_OF_NUMBER)
#define		EEPROM_ADDR_swScenarioNum		 0x0040		//40H - 4FH 场景编号								31_Byte -(存储地址范围：0x0000 - EEPROM_USE_OF_NUMBER)
#define		EEPROM_ADDR_swScenarioAct	     0x0050		//50H - 5FH 场景响应动作							31_Byte -(存储地址范围：0x0000 - EEPROM_USE_OF_NUMBER)
#define		EEPROM_ADDR_mutualCtrlAddrs		 0x0060		//60h - 6FH	互控组内地址列表					   <15_byte -(存储地址范围：0x0000 - EEPROM_USE_OF_NUMBER) 

//EEPROM地址复用段
#define		EEPROM_ADDR_swTypeForceScenario_scencarioNumKeyBind	0x0070 //70H - 72H 开关类型强制为场景开关时	 	按键绑定场景号数据				03_Byte -(存储地址范围：0x0000 - EEPROM_USE_OF_NUMBER)
#define		EEPROM_ADDR_swTypeForceInfrared_timeUpActNum		0x0070 //70H - 77H 开关类型强制为红外转发器时	定时完成时响应发送的红外指令号	08_Byte -(存储地址范围：0x0000 - EEPROM_USE_OF_NUMBER)
//--------------------------------------------------------------↑↑↑↑↑↑↑限定值小于 EEPROM_USE_OF_NUMBER//
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
	u8 agingCmd_infrareOpreat:1;	//时效_针对红外转发器操作 -bit5
	u8 agingCmd_scenarioSwOpreat:1;	//时效_针对场景开关操作 -bit6
	u8 agingCmd_timeZoneReset:1; //时效_时区校准操作 -bit7
	
	u8 agingCmd_byteReserve[4];	//4字节占位保留
	
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
	
	union devClassfication{ //数据从此处开始分类
	
		struct funParam_curtain{ //窗帘
		
			u8 orbital_Period; //轨道周期时间
			
		}curtain_param;
		
		struct funParam_socket{ //插座
		
			u8 data_elePower[4]; //功率数据
			u8 data_eleConsum[3]; //电量数据
			u8 data_corTime; //当前电量对应小时段
			
			u8 dataDebug_powerFreq[4]; //debug数据-power频率
			
		}socket_param;
		
		struct funParam_infrared{
		
			u8 opreatAct; //操作指令
			u8 opreatInsert; //对应操作遥控序号
			u8 currentTemperature_integerPrt; //温度数据-整数部分
			u8 currentTemperature_decimalPrt; //温度数据-小数部分
			u8 currentOpreatReserveNum; //当前操作口令：用于判断动作截止还是信号不好时重发
			
			u8 irTimeAct_timeUpNum[8]; //八段红外定时响应的指令号
			
		}infrared_param;
		
		struct funParam_scenarioSw{
		
			u8 scenarioOpreatCmd; //操作命令
			u8 scenarioKeyBind[3]; //对应按键绑定的场景号
			
		}scenarioSw_param;
		
	}union_devParam;
	
//	u8	devData_byteReserve[63];
	
}stt_devOpreatDataPonit; //standard_length = 49Bytes + class_extension

/*=======================↑↑↑↑↑定时询访机制专用数据结构↑↑↑↑↑=============================*/

#if(DATASAVE_INTLESS_ENABLEIF)
 #define 	RECORDPERIOD_OPREATION_LOOP		100	//继电器实时状态记录 单循环 存储扇区单元擦除周期
#endif

#if(DEBUG_LOGOUT_EN == 1)	
 extern u8 xdata log_buf[LOGBUFF_LEN];
#endif		

extern u8 	SWITCH_TYPE;
extern u8 	DEV_actReserve;
extern u8 	CTRLEATHER_PORT[3];
extern u16 	dev_currentPanid;

extern u16 idata mutualCtrlDevList[clusterNum_usr][MUTUALCTRL_DEV_NUM_MAX - 1];

extern unsigned char xdata MAC_ID[6];
extern unsigned char xdata MAC_ID_DST[6];

extern bit deviceLock_flag;
extern bit zigbNwk_exist_FLG;

u8 switchTypeReserve_GET(void);
void MAC_ID_Relaes(void);
void portCtrlEachOther_Reales(void);
void devLockInfo_Reales(void);

bit swScenario_oprateSave(u8 scenarioNum, u8 swAct);
bit swScenario_oprateDele(u8 scenarioNum);
u8 swScenario_oprateCheck(u8 scenarioNum);

void Factory_recover(void);
void birthDay_Judge(void);

void statusSave_zigbNwk_nwkExistIF(bit nwkExistIF);
bit statusGet_zigbNwk_nwkExistIF(void);
void zigbNwkExist_detectReales(void);

void mutualCtrlSysParam_checkAndStore(u8 mutualCtrlGroup_insert, u16 devAddr);
void mutualCtrlSysParam_dataReset(u8 opreatBit);
void mutualCtrlSysParam_dataRecover(void);

void infrared_eeprom_dataSave(u8 insertNum, u8 dats[], u8 datsLen);
void infrared_eeprom_dataRead(u8 insertNum, u8 dats[], u8 datsLen);

void devParamDtaaSave_relayStatusRealTime(u8 currentRelayStatus);
u8 devDataRecovery_relayStatus(void);

#endif