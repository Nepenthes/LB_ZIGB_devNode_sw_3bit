#ifndef __DRIVER_IIC_HXD019D_H_
#define __DRIVER_IIC_HXD019D_H_

#include "STC15Fxxxx.H"

#include "dataManage.h"

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)

 #define hxd019d_pinBUSY		P34
 #define hxd019d_pinREST		P35
 #define hxd019d_pinSDA			P32
 #define hxd019d_pinSCL			P36
 
 #define hxd019d_resetLevel		0

 #define SetSDAInput()	{P3M1 |=  0x04;	P3M0 &= ~0x04;}
 #define SetSDAOutput()	{P3M1 &= ~0x04;	P3M0 |=  0x04;}
 #define SetSDAHigh()	hxd019d_pinSDA = 1
 #define SetSDALow()	hxd019d_pinSDA = 0
 #define GetDINStatus()	hxd019d_pinSDA

 #define SetSCLOutput()	{P3M1 &= ~0x40;	P3M0 |=  0x40;}
 #define SetSCLHigh()	hxd019d_pinSCL = 1
 #define SetSCLLow()	hxd019d_pinSCL = 0
 
 #define IR_opStatusLearnningSTBY_TOUT			2000	//红外转发器学习就绪态 单次超时时间
 #define IR_opStatusLearnningSTBY_TOUTLOOP		3		//红外转发器学习就绪态 超时次数限定
 #define IR_opStatusLearnning_TOUT				20000	//红外转发器学习态 超时时间
 #define IR_opStatusSigSendSTBY_TOUT			300		//红外转发器控制就绪态 等待时间
 #define IR_resetOpreatTimeKeep					200		//红外转发器芯片复位低电平保持时间
 
 #define IR_OPREATCMD_CONTROL					0x20	//红外下发指令：开始控制
 #define IR_OPREATCMD_LEARNNING					0x21	//红外下发指令：开始学习
 #define IR_OPREATRES_CONTROL					0x87	//红外操作结果：控制成功（控制失败不回码）
 #define IR_OPREATRES_LEARNNING					0x86	//红外操作结果：学习成功（学习失败不回码）
 
 typedef enum{

	infraredSMStatus_null = 0, //null
	infraredSMStatus_free, //空闲态
	infraredSMStatus_learnningSTBY, //信号学习就绪态
	infraredSMStatus_learnning, //信号学习态
	infraredSMStatus_sigSendSTBY, //信号发送就绪态
	infraredSMStatus_sigSend, //信号发送态
	infraredSMStatus_opStop //操作终止
}enumInfrared_status;
 
 extern u8 IR_currentOpreatRes;
 extern u8 IR_currentOpreatinsert;

 extern u16 xdata infraredAct_timeCounter;

 void infrared_pinInit(void);
 
 void thread_infraredSM(void);
 enumInfrared_status infraredStatus_GET(void);

 void infraredOpreatAct_Stop(void);
 void infraredOpreatAct_learnningStart(u8 opInsert);
 void infraredOpreatAct_remoteControlStart(u8 opInsert);

#endif

#endif