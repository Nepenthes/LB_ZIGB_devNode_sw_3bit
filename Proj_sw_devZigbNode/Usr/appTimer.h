#ifndef __APPTIMER_H_
#define __APPTIMER_H_

void appTimer0_Init(void);
void appTimer4_Init(void);

#define COUNTER_DISENABLE_MASK_SPECIALVAL_U16	65535	//16���Ƶ���ʱ�¼�ʱ���� ����ָ��ʧ������ֵ ��ʱ����ֵ���ڴ�ֵʱ ��ʱʧЧ
#define COUNTER_DISENABLE_MASK_SPECIALVAL_U8	255		//8���Ƶ���ʱ�¼�ʱ���� ����ָ��ʧ������ֵ ��ʱ����ֵ���ڴ�ֵʱ ��ʱʧЧ

#endif