#include "dataTrans.h"

#include "string.h"
#include "stdio.h"

#include "eeprom.h"
#include "USART.h"
#include "delay.h"
#include "Relay.h"

#include "timerAct.h"
#include "pars_Method.h"
#include "dataManage.h"
#include "Tips.h"

/**********************�����ļ�����������************************/
datsAttr_datsTrans xdata datsSend_request;//Զ�����ݴ������󻺴�
datsAttr_datsTrans xdata datsRcv_respond;//Զ�����ݴ�������ȴ���Ӧ���滺��

//zigbee����״̬�л���־
stt_statusChange devStatus_switch = {0, status_NULL};
//����������ɺ��Ƿ���Ҫ��������
bit reConnectAfterDatsReq_IF = 0; //���ڻ���ͨѶ�ؼ���ע�����������ʹ��

bit coordinatorOnline_IF = 0; //Э�������߱�־

//zigb��������ר�ö���ʱ�����
u16 xdata zigbNwkAction_counter = 0;

//����
bit heartBeatCycle_FLG = 0;	//�������ڴ���
u8 	heartBeatCount	   = 0;	//�������ڼ���

//���ڽ��ճ�ʱ��־
bit uartRX_toutFLG 	= 0;
//���ڽ��ճ�ʱ����
bit rxTout_count_EN = 0;
u8  rxTout_count 	= 0;
//�������ݻ���
u8	datsRcv_length	= 0;
uartTout_datsRcv xdata datsRcv_ZIGB = {{0}, 0};

//zigbeeͨ���̵߳�ǰ����״̬��־
threadRunning_Status devRunning_Status = status_NULL;

void zigbUart_pinInit(void){

	//TX�������
	P3M1 &= 0xFD;	
	P3M0 |= 0x02;	
	
	//RX��������
	P3M1 |= 0x01;
	P3M0 &= 0xFE;
	
	//TX�������
	P2M1 &= ~0x08;
	P2M0 |= 0x08;
}
	
/*--------------------------------------------------------------*/
void uartObjZigb_Init(void){

	EA = 0;

	PS = 1;
	SCON = (SCON & 0x3f) | UART_8bit_BRTx;

{
	u32 j = (MAIN_Fosc / 4) / ZIGB_BAUND;	//��1T����
		j = 65536UL - j;
	
	TH2 = (u8)(j>>8);
	TL2 = (u8)j;
}
	AUXR &= ~(1<<4);	//Timer stop
	AUXR |= 0x01;		//S1 BRT Use Timer2;
	AUXR &= ~(1<<3);	//Timer2 set As Timer
	AUXR |=  (1<<2);	//Timer2 set as 1T mode

	IE2  &= ~(1<<2);	//��ֹ�ж�
	AUXR &= ~(1<<3);	//��ʱ
	AUXR |=  (1<<4);	//Timer run enable

	ES 	  = 1;
	REN   = 1;
	P_SW1 = (P_SW1 & 0x3f) | (UART1_SW_P30_P31 & 0xc0);
	
	memset(TX1_Buffer, 0, sizeof(char) * COM_TX1_Lenth);

	EA = 1;

	PrintString1("i'm UART1 for wifi data translate !!!\n");
	PrintString1_logOut("i'm UART1 for datsLog !!!\n");
}

///*----------------------------
//���ʹ�������
//----------------------------*/
//void uartObjWIFI_Send_Byte(u8 dat)	//����1
//{
//	TX1_write2buff(dat);
//}

//void uartObjWIFI_Send_String(char *s,unsigned char ucLength){	 //����1
//	
//	uart1_datsSend(s, ucLength);
//}

//void rxBuff_WIFI_Clr(void){

//	memset(rxBuff_WIFI, 0xff, sizeof(char) * COM_RX1_Lenth);
//	COM1.RX_Cnt = 0;
//}

/********************* UART1(WIIF)�жϺ���_�Զ����ع�************************/
void UART1_Rountine (void) interrupt UART1_VECTOR
{
	
	if(RI)
	{
		RI = 0;
		if(COM1.B_RX_OK == 0)
		{
			
//			if(COM1.RX_Cnt >= COM_RX1_Lenth)	COM1.RX_Cnt = 0;
//			RX1_Buffer[COM1.RX_Cnt++] = SBUF;
//			COM1.RX_TimeOut = TimeOutSet1;
			
			if(!rxTout_count_EN){
			
				rxTout_count_EN = 1;
				rxTout_count 	= 0;
				datsRcv_length  = 0;
				
				memset(RX1_Buffer, 0xff, sizeof(char) * COM_RX1_Lenth);
			}
			
			
			RX1_Buffer[datsRcv_length ++] 	= SBUF;
			rxTout_count = 0;
		}
	}

	if(TI)
	{
		TI = 0;
		if(COM1.TX_read != COM1.TX_write)
		{
		 	SBUF = TX1_Buffer[COM1.TX_read];
			if(++COM1.TX_read >= COM_TX1_Lenth)		COM1.TX_read = 0;
		}
		else	COM1.B_TX_busy = 0;
	}
}

/* �Զ���У��*///�ԼҲ�ƷЭ���
static 
unsigned char frame_Check(unsigned char frame_temp[], u8 check_num){
  
	unsigned char loop 		= 0;
	unsigned char val_Check = 0;
	
	for(loop = 0; loop < check_num; loop ++){
	
		val_Check += frame_temp[loop];
	}
	
	val_Check  = ~val_Check;
	val_Check ^= 0xa7;
	
	return val_Check;
}

/*�����ݷ�װ���������ݰ�����ǰ�����ã��Զ������������ݷ�װ*///����У�鱻��ǰ������
static 
u8 dtasTX_loadBasic_CUST(bit ifRemoteDats,
					     u8 dats_Tx[],
					     u8 datsLen_TX,
					     u8 frame_Type,
					     u8 frame_CMD,
					     bit ifSpecial_CMD){
						   
	dats_Tx[2] 	= frame_Type;
	dats_Tx[3] 	= frame_CMD;
	
	if(!ifSpecial_CMD)dats_Tx[10] = SWITCH_TYPE;	//�����������
	
	memcpy(&dats_Tx[5], &MAC_ID[1], 5);	//MAC���
						  
	dats_Tx[4] 	= frame_Check(&dats_Tx[5], 28);
							   
	if(ifRemoteDats){
		
		u8 xdata dats_Temp[64] = {0};
	
		dats_Tx[0] = ZIGB_FRAMEHEAD_CTRLREMOTE;
		dats_Tx[1] 	= datsLen_TX + 12;
		
		memcpy(dats_Temp, &dats_Tx[1], datsLen_TX - 13);
		memset(&dats_Tx[1], 0, datsLen_TX - 1);
		memcpy(&dats_Tx[13], dats_Temp, datsLen_TX - 13);
		memcpy(&dats_Tx[1], MAC_ID_DST, 6);
		memcpy(&dats_Tx[8], &MAC_ID[1], 5);
		
		return 45;
		
	}else{
	
		dats_Tx[0] 	= ZIGB_FRAMEHEAD_CTRLLOCAL;
		dats_Tx[1] 	= datsLen_TX;
		
		return 33;
	}
}

/*�������У��*///ZNPЭ���
static 
u8 XORNUM_CHECK(u8 buf[], u8 length){

	u8 loop = 0;
	u8 valXOR = buf[0];
	
	for(loop = 1;loop < length;loop ++)valXOR ^= buf[loop];
	
	return valXOR;
}

/*zigbee����֡����*/
static 
u8 ZigB_TXFrameLoad(u8 frame[],u8 cmd[],u8 cmdLen,u8 dats[],u8 datsLen){		

	const u8 frameHead = ZIGB_FRAME_HEAD;	//ZNP,SOF֡ͷ
	u8 xor_check = datsLen;					//���У�飬֡β
	u8 loop = 0;
	u8 ptr = 0;
	
	frame[ptr ++] = frameHead;
	frame[ptr ++] = datsLen;
	
	memcpy(&frame[ptr],cmd,cmdLen);
	ptr += cmdLen;
	for(loop = 0;loop < cmdLen;loop ++)xor_check ^= cmd[loop];

	memcpy(&frame[ptr],dats,datsLen);
	ptr += datsLen;
	for(loop = 0;loop < datsLen;loop ++)xor_check ^= dats[loop];	
	
	frame[ptr ++] = xor_check;
	
	return ptr;
}

/*zigbee��ָ���������󣬷���Ӧ�����ݳ���*/
static 
u8 zigb_datsRequest( u8 frameREQ[],		//����֡
					 u8 frameREQ_Len,	//����֡��
					 u8 resp_cmd[2],	//����Ӧ��ָ��
					 u8 resp_dats[],	//Ӧ�����ݻ���
					 u8 loopReapt,u16 timeWait){	//ѭ�����������εȴ�ʱ��
					  
	u16 Local_Delay = timeWait;		
	u8 	loop = 0;
						 
	for(loop = 0;loop < loopReapt;loop ++){
	
		uartRX_toutFLG = 0;
		zigbNwkAction_counter = timeWait;	
	    uartZigB_datsSend(frameREQ, frameREQ_Len);
		
		while(zigbNwkAction_counter --){
			
			delayMs(2);

			if(uartRX_toutFLG){
			
				uartRX_toutFLG = 0;
				
				if(!memcmp(&(datsRcv_ZIGB.rcvDats[2]), resp_cmd, 2)){
				
					memcpy(resp_dats, datsRcv_ZIGB.rcvDats, datsRcv_ZIGB.rcvDatsLen);
					return datsRcv_ZIGB.rcvDatsLen;
					
				}
			}
		}
	}	

	return 0;
}

