#ifndef __DATATRANS_H_
#define __DATATRANS_H_

#include "STC15Fxxxx.H"
#include "USART.h"

#include "dataManage.h"

#include "Relay.h"

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
 #define zigbPin_RESET	P26
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)
 #define zigbPin_RESET	P21
#else
 #define zigbPin_RESET	P23
#endif

#define DATATRANS_WORKMODE_HEARTBEAT	0x0A
#define DATATRANS_WORKMODE_KEEPACESS	0x0B

#define ZIGB_DATATRANS_WORKMODE			DATATRANS_WORKMODE_KEEPACESS //定时类通讯模式选择
#define COLONYINFO_QUERYPERIOD_EN		DISABLE //集控信息周期性获取使能（集群控制信息查询周期为较短，失能将给通讯减负）

#define DTMODEKEEPACESS_FRAMEHEAD_ONLINE	0xFA	//定时寻访模式通讯帧头_在线
#define DTMODEKEEPACESS_FRAMEHEAD_OFFLINE	0xFB	//定时寻访模式通讯帧头_离线
#define	DTMODEKEEPACESS_FRAMECMD_ASR		0xA1	//定时寻访模式通讯帧命令——被动 远端控制应答/上传
#define	DTMODEKEEPACESS_FRAMECMD_PST		0xA2	//定时寻访模式通讯帧命令——主动 本地控制更新/上传

#define ZIGB_FRAME_HEAD 			0xFE

#define ZIGB_UTCTIME_START			946684800UL //zigbee时间戳从unix纪元946684800<2000/01/01 00:00:00>开始计算

#if(ZIGB_DATATRANS_WORKMODE == DATATRANS_WORKMODE_HEARTBEAT) //根据宏判做定义
	#define PERIOD_HEARTBEAT_ASR		6  	//心跳包发送周期  单位：s
#elif(ZIGB_DATATRANS_WORKMODE == DATATRANS_WORKMODE_KEEPACESS)
	#define PERIOD_HEARTBEAT_ASR		20  //周期询访数据询访周期_主动  单位：s
	#define PERIOD_HEARTBEAT_PST		2	//周期询访数据询访周期_被动	 单位：s
#endif

#define PERIOD_SYSTIMEREALES			10 	//系统时间更新周期  单位：s
#define ZIGBNWK_OPNETIME_DEFAULT		30	//默认zigb网络开放时间 单位：s
#define DEVHOLD_TIME_DEFAULT			240 //设备挂起默认时间，时间到后重启网络 单位：s
#define COLONYCTRLGET_QUERYPERIOD		3	//集群受控状态信息周期性轮询周期 单位：s
#define COORDINATOR_LOST_PERIOD_CONFIRM	30	//网关主机丢失确认周期	单位：s
#define REMOTE_DATAREQ_TIMEOUT			1000//远端数据请求系统响应超时时间  单位：ms
#define REMOTE_RESPOND_TIMEOUT			500 //远端数据请求节点响应超时时间  单位：ms

#define ZIGB_FRAMEHEAD_CTRLLOCAL	0xAA
#define ZIGB_FRAMEHEAD_CTRLREMOTE	0xCC
#define ZIGB_FRAMEHEAD_HEARTBEAT	0xAB
#define ZIGB_FRAMEHEAD_HBOFFLINE	0xBB

#define ZIGB_NWKADDR_CORDINATER		0 	//主机网络短地址，即协调器短地址 为0

//#define FRAME_TYPE_MtoZIGB_CMD			0xA1	/*数据类型*///手机至开关
//#define FRAME_TYPE_ZIGBtoM_RCVsuccess		0x1A	/*数据类型*///开关至手机
#define FRAME_TYPE_MtoS_CMD					0xA0	/*数据类型*///手机至开关
#define FRAME_TYPE_StoM_RCVsuccess			0x0A	/*数据类型*///开关至手机

#define FRAME_HEARTBEAT_cmdOdd				0x22	/*命令*///奇数心跳包
#define FRAME_HEARTBEAT_cmdEven				0x23	/*命令*///偶数心跳包

