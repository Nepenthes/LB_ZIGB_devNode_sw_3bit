#ifndef __DEVLOPEDEBUG_H_
#define __DEVLOPEDEBUG_H_

#include "STC15Fxxxx.H"

typedef struct{

	u8 frameIllegal_FLG:1;
	u8 frameParsing_NUM:7;
	u8 frame_aLength;
	u8 frame_rLength;
}stt_frameDebugInfo;

typedef struct{

	u8 delayAct_Up:1;
}stt_inptDelayInfo;

typedef struct{

	u8 soureFreq;
}stt_dimmerInfo;

typedef union{

	stt_frameDebugInfo frameInfo;
	stt_inptDelayInfo delayActInfo;
	stt_dimmerInfo dimmerInfo;
}unn_debugInfoStt;

typedef enum{

	infoType_null = 0,
	infoType_frameUart,
	infoType_delayUp,
	infoType_dimmerFreq,
}enum_debugInfoType;

typedef struct{

	unn_debugInfoStt debugInfoData;
	enum_debugInfoType debugInfoType;
}stt_debugInfoLog;

extern stt_debugInfoLog xdata dev_debugInfoLog;

void thread_devlopeDebug(void);

#endif