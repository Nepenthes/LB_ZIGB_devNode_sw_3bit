#include "devlopeDebug.h"

#include "dataManage.h"
#include "dataTrans.h"
#include "Tips.h"

#include "USART.h"

#include "string.h"
#include "stdio.h"

extern uartTout_datsRcv xdata datsRcv_ZIGB;

stt_debugInfoLog xdata dev_debugInfoLog = {0}; //用户debug数据缓存，一般用于中断内业务调试信息的输出

void thread_devlopeDebug(void){
	
//	counter_ifTipsFree = TIPS_SWFREELOOP_TIME; //流水灯强关

#if(DEBUG_LOGOUT_EN == 1)				
	{ //输出打印，谨记 用后注释，否则占用大量代码空间
		
		if(dev_debugInfoLog.debugInfoType != infoType_null){
			
			switch(dev_debugInfoLog.debugInfoType){ //根据打印类型执行打印
			
				case infoType_frameUart:{ //串口收发帧调试信息打印
				
					if(dev_debugInfoLog.debugInfoData.frameInfo.frameIllegal_FLG){
					
						dev_debugInfoLog.debugInfoData.frameInfo.frameIllegal_FLG = 0;
						
						memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
						sprintf(log_buf, "F_head:%02X, A_Len(+5):%02X, R_Len:%02X, frame_P:%d.\n", 	(int)datsRcv_ZIGB.rcvDats[0],
																									(int)dev_debugInfoLog.debugInfoData.frameInfo.frame_aLength,
																									(int)dev_debugInfoLog.debugInfoData.frameInfo.frame_rLength,
																									(int)dev_debugInfoLog.debugInfoData.frameInfo.frameParsing_NUM);
						PrintString1_logOut(log_buf);
					}
					
				}break;
					
				case infoType_delayUp:{ //延时业务调试信息打印
				
					if(dev_debugInfoLog.debugInfoData.delayActInfo.delayAct_Up){
					
						dev_debugInfoLog.debugInfoData.delayActInfo.delayAct_Up = 0;

						memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
						sprintf(log_buf, ">>>>>>>>delayAct opreation up.\n");
						PrintString1_logOut(log_buf);				
					}
					
				}break;
				
				case infoType_dimmerFreq:{ //窗帘类型开关业务调试信息打印
					
					memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
					sprintf(log_buf, ">>>>>>>>dimmer source beat:%dots.\n", (int)dev_debugInfoLog.debugInfoData.dimmerInfo.soureFreq);
					PrintString1_logOut(log_buf);	
				}
					
				default:{}break;
			}
			
			dev_debugInfoLog.debugInfoType = infoType_null; //打印类型复位
		}
	}
#endif	
}