#define FRAME_MtoZIGBCMD_cmdControl			0x10	/*命令*///控制
#define FRAME_MtoZIGBCMD_cmdConfigSearch	0x39	/*命令*///配置搜索
#define FRAME_MtoZIGBCMD_cmdQuery			0x11	/*命令*///配置查询
#define FRAME_MtoZIGBCMD_cmdInterface		0x15	/*命令*///配置交互
#define FRAME_MtoZIGBCMD_cmdReset			0x16	/*命令*///复位
#define FRAME_MtoZIGBCMD_cmdDevLockON		0x17	/*命令*///设备上锁
#define FRAME_MtoZIGBCMD_cmdDevLockOFF		0x18	/*命令*///设备解锁
#define FRAME_MtoZIGBCMD_cmdswTimQuery		0x19	/*命令*///普通开关定时查询
#define FRAME_MtoZIGBCMD_cmdConfigAP		0x50	/*命令*///AP配置
#define FRAME_MtoZIGBCMD_cmdBeepsON			0x1A	/*命令*///开提示音
#define FRAME_MtoZIGBCMD_cmdBeepsOFF		0x1B	/*命令*///关提示音
#define FRAME_MtoZIGBCMD_cmdftRecoverRQ		0x22	/*命令*///查询是否支持恢复出厂
#define FRAME_MtoZIGBCMD_cmdRecoverFactory	0x1F	/*命令*///恢复出厂
#define FRAME_MtoZIGBCMD_cmdCfg_swTim		0x14	/*命令*///普通开关定时配置
#define FRAME_MtoZIGBCMD_cmdCfg_ctrlEachO	0x41	/*命令*///普通开关互控配置
#define FRAME_MtoZIGBCMD_cmdQue_ctrlEachO	0x42	/*命令*///普通开关互控查询
#define FRAME_MtoZIGBCMD_cmdCfg_ledBackSet	0x43	/*命令*///普通开关背景灯设置
#define FRAME_MtoZIGBCMD_cmdQue_ledBackSet	0x44	/*命令*///普通开关背景灯查询
#define FRAME_MtoZIGBCMD_cmdCfg_scenarioSet	0x45	/*命令*///普通开关场景配置
#define FRAME_MtoZIGBCMD_cmdCfg_scenarioCtl	0x47	/*命令*///普通开关场景控制
#define FRAME_MtoZIGBCMD_cmdCfg_scenarioDel	0x48	/*命令*///普通开关场景删除

#define	cmdConfigTim_normalSwConfig			0xA0	/*数据1*///普通开关定时辨识-数据2
#define cmdConfigTim_onoffDelaySwConfig		0xA1	/*数据1*///延时开关辨识-数据2
#define cmdConfigTim_closeLoopSwConfig		0xA2	/*数据1*///循环关闭辨识-数据2
#define cmdConfigTim_nightModeSwConfig		0xA3	/*数据1*///夜间模式辨识-数据2

#define ZIGB_BAUND	 115200UL   //串口波特率->ZIGB模块通讯

#define rxBuff_WIFI	 RX1_Buffer

#define NORMALDATS_DEFAULT_LENGTH	(128 + 25) //默认数据发送缓存长度

#define STATUSLOCALEACTRL_VALMASKRESERVE_ON		0x0A //互控本地轮询更新值，操作状态掩码 - 开
#define STATUSLOCALEACTRL_VALMASKRESERVE_OFF	0x0B //互控本地轮询更新值，操作状态掩码 - 关

#define CTRLSECENARIO_RESPCMD_SPECIAL			0xCE //场景控制回复专用数据

#define ZIGB_ENDPOINT_CTRLSECENARIO		12 //场景集群控制专用端口
#define ZIGB_ENDPOINT_CTRLNORMAL		13 //常规数据转发专用端口
#define ZIGB_ENDPOINT_CTRLSYSZIGB		14 //zigb系统交互专用端口

#define CTRLEATHER_PORT_NUMSTART		0x10 //互控端口起始编号
#define CTRLEATHER_PORT_NUMTAIL			0xFF //互控端口起始编号

#define ZIGB_SYSCMD_NWKOPEN					0x68  //zigb系统控制指令，开放网络
#define ZIGB_SYSCMD_TIMESET					0x69  //zigb系统控制指令，时间设定
#define ZIGB_SYSCMD_DEVHOLD					0x6A  //zigb系统控制指令，设备挂起
#define ZIGB_SYSCMD_EACHCTRL_REPORT			0x6B  //zigb系统控制指令，子设备向网关上报互控初发状态
#define ZIGB_SYSCMD_COLONYPARAM_REQPERIOD	0x6C  //zigb集群控制本地受控状态周期轮询应答(包括场景和互控)
#define ZIGB_SYSCMD_DATATRANS_HOLD			0x6D  //zigb系统控制指令，定时挂起周期性通信，进行通讯避障

#define zigbDatsDefault_GroupID		13
#define zigbDatsDefault_ClustID		13
#define zigbDatsDefault_TransID		13
#define zigbDatsDefault_Option		0
#define zigbDatsDefault_Radius		8   //多跳次数

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_FANS)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SCENARIO)
 #define DATATRANS_KEEPACESS_SCENARIOSW_SONSCOMMAND_SCENARIOreg		0x20    //场景开关时效操作子命令：按键绑定场景号注册
 #define DATATRANS_KEEPACESS_SCENARIOSW_SONSCOMMAND_SCENARIOdel		0x21	//场景开关时效操作子命令：按键绑定场景号注销
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_HEATER)
#else
#endif

