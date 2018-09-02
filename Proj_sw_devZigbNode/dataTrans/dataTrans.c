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

/**********************本地文件变量定义区************************/
datsAttr_datsTrans xdata datsSend_request;//远端数据传输请求缓存
datsAttr_datsTrans xdata datsRcv_respond;//远端数据传输请求等待响应缓存缓存

//zigbee运行状态切换标志
stt_statusChange devStatus_switch = {0, status_NULL};
//数据请求完成后是否需要重启网络
bit reConnectAfterDatsReq_IF = 0; //用于互控通讯簇即刻注册特殊情况下使用

bit coordinatorOnline_IF = 0; //协调器在线标志

//zigb网络重连专用动作时间计数
u16 xdata zigbNwkAction_counter = 0;

//心跳
bit heartBeatCycle_FLG = 0;	//心跳周期触发
u8 	heartBeatCount	   = 0;	//心跳周期计数

//串口接收超时标志
bit uartRX_toutFLG 	= 0;
//串口接收超时计数
bit rxTout_count_EN = 0;
u8  rxTout_count 	= 0;
//串口数据缓存
u8	datsRcv_length	= 0;
uartTout_datsRcv xdata datsRcv_ZIGB = {{0}, 0};

//zigbee通信线程当前运行状态标志
threadRunning_Status devRunning_Status = status_NULL;

void zigbUart_pinInit(void){

	//TX推挽输出
	P3M1 &= 0xFD;	
	P3M0 |= 0x02;	
	
	//RX高阻输入
	P3M1 |= 0x01;
	P3M0 &= 0xFE;
	
	//TX推挽输出
	P2M1 &= ~0x08;
	P2M0 |= 0x08;
}
	