/*zigbee��ָ���·�����Ӧ��֤*///����
bit zigb_VALIDA_INPUT(u8 REQ_CMD[2],		//ָ��
					  u8 REQ_DATS[],		//����
					  u8 REQdatsLen,		//���ݳ���
					  u8 ANSR_frame[],		//��Ӧ֡
					  u8 ANSRdatsLen,		//��Ӧ֡����
					  u8 times,u16 timeDelay){	//ѭ�����������εȴ�ʱ��
					   
#define	dataLen_validaInput	96
	u8 xdata dataTXBUF[dataLen_validaInput] = {0};
	u8 	loop = 0;
	u8 	datsTX_Len;

	datsTX_Len = ZigB_TXFrameLoad(dataTXBUF,REQ_CMD, 2, REQ_DATS, REQdatsLen);

	for(loop = 0;loop < times;loop ++){
	
		uartRX_toutFLG = 0;
		zigbNwkAction_counter = timeDelay;
		uartZigB_datsSend(dataTXBUF, datsTX_Len);
		
		while(zigbNwkAction_counter --){
			
			delayMs(2);

			if(uartRX_toutFLG){
			
				uartRX_toutFLG = 0;
				
				if(memmem(datsRcv_ZIGB.rcvDats, COM_RX1_Lenth, ANSR_frame, ANSRdatsLen)){
				
					delayMs(2);
					return 1;
				}
			}
		}
	}
	
	return 0;
}

///*zigbeeͨ�Ŵ�����*///����
//bit zigb_clusterSet(u16 deviveID, u8 endPoint){

//	datsAttr_ZigbInit code default_param = {{0x24,0x00},{0x0E,0x0D,0x00,0x0D,0x00,0x0D,0x00,0x01,0x00,0x00,0x01,0x00,0x00},0x0D,{0xFE,0x01,0x64,0x00,0x00,0x65},0x06,300};	//���ݴ�ע��,Ĭ�ϲ���
//	u8 code frameResponse_Subs[6] = {0xFE,0x01,0x64,0x00,0xB8,0xDD}; //��Ӧ֡�油�������ݴ��Ѿ�ע��
//		
//#define	dataLen_zigbClusterSet	64
//	u8 xdata paramTX_temp[dataLen_zigbClusterSet] = {0};
//	
//	bit setResult = 0;
//	
//	memcpy(paramTX_temp, default_param.zigbInit_reqDAT, default_param.reqDAT_num);
//	paramTX_temp[0] = endPoint;
//	paramTX_temp[3] = (u8)((deviveID & 0x00ff) >> 0);
//	paramTX_temp[4] = (u8)((deviveID & 0xff00) >> 8);
//	
//	setResult =  zigb_VALIDA_INPUT(	(u8 *)default_param.zigbInit_reqCMD,
//									(u8 *)paramTX_temp,
//									default_param.reqDAT_num,
//									(u8 *)default_param.zigbInit_REPLY,
//									default_param.REPLY_num,
//									2,		//2������û����ȷ��Ӧ��ʧ��
//									default_param.timeTab_waitAnsr);
//	
//	if(setResult)return setResult;
//	else{
//	
//		return zigb_VALIDA_INPUT((u8 *)default_param.zigbInit_reqCMD,
//								 (u8 *)paramTX_temp,
//								 default_param.reqDAT_num,
//								 (u8 *)frameResponse_Subs,
//								 6,
//								 2,		//2������û����ȷ��Ӧ��ʧ��
//								 default_param.timeTab_waitAnsr);
//	}
//}

///*zigbee��������*///������������������ʹ��
//bit ZigB_NwkJoin(u16 PANID, u8 CHANNELS){

//#define	cmdNum_zigbNwkJoin	8	
//	
//#define	 loop_PANID		5
//#define	 loop_CHANNELS	6

//	datsAttr_ZigbInit code ZigbInit_dats[cmdNum_zigbNwkJoin] = {
//		
//		{	{0x46,0x09},	{0},					0x00,	{0xFE,0x06,0x41,0x80,0x01,0x02,0x00,0x02,0x06,0x03,0xC3},	0x0B,	4000	},	//��λ
//		{	{0x46,0x09},	{0},					0x00,	{0xFE,0x06,0x41,0x80,0x01,0x02,0x00,0x02,0x06,0x03,0xC3},	0x0B,	4000	},	//��λ
//		{	{0x26,0x05},	{0x03,0x01,0x03},		0x03,	{0xFE,0x01,0x66,0x05,0x00,0x62},							0x06,	500		},	//�Ĵ�����ʼ����ȫ�����
//		{	{0x46,0x09},	{0},					0x00,	{0xFE,0x06,0x41,0x80,0x01,0x02,0x00,0x02,0x06,0x03,0xC3},	0x0B,	4000	},	//���θ�λ
//		
////		{	{0x26,0x05},	{0x87,0x01,0x00},		0x03,	{0xFE,0x01,0x66,0x05,0x00,0x62},							0x06,	500		},	//��ɫ���ã�Э������
//		{	{0x26,0x05},	{0x87,0x01,0x01},		0x03,	{0xFE,0x01,0x66,0x05,0x00,0x62},							0x06,	500		},	//��ɫ���ã�·������
////		{	{0x26,0x05},	{0x87,0x01,0x02},		0x03,	{0xFE,0x01,0x66,0x05,0x00,0x62},							0x06,	500		},	//��ɫ���ã��նˣ�
//		
//		{	{0x27,0x02},	{0x34,0x12},			0x02,	{0xFE,0x01,0x67,0x02,0x00,0x64},							0x06,	500		},	//PAN_ID�Ĵ�������
//		{	{0x27,0x03},	{0x00,0x80,0x00,0x00},	0x04,	{0xFE,0x01,0x67,0x03,0x00,0x65},							0x06,	500		},	//�ŵ��Ĵ�������
////		{	{0x26,0x00},	{0},					0x00,	{0xFE,0x01,0x45,0xC0,0x09,0x8D},							0x06,	12000	},	//��ʼ�������Լȶ���ɫЭ������Э������Ӧ��
//		{	{0x26,0x00},	{0},					0x00,	{0xFE,0x01,0x45,0xC0,0x07,0x83},							0x06,	12000	},	//��ʼ�������Լȶ���ɫЭ������·������Ӧ��
////		{	{0x26,0x00},	{0},					0x00,	{0xFE,0x01,0x45,0xC0,0x06,0x82},							0x06,	12000	},	//��ʼ�������Լȶ���ɫЭ�������ն���Ӧ��
//	};
//	
//#define	dataLen_zigbNwkJoin 64
//	u8 xdata paramTX_temp[dataLen_zigbNwkJoin] = {0};
//	
//	u8  loop;
//	u32 chnl_temp = 0x00000800UL << CHANNELS;
//	
//	for(loop = 1; loop < cmdNum_zigbNwkJoin; loop ++){
//		
//		memset(paramTX_temp, 0, dataLen_zigbNwkJoin * sizeof(u8));
//		
//		switch(loop){	//��ѡ����&Ĭ�ϲ���
//		
//			case loop_PANID:{
//			
//				paramTX_temp[0] = (u8)((PANID & 0x00ff) >> 0);
//				paramTX_temp[1] = (u8)((PANID & 0xff00) >> 8);
//				
//			}break;
//			
//			case loop_CHANNELS:{
//			
//				paramTX_temp[0] = (u8)((chnl_temp & 0x000000ff) >>  0);
//				paramTX_temp[1] = (u8)((chnl_temp & 0x0000ff00) >>  8);
//				paramTX_temp[2] = (u8)((chnl_temp & 0x00ff0000) >> 16);
//				paramTX_temp[3] = (u8)((chnl_temp & 0xff000000) >> 24);
//				
//			}break;
//			
//			default:{
//			
//				memcpy(paramTX_temp,ZigbInit_dats[loop].zigbInit_reqDAT,ZigbInit_dats[loop].reqDAT_num);
//				
//			}break;
//		}
//	
//		delayMs(100);
//		if(0 == zigb_VALIDA_INPUT((u8 *)ZigbInit_dats[loop].zigbInit_reqCMD,
//								  (u8 *)paramTX_temp,
//								  ZigbInit_dats[loop].reqDAT_num,
//								  (u8 *)ZigbInit_dats[loop].zigbInit_REPLY,
//								  ZigbInit_dats[loop].REPLY_num,
//								  3,
//								  ZigbInit_dats[loop].timeTab_waitAnsr)
//								 )loop = 0;
//	}
//	
//	return zigb_clusterSet(13, 13);	//�豸ID 13���ն˵� 13��
//}

/*zigbee ������������*///����
bit ZigB_nwkOpen(bit openIF, u8 openTime){

	datsAttr_ZigbInit code default_param = {{0x26,0x08}, {0xFC,0xFF,0x00}, 0x03, {0xFE,0x01,0x66,0x08,0x00,0x6F}, 0x06, 150}; //zigbeeָ���´�Ĭ�ϲ���
	
	bit resultSet = 0;
	
	u8 openTime_temp = 0;
	
#define	paramLen_zigbNwkOpen 3
	u8 xdata paramTX_temp[paramLen_zigbNwkOpen] = {0xFC,0xFF,0x00};
	
	(openIF)?(paramTX_temp[2] = openTime):(paramTX_temp[2] = 0);
	
	resultSet = zigb_VALIDA_INPUT((u8 *)default_param.zigbInit_reqCMD,
								  (u8 *)paramTX_temp,
								  default_param.reqDAT_num,
								  (u8 *)default_param.zigbInit_REPLY,
								  default_param.REPLY_num,
								  2,	//2���޻ظ�Ϊʧ��
								  default_param.timeTab_waitAnsr);

#if(DEBUG_LOGOUT_EN == 1)	
//	{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�
//		u8 xdata log_buf[64];
//		
//		sprintf(log_buf, "nwkOpen result:%d.\n", (int)resultSet);
//		PrintString1_logOut(log_buf);
//	}
#endif
	
	return resultSet;
}