typedef enum{

	status_NULL = 0,
	status_passiveDataRcv, //被动接受
	status_nwkREQ, //主动加入新网络请求
	status_nwkReconnect, //主动重连网络
	status_dataTransRequestDatsSend, //网络数据发送请求
	status_devNwkHold, //设备网络挂起
	status_devFactoryRecoverStandBy, //恢复出厂预置挂起
}threadRunning_Status;

typedef enum{

	step_standBy = 0,
	step_nwkActive,
	step_hwReset,
	step_complete
}status_Reconnect;

typedef struct{

	u8 zigbNwkSystemNote_IF:1; //是否需要进行广播通知其他子设备挂起
	u8 devHoldTime_counter; //挂起倒计时时间设定
}attr_devNwkHold;

typedef struct{

	u8 statusChange_IF:1; //状态切换使能
	threadRunning_Status statusChange_standBy; //需要切换到的状态
}stt_statusChange;

typedef struct{

	enum{

		zigbScenarioReverseCtrlCMD_scenarioCtrl = 0xCA,

	}command; //命令

	u8 scenario_Num; //场景号
	u8 dataOnceReserve_pWord; //数据包口令 -用于在通信恶劣的环境下判断数据重发
}frame_zigbScenarioReverseCtrl;

typedef struct{

	u8 command;
	u8 dats[32];
	u8 datsLen;
}frame_zigbSysCtrl;

#define DATBASE_LENGTH	128
typedef struct{

	u8 dats[DATBASE_LENGTH];
	u8 datsLen;
}datsAttr_datsBase;

typedef struct{

	u16 keepTxUntilCmp_IF:1; // 0:持续发送直到有响应码(无论对错) / 1：持续发送直到收到正确响应码(直到正确)，即是否死磕
	u16 datsTxKeep_Period:15; // 持续发送周期/频次 单位：ms
}remoteDataReq_method;

typedef struct{

	datsAttr_datsBase datsTrans;
	u8 	portPoint;
	u16	nwkAddr;
}datsAttr_datsTrans;

typedef struct{

	u8  dats[10];
	u8  datsLen;
	
	u8 	portPoint;
	u16	nwkAddr;
	
	u8 constant_Loop; //重复次数
	
}datsAttr_dtCtrlEach;

typedef struct{

	u8 	endpoint;
	u16 devID;
}datsAttr_clusterREG;

typedef struct{

	u8 rcvDats[COM_RX1_Lenth];
	u8 rcvDatsLen;
}uartTout_datsRcv;

typedef struct ZigB_Init_datsAttr{

	u8 	 zigbInit_reqCMD[2];	//请求指令
	u8 	 zigbInit_reqDAT[64];	//请求数据
	u8	 reqDAT_num;			//请求数据长度
	u8 	 zigbInit_REPLY[64];	//响应内容
	u8 	 REPLY_num;				//响应内容总长度
	u16  timeTab_waitAnsr;		//等待响应时间
}datsAttr_ZigbInit;

extern threadRunning_Status devRunning_Status;

extern attr_devNwkHold	xdata devNwkHoldTime_Param;
extern stt_statusChange xdata devStatus_switch;
extern u8 xdata devNwkHoldTime_counter;

extern stt_agingDataSet_bitHold xdata dev_agingCmd_rcvPassive;
extern stt_agingDataSet_bitHold xdata dev_agingCmd_sndInitative;

extern u8 xdata factoryRecover_HoldTimeCount;

extern u8 xdata timeCounter_coordinatorLost_detecting;

void zigbUart_pinInit(void);
void uartObjZigb_Init(void);
void rxBuff_Zigb_Clr(void);
void uartObjZigb_Send_Byte(u8 dat);
void uartObjZigb_Send_String(char *s,unsigned char ucLength);

void thread_dataTrans(void);
bit zigb_clusterSet(u16 deviveID, u8 endPoint);
bit ZigB_NwkJoin(u16 PANID, u8 CHANNELS);
bit ZigB_nwkOpen(bit openIF, u8 openTime);
bit ZigB_inspectionSelf(void);

bit zigb_VALIDA_INPUT(u8 REQ_CMD[2],		//指令
					  u8 REQ_DATS[],		//数据
					  u8 REQdatsLen,		//数据长度
					  u8 ANSR_frame[],		//响应帧
					  u8 ANSRdatsLen,		//响应帧长度
					  u8 times,u16 timeDelay);	//循环次数，单次等待时间
						  
void ZigB_datsTX(u16  DstAddr,
				 u8  SrcPoint,
				 u8  DstPoint,
				 u8  ClustID,
				 u8  dats[],
				 u8  datsLen);

void thread_dataTrans(void);

void devStatusChangeTo_devHold(bit zigbNwkSysNote_IF);
void devHoldStop_makeInAdvance(void);

#endif