#include "devlopeDebug.h"

#include "dataManage.h"
#include "dataTrans.h"
#include "Tips.h"

#include "USART.h"

#include "string.h"
#include "stdio.h"

extern uartTout_datsRcv xdata datsRcv_ZIGB;

bit idata frame_illegalFLG = 0;
stt_frameDebugInfo xdata frameDebug_data = {0};

void thread_devlopeDebug(void){
	
	counter_ifTipsFree = TIPS_SWFREELOOP_TIME; //流水灯强关

#if(DEBUG_LOGOUT_EN == 1)				
	{ //输出打印，谨记 用后注释，否则占用大量代码空间
		
		if(frameDebug_data.frameIllegal_FLG){
		
			frameDebug_data.frameIllegal_FLG = 0;
			
			memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
			sprintf(log_buf, "F_head:%02X, A_Len(+5):%02X, R_Len:%02X, frame_P:%d.\n", 	(int)datsRcv_ZIGB.rcvDats[0],
																						(int)frameDebug_data.frame_aLength,
																						(int)frameDebug_data.frame_rLength,
																						(int)frameDebug_data.frameParsing_NUM);
			PrintString1_logOut(log_buf);
		}
	}
#endif	
}