/*zigbee PANID��ȡ*///����
static u16 ZigB_getPanIDCurrent(void){

	u16 PANID_temp = 0;
	
#define	paramLen_zigbPanIDGet 32
	u8 xdata paramTX_temp[paramLen_zigbPanIDGet] = {0};
	
	u8 code frameREQ_zigbPanIDGet[6] = {0xFE, 0x01, 0x26, 0x06, 0x06, 0x27};	//zigb PANID��ȡָ��֡
	u8 code cmdResp_zigbPanIDGet[2]  = {0x66, 0x06};	//zigb PANID��ȡԤ����Ӧָ��
	u8 datsResp_Len = 0;

	datsResp_Len = zigb_datsRequest(frameREQ_zigbPanIDGet, 6, cmdResp_zigbPanIDGet, paramTX_temp, 2, 300);

	if(datsResp_Len){

		PANID_temp |= (((u16)(paramTX_temp[5]) << 0) & 0x00FF);
		PANID_temp |= (((u16)(paramTX_temp[6]) << 8) & 0xFF00);

//		printf_datsHtoA("[Tips_uartZigb]: resultDats:", local_datsParam->frameResp, local_datsParam->frameRespLen);
	}

	return PANID_temp;
}

/*zigbeeϵͳʱ���ȡ������*///����
static bit getSystemTime_reales(void){
	
	bit resultOpreat = 0;

#define	paramLen_zigbSystimeGet 32
	u8 xdata paramTX_temp[paramLen_zigbSystimeGet] = {0};
	
	u8 code frameREQ_zigbSystimeGet[5] = {0xFE, 0x00, 0x21, 0x11, 0x30};	//zigb PANID��ȡָ��֡
	u8 code cmdResp_zigbSystimeGet[2]  = {0x61, 0x11};	//zigb PANID��ȡԤ����Ӧָ��
	u8 datsResp_Len = 0;

	datsResp_Len = zigb_datsRequest(frameREQ_zigbSystimeGet, 5, cmdResp_zigbSystimeGet, paramTX_temp, 2, 300);
	
	if(!datsResp_Len)resultOpreat = 0;
	else{
		
		u16 Y_temp16 = ((u16)paramTX_temp[13] << 0) | ((u16)paramTX_temp[14] << 8);
		u8  Y_temp8 = 0;
		u8  M_temp8 = 0;
		
		u8 Y = (u8)(Y_temp16 % 2000);
		u8 M = paramTX_temp[11];
		u8 D = paramTX_temp[12];
		u8 W = 0;
		
		/*���㻺�渳ֵ*/
		Y_temp8 = Y;
		if(M == 1 || M == 2){ //һ�ºͶ��µ�����һ��ʮ���º�ʮ����
		
			M_temp8 = M + 12;
			Y_temp8 --;
		}
		else M_temp8 = M;
		
		/*��ʼ����*/
		W =	 Y_temp8 + (Y_temp8 / 4) + 5 - 40 + (26 * (M_temp8 + 1) / 10) + D - 1;	//���չ�ʽ
		W %= 7; 
		
		/*��������ֵ*/
		W?(systemTime_current.time_Week = W):(systemTime_current.time_Week = 7);
		
		systemTime_current.time_Month = 	M;
		systemTime_current.time_Day = 		D;
		systemTime_current.time_Year = 		Y;
		
		systemTime_current.time_Hour = 		paramTX_temp[8];
		systemTime_current.time_Minute =	paramTX_temp[9];
		systemTime_current.time_Second = 	paramTX_temp[10];
		
		/*����ʱ��ά�ּ���ֵУ׼����*/
		sysTimeKeep_counter = systemTime_current.time_Minute * 60 + systemTime_current.time_Second; //ϵͳʱ��ά�ּ���ֵ����
		
		resultOpreat = 1;
	}
	
#if(DEBUG_LOGOUT_EN == 1)	
//	{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�
//		u8 xdata log_buf[64];
//		
//		sprintf(log_buf, "sysTime reales result:%d.\n", (int)resultOpreat);
//		PrintString1_logOut(log_buf);
//	}
#endif
	
	return resultOpreat;
}

/*zigbeeϵͳʱ������*///����
bit zigB_sysTimeSet(u32 timeStamp){

	datsAttr_ZigbInit code default_param = {{0x21,0x10},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},0x0B,{0xFE,0x01,0x61,0x10,0x00},0x05,100}; //zigbeeָ���´�Ĭ�ϲ���
	u8 xdata timeStampArray[0x0B] = {0};
	bit resultSet = 0;
	u32 timeStamp_temp = timeStamp;
	
	timeStamp_temp += (3600UL * (long)sysTimeZone_H + 60UL * (long)sysTimeZone_M); //ʱ��

	timeStampArray[0] = (u8)((timeStamp_temp & 0x000000ff) >> 0);
	timeStampArray[1] = (u8)((timeStamp_temp & 0x0000ff00) >> 8);
	timeStampArray[2] = (u8)((timeStamp_temp & 0x00ff0000) >> 16);
	timeStampArray[3] = (u8)((timeStamp_temp & 0xff000000) >> 24);
	
	resultSet = zigb_VALIDA_INPUT((u8 *)default_param.zigbInit_reqCMD,
								  (u8 *)timeStampArray,
								  default_param.reqDAT_num,
								  (u8 *)default_param.zigbInit_REPLY,
								  default_param.REPLY_num,
								  2,	//2���޻ظ�Ϊʧ��
								  default_param.timeTab_waitAnsr);
	
#if(DEBUG_LOGOUT_EN == 1)	
//	{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�
//		u8 xdata log_buf[64];
//		
//		sprintf(log_buf, "sysTime set result:%d.\n", (int)resultSet);
//		PrintString1_logOut(log_buf);
//	}
#endif
	
	return resultSet;
}

///*zigbeeӲ����λ��ʼ��*///����
//bit ZigB_resetInit(void){

//#define zigbInit_loopTry 		3
//#define zigbInit_onceWait 	5000

//	u8 code initCmp_Frame[11] = {0xFE, 0x06, 0x41, 0x80, 0x01, 0x02, 0x00, 0x02, 0x06, 0x03, 0xC3};
//	
//	u8 	loop = 0;
//	u16 timeWait = 0;
//	
//	for(loop = 0; loop < zigbInit_loopTry; loop ++){
//	
//		zigbPin_RESET = 0;
//		delayMs(100);
//		zigbPin_RESET = 1;
//		
//		timeWait = zigbInit_onceWait;
//		while(timeWait --){
//		
//			delayMs(2);	//������ʱ
//			if(uartRX_toutFLG){
//			
//				uartRX_toutFLG = 0;
//				
//				if(!memcmp(datsRcv_ZIGB.rcvDats, initCmp_Frame, 11)){
//				
//					return 1;
//					
//				}else{
//					
//					delayMs(1);	//������ʱ
//				}
//			}
//		}
//	}
//	
//	return 0;
//}

///*zigbee��ʼ���Լ�*///����
//bit ZigB_inspectionSelf(void){	
//	
//#define	paramLen_zigbInspection 64
//	u8 xdata paramTX_temp[paramLen_zigbInspection] = {0};
//	
////	bit REQResult = 0;
//	
////	u8 code frameREQ_zigbStatusCheck[5] = {0xFE, 0x00, 0x27, 0x00, 0x27};	//zigb״̬��ѯָ��֡
////	u8 code cmdResp_zigbStatusCheck[2] 	= {0x67, 0x00};	//zigb״̬��ѯ��Ӧָ��
//	u8 code frameREQ_zigbJoinNWK[5] 	= {0xFE, 0x00, 0x26, 0x00, 0x26};	//zigb��������ָ��֡
//	u8 code cmdResp_zigbJoinNWK[2] 		= {0x45, 0xC0};	//zigb����������Ӧָ��
//	u8 datsResp_Len = 0;
//	
////	datsResp_Len = zigb_datsRequest(frameREQ_zigbStatusCheck, 5, cmdResp_zigbStatusCheck, paramTX_temp, 2, 500);
////	if(paramTX_temp[16] == 0x07)REQResult
//	
//	datsResp_Len = zigb_datsRequest(frameREQ_zigbJoinNWK, 5, cmdResp_zigbJoinNWK, paramTX_temp, 2, 5000);
//	if(paramTX_temp[4] == 0x07)return (zigb_clusterSet(13, 13) & zigb_clusterSet(13, 14));	//�豸ID 13���ն˵� 13��	
//	else{
//	
//		return 0;
//	}
//}

