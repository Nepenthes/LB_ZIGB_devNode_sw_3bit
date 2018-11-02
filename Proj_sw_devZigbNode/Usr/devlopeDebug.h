#ifndef __DEVLOPEDEBUG_H_
#define __DEVLOPEDEBUG_H_

#include "STC15Fxxxx.H"

typedef struct{

	u8 frameIllegal_FLG:1;
	u8 frameParsing_NUM:7;
	u8 frame_aLength;
	u8 frame_rLength;
}stt_frameDebugInfo;

extern bit idata frame_illegalFLG;
extern stt_frameDebugInfo xdata frameDebug_data;

void thread_devlopeDebug(void);

#endif