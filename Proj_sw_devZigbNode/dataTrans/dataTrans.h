#ifndef __DATATRANS_H_
#define __DATATRANS_H_

#include "STC15Fxxxx.H"
#include "USART.h"
#include "timerAct.h"

#define zigbPin_RESET		P23

#define ZIGB_FRAME_HEAD 	0xFE

#define PERIOD_HEARTBEAT		6  //心跳包发送周期  单位：s
#define PERIOD_SYSTIMEREALES	10 //系统时间更新周期  单位：s

#define ZIGB_FRAMEHEAD_CTRLLOCAL	0xAA
#define ZIGB_FRAMEHEAD_CTRLREMOTE	0xCC
#define ZIGB_FRAMEHEAD_HEARTBEAT	0xAB
#define ZIGB_FRAMEHEAD_HBOFFLINE	0xBB

#define ZIGB_NWKADDR_CORDINATER		0

#define PORTPOINT_OBJ_CTRLNOMAL			13
#define PORTPOINT_OBJ_CTRLSYSZIGB		14

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

#define ZIGB_SYSCMD_NWKOPEN	0x68  //zigb系统控制指令，开放网络
#define ZIGB_SYSCMD_TIMESET	0x69  //zigb系统控制指令，时间设定

#define ZIGB_BAUND	 115200UL   //串口波特率->ZIGB模块通讯

#define rxBuff_WIFI	 RX1_Buffer

#define NORMALDATS_DEFAULT_LENGTH	96

#define zigbDatsDefault_GroupID		13
#define zigbDatsDefault_ClustID		13
#define zigbDatsDefault_TransID		13
#define zigbDatsDefault_Option		0
#define zigbDatsDefault_Radius		7

typedef enum{

	status_NULL = 0,
	status_passiveDataRcv, //被动接受
	status_nwkREQ, //主动加入新网络请求
	status_nwkReconnect, //主动重连网络
	status_dataTransRequestDatsSend, //网络数据发送请求
}threadRunning_Status;

typedef enum{

	step_standBy = 0,
	step_nwkActive,
	step_hwReset,
	step_complete
}status_Reconnect;

typedef struct{

	u8 statusChange_IF:1;
	threadRunning_Status statusChange_standBy;
}stt_statusChange;

typedef struct{

	u8 command;
	u8 dats[32];
	u8 datsLen;
}frame_zigbSysCtrl;

#define DATBASE_LENGTH	72
typedef struct{

	u8 dats[DATBASE_LENGTH];
	u8 datsLen;
}datsAttr_datsBase;

typedef struct{

	datsAttr_datsBase datsTrans;
	u8 	portPoint;
	u16	nwkAddr;
}datsAttr_datsTrans;

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
extern stt_statusChange devStatus_switch;

void zigbUart_pinInit(void);
void uartObjZigb_Init(void);
void rxBuff_Zigb_Clr(void);
void uartObjZigb_Send_Byte(u8 dat);
void uartObjZigb_Send_String(char *s,unsigned char ucLength);

void thread_dataTrans(void);
bit zigB_sysTimeSet(u32 timeStamp);
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

void zigB_nwkReconnect(void);

void zigB_nwkJoinRequest(bit reJoin_IF);

#endif