/*--------------------------------------------------------------*/
void uartObjZigb_Init(void){

	EA = 0;

	PS = 1;
	SCON = (SCON & 0x3f) | UART_8bit_BRTx;

{
	u32 j = (MAIN_Fosc / 4) / ZIGB_BAUND;	//按1T计算
		j = 65536UL - j;
	
	TH2 = (u8)(j>>8);
	TL2 = (u8)j;
}
	AUXR &= ~(1<<4);	//Timer stop
	AUXR |= 0x01;		//S1 BRT Use Timer2;
	AUXR &= ~(1<<3);	//Timer2 set As Timer
	AUXR |=  (1<<2);	//Timer2 set as 1T mode

	IE2  &= ~(1<<2);	//禁止中断
	AUXR &= ~(1<<3);	//定时
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
//发送串口数据
//----------------------------*/
//void uartObjWIFI_Send_Byte(u8 dat)	//串口1
//{
//	TX1_write2buff(dat);
//}

//void uartObjWIFI_Send_String(char *s,unsigned char ucLength){	 //串口1
//	
//	uart1_datsSend(s, ucLength);
//}

//void rxBuff_WIFI_Clr(void){

//	memset(rxBuff_WIFI, 0xff, sizeof(char) * COM_RX1_Lenth);
//	COM1.RX_Cnt = 0;
//}

/********************* UART1(WIIF)中断函数_自定义重构************************/
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

/* 自定义校验*///自家产品协议层
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

/*此数据封装必须在数据包发送前最后调用，自定义对象进行数据封装*///避免校验被提前而出错
static 
u8 dtasTX_loadBasic_CUST(bit ifRemoteDats,
					     u8 dats_Tx[],
					     u8 datsLen_TX,
					     u8 frame_Type,
					     u8 frame_CMD,
					     bit ifSpecial_CMD){
						   
	dats_Tx[2] 	= frame_Type;
	dats_Tx[3] 	= frame_CMD;
	
	if(!ifSpecial_CMD)dats_Tx[10] = SWITCH_TYPE;	//开关类型填充
	
	memcpy(&dats_Tx[5], &MAC_ID[1], 5);	//MAC填充
						  
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

/*数据异或校验*///ZNP协议层
static 
u8 XORNUM_CHECK(u8 buf[], u8 length){

	u8 loop = 0;
	u8 valXOR = buf[0];
	
	for(loop = 1;loop < length;loop ++)valXOR ^= buf[loop];
	
	return valXOR;
}

/*zigbee数据帧加载*/
static 
u8 ZigB_TXFrameLoad(u8 frame[],u8 cmd[],u8 cmdLen,u8 dats[],u8 datsLen){		

	const u8 frameHead = ZIGB_FRAME_HEAD;	//ZNP,SOF帧头
	u8 xor_check = datsLen;					//异或校验，帧尾
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

/*zigbee单指令数据请求，返回应答数据长度*/
static 
u8 zigb_datsRequest( u8 frameREQ[],		//请求帧
					 u8 frameREQ_Len,	//请求帧长
					 u8 resp_cmd[2],	//所需应答指令
					 u8 resp_dats[],	//应答数据缓存
					 u8 loopReapt,u16 timeWait){	//循环次数，单次等待时间
					  
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

/*zigbee单指令下发及响应验证*///阻塞
bit zigb_VALIDA_INPUT(u8 REQ_CMD[2],		//指令
					  u8 REQ_DATS[],		//数据
					  u8 REQdatsLen,		//数据长度
					  u8 ANSR_frame[],		//响应帧
					  u8 ANSRdatsLen,		//响应帧长度
					  u8 times,u16 timeDelay){	//循环次数，单次等待时间
					   
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

///*zigbee通信簇设置*///阻塞
//bit zigb_clusterSet(u16 deviveID, u8 endPoint){

//	datsAttr_ZigbInit code default_param = {{0x24,0x00},{0x0E,0x0D,0x00,0x0D,0x00,0x0D,0x00,0x01,0x00,0x00,0x01,0x00,0x00},0x0D,{0xFE,0x01,0x64,0x00,0x00,0x65},0x06,300};	//数据簇注册,默认参数
//	u8 code frameResponse_Subs[6] = {0xFE,0x01,0x64,0x00,0xB8,0xDD}; //响应帧替补，若数据簇已经注册
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
//									2,		//2次以内没有正确响应就失败
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
//								 2,		//2次以内没有正确响应就失败
//								 default_param.timeTab_waitAnsr);
//	}
//}

///*zigbee重新入网*///阻塞函数，仅供测试使用
//bit ZigB_NwkJoin(u16 PANID, u8 CHANNELS){

//#define	cmdNum_zigbNwkJoin	8	
//	
//#define	 loop_PANID		5
//#define	 loop_CHANNELS	6

//	datsAttr_ZigbInit code ZigbInit_dats[cmdNum_zigbNwkJoin] = {
//		
//		{	{0x46,0x09},	{0},					0x00,	{0xFE,0x06,0x41,0x80,0x01,0x02,0x00,0x02,0x06,0x03,0xC3},	0x0B,	4000	},	//复位
//		{	{0x46,0x09},	{0},					0x00,	{0xFE,0x06,0x41,0x80,0x01,0x02,0x00,0x02,0x06,0x03,0xC3},	0x0B,	4000	},	//复位
//		{	{0x26,0x05},	{0x03,0x01,0x03},		0x03,	{0xFE,0x01,0x66,0x05,0x00,0x62},							0x06,	500		},	//寄存器初始化，全部清空
//		{	{0x46,0x09},	{0},					0x00,	{0xFE,0x06,0x41,0x80,0x01,0x02,0x00,0x02,0x06,0x03,0xC3},	0x0B,	4000	},	//二次复位
//		
////		{	{0x26,0x05},	{0x87,0x01,0x00},		0x03,	{0xFE,0x01,0x66,0x05,0x00,0x62},							0x06,	500		},	//角色设置（协调器）
//		{	{0x26,0x05},	{0x87,0x01,0x01},		0x03,	{0xFE,0x01,0x66,0x05,0x00,0x62},							0x06,	500		},	//角色设置（路由器）
////		{	{0x26,0x05},	{0x87,0x01,0x02},		0x03,	{0xFE,0x01,0x66,0x05,0x00,0x62},							0x06,	500		},	//角色设置（终端）
//		
//		{	{0x27,0x02},	{0x34,0x12},			0x02,	{0xFE,0x01,0x67,0x02,0x00,0x64},							0x06,	500		},	//PAN_ID寄存器设置
//		{	{0x27,0x03},	{0x00,0x80,0x00,0x00},	0x04,	{0xFE,0x01,0x67,0x03,0x00,0x65},							0x06,	500		},	//信道寄存器配置
////		{	{0x26,0x00},	{0},					0x00,	{0xFE,0x01,0x45,0xC0,0x09,0x8D},							0x06,	12000	},	//开始入网，以既定角色协调器（协调器响应）
//		{	{0x26,0x00},	{0},					0x00,	{0xFE,0x01,0x45,0xC0,0x07,0x83},							0x06,	12000	},	//开始入网，以既定角色协调器（路由器响应）
////		{	{0x26,0x00},	{0},					0x00,	{0xFE,0x01,0x45,0xC0,0x06,0x82},							0x06,	12000	},	//开始入网，以既定角色协调器（终端响应）
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
//		switch(loop){	//自选参数&默认参数
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
//	return zigb_clusterSet(13, 13);	//设备ID 13，终端点 13；
//}

/*zigbee 主动开放网络*///阻塞
bit ZigB_nwkOpen(bit openIF, u8 openTime){

	datsAttr_ZigbInit code default_param = {{0x26,0x08}, {0xFC,0xFF,0x00}, 0x03, {0xFE,0x01,0x66,0x08,0x00,0x6F}, 0x06, 150}; //zigbee指令下达默认参数
	
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
								  2,	//2次无回复为失败
								  default_param.timeTab_waitAnsr);

#if(DEBUG_LOGOUT_EN == 1)	
//	{ //输出打印，谨记 用后注释，否则占用大量代码空间
//		u8 xdata log_buf[64];
//		
//		sprintf(log_buf, "nwkOpen result:%d.\n", (int)resultSet);
//		PrintString1_logOut(log_buf);
//	}
#endif
	
	return resultSet;
}

/*zigbee PANID获取*///阻塞
static u16 ZigB_getPanIDCurrent(void){

	u16 PANID_temp = 0;
	
#define	paramLen_zigbPanIDGet 32
	u8 xdata paramTX_temp[paramLen_zigbPanIDGet] = {0};
	
	u8 code frameREQ_zigbPanIDGet[6] = {0xFE, 0x01, 0x26, 0x06, 0x06, 0x27};	//zigb PANID获取指令帧
	u8 code cmdResp_zigbPanIDGet[2]  = {0x66, 0x06};	//zigb PANID获取预期响应指令
	u8 datsResp_Len = 0;

	datsResp_Len = zigb_datsRequest(frameREQ_zigbPanIDGet, 6, cmdResp_zigbPanIDGet, paramTX_temp, 2, 300);

	if(datsResp_Len){

		PANID_temp |= (((u16)(paramTX_temp[5]) << 0) & 0x00FF);
		PANID_temp |= (((u16)(paramTX_temp[6]) << 8) & 0xFF00);

//		printf_datsHtoA("[Tips_uartZigb]: resultDats:", local_datsParam->frameResp, local_datsParam->frameRespLen);
	}

	return PANID_temp;
}

/*zigbee系统时间获取并更新*///阻塞
static bit getSystemTime_reales(void){
	
	bit resultOpreat = 0;

#define	paramLen_zigbSystimeGet 32
	u8 xdata paramTX_temp[paramLen_zigbSystimeGet] = {0};
	
	u8 code frameREQ_zigbSystimeGet[5] = {0xFE, 0x00, 0x21, 0x11, 0x30};	//zigb PANID获取指令帧
	u8 code cmdResp_zigbSystimeGet[2]  = {0x61, 0x11};	//zigb PANID获取预期响应指令
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
		
		/*计算缓存赋值*/
		Y_temp8 = Y;
		if(M == 1 || M == 2){ //一月和二月当作上一年十三月和十四月
		
			M_temp8 = M + 12;
			Y_temp8 --;
		}
		else M_temp8 = M;
		
		/*开始计算*/
		W =	 Y_temp8 + (Y_temp8 / 4) + 5 - 40 + (26 * (M_temp8 + 1) / 10) + D - 1;	//蔡勒公式
		W %= 7; 
		
		/*计算结果赋值*/
		W?(systemTime_current.time_Week = W):(systemTime_current.time_Week = 7);
		
		systemTime_current.time_Month = 	M;
		systemTime_current.time_Day = 		D;
		systemTime_current.time_Year = 		Y;
		
		systemTime_current.time_Hour = 		paramTX_temp[8];
		systemTime_current.time_Minute =	paramTX_temp[9];
		systemTime_current.time_Second = 	paramTX_temp[10];
		
		/*本地时间维持计数值校准更新*/
		sysTimeKeep_counter = systemTime_current.time_Minute * 60 + systemTime_current.time_Second; //系统时间维持计数值更新
		
		resultOpreat = 1;
	}
	
#if(DEBUG_LOGOUT_EN == 1)	
//	{ //输出打印，谨记 用后注释，否则占用大量代码空间
//		u8 xdata log_buf[64];
//		
//		sprintf(log_buf, "sysTime reales result:%d.\n", (int)resultOpreat);
//		PrintString1_logOut(log_buf);
//	}
#endif
	
	return resultOpreat;
}

/*zigbee系统时间设置*///阻塞
bit zigB_sysTimeSet(u32 timeStamp){

	datsAttr_ZigbInit code default_param = {{0x21,0x10},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},0x0B,{0xFE,0x01,0x61,0x10,0x00},0x05,100}; //zigbee指令下达默认参数
	u8 xdata timeStampArray[0x0B] = {0};
	bit resultSet = 0;
	u32 timeStamp_temp = timeStamp;
	
	timeStamp_temp += (3600UL * (long)sysTimeZone_H + 60UL * (long)sysTimeZone_M); //时区

	timeStampArray[0] = (u8)((timeStamp_temp & 0x000000ff) >> 0);
	timeStampArray[1] = (u8)((timeStamp_temp & 0x0000ff00) >> 8);
	timeStampArray[2] = (u8)((timeStamp_temp & 0x00ff0000) >> 16);
	timeStampArray[3] = (u8)((timeStamp_temp & 0xff000000) >> 24);
	
	resultSet = zigb_VALIDA_INPUT((u8 *)default_param.zigbInit_reqCMD,
								  (u8 *)timeStampArray,
								  default_param.reqDAT_num,
								  (u8 *)default_param.zigbInit_REPLY,
								  default_param.REPLY_num,
								  2,	//2次无回复为失败
								  default_param.timeTab_waitAnsr);
	
#if(DEBUG_LOGOUT_EN == 1)	
//	{ //输出打印，谨记 用后注释，否则占用大量代码空间
//		u8 xdata log_buf[64];
//		
//		sprintf(log_buf, "sysTime set result:%d.\n", (int)resultSet);
//		PrintString1_logOut(log_buf);
//	}
#endif
	
	return resultSet;
}

///*zigbee硬件复位初始化*///阻塞
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
//			delayMs(2);	//必须延时
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
//					delayMs(1);	//必须延时
//				}
//			}
//		}
//	}
//	
//	return 0;
//}

///*zigbee初始化自检*///阻塞
//bit ZigB_inspectionSelf(void){	
//	
//#define	paramLen_zigbInspection 64
//	u8 xdata paramTX_temp[paramLen_zigbInspection] = {0};
//	
////	bit REQResult = 0;
//	
////	u8 code frameREQ_zigbStatusCheck[5] = {0xFE, 0x00, 0x27, 0x00, 0x27};	//zigb状态查询指令帧
////	u8 code cmdResp_zigbStatusCheck[2] 	= {0x67, 0x00};	//zigb状态查询响应指令
//	u8 code frameREQ_zigbJoinNWK[5] 	= {0xFE, 0x00, 0x26, 0x00, 0x26};	//zigb激活网络指令帧
//	u8 code cmdResp_zigbJoinNWK[2] 		= {0x45, 0xC0};	//zigb激活网络响应指令
//	u8 datsResp_Len = 0;
//	
////	datsResp_Len = zigb_datsRequest(frameREQ_zigbStatusCheck, 5, cmdResp_zigbStatusCheck, paramTX_temp, 2, 500);
////	if(paramTX_temp[16] == 0x07)REQResult
//	
//	datsResp_Len = zigb_datsRequest(frameREQ_zigbJoinNWK, 5, cmdResp_zigbJoinNWK, paramTX_temp, 2, 5000);
//	if(paramTX_temp[4] == 0x07)return (zigb_clusterSet(13, 13) & zigb_clusterSet(13, 14));	//设备ID 13，终端点 13；	
//	else{
//	
//		return 0;
//	}
//}

/*zigbee非阻塞入网请求*///非阻塞 ---信道默认第四信道
static 
void zigB_nwkJoinRequest(bit reJoin_IF){

#define	cmdNum_zigbNwkREQ	9	

	datsAttr_ZigbInit code ZigbInit_dats[cmdNum_zigbNwkREQ] = {

		{	{0x46,0x09},	{0},					0x00,	{0xFE,0x06,0x41,0x80,0x01,0x02,0x00,0x02,0x06,0x03,0xC3},	0x0B,	4000	},	//复位(硬件)
//		{	{0x46,0x09},	{0},					0x00,	{0xFE,0x06,0x41,0x80,0x00,0x02,0x00,0x02,0x06,0x03,0xC2},	0x0B,	4000	},	//复位(替补)
		{	{0x46,0x09},	{0},					0x00,	{0xFE,0x06,0x41,0x80,0x01,0x02,0x00,0x02,0x06,0x03,0xC3},	0x0B,	4000	},	//复位(软件)
		{	{0x26,0x05},	{0x03,0x01,0x03},		0x03,	{0xFE,0x01,0x66,0x05,0x00,0x62},							0x06,	500		},	//寄存器初始化，全部清空
		{	{0x46,0x09},	{0},					0x00,	{0xFE,0x06,0x41,0x80,0x01,0x02,0x00,0x02,0x06,0x03,0xC3},	0x0B,	4000	},	//二次复位(软件)
		
//		{	{0x26,0x05},	{0x87,0x01,0x00},		0x03,	{0xFE,0x01,0x66,0x05,0x00,0x62},							0x06,	500		},	//角色设置（协调器）
		{	{0x26,0x05},	{0x87,0x01,0x01},		0x03,	{0xFE,0x01,0x66,0x05,0x00,0x62},							0x06,	500		},	//角色设置（路由器）
//		{	{0x26,0x05},	{0x87,0x01,0x02},		0x03,	{0xFE,0x01,0x66,0x05,0x00,0x62},							0x06,	500		},	//角色设置（终端）
		
		{	{0x27,0x02},	{0xFF,0xFF},			0x02,	{0xFE,0x01,0x67,0x02,0x00,0x64},							0x06,	500		},	//PAN_ID寄存器设置
		{	{0x27,0x03},	{0x00,0x80,0x00,0x00},	0x04,	{0xFE,0x01,0x67,0x03,0x00,0x65},							0x06,	500		},	//信道寄存器配置
//		{	{0x26,0x00},	{0},					0x00,	{0xFE,0x01,0x45,0xC0,0x09,0x8D},							0x06,	12000	},	//开始入网，以既定角色协调器（协调器响应）
		{	{0x26,0x00},	{0},					0x00,	{0xFE,0x01,0x45,0xC0,0x07,0x83},							0x06,	8000	},	//开始入网，以既定角色协调器（路由器响应）
//		{	{0x26,0x00},	{0},					0x00,	{0xFE,0x01,0x45,0xC0,0x06,0x82},							0x06,	12000	},	//开始入网，以既定角色协调器（终端响应）
		{	{0x26,0x08}, 	{0xFC,0xFF,0x00}, 		0x03,	{0xFE,0x01,0x66,0x08,0x00,0x6F}, 							0x06, 	150		},  //关闭网络
	};
	
	datsAttr_ZigbInit code defaultParam_clusterRegister = {{0x24,0x00},{0x0E,0x0D,0x00,0x0D,0x00,0x0D,0x00,0x01,0x00,0x00,0x01,0x00,0x00},0x0D,{0xFE,0x01,0x64,0x00,0x00,0x65},0x06,500};	//数据簇注册,默认参数
	u8 code frameResponseSubs_clusterRegister[6] = {0xFE,0x01,0x64,0x00,0xB8,0xDD}; //响应帧替补，若数据簇已经注册
	
#define	clusterNum_default 2
	datsAttr_clusterREG code cluster_Default[clusterNum_default] = {{13, 13}, {14, 13}};
	
#define	dataLen_zigbNwkREQ 64
	u8 xdata paramTX_temp[dataLen_zigbNwkREQ] = {0};
	
	static u8 step_CortexA = 0,
			  step_CortexB = 0;
	static u8 reactionLoop = 0;
	
	u8 datsTX_Len = 0;
	
	if(devStatus_switch.statusChange_IF){ //状态强制切换时，将当前子状态内静态变量初始化后再进行外部切换
	
		devStatus_switch.statusChange_IF = 0;
		devRunning_Status = devStatus_switch.statusChange_standBy;
		
		step_CortexA = 0;
		step_CortexB = 0;
		reactionLoop = 0;
		zigbPin_RESET = 1;
		
		return;
	}
	
	if(step_CortexA > (cmdNum_zigbNwkREQ + clusterNum_usr + clusterNum_default)){ //内部状态完成
	
		step_CortexA = 0;
		step_CortexB = 0;
		reactionLoop = 0;
		zigbPin_RESET = 1;
		
		sysTimeReales_counter = PERIOD_SYSTIMEREALES; //systime更新周期重置，防止多指令堵塞冲突
		
		devRunning_Status = status_passiveDataRcv; //外部状态切换
		devTips_status = status_Normal; //tips状态切换
		
		return;
	}
	
	if(!reJoin_IF)if(step_CortexA == 0)step_CortexA = 7; //是否为重新主动加入新网络，否则不进行硬件复位(硬件复位将导致本地时间被重置)
	if((step_CortexA == 7) || (step_CortexA == 0))sysTimeReales_counter	= PERIOD_SYSTIMEREALES; //非阻塞关键指令不能被阻塞指令打断（硬件复位 和 入网时 中断阻塞指令下达）
	if(step_CortexA == 0){ //特殊指令_硬件复位:<0>
	
		switch(step_CortexA){
		
			case 0:{ //首条指令，硬件复位
			
				switch(step_CortexB){
				
					case 0:{ //步骤一：硬件拉低100ms
					
						zigbPin_RESET = 0;
						zigbNwkAction_counter = 200;
						step_CortexB = 1;
					
					}break;
				
					case 1:{ //步骤二：硬件拉低完毕后确认应答帧时长
					
						if(!zigbNwkAction_counter){ //非阻塞等待
						
							zigbPin_RESET = 1;
							zigbNwkAction_counter = 6000;
							step_CortexB = 2;
						}
						
					}break;
					
					case 2:{ //步骤二：确认应答帧
						
						if(!zigbNwkAction_counter)step_CortexB = 0; //非阻塞等待响应
					
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
	if(step_CortexA > 0 && step_CortexA < cmdNum_zigbNwkREQ){ //常规指令:<1 - 9>
		
//		if(!reJoin_IF)if(step_CortexA == 2)step_CortexA = 7;	//是否为重新主动加入新网络，否则只进行被动网络激活

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
				
				if(!zigbNwkAction_counter){ //非阻塞等待响应
				
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
	if(step_CortexA >= cmdNum_zigbNwkREQ){ //特殊指令_常规通信簇注册:<10 - n>
		
		u8 datsREG_cluster[16] = {0};
		memcpy(datsREG_cluster, defaultParam_clusterRegister.zigbInit_reqDAT, defaultParam_clusterRegister.reqDAT_num);
		if(step_CortexA < (cmdNum_zigbNwkREQ + clusterNum_default)){ //默认通信簇参数填装
		
			datsREG_cluster[0] = cluster_Default[step_CortexA - cmdNum_zigbNwkREQ].endpoint;
			datsREG_cluster[3] = (u8)((cluster_Default[step_CortexA - cmdNum_zigbNwkREQ].devID & 0x00ff) >> 0);
			datsREG_cluster[4] = (u8)((cluster_Default[step_CortexA - cmdNum_zigbNwkREQ].devID & 0xff00) >> 8);
			
		}else{	//用户通信簇（互控）注册参数填装
		
			if((CTRLEATHER_PORT[step_CortexA - cmdNum_zigbNwkREQ - clusterNum_usr] >= 0x10) && (CTRLEATHER_PORT[step_CortexA - cmdNum_zigbNwkREQ - clusterNum_usr] < 255)){ //通信簇端口合法性判断
			
				datsREG_cluster[0] = CTRLEATHER_PORT[step_CortexA - cmdNum_zigbNwkREQ - clusterNum_usr];
				datsREG_cluster[3] = zigbDatsDefault_ClustID; //默认簇ID <LSB>
				datsREG_cluster[4] = 0; //默认簇ID <MSB>
				
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
				
				if(!zigbNwkAction_counter){ //非阻塞等待响应
				
					reactionLoop ++;
					step_CortexB = 0;
				}
				else
				if(uartRX_toutFLG){
				
					uartRX_toutFLG = 0;
					
					if(memmem(datsRcv_ZIGB.rcvDats, COM_RX1_Lenth, defaultParam_clusterRegister.zigbInit_REPLY, defaultParam_clusterRegister.REPLY_num) || //预期响应
					   memmem(datsRcv_ZIGB.rcvDats, COM_RX1_Lenth, frameResponseSubs_clusterRegister, 6)){ //替补响应
					
						step_CortexB = 0;
						reactionLoop = 0;
						step_CortexA ++;
					}
				}
				
			}break;
		}
	}
}

/*zigbee网络数据发送格式化填装*/
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

	//发送帧填装
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

/*zigbee网络数据发送请求状态*///非阻塞
static
void dataTransRequest_datsSend(void){

	u8 xdata buf_datsTX[NORMALDATS_DEFAULT_LENGTH] = {0};
	u8 datsTX_Len = 0;
	
#define zigbDatsSend_datsRespLen	64
	u8 xdata buf_datsRX[zigbDatsSend_datsRespLen] = {0};
	u8 datsRX_Len = 0;
	
#define zigbDatsSend_ASR_datsLen	3
	u8 		ASR_dats[zigbDatsSend_ASR_datsLen] = {0};
	u8 code ASR_cmd[2] = {0x44,0x80};	//本地ZNP协议层确认发送响应
	
#define resCODE_datsSend_NOROUTER 0xCD	//数据发送协议层响应代码-路由失联
#define resCODE_datsSend_NOREMOTE 0xE9	//数据发送协议层响应代码-对方不在线
#define resCODE_datsSend_TIMEOUT  0x01  //数据发送协议层响应代码-发送超时
#define resCODE_datsSend_SUCCESS  0x00  //数据发送协议层响应代码-发送成功
	static u8 datsTrans_respondCode = 0; //发送完成响应码
	
	static u8 step = 0;
	static u8 reactionLoop = 0;
	
	if(devStatus_switch.statusChange_IF){	//状态强制切换时，将当前子状态内静态变量初始化后再进行外部切换
	
		devStatus_switch.statusChange_IF = 0;
		devRunning_Status = devStatus_switch.statusChange_standBy;
		
		step = 0;
		reactionLoop = 0;
		
		return;
	}
	
	//接收帧填装_本地
	ASR_dats[0] = 0x00; //发送成功响应代码
	ASR_dats[1] = datsSend_request.portPoint;
	ASR_dats[2] = zigbDatsDefault_TransID;
	datsRX_Len = ZigB_TXFrameLoad(buf_datsRX, (u8 *)ASR_cmd, 2, ASR_dats, zigbDatsSend_ASR_datsLen);
	
	datsTX_Len = zigb_datsLoad_datsSend(buf_datsTX, datsSend_request.nwkAddr, datsSend_request.portPoint, datsSend_request.datsTrans.dats, datsSend_request.datsTrans.datsLen);
	
	switch(step){
	
		case 0:{ //响应接收就绪，设置响应时间
			
			if(reactionLoop > 3){
				
				datsTrans_respondCode = resCODE_datsSend_TIMEOUT; //响应码改为超时
				
				reactionLoop = 0;
				step = 4;
				
				break;
			}
		
			zigbPin_RESET = 1; //保险起见，复位拉高
			uartZigB_datsSend(buf_datsTX, datsTX_Len);
			zigbNwkAction_counter = 1000; //默认协议层响应时间<时间太短无法收到后面的接收状态响应指令，只能收到系统响应>
			step = 1;
			
		}break;
		
		case 1:{ //非阻塞等待系统响应
		
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
							zigbNwkAction_counter = 500; //默认远端响应时间<对方节点响应>
						}
						
					}else{	
						
						datsTrans_respondCode = datsRcv_ZIGB.rcvDats[4]; //错误响应码装载
						
//#if(DEBUG_LOGOUT_EN == 1)
//						{ //输出打印，谨记 用后注释，否则占用大量代码空间
//							u8 xdata log_buf[64]; //数据传输失败协议层响应代码打印
//							
//							sprintf(log_buf, "dats_TX fail code: %02X %02X %02X.\n", (int)datsRcv_ZIGB.rcvDats[2], (int)datsRcv_ZIGB.rcvDats[3], (int)datsRcv_ZIGB.rcvDats[4]);
//							PrintString1_logOut(log_buf);
//						}	
//#endif				
					}
				}
			}
			
		}break;
		
		case 2:{ //非阻塞等待远端响应

			if(!zigbNwkAction_counter){
			
				reactionLoop ++;
				step = 0;
			}
			else{
				
				if(uartRX_toutFLG){
					
					u16 idata datsFrom_addr = ((u16)(datsRcv_ZIGB.rcvDats[9]) << 8) | ((u16)(datsRcv_ZIGB.rcvDats[8]) << 0); //数据发送方网络地址
					u8 	idata dstPoint =  datsRcv_ZIGB.rcvDats[11];	//远端	
					
					uartRX_toutFLG = 0;

					if(!memcmp(&(datsRcv_ZIGB.rcvDats[21]), datsRcv_respond.datsTrans.dats, datsRcv_respond.datsTrans.datsLen) && 
					   (datsRcv_respond.nwkAddr == datsFrom_addr) &&
						(datsRcv_respond.portPoint == dstPoint)){
					
						step = 3;
					}
				}
			}
			
		}break;
		
		case 3:{ //响应成功
		
			if(reConnectAfterDatsReq_IF){ //针对即刻注册互控特殊情况 状态切换
			
				reConnectAfterDatsReq_IF = 0;
				devRunning_Status = status_nwkReconnect;
				
			}else{ 
			
				devRunning_Status = status_passiveDataRcv;
			}
			
			reactionLoop = 0;
			step = 0;
			
		}break;
		
		case 4:{ //响应失败
		
			if(reConnectAfterDatsReq_IF){ //针对即刻注册互控特殊情况 状态切换
			
				reConnectAfterDatsReq_IF = 0;
				devRunning_Status = status_nwkReconnect;
				
			}else{ 
			
				devRunning_Status = status_passiveDataRcv;
			}
			
			//针对数据传输失败响应代码情况进行选择性重连，否则仅时区协调器设备就gg
			if(datsTrans_respondCode){ 
				
				switch(datsTrans_respondCode){ //响应失败码分析
				
					case resCODE_datsSend_NOROUTER:
					case resCODE_datsSend_NOREMOTE:
					case resCODE_datsSend_SUCCESS:{
					
						devTips_nwkZigb = nwkZigb_outLine; //暂时只做失败提示，不做其他动作
						
					}break;
					
					default:{
					
						devTips_nwkZigb = nwkZigb_outLine; //暂时只做失败提示，不做其他动作
						
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

/*zigbee常规控制数据解析*/
static 
void dataParing_zigbSysCtrl(u8 datsFrame[]){

	frame_zigbSysCtrl xdata dats = {0};
	
	dats.command = datsFrame[0];
	memcpy(dats.dats, &datsFrame[2], datsFrame[1]);
	dats.datsLen = datsFrame[1];
	
	switch(dats.command){
	
		case ZIGB_SYSCMD_NWKOPEN:{ //网络开放
			
			bit resultSet = 0;
			
			resultSet = ZigB_nwkOpen(1, dats.dats[0]);
			
		}break;
		
		case ZIGB_SYSCMD_TIMESET:{ //系统时间设定
		
			bit resultSet = 0;
			u32 time_Temp = 0UL;
			
			time_Temp |= (u32)dats.dats[0] << 0;
			time_Temp |= (u32)dats.dats[1] << 8;
			time_Temp |= (u32)dats.dats[2] << 16;
			time_Temp |= (u32)dats.dats[3] << 24;
			
			resultSet = zigB_sysTimeSet(time_Temp - 946713600UL); //zigbee时间戳从unix纪元946713600<2000/01/01 00:00:00>开始计算
			
		}break;
			
		default:break;
	}
}

/*zigbee常规控制转发数据解析*/
static 
void dataParing_Nomal(u8 datsParam[], u16 nwkAddr_from, u8 port_from){
	
#define	dataLen_dataParingNomal 96
	u8 xdata paramTX_temp[dataLen_dataParingNomal] = {0};
	
	bit dataFromRemote_IF = 0;	//是否为服务器端数据标志

	/*产品二级协议核对_常规控制*///控制下达
	switch(datsParam[0]){
	
		/*远端*/
		case ZIGB_FRAMEHEAD_CTRLREMOTE:{
			
			dataFromRemote_IF = 1;
			
			memcpy(MAC_ID_DST, &datsParam[7], 6);
			memcpy(&datsParam[1], &datsParam[13], datsRcv_ZIGB.rcvDats[20] - 13);
		}
		
		/*本地*/
		case ZIGB_FRAMEHEAD_CTRLLOCAL:{
			
			bit frameCheck_Done = 0; //数据检测合格标志
			
			{
				bit frameCodeCheck_PASS = 0; //校验码检查通过标志
				bit frameMacCheck_PASS  = 0; //mac地址待检查通过标志
				
				if(datsParam[4] == frame_Check(&datsParam[5], 28))frameCodeCheck_PASS = 1; //校验码检测
				if(!memcmp(&datsParam[5], &MAC_ID[1], 5))frameMacCheck_PASS = 1; //MAC检测

				if(datsParam[3] == FRAME_MtoZIGBCMD_cmdConfigSearch){ //特殊指令不做MAC检测
				
					frameMacCheck_PASS = 1;
					
				}else
				if((datsParam[3] == FRAME_MtoZIGBCMD_cmdCfg_swTim) || //特殊指令不做校验码检测
				   (datsParam[3] == FRAME_MtoZIGBCMD_cmdswTimQuery)){
				   
					frameCodeCheck_PASS = 1;
				}
				   
				if(frameCodeCheck_PASS && frameCodeCheck_PASS)frameCheck_Done = 1;
			}
			   
			if(frameCheck_Done){ //帧检查通过，开始解析、动作及响应
				
				bit respond_IF 		= 0;	//是否回复
				bit specialCmd_IF 	= 0;	//是否为特殊指令（特殊指令占用开关类型那一个字节）
				
#if(DEBUG_LOGOUT_EN == 1)
				{ //输出打印，谨记 用后注释，否则占用大量代码空间
					u8 xdata log_buf[64];
					
					sprintf(log_buf, "cmdComing:%02X.\n", (int)datsParam[3]);
					PrintString1_logOut(log_buf);
				}			
#endif		
				memset(paramTX_temp, 0, sizeof(u8) * dataLen_dataParingNomal);
			
				switch(datsParam[3]){
				
					case FRAME_MtoZIGBCMD_cmdConfigSearch:{
						
						if(!deviceLock_flag){ //设备是否上锁
							
							u16 xdata panid_Temp = ZigB_getPanIDCurrent(); //配置回复添加PANID
							
							paramTX_temp[14] = (u8)((panid_Temp & 0xFF00) >> 8);
							paramTX_temp[15] = (u8)((panid_Temp & 0x00FF) >> 0);
						
							respond_IF 		= 1; //响应回复
							specialCmd_IF 	= 0;
							
						}else{
						
							
						}
						
					}break;
					
					case FRAME_MtoZIGBCMD_cmdControl:{
						
						swCommand_fromUsr.objRelay = datsParam[11];
						swCommand_fromUsr.actMethod = relay_OnOff;
						
						respond_IF 		= 1; //响应回复
						specialCmd_IF 	= 0;	

						paramTX_temp[11] = datsParam[11];						
						
					}break;
						
					case FRAME_MtoZIGBCMD_cmdQuery:{}break;
						
					case FRAME_MtoZIGBCMD_cmdInterface:{}break;
						
					case FRAME_MtoZIGBCMD_cmdReset:{}break;
						
					case FRAME_MtoZIGBCMD_cmdDevLockON:{
					
						//数据处理及动作响应
						{
							u8 deviceLock_IF = 1;
							
							deviceLock_flag  = 1;
							coverEEPROM_write_n(EEPROM_ADDR_deviceLockFLAG, &deviceLock_IF, 1);
						}		
						
					}break;
						
					case FRAME_MtoZIGBCMD_cmdDevLockOFF:{
					
						//数据处理及动作响应
						{
							u8 deviceLock_IF = 0;
							
							deviceLock_flag  = 0;
							coverEEPROM_write_n(EEPROM_ADDR_deviceLockFLAG, &deviceLock_IF, 1);
						}	
					
					}break;
						
					case FRAME_MtoZIGBCMD_cmdswTimQuery:{
					
						//分类回复
						switch(datsParam[13]){ //子命令解析
						
							case 0: /*上位机在定时的时候给0，待协商*/
							case cmdConfigTim_normalSwConfig:{
							
								u8 loop = 0;
							
								//数据响应及回复
								EEPROM_read_n(EEPROM_ADDR_swTimeTab, &paramTX_temp[14], 12);	//定时表回复填装
								
								//回复数据二次处理（针对一次性定时数据）
								for(loop = 0; loop < 4; loop ++){
								
									if(swTim_onShoot_FLAG & (1 << loop)){
										
										paramTX_temp[14 + loop * 3] &= 0x80;
									}
								}
										
								specialCmd_IF = 1; //特殊占位指令
								
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
							
								EEPROM_read_n(EEPROM_ADDR_TimeTabNightMode, &paramTX_temp[14], 6);	//夜间模式定时表回复填装
								
								(deviceLock_flag)?(paramTX_temp[12] |= 0x01):(paramTX_temp[12] &= ~0x01);
								(ifNightMode_sw_running_FLAG)?(paramTX_temp[12] |= 0x02):(paramTX_temp[12] &= ~0x02);
								
							}break;
							
							default:break;
						}
						
						paramTX_temp[13] = datsParam[13]; //定时子命令同步回复
						
						respond_IF = 1; //响应回复使能
						
					}break;
						
					case FRAME_MtoZIGBCMD_cmdConfigAP:{}break;
						
					case FRAME_MtoZIGBCMD_cmdBeepsON:{ //夜间模式关
					
						u8 datsTemp = 0;
						
						EEPROM_read_n(EEPROM_ADDR_TimeTabNightMode, &datsTemp, 1);
						datsTemp &= ~0x7f; //夜间模式定时表存储,取消头字节全占满,失能全天
						coverEEPROM_write_n(EEPROM_ADDR_TimeTabNightMode, &datsTemp, 1);
						
						respond_IF = 1; //响应回复使能
						
					}break;
						
					case FRAME_MtoZIGBCMD_cmdBeepsOFF:{ //夜间模式开
					
						u8 datsTemp = 0;
						
						EEPROM_read_n(EEPROM_ADDR_TimeTabNightMode, &datsTemp, 1);
						datsTemp |= 0x7f; //夜间模式定时表存储,头字节全占满,强制全天
						coverEEPROM_write_n(EEPROM_ADDR_TimeTabNightMode, &datsTemp, 1);	
						
						respond_IF = 1; //响应回复使能
						
					}break;
						
					case FRAME_MtoZIGBCMD_cmdftRecoverRQ:{
					
						respond_IF = 1;
						
					}break;
						
					case FRAME_MtoZIGBCMD_cmdRecoverFactory:{
					
						Factory_recover();
					
					}break;
						
					case FRAME_MtoZIGBCMD_cmdCfg_swTim:{
						
						u8 loop = 0;
						
						switch(datsParam[13]){ //定时数据处理及更新,分类处理
						
							case cmdConfigTim_normalSwConfig:{	/*普通定时*/
								
								for(loop = 0; loop < 4; loop ++){
								
									if(datsParam[14 + loop * 3] == 0x80){	/*一次性定时判断*///周占位为空，而定时器被打开，说明是一次性
									
										swTim_onShoot_FLAG 	|= (1 << loop);	//一次性定时标志开启
										datsParam[14 + loop * 3] |= (1 << (datsParam[31] - 1)); //强行进行当前周占位，当次执行完毕后清除
									}
								}
								coverEEPROM_write_n(EEPROM_ADDR_swTimeTab, &datsParam[14], 12);	//定时表

							}break;
							
							case cmdConfigTim_onoffDelaySwConfig:{	/*开关延时*/
							
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
							
							case cmdConfigTim_closeLoopSwConfig:{	/*绿色功能(自动循环关闭)*/
							
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

							case cmdConfigTim_nightModeSwConfig:{  /*夜间模式 背光半亮*/
							
								coverEEPROM_write_n(EEPROM_ADDR_TimeTabNightMode, &datsParam[14], 6);	//夜间模式定时表存储
								
							}break;
							
							default:break;
						}
						
						respond_IF = 1; //响应回复使能
						
					}break;
					
					case FRAME_MtoZIGBCMD_cmdCfg_ctrlEachO:{
					
						u8 loop = 0;
						u8 effective_oprate = datsParam[12]; //有效操作数据占位获取
						
						for(loop = 0; loop < clusterNum_usr; loop ++){
						
							if((effective_oprate >> loop) & 0x01){ //有效数据判断
							
								coverEEPROM_write_n(EEPROM_ADDR_portCtrlEachOther + loop, &datsParam[14 + loop], 1);
								CTRLEATHER_PORT[loop] = datsParam[14 + loop];
								reConnectAfterDatsReq_IF = 1; //即刻注册互控通讯簇端口
							}
						}
						
						respond_IF = 1; //响应回复使能
					
					}break;
					
					case FRAME_MtoZIGBCMD_cmdQue_ctrlEachO:{
					
						u8 loop = 0;
						
						for(loop = 0; loop < clusterNum_usr; loop ++){
						
							EEPROM_read_n(EEPROM_ADDR_portCtrlEachOther + loop, &paramTX_temp[14 + loop], 1);
						}
						
						respond_IF = 1; //响应回复使能
					
					}break;
						
					case FRAME_MtoZIGBCMD_cmdCfg_ledBackSet:{
					
						coverEEPROM_write_n(EEPROM_ADDR_ledSWBackGround, &datsParam[14], 1);
						coverEEPROM_write_n(EEPROM_ADDR_ledSWBackGround + 1, &datsParam[15], 1);
						tipsInsert_swLedBKG_ON 	= datsParam[14];
						tipsInsert_swLedBKG_OFF = datsParam[15];
						
						respond_IF = 1; //响应回复使能
					
					}break;
					
					case FRAME_MtoZIGBCMD_cmdQue_ledBackSet:{
					
						EEPROM_read_n(EEPROM_ADDR_ledSWBackGround, &paramTX_temp[14], 1);
						EEPROM_read_n(EEPROM_ADDR_ledSWBackGround + 1, &paramTX_temp[15], 1);
						
						respond_IF = 1; //响应回复使能
					
					}break;
					
					case FRAME_MtoZIGBCMD_cmdCfg_scenarioSet:{
						
						u16 xdata panid_Temp = ZigB_getPanIDCurrent(); //配置回复添加PANID
					
						bit opt_result = swScenario_oprateSave(datsParam[12], datsParam[14]);
						if(opt_result)paramTX_temp[12] = 0;
						else paramTX_temp[12] = 0x0A; //场景设置无效回复（场景存储已满）
						
						paramTX_temp[14] = (u8)((panid_Temp & 0xFF00) >> 8);
						paramTX_temp[15] = (u8)((panid_Temp & 0x00FF) >> 0);
						
						respond_IF = 1; //响应回复使能（本地存储已被占满）
					
					}break;
					
					case FRAME_MtoZIGBCMD_cmdCfg_scenarioCtl:{
						
						u8 sw_Act = swScenario_oprateCheck(datsParam[12]);
						if(sw_Act != SW_SCENCRAIO_ACTINVALID){ //若索引到有效操作位
							
							swCommand_fromUsr.actMethod = relay_OnOff;
							swCommand_fromUsr.objRelay = sw_Act;
						
							paramTX_temp[12] = 0;
							
						}else{ //若无法索引到有效操作位
						
							paramTX_temp[12] = 0x0A; //场景控制无效回复（场景号无法被索引）
						}
					
						respond_IF = 1; //响应回复使能
					
					}break;
					
					case FRAME_MtoZIGBCMD_cmdCfg_scenarioDel:{
						
						swScenario_oprateDele(datsParam[12]);
						paramTX_temp[12] = 0;
					
						respond_IF = 1; //响应回复使能
					
					}break;
					
					default:{
					
						respond_IF = 0;
					
					}break;
				}
				
				/*回复响应*/
				if(respond_IF){ //数据包回复响应动作
				
					u8 datsTX_Len = 0;
					
					respond_IF = 0;
					
					datsTX_Len = dtasTX_loadBasic_CUST(dataFromRemote_IF,
													   paramTX_temp,
													   33,
													   FRAME_TYPE_StoM_RCVsuccess,
													   datsParam[3],
													   specialCmd_IF);
					
					heartBeatCount = 1; //回复响应抵消一次心跳
					
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
		
		/*心跳_网关在线*/
		case ZIGB_FRAMEHEAD_HEARTBEAT:{
		
			
			
		}break;
		
		/*心跳_网关离线*///internet离线，不是zigb网络离线
		case ZIGB_FRAMEHEAD_HBOFFLINE:{
		
			
			
		}break;
		
		default:{}break;
	}
}

/*zigbee主线程*///动作阻塞大于200ms的函数都设为状态机运行，其它小于200ms函数，阻塞维持，否则状态机复杂度加大
void thread_dataTrans(void){
	
	u8 code cmd_datsComing[2] = {0x44, 0x81};

#define	dataLen_zigbDatsTrans 96
	u8 xdata paramTX_temp[dataLen_zigbDatsTrans] = {0};
	u8 xdata paramRX_temp[dataLen_zigbDatsTrans] = {0};
	
	static bit heartBeat_cmdFLG = 0; //心跳奇偶标志
	
	/*zigb主线程系统时间更新*/
	if(!sysTimeReales_counter){ 
	
		sysTimeReales_counter = PERIOD_SYSTIMEREALES;
		getSystemTime_reales();
	}
	
	/*zigb主线程状态机：根据状态标志运行*/
	switch(devRunning_Status){
	
		case status_passiveDataRcv:{
			
			if(devStatus_switch.statusChange_IF){ //状态强制切换时，将当前子状态内静态变量初始化后再进行外部切换
			
				devStatus_switch.statusChange_IF = 0;
				devRunning_Status = devStatus_switch.statusChange_standBy;
				
				break;
			}
			
			{/*初始化时间赋值*///仅开机赋值一次
				static bit FLG_timeSetInit = 1;
				
				if(FLG_timeSetInit){
				
					FLG_timeSetInit = 0;
					zigB_sysTimeSet(1533810700UL - 946713600UL); //zigbee时间戳从unix纪元946713600<2000/01/01 00:00:00>开始计算
				}
			}
	
			//--------------------------------主状态：心跳--------------------------------------------------------//
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
			
			//--------------------------------主状态：互控同步---------------------------------------------------//
			if(EACHCTRL_realesFLG){
			
				if(devRunning_Status == status_passiveDataRcv){
				
					u8 idata loop;
					
					for(loop = 0; loop < 3; loop ++){ //三个开关位分别判定
					
						if(EACHCTRL_realesFLG & (1 << loop)){ //互控有效位判断
						
							EACHCTRL_realesFLG &= ~(1 << loop); //互控有效位清零
							
							paramTX_temp[0] = (status_Relay >> loop) & 0x01; //开关状态填装
							
							if((CTRLEATHER_PORT[loop] > 0x10) && CTRLEATHER_PORT[loop] < 0xFF){ //是否为有效互控端口
							
								datsSend_request.nwkAddr = 0xffff;
								datsSend_request.portPoint = CTRLEATHER_PORT[loop];
								memset(datsSend_request.datsTrans.dats, 0, DATBASE_LENGTH * sizeof(u8));
								memcpy(datsSend_request.datsTrans.dats, paramTX_temp, 1);
								datsSend_request.datsTrans.datsLen = 1;
								datsRcv_respond.datsTrans.datsLen = 0;
								devRunning_Status = status_dataTransRequestDatsSend;
								
								break; //顺序执行，先执行先break，每个总调度周期执行一个有效互控
							}
						}
					}
				}	
			}memset(paramTX_temp, 0, sizeof(u8) * dataLen_zigbDatsTrans);
			
			//--------------------------------主状态：数据解析响应-----------------------------------------------//
			if(uartRX_toutFLG){ //数据接收(帧超时)
				
				uartRX_toutFLG = 0;
				
				/*Zigbee一级协议核对解析*/
				if((datsRcv_ZIGB.rcvDats[0] == ZIGB_FRAME_HEAD) &&
					!memcmp(&datsRcv_ZIGB.rcvDats[2], cmd_datsComing, 2)){
					
					u16 idata datsFrom_addr = ((u16)(datsRcv_ZIGB.rcvDats[9]) << 8) | ((u16)(datsRcv_ZIGB.rcvDats[8]) << 0); //数据发送方网络地址
					u8 	idata srcPoint =  datsRcv_ZIGB.rcvDats[10];	//源端
					u8 	idata dstPoint =  datsRcv_ZIGB.rcvDats[11];	//远端
						
					devTips_nwkZigb = nwkZigb_Normal; //zigbTips状态响应，只要接收到zigb数据，tips状态就切换至正常
					
					memset(paramRX_temp, 0, sizeof(u8) * dataLen_zigbDatsTrans);
					memcpy(paramRX_temp, &(datsRcv_ZIGB.rcvDats[21]), datsRcv_ZIGB.rcvDats[20]);
						
					if(srcPoint > 0x10 && srcPoint < 0xff){ /*互控端口*/
						
						u8 statusRelay_temp = status_Relay; //当前开关状态缓存
					
						if((srcPoint == CTRLEATHER_PORT[0]) && (0 != CTRLEATHER_PORT[0])){ //开关位1 互控绑定判断
						
							swCommand_fromUsr.actMethod = relay_OnOff;
							statusRelay_temp &= ~(1 << 0); //动作位缓存清零
							swCommand_fromUsr.objRelay = statusRelay_temp | paramRX_temp[0] << 0; //bit0 开关位动作响应
						}
						else
						if((srcPoint == CTRLEATHER_PORT[1]) && (0 != CTRLEATHER_PORT[1])){ //开关位2 互控绑定判断
						
							swCommand_fromUsr.actMethod = relay_OnOff;
							statusRelay_temp &= ~(1 << 1); //动作位缓存清零
							swCommand_fromUsr.objRelay = statusRelay_temp | paramRX_temp[0] << 1; //bit1 开关位动作响应
						}
						else
						if((srcPoint == CTRLEATHER_PORT[2]) && (0 != CTRLEATHER_PORT[2])){ //开关位3 互控绑定判断
						
							swCommand_fromUsr.actMethod = relay_OnOff;
							statusRelay_temp &= ~(1 << 2); //动作位缓存清零
							swCommand_fromUsr.objRelay = statusRelay_temp | paramRX_temp[0] << 2; //bit2 开关位动作响应
						}
					
					}else{ /*非互控端口*/
					
						switch(srcPoint){
						
							/*常规控制转发端口*/
							case PORTPOINT_OBJ_CTRLNOMAL:{	
							
								if(datsFrom_addr == ZIGB_NWKADDR_CORDINATER){ //来自协调器
								
									dataParing_Nomal(paramRX_temp, datsFrom_addr, srcPoint); //常规解析
								}
								
							}break;
							
							/*系统控制端口*/
							case PORTPOINT_OBJ_CTRLSYSZIGB:{	
							
								dataParing_zigbSysCtrl(paramRX_temp); //系统控制解析
								
							}break;
								
							default:{
							
								
								
							}break;
						}
					}
				}
			}
			
		}break;
		
		case status_nwkREQ:{
		
			//--------------------------------协状态：网络请求-----------------------------------------------//
			devTips_nwkZigb = nwkZigb_nwkREQ;
			zigB_nwkJoinRequest(1);	//非阻塞主动加入附近开放网络
			
		}break;
			
		case status_nwkReconnect:{
		
			//--------------------------------协状态：掉线处理-----------------------------------------------//
			devTips_nwkZigb = nwkZigb_reConfig;
			zigB_nwkJoinRequest(0);	//非阻塞重连
			
		}break;
		
		case status_dataTransRequestDatsSend:{
			
			//--------------------------------协状态：数据请求-----------------------------------------------//
			dataTransRequest_datsSend(); //非阻塞远端数据传输
		
		}break;
			
		default:{
		
			if(devStatus_switch.statusChange_IF){ //状态强制切换时，将当前子状态内静态变量初始化后再进行外部切换
			
				devStatus_switch.statusChange_IF = 0;
				devRunning_Status = devStatus_switch.statusChange_standBy;
				
				break;
			}
		
		}break;
	}
}
