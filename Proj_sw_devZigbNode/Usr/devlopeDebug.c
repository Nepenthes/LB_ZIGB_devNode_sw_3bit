#include "devlopeDebug.h"

#include "dataManage.h"
#include "dataTrans.h"
#include "Tips.h"

#include "USART.h"

#include "string.h"
#include "stdio.h"

extern uartTout_datsRcv xdata datsRcv_ZIGB;

stt_debugInfoLog xdata dev_debugInfoLog = {0}; //�û�debug���ݻ��棬һ�������ж���ҵ�������Ϣ�����

void thread_devlopeDebug(void){
	
//	counter_ifTipsFree = TIPS_SWFREELOOP_TIME; //��ˮ��ǿ��

#if(DEBUG_LOGOUT_EN == 1)				
	{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�
		
		if(dev_debugInfoLog.debugInfoType != infoType_null){
			
			switch(dev_debugInfoLog.debugInfoType){ //���ݴ�ӡ����ִ�д�ӡ
			
				case infoType_frameUart:{ //�����շ�֡������Ϣ��ӡ
				
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
					
				case infoType_delayUp:{ //��ʱҵ�������Ϣ��ӡ
				
					if(dev_debugInfoLog.debugInfoData.delayActInfo.delayAct_Up){
					
						dev_debugInfoLog.debugInfoData.delayActInfo.delayAct_Up = 0;

						memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
						sprintf(log_buf, ">>>>>>>>delayAct opreation up.\n");
						PrintString1_logOut(log_buf);				
					}
					
				}break;
				
				case infoType_dimmerFreq:{ //�������Ϳ���ҵ�������Ϣ��ӡ
					
					memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
					sprintf(log_buf, ">>>>>>>>dimmer source beat:%dots.\n", (int)dev_debugInfoLog.debugInfoData.dimmerInfo.soureFreq);
					PrintString1_logOut(log_buf);	
				}
					
				default:{}break;
			}
			
			dev_debugInfoLog.debugInfoType = infoType_null; //��ӡ���͸�λ
		}
	}
#endif	
}