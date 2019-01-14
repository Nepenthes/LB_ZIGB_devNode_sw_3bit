#ifndef __APPTIMER_H_
#define __APPTIMER_H_

void appTimer0_Init(void);
void appTimer4_Init(void);

#define COUNTER_DISENABLE_MASK_SPECIALVAL_U16	65535	//16进制倒计时下计时计数 特殊指定失能掩码值 计时计数值等于此值时 计时失效
#define COUNTER_DISENABLE_MASK_SPECIALVAL_U8	255		//8进制倒计时下计时计数 特殊指定失能掩码值 计时计数值等于此值时 计时失效

#endif