/*zigbee��������������*///������ ---�ŵ�Ĭ�ϵ����ŵ�
static 
void zigB_nwkJoinRequest(bit reJoin_IF){

#define	cmdNum_zigbNwkREQ	9	

	datsAttr_ZigbInit code ZigbInit_dats[cmdNum_zigbNwkREQ] = {

		{	{0x46,0x09},	{0},					0x00,	{0xFE,0x06,0x41,0x80,0x01,0x02,0x00,0x02,0x06,0x03,0xC3},	0x0B,	4000	},	//��λ(Ӳ��)
//		{	{0x46,0x09},	{0},					0x00,	{0xFE,0x06,0x41,0x80,0x00,0x02,0x00,0x02,0x06,0x03,0xC2},	0x0B,	4000	},	//��λ(�油)
		{	{0x46,0x09},	{0},					0x00,	{0xFE,0x06,0x41,0x80,0x01,0x02,0x00,0x02,0x06,0x03,0xC3},	0x0B,	4000	},	//��λ(���)
		{	{0x26,0x05},	{0x03,0x01,0x03},		0x03,	{0xFE,0x01,0x66,0x05,0x00,0x62},							0x06,	500		},	//�Ĵ�����ʼ����ȫ�����
		{	{0x46,0x09},	{0},					0x00,	{0xFE,0x06,0x41,0x80,0x01,0x02,0x00,0x02,0x06,0x03,0xC3},	0x0B,	4000	},	//���θ�λ(���)
		
//		{	{0x26,0x05},	{0x87,0x01,0x00},		0x03,	{0xFE,0x01,0x66,0x05,0x00,0x62},							0x06,	500		},	//��ɫ���ã�Э������
		{	{0x26,0x05},	{0x87,0x01,0x01},		0x03,	{0xFE,0x01,0x66,0x05,0x00,0x62},							0x06,	500		},	//��ɫ���ã�·������
//		{	{0x26,0x05},	{0x87,0x01,0x02},		0x03,	{0xFE,0x01,0x66,0x05,0x00,0x62},							0x06,	500		},	//��ɫ���ã��նˣ�
		
		{	{0x27,0x02},	{0xFF,0xFF},			0x02,	{0xFE,0x01,0x67,0x02,0x00,0x64},							0x06,	500		},	//PAN_ID�Ĵ�������
		{	{0x27,0x03},	{0x00,0x80,0x00,0x00},	0x04,	{0xFE,0x01,0x67,0x03,0x00,0x65},							0x06,	500		},	//�ŵ��Ĵ�������
//		{	{0x26,0x00},	{0},					0x00,	{0xFE,0x01,0x45,0xC0,0x09,0x8D},							0x06,	12000	},	//��ʼ�������Լȶ���ɫЭ������Э������Ӧ��
		{	{0x26,0x00},	{0},					0x00,	{0xFE,0x01,0x45,0xC0,0x07,0x83},							0x06,	8000	},	//��ʼ�������Լȶ���ɫЭ������·������Ӧ��
//		{	{0x26,0x00},	{0},					0x00,	{0xFE,0x01,0x45,0xC0,0x06,0x82},							0x06,	12000	},	//��ʼ�������Լȶ���ɫЭ�������ն���Ӧ��
		{	{0x26,0x08}, 	{0xFC,0xFF,0x00}, 		0x03,	{0xFE,0x01,0x66,0x08,0x00,0x6F}, 							0x06, 	150		},  //�ر�����
	};
	
	datsAttr_ZigbInit code defaultParam_clusterRegister = {{0x24,0x00},{0x0E,0x0D,0x00,0x0D,0x00,0x0D,0x00,0x01,0x00,0x00,0x01,0x00,0x00},0x0D,{0xFE,0x01,0x64,0x00,0x00,0x65},0x06,500};	//���ݴ�ע��,Ĭ�ϲ���
	u8 code frameResponseSubs_clusterRegister[6] = {0xFE,0x01,0x64,0x00,0xB8,0xDD}; //��Ӧ֡�油�������ݴ��Ѿ�ע��
	
#define	clusterNum_default 2
	datsAttr_clusterREG code cluster_Default[clusterNum_default] = {{13, 13}, {14, 13}};
	
#define	dataLen_zigbNwkREQ 64
	u8 xdata paramTX_temp[dataLen_zigbNwkREQ] = {0};
	
	static u8 step_CortexA = 0,
			  step_CortexB = 0;
	static u8 reactionLoop = 0;
	
	u8 datsTX_Len = 0;
	
	if(devStatus_switch.statusChange_IF){ //״̬ǿ���л�ʱ������ǰ��״̬�ھ�̬������ʼ�����ٽ����ⲿ�л�
	
		devStatus_switch.statusChange_IF = 0;
		devRunning_Status = devStatus_switch.statusChange_standBy;
		
		step_CortexA = 0;
		step_CortexB = 0;
		reactionLoop = 0;
		zigbPin_RESET = 1;
		
		return;
	}
	
	if(step_CortexA > (cmdNum_zigbNwkREQ + clusterNum_usr + clusterNum_default)){ //�ڲ�״̬���
	
		step_CortexA = 0;
		step_CortexB = 0;
		reactionLoop = 0;
		zigbPin_RESET = 1;
		
		sysTimeReales_counter = PERIOD_SYSTIMEREALES; //systime�����������ã���ֹ��ָ�������ͻ
		
		devRunning_Status = status_passiveDataRcv; //�ⲿ״̬�л�
		devTips_status = status_Normal; //tips״̬�л�
		
		return;
	}
	
	if(!reJoin_IF)if(step_CortexA == 0)step_CortexA = 7; //�Ƿ�Ϊ�����������������磬���򲻽���Ӳ����λ(Ӳ����λ�����±���ʱ�䱻����)
	if((step_CortexA == 7) || (step_CortexA == 0))sysTimeReales_counter	= PERIOD_SYSTIMEREALES; //�������ؼ�ָ��ܱ�����ָ���ϣ�Ӳ����λ �� ����ʱ �ж�����ָ���´
	if(step_CortexA == 0){ //����ָ��_Ӳ����λ:<0>
	
		switch(step_CortexA){
		
			case 0:{ //����ָ�Ӳ����λ
			
				switch(step_CortexB){
				
					case 0:{ //����һ��Ӳ������100ms
					
						zigbPin_RESET = 0;
						zigbNwkAction_counter = 200;
						step_CortexB = 1;
					
					}break;
				
					case 1:{ //�������Ӳ��������Ϻ�ȷ��Ӧ��֡ʱ��
					
						if(!zigbNwkAction_counter){ //�������ȴ�
						
							zigbPin_RESET = 1;
							zigbNwkAction_counter = 6000;
							step_CortexB = 2;
						}
						
					}break;
					
					case 2:{ //�������ȷ��Ӧ��֡
						
						if(!zigbNwkAction_counter)step_CortexB = 0; //�������ȴ���Ӧ
					
						if(uartRX_toutFLG){
						
							uartRX_toutFLG = 0;
							
							if(memmem(datsRcv_ZIGB.rcvDats, COM_RX1_Lenth, ZigbInit_dats[0].zigbInit_REPLY, ZigbInit_dats[0].REPLY_num)){
			
								step_CortexB = 0;
								reactionLoop = 0;
								step_CortexA ++;
							}
						}
						
					}break;
				}
				
			}break;
		}
	}else
	if(step_CortexA > 0 && step_CortexA < cmdNum_zigbNwkREQ){ //����ָ��:<1 - 9>
		
//		if(!reJoin_IF)if(step_CortexA == 2)step_CortexA = 7;	//�Ƿ�Ϊ�����������������磬����ֻ���б������缤��

		switch(step_CortexB){
		
			case 0:{
				
				if(reactionLoop > 2){
					
					reactionLoop = 0;
					step_CortexA = 0;
					break;
				}
				
				datsTX_Len = ZigB_TXFrameLoad(paramTX_temp, 
											  ZigbInit_dats[step_CortexA].zigbInit_reqCMD, 
											  2, 
											  ZigbInit_dats[step_CortexA].zigbInit_reqDAT, 
											  ZigbInit_dats[step_CortexA].reqDAT_num);
				
				uartZigB_datsSend(paramTX_temp, datsTX_Len);
				
				zigbNwkAction_counter = ZigbInit_dats[step_CortexA].timeTab_waitAnsr;
				step_CortexB = 1;
				
			}break;
				
			case 1:{
				
				if(!zigbNwkAction_counter){ //�������ȴ���Ӧ
				
					reactionLoop ++;
					step_CortexB = 0;
				}
				else
				if(uartRX_toutFLG){
				
					uartRX_toutFLG = 0;
					
					if(memmem(datsRcv_ZIGB.rcvDats, COM_RX1_Lenth, ZigbInit_dats[step_CortexA].zigbInit_REPLY, ZigbInit_dats[step_CortexA].REPLY_num)){
					
						step_CortexB = 0;
						reactionLoop = 0;
						step_CortexA ++;
					}
				}
				
			}break;
		}
		
	}else
	if(step_CortexA >= cmdNum_zigbNwkREQ){ //����ָ��_����ͨ�Ŵ�ע��:<10 - n>
		
		u8 datsREG_cluster[16] = {0};
		memcpy(datsREG_cluster, defaultParam_clusterRegister.zigbInit_reqDAT, defaultParam_clusterRegister.reqDAT_num);
		if(step_CortexA < (cmdNum_zigbNwkREQ + clusterNum_default)){ //Ĭ��ͨ�Ŵز�����װ
		
			datsREG_cluster[0] = cluster_Default[step_CortexA - cmdNum_zigbNwkREQ].endpoint;
			datsREG_cluster[3] = (u8)((cluster_Default[step_CortexA - cmdNum_zigbNwkREQ].devID & 0x00ff) >> 0);
			datsREG_cluster[4] = (u8)((cluster_Default[step_CortexA - cmdNum_zigbNwkREQ].devID & 0xff00) >> 8);
			
		}else{	//�û�ͨ�Ŵأ����أ�ע�������װ
		
			if((CTRLEATHER_PORT[step_CortexA - cmdNum_zigbNwkREQ - clusterNum_usr] >= 0x10) && (CTRLEATHER_PORT[step_CortexA - cmdNum_zigbNwkREQ - clusterNum_usr] < 255)){ //ͨ�Ŵض˿ںϷ����ж�
			
				datsREG_cluster[0] = CTRLEATHER_PORT[step_CortexA - cmdNum_zigbNwkREQ - clusterNum_usr];
				datsREG_cluster[3] = zigbDatsDefault_ClustID; //Ĭ�ϴ�ID <LSB>
				datsREG_cluster[4] = 0; //Ĭ�ϴ�ID <MSB>
				
			}else{
			
				step_CortexA ++;
				return;
			}
		}
	
		switch(step_CortexB){
		
			case 0:{
				
				if(reactionLoop > 2){
					
					reactionLoop = 0;
					step_CortexA = 0;
					break;
				}
				
				datsTX_Len = ZigB_TXFrameLoad(paramTX_temp, 
											  defaultParam_clusterRegister.zigbInit_reqCMD, 
											  2, 
											  datsREG_cluster, 
											  defaultParam_clusterRegister.reqDAT_num);
				
				uartZigB_datsSend(paramTX_temp, datsTX_Len);
				
				zigbNwkAction_counter = defaultParam_clusterRegister.timeTab_waitAnsr;
				step_CortexB = 1;
				
			}break;
				
			case 1:{
				
				if(!zigbNwkAction_counter){ //�������ȴ���Ӧ
				
					reactionLoop ++;
					step_CortexB = 0;
				}
				else
				if(uartRX_toutFLG){
				
					uartRX_toutFLG = 0;
					
					if(memmem(datsRcv_ZIGB.rcvDats, COM_RX1_Lenth, defaultParam_clusterRegister.zigbInit_REPLY, defaultParam_clusterRegister.REPLY_num) || //Ԥ����Ӧ
					   memmem(datsRcv_ZIGB.rcvDats, COM_RX1_Lenth, frameResponseSubs_clusterRegister, 6)){ //�油��Ӧ
					
						step_CortexB = 0;
						reactionLoop = 0;
						step_CortexA ++;
					}
				}
				
			}break;
		}
	}
}

/*zigbee�������ݷ��͸�ʽ����װ*/
static 
u8 zigb_datsLoad_datsSend(u8  frame_Temp[NORMALDATS_DEFAULT_LENGTH],
						  u16 DstAddr,
						  u8  portPoint,
						  u8  dats[],
						  u8  datsLen){
	
	u8 code zigbCMD_DatsSend[2] = {0x24, 0x01};
	
#define zigbDatsSend_datsTransLen	72
	u8 xdata buf_datsLOAD[zigbDatsSend_datsTransLen] = {0};
	u8 datsTX_Len = 0;
							  
	memset(frame_Temp, 0, NORMALDATS_DEFAULT_LENGTH * sizeof(u8));	

	//����֡��װ
	buf_datsLOAD[0] = (u8)((DstAddr & 0x00ff) >> 0);
	buf_datsLOAD[1] = (u8)((DstAddr & 0xff00) >> 8);
	buf_datsLOAD[2] = portPoint;
	buf_datsLOAD[3] = portPoint;
	buf_datsLOAD[4] = zigbDatsDefault_ClustID;
	buf_datsLOAD[6] = zigbDatsDefault_TransID;
	buf_datsLOAD[7] = zigbDatsDefault_Option;
	buf_datsLOAD[8] = zigbDatsDefault_Radius;
	buf_datsLOAD[9] = datsLen;
	memcpy(&buf_datsLOAD[10], dats, datsLen);	
	
	return ZigB_TXFrameLoad(frame_Temp, (u8 *)zigbCMD_DatsSend, 2, buf_datsLOAD, datsLen + 10);
}

/*zigbee�������ݷ�������״̬*///������
static
void dataTransRequest_datsSend(void){

	u8 xdata buf_datsTX[NORMALDATS_DEFAULT_LENGTH] = {0};
	u8 datsTX_Len = 0;
	
#define zigbDatsSend_datsRespLen	64
	u8 xdata buf_datsRX[zigbDatsSend_datsRespLen] = {0};
	u8 datsRX_Len = 0;
	
#define zigbDatsSend_ASR_datsLen	3
	u8 		ASR_dats[zigbDatsSend_ASR_datsLen] = {0};
	u8 code ASR_cmd[2] = {0x44,0x80};	//����ZNPЭ���ȷ�Ϸ�����Ӧ
	
#define resCODE_datsSend_NOROUTER 0xCD	//���ݷ���Э�����Ӧ����-·��ʧ��
#define resCODE_datsSend_NOREMOTE 0xE9	//���ݷ���Э�����Ӧ����-�Է�������
#define resCODE_datsSend_TIMEOUT  0x01  //���ݷ���Э�����Ӧ����-���ͳ�ʱ
#define resCODE_datsSend_SUCCESS  0x00  //���ݷ���Э�����Ӧ����-���ͳɹ�
	static u8 datsTrans_respondCode = 0; //���������Ӧ��
	
	static u8 step = 0;
	static u8 reactionLoop = 0;
	
	if(devStatus_switch.statusChange_IF){	//״̬ǿ���л�ʱ������ǰ��״̬�ھ�̬������ʼ�����ٽ����ⲿ�л�
	
		devStatus_switch.statusChange_IF = 0;
		devRunning_Status = devStatus_switch.statusChange_standBy;
		
		step = 0;
		reactionLoop = 0;
		
		return;
	}
	
	//����֡��װ_����
	ASR_dats[0] = 0x00; //���ͳɹ���Ӧ����
	ASR_dats[1] = datsSend_request.portPoint;
	ASR_dats[2] = zigbDatsDefault_TransID;
	datsRX_Len = ZigB_TXFrameLoad(buf_datsRX, (u8 *)ASR_cmd, 2, ASR_dats, zigbDatsSend_ASR_datsLen);
	
	datsTX_Len = zigb_datsLoad_datsSend(buf_datsTX, datsSend_request.nwkAddr, datsSend_request.portPoint, datsSend_request.datsTrans.dats, datsSend_request.datsTrans.datsLen);
	
	switch(step){
	
		case 0:{ //��Ӧ���վ�����������Ӧʱ��
			
			if(reactionLoop > 3){
				
				datsTrans_respondCode = resCODE_datsSend_TIMEOUT; //��Ӧ���Ϊ��ʱ
				
				reactionLoop = 0;
				step = 4;
				
				break;
			}
		
			zigbPin_RESET = 1; //�����������λ����
			uartZigB_datsSend(buf_datsTX, datsTX_Len);
			zigbNwkAction_counter = 1000; //Ĭ��Э�����Ӧʱ��<ʱ��̫���޷��յ�����Ľ���״̬��Ӧָ�ֻ���յ�ϵͳ��Ӧ>
			step = 1;
			
		}break;
		
		case 1:{ //�������ȴ�ϵͳ��Ӧ
		
			if(!zigbNwkAction_counter){
			
				reactionLoop ++;
				step = 0;
			}
			else{
				
				if(uartRX_toutFLG){
				
					uartRX_toutFLG = 0;

					if(memmem(datsRcv_ZIGB.rcvDats, COM_RX1_Lenth, buf_datsRX, datsRX_Len)){
					
						if(datsRcv_respond.datsTrans.datsLen == 0){
						
							step = 3;
							
						}else{
						
							step = 2;
							zigbNwkAction_counter = 500; //Ĭ��Զ����Ӧʱ��<�Է��ڵ���Ӧ>
						}
						
					}else{	
						
						datsTrans_respondCode = datsRcv_ZIGB.rcvDats[4]; //������Ӧ��װ��
						
//#if(DEBUG_LOGOUT_EN == 1)
//						{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�
//							u8 xdata log_buf[64]; //���ݴ���ʧ��Э�����Ӧ�����ӡ
//							
//							sprintf(log_buf, "dats_TX fail code: %02X %02X %02X.\n", (int)datsRcv_ZIGB.rcvDats[2], (int)datsRcv_ZIGB.rcvDats[3], (int)datsRcv_ZIGB.rcvDats[4]);
//							PrintString1_logOut(log_buf);
//						}	
//#endif				
					}
				}
			}
			
		}break;
		
		case 2:{ //�������ȴ�Զ����Ӧ

			if(!zigbNwkAction_counter){
			
				reactionLoop ++;
				step = 0;
			}
			else{
				
				if(uartRX_toutFLG){
					
					u16 idata datsFrom_addr = ((u16)(datsRcv_ZIGB.rcvDats[9]) << 8) | ((u16)(datsRcv_ZIGB.rcvDats[8]) << 0); //���ݷ��ͷ������ַ
					u8 	idata dstPoint =  datsRcv_ZIGB.rcvDats[11];	//Զ��	
					
					uartRX_toutFLG = 0;

					if(!memcmp(&(datsRcv_ZIGB.rcvDats[21]), datsRcv_respond.datsTrans.dats, datsRcv_respond.datsTrans.datsLen) && 
					   (datsRcv_respond.nwkAddr == datsFrom_addr) &&
						(datsRcv_respond.portPoint == dstPoint)){
					
						step = 3;
					}
				}
			}
			
		}break;
		
		case 3:{ //��Ӧ�ɹ�
		
			if(reConnectAfterDatsReq_IF){ //��Լ���ע�ụ��������� ״̬�л�
			
				reConnectAfterDatsReq_IF = 0;
				devRunning_Status = status_nwkReconnect;
				
			}else{ 
			
				devRunning_Status = status_passiveDataRcv;
			}
			
			reactionLoop = 0;
			step = 0;
			
		}break;
		
		case 4:{ //��Ӧʧ��
		
			if(reConnectAfterDatsReq_IF){ //��Լ���ע�ụ��������� ״̬�л�
			
				reConnectAfterDatsReq_IF = 0;
				devRunning_Status = status_nwkReconnect;
				
			}else{ 
			
				devRunning_Status = status_passiveDataRcv;
			}
			
			//������ݴ���ʧ����Ӧ�����������ѡ���������������ʱ��Э�����豸��gg
			if(datsTrans_respondCode){ 
				
				switch(datsTrans_respondCode){ //��Ӧʧ�������
				
					case resCODE_datsSend_NOROUTER:
					case resCODE_datsSend_NOREMOTE:
					case resCODE_datsSend_SUCCESS:{
					
						devTips_nwkZigb = nwkZigb_outLine; //��ʱֻ��ʧ����ʾ��������������
						
					}break;
					
					default:{
					
						devTips_nwkZigb = nwkZigb_outLine; //��ʱֻ��ʧ����ʾ��������������
						
					}break;
				}
				
				datsTrans_respondCode = 0;
			}
			
			reactionLoop = 0;
			step = 0;
			
		}break;
			
		default:{
		
			step = 4;
			
		}break;
	}
}

/*zigbee����������ݽ���*/
static 
void dataParing_zigbSysCtrl(u8 datsFrame[]){

	frame_zigbSysCtrl xdata dats = {0};
	
	dats.command = datsFrame[0];
	memcpy(dats.dats, &datsFrame[2], datsFrame[1]);
	dats.datsLen = datsFrame[1];
	
	switch(dats.command){
	
		case ZIGB_SYSCMD_NWKOPEN:{ //���翪��
			
			bit resultSet = 0;
			
			resultSet = ZigB_nwkOpen(1, dats.dats[0]);
			
		}break;
		
		case ZIGB_SYSCMD_TIMESET:{ //ϵͳʱ���趨
		
			bit resultSet = 0;
			u32 time_Temp = 0UL;
			
			time_Temp |= (u32)dats.dats[0] << 0;
			time_Temp |= (u32)dats.dats[1] << 8;
			time_Temp |= (u32)dats.dats[2] << 16;
			time_Temp |= (u32)dats.dats[3] << 24;
			
			resultSet = zigB_sysTimeSet(time_Temp - 946713600UL); //zigbeeʱ�����unix��Ԫ946713600<2000/01/01 00:00:00>��ʼ����
			
		}break;
			
		default:break;
	}
}

/*zigbee�������ת�����ݽ���*/
static 
void dataParing_Nomal(u8 datsParam[], u16 nwkAddr_from, u8 port_from){
	
#define	dataLen_dataParingNomal 96
	u8 xdata paramTX_temp[dataLen_dataParingNomal] = {0};
	
	bit dataFromRemote_IF = 0;	//�Ƿ�Ϊ�����������ݱ�־

	/*��Ʒ����Э��˶�_�������*///�����´�
	switch(datsParam[0]){
	
		/*Զ��*/
		case ZIGB_FRAMEHEAD_CTRLREMOTE:{
			
			dataFromRemote_IF = 1;
			
			memcpy(MAC_ID_DST, &datsParam[7], 6);
			memcpy(&datsParam[1], &datsParam[13], datsRcv_ZIGB.rcvDats[20] - 13);
		}
		
		/*����*/
		case ZIGB_FRAMEHEAD_CTRLLOCAL:{
			
			bit frameCheck_Done = 0; //���ݼ��ϸ��־
			
			{
				bit frameCodeCheck_PASS = 0; //У������ͨ����־
				bit frameMacCheck_PASS  = 0; //mac��ַ�����ͨ����־
				
				if(datsParam[4] == frame_Check(&datsParam[5], 28))frameCodeCheck_PASS = 1; //У������
				if(!memcmp(&datsParam[5], &MAC_ID[1], 5))frameMacCheck_PASS = 1; //MAC���

				if(datsParam[3] == FRAME_MtoZIGBCMD_cmdConfigSearch){ //����ָ���MAC���
				
					frameMacCheck_PASS = 1;
					
				}else
				if((datsParam[3] == FRAME_MtoZIGBCMD_cmdCfg_swTim) || //����ָ���У������
				   (datsParam[3] == FRAME_MtoZIGBCMD_cmdswTimQuery)){
				   
					frameCodeCheck_PASS = 1;
				}
				   
				if(frameCodeCheck_PASS && frameCodeCheck_PASS)frameCheck_Done = 1;
			}
			   
			if(frameCheck_Done){ //֡���ͨ������ʼ��������������Ӧ
				
				bit respond_IF 		= 0;	//�Ƿ�ظ�
				bit specialCmd_IF 	= 0;	//�Ƿ�Ϊ����ָ�����ָ��ռ�ÿ���������һ���ֽڣ�
				
#if(DEBUG_LOGOUT_EN == 1)
				{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�
					u8 xdata log_buf[64];
					
					sprintf(log_buf, "cmdComing:%02X.\n", (int)datsParam[3]);
					PrintString1_logOut(log_buf);
				}			
#endif		
				memset(paramTX_temp, 0, sizeof(u8) * dataLen_dataParingNomal);
			
				switch(datsParam[3]){
				
					case FRAME_MtoZIGBCMD_cmdConfigSearch:{
						
						if(!deviceLock_flag){ //�豸�Ƿ�����
							
							u16 xdata panid_Temp = ZigB_getPanIDCurrent(); //���ûظ����PANID
							
							paramTX_temp[14] = (u8)((panid_Temp & 0xFF00) >> 8);
							paramTX_temp[15] = (u8)((panid_Temp & 0x00FF) >> 0);
						
							respond_IF 		= 1; //��Ӧ�ظ�
							specialCmd_IF 	= 0;
							
						}else{
						
							
						}
						
					}break;
					
					case FRAME_MtoZIGBCMD_cmdControl:{
						
						swCommand_fromUsr.objRelay = datsParam[11];
						swCommand_fromUsr.actMethod = relay_OnOff;
						
						respond_IF 		= 1; //��Ӧ�ظ�
						specialCmd_IF 	= 0;	

						paramTX_temp[11] = datsParam[11];						
						
					}break;
						
					case FRAME_MtoZIGBCMD_cmdQuery:{}break;
						
					case FRAME_MtoZIGBCMD_cmdInterface:{}break;
						
					case FRAME_MtoZIGBCMD_cmdReset:{}break;
						
					case FRAME_MtoZIGBCMD_cmdDevLockON:{
					
						//���ݴ���������Ӧ
						{
							u8 deviceLock_IF = 1;
							
							deviceLock_flag  = 1;
							coverEEPROM_write_n(EEPROM_ADDR_deviceLockFLAG, &deviceLock_IF, 1);
						}		
						
					}break;
						
					case FRAME_MtoZIGBCMD_cmdDevLockOFF:{
					
						//���ݴ���������Ӧ
						{
							u8 deviceLock_IF = 0;
							
							deviceLock_flag  = 0;
							coverEEPROM_write_n(EEPROM_ADDR_deviceLockFLAG, &deviceLock_IF, 1);
						}	
					
					}break;
						
					case FRAME_MtoZIGBCMD_cmdswTimQuery:{
					
						//����ظ�
						switch(datsParam[13]){ //���������
						
							case 0: /*��λ���ڶ�ʱ��ʱ���0����Э��*/
							case cmdConfigTim_normalSwConfig:{
							
								u8 loop = 0;
							
								//������Ӧ���ظ�
								EEPROM_read_n(EEPROM_ADDR_swTimeTab, &paramTX_temp[14], 12);	//��ʱ��ظ���װ
								
								//�ظ����ݶ��δ������һ���Զ�ʱ���ݣ�
								for(loop = 0; loop < 4; loop ++){
								
									if(swTim_onShoot_FLAG & (1 << loop)){
										
										paramTX_temp[14 + loop * 3] &= 0x80;
									}
								}
										
								specialCmd_IF = 1; //����ռλָ��
								
							}break;
							
							case cmdConfigTim_onoffDelaySwConfig:{
							
								if(!delayCnt_onoff)paramTX_temp[14] = 0;
								else paramTX_temp[14] = delayPeriod_onoff - (u8)(delayCnt_onoff / 60);
								paramTX_temp[15] = delayUp_act;
								
							}break;
							
							case cmdConfigTim_closeLoopSwConfig:{
							
								paramTX_temp[14] = delayPeriod_closeLoop;
								
							}break;
							
							case cmdConfigTim_nightModeSwConfig:{  
							
								EEPROM_read_n(EEPROM_ADDR_TimeTabNightMode, &paramTX_temp[14], 6);	//ҹ��ģʽ��ʱ��ظ���װ
								
								(deviceLock_flag)?(paramTX_temp[12] |= 0x01):(paramTX_temp[12] &= ~0x01);
								(ifNightMode_sw_running_FLAG)?(paramTX_temp[12] |= 0x02):(paramTX_temp[12] &= ~0x02);
								
							}break;
							
							default:break;
						}
						
						paramTX_temp[13] = datsParam[13]; //��ʱ������ͬ���ظ�
						
						respond_IF = 1; //��Ӧ�ظ�ʹ��
						
					}break;
						
					case FRAME_MtoZIGBCMD_cmdConfigAP:{}break;
						
					case FRAME_MtoZIGBCMD_cmdBeepsON:{ //ҹ��ģʽ��
					
						u8 datsTemp = 0;
						
						EEPROM_read_n(EEPROM_ADDR_TimeTabNightMode, &datsTemp, 1);
						datsTemp &= ~0x7f; //ҹ��ģʽ��ʱ��洢,ȡ��ͷ�ֽ�ȫռ��,ʧ��ȫ��
						coverEEPROM_write_n(EEPROM_ADDR_TimeTabNightMode, &datsTemp, 1);
						
						respond_IF = 1; //��Ӧ�ظ�ʹ��
						
					}break;
						
					case FRAME_MtoZIGBCMD_cmdBeepsOFF:{ //ҹ��ģʽ��
					
						u8 datsTemp = 0;
						
						EEPROM_read_n(EEPROM_ADDR_TimeTabNightMode, &datsTemp, 1);
						datsTemp |= 0x7f; //ҹ��ģʽ��ʱ��洢,ͷ�ֽ�ȫռ��,ǿ��ȫ��
						coverEEPROM_write_n(EEPROM_ADDR_TimeTabNightMode, &datsTemp, 1);	
						
						respond_IF = 1; //��Ӧ�ظ�ʹ��
						
					}break;
						
					case FRAME_MtoZIGBCMD_cmdftRecoverRQ:{
					
						respond_IF = 1;
						
					}break;
						
					case FRAME_MtoZIGBCMD_cmdRecoverFactory:{
					
						Factory_recover();
					
					}break;
						
					case FRAME_MtoZIGBCMD_cmdCfg_swTim:{
						
						u8 loop = 0;
						
						switch(datsParam[13]){ //��ʱ���ݴ�������,���ദ��
						
							case cmdConfigTim_normalSwConfig:{	/*��ͨ��ʱ*/
								
								for(loop = 0; loop < 4; loop ++){
								
									if(datsParam[14 + loop * 3] == 0x80){	/*һ���Զ�ʱ�ж�*///��ռλΪ�գ�����ʱ�����򿪣�˵����һ����
									
										swTim_onShoot_FLAG 	|= (1 << loop);	//һ���Զ�ʱ��־����
										datsParam[14 + loop * 3] |= (1 << (datsParam[31] - 1)); //ǿ�н��е�ǰ��ռλ������ִ����Ϻ����
									}
								}
								coverEEPROM_write_n(EEPROM_ADDR_swTimeTab, &datsParam[14], 12);	//��ʱ��

							}break;
							
							case cmdConfigTim_onoffDelaySwConfig:{	/*������ʱ*/
							
								if(datsParam[14]){
								
									ifDelay_sw_running_FLAG |= (1 << 1);
									delayPeriod_onoff 		= datsParam[14];
									
									delayUp_act		  		= datsParam[15];
									
									delayCnt_onoff			= 0;
									
								}else{
								
									ifDelay_sw_running_FLAG &= ~(1 << 1);
									delayPeriod_onoff 		= 0;
									delayCnt_onoff			= 0;
								}
								
							}break;
							
							case cmdConfigTim_closeLoopSwConfig:{	/*��ɫ����(�Զ�ѭ���ر�)*/
							
								if(datsParam[14]){
								
									ifDelay_sw_running_FLAG |= (1 << 0);
									delayPeriod_closeLoop	= datsParam[14];
									delayCnt_closeLoop		= 0;
								}else{
								
									ifDelay_sw_running_FLAG &= ~(1 << 0);
									delayPeriod_closeLoop	= 0;
									delayCnt_closeLoop		= 0;
								}
								
								coverEEPROM_write_n(EEPROM_ADDR_swDelayFLAG, &ifDelay_sw_running_FLAG, 1);
								coverEEPROM_write_n(EEPROM_ADDR_periodCloseLoop, &delayPeriod_closeLoop, 1);
								
							}break;		

							case cmdConfigTim_nightModeSwConfig:{  /*ҹ��ģʽ �������*/
							
								coverEEPROM_write_n(EEPROM_ADDR_TimeTabNightMode, &datsParam[14], 6);	//ҹ��ģʽ��ʱ��洢
								
							}break;
							
							default:break;
						}
						
						respond_IF = 1; //��Ӧ�ظ�ʹ��
						
					}break;
					
					case FRAME_MtoZIGBCMD_cmdCfg_ctrlEachO:{
					
						u8 loop = 0;
						u8 effective_oprate = datsParam[12]; //��Ч��������ռλ��ȡ
						
						for(loop = 0; loop < clusterNum_usr; loop ++){
						
							if((effective_oprate >> loop) & 0x01){ //��Ч�����ж�
							
								coverEEPROM_write_n(EEPROM_ADDR_portCtrlEachOther + loop, &datsParam[14 + loop], 1);
								CTRLEATHER_PORT[loop] = datsParam[14 + loop];
								reConnectAfterDatsReq_IF = 1; //����ע�ụ��ͨѶ�ض˿�
							}
						}
						
						respond_IF = 1; //��Ӧ�ظ�ʹ��
					
					}break;
					
					case FRAME_MtoZIGBCMD_cmdQue_ctrlEachO:{
					
						u8 loop = 0;
						
						for(loop = 0; loop < clusterNum_usr; loop ++){
						
							EEPROM_read_n(EEPROM_ADDR_portCtrlEachOther + loop, &paramTX_temp[14 + loop], 1);
						}
						
						respond_IF = 1; //��Ӧ�ظ�ʹ��
					
					}break;
						
					case FRAME_MtoZIGBCMD_cmdCfg_ledBackSet:{
					
						coverEEPROM_write_n(EEPROM_ADDR_ledSWBackGround, &datsParam[14], 1);
						coverEEPROM_write_n(EEPROM_ADDR_ledSWBackGround + 1, &datsParam[15], 1);
						tipsInsert_swLedBKG_ON 	= datsParam[14];
						tipsInsert_swLedBKG_OFF = datsParam[15];
						
						respond_IF = 1; //��Ӧ�ظ�ʹ��
					
					}break;
					
					case FRAME_MtoZIGBCMD_cmdQue_ledBackSet:{
					
						EEPROM_read_n(EEPROM_ADDR_ledSWBackGround, &paramTX_temp[14], 1);
						EEPROM_read_n(EEPROM_ADDR_ledSWBackGround + 1, &paramTX_temp[15], 1);
						
						respond_IF = 1; //��Ӧ�ظ�ʹ��
					
					}break;
					
					case FRAME_MtoZIGBCMD_cmdCfg_scenarioSet:{
						
						u16 xdata panid_Temp = ZigB_getPanIDCurrent(); //���ûظ����PANID
					
						bit opt_result = swScenario_oprateSave(datsParam[12], datsParam[14]);
						if(opt_result)paramTX_temp[12] = 0;
						else paramTX_temp[12] = 0x0A; //����������Ч�ظ��������洢������
						
						paramTX_temp[14] = (u8)((panid_Temp & 0xFF00) >> 8);
						paramTX_temp[15] = (u8)((panid_Temp & 0x00FF) >> 0);
						
						respond_IF = 1; //��Ӧ�ظ�ʹ�ܣ����ش洢�ѱ�ռ����
					
					}break;
					
					case FRAME_MtoZIGBCMD_cmdCfg_scenarioCtl:{
						
						u8 sw_Act = swScenario_oprateCheck(datsParam[12]);
						if(sw_Act != SW_SCENCRAIO_ACTINVALID){ //����������Ч����λ
							
							swCommand_fromUsr.actMethod = relay_OnOff;
							swCommand_fromUsr.objRelay = sw_Act;
						
							paramTX_temp[12] = 0;
							
						}else{ //���޷���������Ч����λ
						
							paramTX_temp[12] = 0x0A; //����������Ч�ظ����������޷���������
						}
					
						respond_IF = 1; //��Ӧ�ظ�ʹ��
					
					}break;
					
					case FRAME_MtoZIGBCMD_cmdCfg_scenarioDel:{
						
						swScenario_oprateDele(datsParam[12]);
						paramTX_temp[12] = 0;
					
						respond_IF = 1; //��Ӧ�ظ�ʹ��
					
					}break;
					
					default:{
					
						respond_IF = 0;
					
					}break;
				}
				
				/*�ظ���Ӧ*/
				if(respond_IF){ //���ݰ��ظ���Ӧ����
				
					u8 datsTX_Len = 0;
					
					respond_IF = 0;
					
					datsTX_Len = dtasTX_loadBasic_CUST(dataFromRemote_IF,
													   paramTX_temp,
													   33,
													   FRAME_TYPE_StoM_RCVsuccess,
													   datsParam[3],
													   specialCmd_IF);
					
					heartBeatCount = 1; //�ظ���Ӧ����һ������
					
					datsSend_request.nwkAddr = nwkAddr_from;
					datsSend_request.portPoint = port_from;
					memset(datsSend_request.datsTrans.dats, 0, DATBASE_LENGTH * sizeof(u8));
					memcpy(datsSend_request.datsTrans.dats, paramTX_temp, datsTX_Len);
					datsSend_request.datsTrans.datsLen = datsTX_Len;
					datsRcv_respond.datsTrans.datsLen = 0;
					devRunning_Status = status_dataTransRequestDatsSend;
				}
			}
		}break;
		
		/*����_��������*/
		case ZIGB_FRAMEHEAD_HEARTBEAT:{
		
			
			
		}break;
		
		/*����_��������*///internet���ߣ�����zigb��������
		case ZIGB_FRAMEHEAD_HBOFFLINE:{
		
			
			
		}break;
		
		default:{}break;
	}
}

/*zigbee���߳�*///������������200ms�ĺ�������Ϊ״̬�����У�����С��200ms����������ά�֣�����״̬�����ӶȼӴ�
void thread_dataTrans(void){
	
	u8 code cmd_datsComing[2] = {0x44, 0x81};

#define	dataLen_zigbDatsTrans 96
	u8 xdata paramTX_temp[dataLen_zigbDatsTrans] = {0};
	u8 xdata paramRX_temp[dataLen_zigbDatsTrans] = {0};
	
	static bit heartBeat_cmdFLG = 0; //������ż��־
	
	/*zigb���߳�ϵͳʱ�����*/
	if(!sysTimeReales_counter){ 
	
		sysTimeReales_counter = PERIOD_SYSTIMEREALES;
		getSystemTime_reales();
	}
	
	/*zigb���߳�״̬��������״̬��־����*/
	switch(devRunning_Status){
	
		case status_passiveDataRcv:{
			
			if(devStatus_switch.statusChange_IF){ //״̬ǿ���л�ʱ������ǰ��״̬�ھ�̬������ʼ�����ٽ����ⲿ�л�
			
				devStatus_switch.statusChange_IF = 0;
				devRunning_Status = devStatus_switch.statusChange_standBy;
				
				break;
			}
			
			{/*��ʼ��ʱ�丳ֵ*///��������ֵһ��
				static bit FLG_timeSetInit = 1;
				
				if(FLG_timeSetInit){
				
					FLG_timeSetInit = 0;
					zigB_sysTimeSet(1533810700UL - 946713600UL); //zigbeeʱ�����unix��Ԫ946713600<2000/01/01 00:00:00>��ʼ����
				}
			}
	
			//--------------------------------��״̬������--------------------------------------------------------//
			if(heartBeatCycle_FLG){
			
				heartBeatCycle_FLG = 0;
				heartBeat_cmdFLG = !heartBeat_cmdFLG;
				
				paramTX_temp[0] = ZIGB_FRAMEHEAD_HEARTBEAT;
				paramTX_temp[1] = 14;
				(heartBeat_cmdFLG)?(paramTX_temp[2] = FRAME_HEARTBEAT_cmdOdd):(paramTX_temp[2] = FRAME_HEARTBEAT_cmdEven);
				memcpy(&paramTX_temp[4], &MAC_ID[1], 5);
				
				if(heartBeat_cmdFLG){
				
					
				
				}else{
				
					
				}
				
				datsSend_request.nwkAddr = 0;
				datsSend_request.portPoint = 13;
				memset(datsSend_request.datsTrans.dats, 0, DATBASE_LENGTH * sizeof(u8));
				memcpy(datsSend_request.datsTrans.dats, paramTX_temp, 14);
				datsSend_request.datsTrans.datsLen = 14;
				datsRcv_respond.datsTrans.datsLen = 0;
				devRunning_Status = status_dataTransRequestDatsSend;
				
				return;
	
			}memset(paramTX_temp, 0, sizeof(u8) * dataLen_zigbDatsTrans);
			
			//--------------------------------��״̬������ͬ��---------------------------------------------------//
			if(EACHCTRL_realesFLG){
			
				if(devRunning_Status == status_passiveDataRcv){
				
					u8 idata loop;
					
					for(loop = 0; loop < 3; loop ++){ //��������λ�ֱ��ж�
					
						if(EACHCTRL_realesFLG & (1 << loop)){ //������Чλ�ж�
						
							EACHCTRL_realesFLG &= ~(1 << loop); //������Чλ����
							
							paramTX_temp[0] = (status_Relay >> loop) & 0x01; //����״̬��װ
							
							if((CTRLEATHER_PORT[loop] > 0x10) && CTRLEATHER_PORT[loop] < 0xFF){ //�Ƿ�Ϊ��Ч���ض˿�
							
								datsSend_request.nwkAddr = 0xffff;
								datsSend_request.portPoint = CTRLEATHER_PORT[loop];
								memset(datsSend_request.datsTrans.dats, 0, DATBASE_LENGTH * sizeof(u8));
								memcpy(datsSend_request.datsTrans.dats, paramTX_temp, 1);
								datsSend_request.datsTrans.datsLen = 1;
								datsRcv_respond.datsTrans.datsLen = 0;
								devRunning_Status = status_dataTransRequestDatsSend;
								
								break; //˳��ִ�У���ִ����break��ÿ���ܵ�������ִ��һ����Ч����
							}
						}
					}
				}	
			}memset(paramTX_temp, 0, sizeof(u8) * dataLen_zigbDatsTrans);
			
			//--------------------------------��״̬�����ݽ�����Ӧ-----------------------------------------------//
			if(uartRX_toutFLG){ //���ݽ���(֡��ʱ)
				
				uartRX_toutFLG = 0;
				
				/*Zigbeeһ��Э��˶Խ���*/
				if((datsRcv_ZIGB.rcvDats[0] == ZIGB_FRAME_HEAD) &&
					!memcmp(&datsRcv_ZIGB.rcvDats[2], cmd_datsComing, 2)){
					
					u16 idata datsFrom_addr = ((u16)(datsRcv_ZIGB.rcvDats[9]) << 8) | ((u16)(datsRcv_ZIGB.rcvDats[8]) << 0); //���ݷ��ͷ������ַ
					u8 	idata srcPoint =  datsRcv_ZIGB.rcvDats[10];	//Դ��
					u8 	idata dstPoint =  datsRcv_ZIGB.rcvDats[11];	//Զ��
						
					devTips_nwkZigb = nwkZigb_Normal; //zigbTips״̬��Ӧ��ֻҪ���յ�zigb���ݣ�tips״̬���л�������
					
					memset(paramRX_temp, 0, sizeof(u8) * dataLen_zigbDatsTrans);
					memcpy(paramRX_temp, &(datsRcv_ZIGB.rcvDats[21]), datsRcv_ZIGB.rcvDats[20]);
						
					if(srcPoint > 0x10 && srcPoint < 0xff){ /*���ض˿�*/
						
						u8 statusRelay_temp = status_Relay; //��ǰ����״̬����
					
						if((srcPoint == CTRLEATHER_PORT[0]) && (0 != CTRLEATHER_PORT[0])){ //����λ1 ���ذ��ж�
						
							swCommand_fromUsr.actMethod = relay_OnOff;
							statusRelay_temp &= ~(1 << 0); //����λ��������
							swCommand_fromUsr.objRelay = statusRelay_temp | paramRX_temp[0] << 0; //bit0 ����λ������Ӧ
						}
						else
						if((srcPoint == CTRLEATHER_PORT[1]) && (0 != CTRLEATHER_PORT[1])){ //����λ2 ���ذ��ж�
						
							swCommand_fromUsr.actMethod = relay_OnOff;
							statusRelay_temp &= ~(1 << 1); //����λ��������
							swCommand_fromUsr.objRelay = statusRelay_temp | paramRX_temp[0] << 1; //bit1 ����λ������Ӧ
						}
						else
						if((srcPoint == CTRLEATHER_PORT[2]) && (0 != CTRLEATHER_PORT[2])){ //����λ3 ���ذ��ж�
						
							swCommand_fromUsr.actMethod = relay_OnOff;
							statusRelay_temp &= ~(1 << 2); //����λ��������
							swCommand_fromUsr.objRelay = statusRelay_temp | paramRX_temp[0] << 2; //bit2 ����λ������Ӧ
						}
					
					}else{ /*�ǻ��ض˿�*/
					
						switch(srcPoint){
						
							/*�������ת���˿�*/
							case PORTPOINT_OBJ_CTRLNOMAL:{	
							
								if(datsFrom_addr == ZIGB_NWKADDR_CORDINATER){ //����Э����
								
									dataParing_Nomal(paramRX_temp, datsFrom_addr, srcPoint); //�������
								}
								
							}break;
							
							/*ϵͳ���ƶ˿�*/
							case PORTPOINT_OBJ_CTRLSYSZIGB:{	
							
								dataParing_zigbSysCtrl(paramRX_temp); //ϵͳ���ƽ���
								
							}break;
								
							default:{
							
								
								
							}break;
						}
					}
				}
			}
			
		}break;
		
		case status_nwkREQ:{
		
			//--------------------------------Э״̬����������-----------------------------------------------//
			devTips_nwkZigb = nwkZigb_nwkREQ;
			zigB_nwkJoinRequest(1);	//�������������븽����������
			
		}break;
			
		case status_nwkReconnect:{
		
			//--------------------------------Э״̬�����ߴ���-----------------------------------------------//
			devTips_nwkZigb = nwkZigb_reConfig;
			zigB_nwkJoinRequest(0);	//����������
			
		}break;
		
		case status_dataTransRequestDatsSend:{
			
			//--------------------------------Э״̬����������-----------------------------------------------//
			dataTransRequest_datsSend(); //������Զ�����ݴ���
		
		}break;
			
		default:{
		
			if(devStatus_switch.statusChange_IF){ //״̬ǿ���л�ʱ������ǰ��״̬�ھ�̬������ʼ�����ٽ����ⲿ�л�
			
				devStatus_switch.statusChange_IF = 0;
				devRunning_Status = devStatus_switch.statusChange_standBy;
				
				break;
			}
		
		}break;
	}
}
