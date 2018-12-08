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
datsAttr_datsTrans xdata datsSend_request = {0}; //远端数据传输请求缓存
datsAttr_dtCtrlEach xdata datsSend_requestEx[3] = {0}; //扩展型远端数据传输请求缓存（持续发送，无需远端响应）
u16 xdata dtReqEx_counter = 0; //扩展型远端数据传输请求数据发送间隔计时值 单位：ms
datsAttr_datsTrans xdata datsRcv_respond = {0}; //远端数据传输请求等待响应缓存缓存
remoteDataReq_method xdata devRemoteDataReqMethod = {0}; //远端数据请求方式

stt_devOpreatDataPonit xdata dev_currentDataPoint = {0}; //周期询访数据点
stt_agingDataSet_bitHold xdata	dev_agingCmd_rcvPassive = {0}; //周期询访时效占位指令缓存 --被动接收
stt_agingDataSet_bitHold xdata	dev_agingCmd_sndInitative = {0}; //周期询访时效占位指令缓存 --主动上传
u8 dtModeKeepAcess_currentCmd = DTMODEKEEPACESS_FRAMECMD_ASR; //数据传输为定时询访模式时，携带询访指令值，需要主动上传时则更改此值

//zigbee运行状态切换标志
stt_statusChange xdata devStatus_switch = {0, status_NULL};
//数据请求完成后是否需要重启网络
bit reConnectAfterDatsReq_IF = 0; //用于互控通讯簇即刻注册特殊情况下使用

bit coordinatorOnline_IF = 0; //协调器在线标志

//zigb网络动作专用时间计数
u16 xdata zigbNwkAction_counter = 0;

//zigb设备网络挂起属性参数
attr_devNwkHold	xdata devNwkHoldTime_Param = {0};

//心跳
bit heartBeatCycle_FLG 			= 0; //心跳周期触发
u8 	xdata heartBeatCount		= 0; //心跳周期计数
u8	xdata heartBeatPeriod		= PERIOD_HEARTBEAT_ASR; //心跳计数周期
u8	xdata heartBeatHang_timeCnt = 0; //心跳挂起计时(此数据为0时才可以发送心跳，否则心跳挂起，用于通信避让)

//集群受控周期轮询-<包括有互控和场景>
u8	xdata colonyCtrlGet_queryCounter = COLONYCTRLGET_QUERYPERIOD; //集群受控状态周期性询查计时计数值
u8	xdata colonyCtrlGet_statusLocalEaCtrl[clusterNum_usr] = {0}; //集群控制-本地互控状态位记录
u8	xdata localDataRecord_eaCtrl[clusterNum_usr] = {0}; //本地静态数据记录：互控实际值
u8	xdata colonyCtrlGet_statusLocalScene = 0; //集群控制-本地场景状态位记录
u8	xdata colonyCtrlGetHang_timeCnt = 0; //集群受控状态周期性轮询挂起计时(此数据为0时才可以进行周期询查，否则询查挂起，用于通信避让)

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

//zigbee协调器丢失检测计时变量
u8 xdata timeCounter_coordinatorLost_detecting = COORDINATOR_LOST_PERIOD_CONFIRM;

u8	xdata factoryRecover_HoldTimeCount = 0; //恢复出厂等待时间
bit idata factoryRecover_standBy_FLG = 0; //恢复出厂预置标志

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

	PS = 0;
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
					
			RX1_Buffer[datsRcv_length ++] = SBUF;
			rxTout_count = 0;
		}
	}

	if(TI)
	{
		TI = 0;
		if(COM1.TX_read != COM1.TX_write)
		{
		 	SBUF = TX1_Buffer[COM1.TX_read];
			if(++COM1.TX_read >= COM_TX1_Lenth)	COM1.TX_read = 0;
		}
		else COM1.B_TX_busy = 0;
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

/*周期性主动发码通信挂起，通信清障*/
void periodDataTrans_momentHang(u8 hangTime){ //挂起时间 单位：s

	heartBeatCount = 0;
	colonyCtrlGet_queryCounter = COLONYCTRLGET_QUERYPERIOD;
	
	heartBeatHang_timeCnt = colonyCtrlGetHang_timeCnt = hangTime;
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
		
		while(zigbNwkAction_counter){ //定时器中断内进行倒计时

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
		
		while(zigbNwkAction_counter){ //定时器中断内进行倒计时
			
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
static
bit zigB_sysTimeSet(u32 timeStamp, bit timeZoneAdjust_IF){

	datsAttr_ZigbInit code default_param = {{0x21,0x10},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},0x0B,{0xFE,0x01,0x61,0x10,0x00},0x05,100}; //zigbee指令下达默认参数
	u8 xdata timeStampArray[0x0B] = {0};
	bit resultSet = 0;
	u32 timeStamp_temp = timeStamp;
	
	if(timeZoneAdjust_IF){ //是否需要时区调整
	
		if(sysTimeZone_H <= 12){
		
			timeStamp_temp += (3600UL * (long)sysTimeZone_H + 60UL * (long)sysTimeZone_M); //时区正
			
		}else
		if(sysTimeZone_H > 12 && sysTimeZone_H <= 24){
		
			timeStamp_temp -= (3600UL * (long)(sysTimeZone_H - 12) + 60UL * (long)sysTimeZone_M); //时区负
			
		}else
		if(sysTimeZone_H >= 30){
		
			timeStamp_temp += (3600UL * (long)(sysTimeZone_H - 17) + 60UL * (long)sysTimeZone_M); //时区特殊
		}
	}

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

/*zigbee非阻塞入网请求状态机*///非阻塞 ---信道默认第四信道
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
	
#define	clusterNum_default 3
	datsAttr_clusterREG code cluster_Default[clusterNum_default] = {
	
		{ZIGB_ENDPOINT_CTRLSECENARIO, zigbDatsDefault_ClustID}, 
		{ZIGB_ENDPOINT_CTRLNORMAL, zigbDatsDefault_ClustID}, 
		{ZIGB_ENDPOINT_CTRLSYSZIGB, zigbDatsDefault_ClustID}
	};
	
#define	dataLen_zigbNwkREQ 64
	u8 xdata paramTX_temp[dataLen_zigbNwkREQ] = {0};
	
	static u8 xdata step_CortexA = 0, //状态机-主步骤
			  xdata step_CortexB = 0; //状态机-子步骤
	static u8 xdata reactionLoop = 0; //重复次数缓存
		   u8 xdata actionReaptDefine_normal = 2; //重复次数定义_常规指令下
	
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
	
	if(step_CortexA > (cmdNum_zigbNwkREQ + clusterNum_usr + clusterNum_default)){ //判断当前状态机内部状态流程是否完成
	
		step_CortexA = 0;
		step_CortexB = 0;
		reactionLoop = 0;
		zigbPin_RESET = 1;
		
		sysTimeReales_counter = PERIOD_SYSTIMEREALES; //systime更新周期重置，防止多指令堵塞冲突
		
		devRunning_Status = status_passiveDataRcv; //外部状态切换
		devTips_status = status_Normal; //设备系统tips状态切换
		
		dev_currentPanid = ZigB_getPanIDCurrent(); //更新一次PANID,避免二次重新加网残留老的PANID
		
#if(DEBUG_LOGOUT_EN == 1)
		{ //输出打印，谨记 用后注释，否则占用大量代码空间
			memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
			sprintf(log_buf, "nwkZigb connect/rejoin all complete.\n");
			PrintString1_logOut(log_buf);
		}			
#endif
		
		return;
	}
	
	if(!reJoin_IF)if(step_CortexA == 0)step_CortexA = 7; //是否为重新主动加入新网络，否则不进行硬件复位(硬件复位将导致本地时间被重置)
	if((step_CortexA == 7) || (step_CortexA == 0))sysTimeReales_counter	= PERIOD_SYSTIMEREALES; //非阻塞关键指令不能被阻塞指令打断（硬件复位 和 入网时 中断阻塞指令下达）
	
#if(DEBUG_LOGOUT_EN == 1)
	{//输出打印，谨记 用后注释，否则占用大量代码空间
	#define STATUSMACHINE_NWKREQ_DISPINITVAL 255
		static u8 xdata stepDisp_CortexA = STATUSMACHINE_NWKREQ_DISPINITVAL;
		
		if(stepDisp_CortexA != step_CortexA){
		
			stepDisp_CortexA = step_CortexA;
			memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
			sprintf(log_buf, "nwkZigb connect/rejoin mainStep-%02d@method:%d complete.\n", (int)stepDisp_CortexA, (int)reJoin_IF);
			PrintString1_logOut(log_buf);
		}
	}
#endif
	
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
					
						if(!zigbNwkAction_counter){ //非阻塞等待6000ms
						
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
				
				(step_CortexA == 7)?(actionReaptDefine_normal = 12):(actionReaptDefine_normal =  2); //由于入网过程较复杂，入网指令等待时间放长
				
				if(reactionLoop > actionReaptDefine_normal){
					
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
		if(step_CortexA < (cmdNum_zigbNwkREQ + clusterNum_default)){ //默认专用通信簇参数填装
		
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

/*zigbee无视响应回复直接发送数据*///阻塞
static
void dataSendRemote_straightforward( u16 DstAddr, //远端网络短地址
									  u8 port,	 //端点口
									  u8 dats[], //数据
									  u8 datsLen ){ //数据长度
									  								  
	u8 xdata buf_datsTX[NORMALDATS_DEFAULT_LENGTH] = {0};
	u8 datsTX_Len = 0;
	
	datsTX_Len = zigb_datsLoad_datsSend(buf_datsTX, DstAddr, port, dats, datsLen);
	
	uartZigB_datsSend(buf_datsTX, datsTX_Len);
}

/*zigbee网络数据发送请求状态机*///非阻塞
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
	
#define resCODE_datsSend_NOROUTER 0xCD	//数据发送协议层响应代码-路由失联，通讯媒介丢失
#define resCODE_datsSend_NOREMOTE 0xE9	//数据发送协议层响应代码-对方不在线，目标地址节点设备不存在
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
	ASR_dats[0] = resCODE_datsSend_SUCCESS; //发送成功响应代码
	ASR_dats[1] = datsSend_request.portPoint;
	ASR_dats[2] = zigbDatsDefault_TransID;
	datsRX_Len = ZigB_TXFrameLoad(buf_datsRX, (u8 *)ASR_cmd, 2, ASR_dats, zigbDatsSend_ASR_datsLen);
	
	datsTX_Len = zigb_datsLoad_datsSend(buf_datsTX, datsSend_request.nwkAddr, datsSend_request.portPoint, datsSend_request.datsTrans.dats, datsSend_request.datsTrans.datsLen);
	
	switch(step){
	
		case 0:{ //响应接收就绪，设置响应时间
			
			if(reactionLoop > 3){ //重发次数已超出
				
				datsTrans_respondCode = resCODE_datsSend_TIMEOUT; //响应码改为超时
				
				reactionLoop = 0;
				step = 4;
				
				break;
			}
		
			zigbPin_RESET = 1; //保险起见，复位拉高
			if(!devRemoteDataReqMethod.keepTxUntilCmp_IF)uartZigB_datsSend(buf_datsTX, datsTX_Len); //非死磕，发一次就行
			zigbNwkAction_counter = REMOTE_DATAREQ_TIMEOUT; //默认协议层响应时间<时间太短无法收到后面的接收状态响应指令，只能收到系统响应>
			step = 1;
			
		}break;
		
		case 1:{ //非阻塞等待系统响应
			
			static u8 xdata local_funRecord = 0; //本地 除次比 判断
				   u8 xdata funRecord_temp = zigbNwkAction_counter / devRemoteDataReqMethod.datsTxKeep_Period; //除次比，用于周期判断
		
			if(!zigbNwkAction_counter){ //超时判断
			
				reactionLoop ++;
				step = 0;
				local_funRecord = 0;
			}
			else{
				
				if(devRemoteDataReqMethod.keepTxUntilCmp_IF){ //死磕方式下，超时时间内，周期循环下发指令
					
					if(local_funRecord != funRecord_temp){ //按持续频率持续发送
					
						local_funRecord = funRecord_temp; //本地除次比更新
						
						uartZigB_datsSend(buf_datsTX, datsTX_Len);
					}
				}
				
				if(uartRX_toutFLG){
				
					uartRX_toutFLG = 0;

					if(memmem(datsRcv_ZIGB.rcvDats, COM_RX1_Lenth, buf_datsRX, datsRX_Len)){ //应答指令和应答码都正确
					
						if(datsRcv_respond.datsTrans.datsLen == 0){ //判断条件是否需要远端响应
						
							step = 3;
							
						}else{
						
							step = 2;
							zigbNwkAction_counter = REMOTE_RESPOND_TIMEOUT; //默认远端响应时间<对方节点响应>
						}
						
						local_funRecord = 0; //本地除次比复位
						
					}else{	
						
						if(!memcmp(&datsRcv_ZIGB.rcvDats[2], ASR_cmd, 2)){ //应答指令正确，但应答码错误，则取出错误码
						
							datsTrans_respondCode = datsRcv_ZIGB.rcvDats[4]; //错误响应码装载
							
							if(devRemoteDataReqMethod.keepTxUntilCmp_IF){ //发送方式判断-是否为死磕方式
							
								
								
							}else{
							
								step = 4; //非死磕方式，则有应答指令，但应答数据错误，跳至失败步骤
							}
						}
						
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
					
					if(datsRcv_ZIGB.rcvDats[0] != ZIGB_FRAME_HEAD){ //帧头不对，打印输出
					
#if(DEBUG_LOGOUT_EN == 1)
						{ //输出打印，谨记 用后注释，否则占用大量代码空间
							memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
							sprintf(log_buf, "err frameHead:%02X.\n", (int)datsRcv_ZIGB.rcvDats[0]);
							PrintString1_logOut(log_buf);
						}			
#endif	
					}

					if(!memcmp(&(datsRcv_ZIGB.rcvDats[21]), datsRcv_respond.datsTrans.dats, datsRcv_respond.datsTrans.datsLen) && 
					   (datsRcv_respond.nwkAddr == datsFrom_addr) &&
						(datsRcv_respond.portPoint == dstPoint)){
					
						step = 3;
							
					}else{
					
#if(DEBUG_LOGOUT_EN == 1)
						{ //输出打印，谨记 用后注释，否则占用大量代码空间
							memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
//							sprintf(log_buf, "data remoteRcv is %02X %02X %02X.\n", (int)datsRcv_ZIGB.rcvDats[21], (int)datsRcv_ZIGB.rcvDats[22], (int)datsRcv_ZIGB.rcvDats[23]);
							sprintf(log_buf, "rcvPort is %02X, rcvNwkAddr is %04X.\n", (int)dstPoint, (int)datsFrom_addr);
//							sprintf(log_buf, "rcvPort is %02X, rcvNwkAddr is %04X.\n", (int)datsRcv_respond.portPoint, (int)datsRcv_respond.nwkAddr);
							PrintString1_logOut(log_buf);
						}	
#endif	
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
			
			/*死磕属性复位*///属性设置仅生效一次
			devRemoteDataReqMethod.keepTxUntilCmp_IF = 0; //死磕使能复位
			devRemoteDataReqMethod.datsTxKeep_Period = 0; //死磕周期复位
			
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
				
#if(DEBUG_LOGOUT_EN == 1)				
				{ //输出打印，谨记 用后注释，否则占用大量代码空间
					memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
					sprintf(log_buf, "remote dataRequest fail, code:[0x%02X].\n", (int)datsTrans_respondCode);
					PrintString1_logOut(log_buf);
				}
#endif	
				
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
			
			/*死磕属性复位*///属性设置仅生效一次
			devRemoteDataReqMethod.keepTxUntilCmp_IF = 0; //死磕使能复位
			devRemoteDataReqMethod.datsTxKeep_Period = 0; //死磕周期复位
			
		}break;
			
		default:{
		
			step = 4;
			
		}break;
	}
}

/*设备数据传输主状态切换至网络挂起*/
void devStatusChangeTo_devHold(bit zigbNwkSysNote_IF){ //设备网络挂起

	devNwkHoldTime_Param.devHoldTime_counter = DEVHOLD_TIME_DEFAULT;
	if(zigbNwkSysNote_IF)devNwkHoldTime_Param.zigbNwkSystemNote_IF = 1;
	
	devStatus_switch.statusChange_standBy = status_devNwkHold; //数据传输状态机更变
	devStatus_switch.statusChange_IF = 1;
	
	devTips_status = status_devHold; //tips更变
	thread_tipsGetDark(0x0F);
	
#if(DEBUG_LOGOUT_EN == 1)				
	{ //输出打印，谨记 用后注释，否则占用大量代码空间
		memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
		sprintf(log_buf, "devHold start right now.\n");
		PrintString1_logOut(log_buf);
	}
#endif	
}

/*设备网络挂起停止，使提前结束*/
void devHoldStop_makeInAdvance(void){ //停止设备网络挂起（提前）

	if(devNwkHoldTime_Param.devHoldTime_counter)devNwkHoldTime_Param.devHoldTime_counter = 0;
}

/*zigbee设备网络挂起状态机*///非阻塞
static 
void function_devNwkHold(void){
	
	static status_Step = 0; //当前状态机步骤状态指示
	
	if(devStatus_switch.statusChange_IF){ //状态强制切换时，将当前子状态内静态变量初始化后再进行外部切换
	
		devStatus_switch.statusChange_IF = 0;
		devRunning_Status = devStatus_switch.statusChange_standBy;
		
		status_Step = 0;
		zigbNwkAction_counter = 0;
		
		zigbPin_RESET = 1;
		
		return;
	}
	
	if(devNwkHoldTime_Param.devHoldTime_counter){ //直到挂起时间结束
	
		if(devNwkHoldTime_Param.zigbNwkSystemNote_IF){ //通知当前网络内子设备挂起,报一次
			
			u8 xdata dats_Note[3] = {ZIGB_SYSCMD_DEVHOLD, 1, 0}; //命令、数据长度、数据
		
			devNwkHoldTime_Param.zigbNwkSystemNote_IF = 0; //通知使能复位
			
			dataSendRemote_straightforward( 0xFFFF, //广播通知网内所有子设备挂起
											ZIGB_ENDPOINT_CTRLSYSZIGB,
											dats_Note,
											3 );
			
			delayMs(50); //必需延时，否则数据还没发送出去，就跑到下一步复位了
		}
		
		{ //设备挂起,循环复位
			
			switch(status_Step){
			
				case 0:{ //复位200ms
				
					zigbPin_RESET = 0;
					zigbNwkAction_counter = 200;
					status_Step = 1;
				
				}break;
					
				case 1:{ //每6s复位一次
				
					if(!zigbNwkAction_counter){ //非阻塞等待
					
						zigbPin_RESET = 1;
						zigbNwkAction_counter = 6000;
						status_Step = 2;
					}
				
				}break;
				
				case 2:{
				
					if(!zigbNwkAction_counter){ //非阻塞等待
					
						status_Step = 0;
						
#if(DEBUG_LOGOUT_EN == 1)				
						{ //输出打印，谨记 用后注释，否则占用大量代码空间
							memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
							sprintf(log_buf, "devHold time count remind: %02d s.\n", (int)devNwkHoldTime_Param.devHoldTime_counter);
							PrintString1_logOut(log_buf);
						}
#endif
					}
				
				}break;
				
				default:{
					
					status_Step = 0;
					
				}break;
			}
		}

	}else{
	
		//挂起时间结束,主状态恢复至重连，本地状态恢复
		status_Step = 0;
		zigbPin_RESET = 1;
		
		devRunning_Status = status_nwkReconnect; //直接将主状态切换至网络重连,不走standby流程
		devTips_status = status_Normal; //tips更变
	
#if(DEBUG_LOGOUT_EN == 1)				
		{ //输出打印，谨记 用后注释，否则占用大量代码空间
			memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
			sprintf(log_buf, "devHold stop.\n");
			PrintString1_logOut(log_buf);
		}
#endif	
	}
}

/*zigbee集群控制数据解析*/
static 
void dataParing_scenarioCtrl(u8 datsFrame[]){

	u8 dataTX_temp = CTRLSECENARIO_RESPCMD_SPECIAL;
	
	swCommand_fromUsr.objRelay = datsFrame[0]; //继电器响应即可
	swCommand_fromUsr.actMethod = relay_OnOff;
	
	colonyCtrlGet_statusLocalScene = datsFrame[0]; //本地场景控制轮询值更新(场景控制仅来自于手机控制)
	
	dataSendRemote_straightforward(0, ZIGB_ENDPOINT_CTRLSECENARIO, &dataTX_temp, 1); //场景控制远端回复
	
#if(DEBUG_LOGOUT_EN == 1)
	{ //输出打印，谨记 用后注释，否则占用大量代码空间
		memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
		sprintf(log_buf, "cmdScenarioCtrl comming, statusData:%02X.\n", (int)datsFrame[0]);
		PrintString1_logOut(log_buf);
	}			
#endif		
}

/*zigbee系统交互数据解析*/
static 
void dataParing_zigbSysCtrl(u8 datsFrame[]){

	/*>>>>>>>>>>>>>>frame reference<<<<<<<<<<<<<<<<<*/
	/*----------------------------------------------*/
	/*  frame_CMD	|	frame_dataLen|	frame_data	|
	/*----------------------------------------------*/
	/*	1byte		|	1byte		 |	< 256byte	|	
	/*----------------------------------------------*/
	
	frame_zigbSysCtrl xdata dats = {0};
	
	dats.command = datsFrame[0];
	memcpy(dats.dats, &datsFrame[2], datsFrame[1]);
	dats.datsLen = datsFrame[1];
	
	switch(dats.command){
	
		case ZIGB_SYSCMD_NWKOPEN:{ //网络开放
			
			bit resultSet = 0;
			
			resultSet = ZigB_nwkOpen(1, dats.dats[0]); //功能触发
			tips_statusChangeToZigbNwkOpen(dats.dats[0]); //tips触发
			
#if(DEBUG_LOGOUT_EN == 1)
			{ //输出打印，谨记 用后注释，否则占用大量代码空间
				memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
				sprintf(log_buf, "master cmdComing:nwkOpen:%02ds.\n", (int)dats.dats[0]);
				PrintString1_logOut(log_buf);
			}			
#endif		
			
		}break;
		
		case ZIGB_SYSCMD_TIMESET:{ //系统时间设定
		
			bit resultSet = 0;
			bit timeZoneAdjust_needIF = 0;
			u32 time_Temp = 0UL;
			
			time_Temp |= (u32)dats.dats[0] << 0;
			time_Temp |= (u32)dats.dats[1] << 8;
			time_Temp |= (u32)dats.dats[2] << 16;
			time_Temp |= (u32)dats.dats[3] << 24;
			if((sysTimeZone_H != dats.dats[4]) || (sysTimeZone_M != dats.dats[5])){ //时区同步
			
				sysTimeZone_H = dats.dats[4];
				sysTimeZone_M = dats.dats[5];
				coverEEPROM_write_n(EEPROM_ADDR_timeZone_H, &sysTimeZone_H, 1);
				coverEEPROM_write_n(EEPROM_ADDR_timeZone_M, &sysTimeZone_M, 1);
			}
			
			if(dats.dats[6])timeZoneAdjust_needIF = 1; //时区补偿使能判断
			if(time_Temp > ZIGB_UTCTIME_START)resultSet = zigB_sysTimeSet(time_Temp - ZIGB_UTCTIME_START, timeZoneAdjust_needIF); //zigbee 本地系统时间设置<UTC负补偿>
			
#if(DEBUG_LOGOUT_EN == 1)
			{ //输出打印，谨记 用后注释，否则占用大量代码空间
				u8 xdata log_buf[64];
				
				sprintf(log_buf, "master UTC coming:[0x%02X%02X%02X%02X].\n", (int)dats.dats[3], (int)dats.dats[2], (int)dats.dats[1], (int)dats.dats[0]);
				PrintString1_logOut(log_buf);
			}			
#endif	
		}break;
		
		case ZIGB_SYSCMD_DEVHOLD:{ //网络挂起（用作网关切换）
			
#if(DEBUG_LOGOUT_EN == 1)
			{ //输出打印，谨记 用后注释，否则占用大量代码空间
				memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
				sprintf(log_buf, "node cmdComing:devNwk hold.\n");
				PrintString1_logOut(log_buf);
			}			
#endif	
			devStatusChangeTo_devHold(0); //设备网络被动挂起,不进行网络通知
			
		}break;
		
		case ZIGB_SYSCMD_DATATRANS_HOLD:{ //通信主动避障，将周期性通信挂起
			
#if(DEBUG_LOGOUT_EN == 1)
			{ //输出打印，谨记 用后注释，否则占用大量代码空间
				u8 xdata log_buf[64];
				
				sprintf(log_buf, "master cmd: dtPeriod hold, time:%d.\n", (int)dats.dats[0]);
				PrintString1_logOut(log_buf);
			}			
#endif	
			if(!heartBeatHang_timeCnt && !colonyCtrlGetHang_timeCnt){ //有效周期内，辅助补充广播一次，防止其他节点没收到
				
//				PrintString1_logOut("dtPeriod hold one more time.\n");
//				dataSendRemote_straightforward(0xffff, ZIGB_ENDPOINT_CTRLSYSZIGB, datsFrame, datsFrame[1] + 2);
				
			}else{
			
//#if(DEBUG_LOGOUT_EN == 1)
//				{ //输出打印，谨记 用后注释，否则占用大量代码空间
//					u8 xdata log_buf[64];
//					
//					sprintf(log_buf, "dtPeriod holdAdd err, parm1: %d, parm2: %d.\n", (int)heartBeatHang_timeCnt, (int)colonyCtrlGetHang_timeCnt);
//					PrintString1_logOut(log_buf);
//				}			
//#endif	
			}
			
			periodDataTrans_momentHang(dats.dats[0]);  //避障时间加载，动作执行
			
		}break;

#if(COLONYINFO_QUERYPERIOD_EN == ENABLE) /*宏判头*///集群控制信息周期查询使能
		case ZIGB_SYSCMD_COLONYPARAM_REQPERIOD:{ /*宏使能*///更新集群信息并动作
		
			/*>>>>>>>>>>>>>>>>>>>frame_data reference<<<<<<<<<<<<<<<*/
			/*------------------------------------------------------*/
			/*  frame_data[0]			|	frame_data[1...3]		|
			/*------------------------------------------------------*/
			/*	最近一次场景控制状态值	|	最近一次互控更新状态值	|
			/*------------------------------------------------------*/
			
			u8 idata statusRelay_temp = status_Relay; //开关动作执行缓存
			
//#if(DEBUG_LOGOUT_EN == 1)
//				{ //输出打印，谨记 用后注释，否则占用大量代码空间
//					u8 xdata log_buf[64];
//					
//					sprintf(log_buf, "curRealy_Val:%02X, dataQuery result:%02X %02X %02X %02X.\n",
//									 (int)status_Relay,
//									 (int)dats.dats[0],
//									 (int)dats.dats[1],
//									 (int)dats.dats[2],
//									 (int)dats.dats[3]);
//					PrintString1_logOut(log_buf);
//				}			
//#endif		
			if(dats.dats[0] != colonyCtrlGet_statusLocalScene){ //场景状态值轮询更新
			
#if(DEBUG_LOGOUT_EN == 1)
				{ //输出打印，谨记 用后注释，否则占用大量代码空间
					memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
					sprintf(log_buf, "differ scene detect from poling.\n");
					PrintString1_logOut(log_buf);
				}			
#endif	
				
				//上一次场景控制若没有收到，则进行补偿操作
				colonyCtrlGet_statusLocalScene = dats.dats[0];
				
				if(colonyCtrlGet_statusLocalScene <= 0x07){ //3bit以内操作为有效值，便于初始化操作抛弃无效值
				
					swCommand_fromUsr.actMethod = relay_OnOff;
					swCommand_fromUsr.objRelay = colonyCtrlGet_statusLocalScene;
				}
			}
			
			if(memcmp(&dats.dats[1], colonyCtrlGet_statusLocalEaCtrl, clusterNum_usr)){ //互控状态值值轮询更新
				
				u8 idata loop;
				
#if(DEBUG_LOGOUT_EN == 1)
				{ //输出打印，谨记 用后注释，否则占用大量代码空间
					memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
					sprintf(log_buf, "differ eachCtrl detect from poling, currentVal is: %02X %02X %02X.\n", (int)colonyCtrlGet_statusLocalEaCtrl[0],
																											 (int)colonyCtrlGet_statusLocalEaCtrl[1],
																											 (int)colonyCtrlGet_statusLocalEaCtrl[2]);
					PrintString1_logOut(log_buf);
				}			
#endif
				//上一次互控若没有收到，则进行补偿操作
				memcpy(colonyCtrlGet_statusLocalEaCtrl, &dats.dats[1], clusterNum_usr);
				
				for(loop = 0; loop < clusterNum_usr; loop ++){ //掩码判断操作位，便于初始化操作抛弃无效值
				
					if(colonyCtrlGet_statusLocalEaCtrl[loop] == STATUSLOCALEACTRL_VALMASKRESERVE_ON)statusRelay_temp |= (1 << loop);
					if(colonyCtrlGet_statusLocalEaCtrl[loop] == STATUSLOCALEACTRL_VALMASKRESERVE_OFF)statusRelay_temp &= ~(1 << loop);
					
					swCommand_fromUsr.actMethod = relay_OnOff;
					swCommand_fromUsr.objRelay = statusRelay_temp;
				}
			}
			
		}break;
#else /*宏判中*///集群控制信息周期查询使能
		case ZIGB_SYSCMD_COLONYPARAM_REQPERIOD:{ /*宏失能*///仅接收场景控制信息并动作
		
			u8 idata statusRelay_temp = status_Relay; //开关动作执行缓存
			
			if(dats.dats[0] != colonyCtrlGet_statusLocalScene){ //场景状态值轮询更新
			
#if(DEBUG_LOGOUT_EN == 1)
				{ //输出打印，谨记 用后注释，否则占用大量代码空间
					memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
					sprintf(log_buf, "differ scene detect from poling.\n");
					PrintString1_logOut(log_buf);
				}			
#endif	
				
				//上一次场景控制若没有收到，则进行补偿操作
				colonyCtrlGet_statusLocalScene = dats.dats[0];
				
				if(colonyCtrlGet_statusLocalScene <= 0x07){ //3bit以内操作为有效值，便于初始化操作抛弃无效值
				
					swCommand_fromUsr.actMethod = relay_OnOff;
					swCommand_fromUsr.objRelay = colonyCtrlGet_statusLocalScene;
				}
			}
			
		}break;
#endif /*宏判尾*///集群控制信息周期查询使能
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
					memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
					sprintf(log_buf, "cmdComing:%02X.\n", (int)datsParam[3]);
					PrintString1_logOut(log_buf);
				}			
#endif		
				memset(paramTX_temp, 0, sizeof(u8) * dataLen_dataParingNomal);
			
				switch(datsParam[3]){
				
					case FRAME_MtoZIGBCMD_cmdConfigSearch:{
						
						if(!deviceLock_flag){ //设备是否上锁
							
							tips_statusChangeToNormal();
							if(countEN_ifTipsFree)countEN_ifTipsFree = 0; //触摸释放计时失能
							
							paramTX_temp[11] = status_Relay; //开关状态回复填装
							paramTX_temp[12] = DEVICE_VERSION_NUM; //设备版本号填装
							
							paramTX_temp[14] = (u8)((dev_currentPanid & 0xFF00) >> 8); //网络PANID回复填装
							paramTX_temp[15] = (u8)((dev_currentPanid & 0x00FF) >> 0);
							
							sysTimeZone_H = datsParam[12];
							sysTimeZone_M = datsParam[13];
							coverEEPROM_write_n(EEPROM_ADDR_timeZone_H, &sysTimeZone_H, 1);
							coverEEPROM_write_n(EEPROM_ADDR_timeZone_M, &sysTimeZone_M, 1);
							
							periodDataTrans_momentHang(10); //接收到搜索码后 将其他周期主动发码通信进行避让 为搜索响应清路 10s
						
							respond_IF 		= 1; //响应回复
							specialCmd_IF 	= 0;
							
						}else{
						
							
						}
						
					}break;
					
					case FRAME_MtoZIGBCMD_cmdControl:{

						paramTX_temp[11] = 0;
						paramTX_temp[11] |= (datsParam[11] & 0x07);	//状态位填装
						if(		(datsParam[11] & 0x01) != (status_Relay & 0x01))paramTX_temp[11] |= 0x20; //更改值填装<高三位>第一位
						else if((datsParam[11] & 0x02) != (status_Relay & 0x02))paramTX_temp[11] |= 0x40; //更改值填装<高三位>第二位
						else if((datsParam[11] & 0x04) != (status_Relay & 0x04))paramTX_temp[11] |= 0x80; //更改值填装<高三位>第三位
						
						swCommand_fromUsr.objRelay = datsParam[11];
						swCommand_fromUsr.actMethod = relay_OnOff;

						respond_IF 		= 1; //响应回复
						specialCmd_IF 	= 0;							
						
					}break;
						
					case FRAME_MtoZIGBCMD_cmdQuery:{
					
						paramTX_temp[11] = status_Relay & 0x07;
						paramTX_temp[12] = 0;
						(deviceLock_flag)?(paramTX_temp[12] |= 0x01):(paramTX_temp[12] &= ~0x01);
						(ifNightMode_sw_running_FLAG)?(paramTX_temp[12] |= 0x02):(paramTX_temp[12] &= ~0x02);
						
						respond_IF 		= 1; //响应回复
						specialCmd_IF 	= 0;	
					
					}break;
						
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
								
									if(swTim_oneShoot_FLAG & (1 << loop)){
										
										paramTX_temp[14 + loop * 3] &= 0x80;
									}
								}
										
								specialCmd_IF = 1; //特殊占位指令
								
#if(DEBUG_LOGOUT_EN == 1)
								{ //输出打印，谨记 用后注释，否则占用大量代码空间
									memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
									sprintf(log_buf, ">>>>>>>>timer_tab3 respond:[%02X-%02X-%02X].\n", (int)paramTX_temp[20], (int)paramTX_temp[21], (int)paramTX_temp[22]);
									PrintString1_logOut(log_buf);
								}			
#endif	
								
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
									
										swTim_oneShoot_FLAG |= (1 << loop);	//一次性定时标志开启
										datsParam[14 + loop * 3] |= (1 << (systemTime_current.time_Week - 1)); //强行进行当前周占位，当次执行完毕后清除
										
									}else{
									
										swTim_oneShoot_FLAG &= ~(1 << loop);//一次性定时标志关闭
									}
								}
								coverEEPROM_write_n(EEPROM_ADDR_swTimeTab, &datsParam[14], 4 * 3);	//定时表
								itrf_datsTiming_read_eeprom(); //普通开关定时表更新<<<运行缓存更新
#if(DEBUG_LOGOUT_EN == 1)
								{ //输出打印，谨记 用后注释，否则占用大量代码空间
									memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
									sprintf(log_buf, ">>>>>>>>timer_tab3 has been set:[%02X-%02X-%02X].\n", (int)datsParam[20], (int)datsParam[21], (int)datsParam[22]);
									PrintString1_logOut(log_buf);
								}			
#endif	
							
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
								itrf_datsTimNight_read_eeprom(); //夜间模式定时表更新<<<运行缓存更新
								
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
					
//					case FRAME_MtoZIGBCMD_cmdCfg_scenarioSet:{
//						
//						u16 xdata panid_Temp = ZigB_getPanIDCurrent(); //配置回复添加PANID
//					
//						bit opt_result = swScenario_oprateSave(datsParam[12], datsParam[14]);
//						if(opt_result)paramTX_temp[12] = 0;
//						else paramTX_temp[12] = 0x0A; //场景设置无效回复（场景存储已满）
//						
//						paramTX_temp[14] = (u8)((panid_Temp & 0xFF00) >> 8);
//						paramTX_temp[15] = (u8)((panid_Temp & 0x00FF) >> 0);
//						
//						respond_IF = 1; //响应回复使能（本地存储已被占满）
//					
//					}break;
//					
//					case FRAME_MtoZIGBCMD_cmdCfg_scenarioCtl:{
//						
//						u8 sw_Act = swScenario_oprateCheck(datsParam[12]);
//						if(sw_Act != SW_SCENCRAIO_ACTINVALID){ //若索引到有效操作位
//							
//							swCommand_fromUsr.actMethod = relay_OnOff;
//							swCommand_fromUsr.objRelay = sw_Act;
//						
//							paramTX_temp[12] = 0;
//							
//						}else{ //若无法索引到有效操作位
//						
//							paramTX_temp[12] = 0x0A; //场景控制无效回复（场景号无法被索引）
//						}
//					
//						respond_IF = 1; //响应回复使能
//					
//					}break;
//					
//					case FRAME_MtoZIGBCMD_cmdCfg_scenarioDel:{
//						
//						swScenario_oprateDele(datsParam[12]);
//						paramTX_temp[12] = 0;
//					
//						respond_IF = 1; //响应回复使能
//					
//					}break;
					
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
					
					heartBeatCount += 1; //延时性协调心跳滞后 1s
					
					datsSend_request.nwkAddr = nwkAddr_from;
					datsSend_request.portPoint = port_from;
					memset(datsSend_request.datsTrans.dats, 0, DATBASE_LENGTH * sizeof(u8));
					memcpy(datsSend_request.datsTrans.dats, paramTX_temp, datsTX_Len);
					datsSend_request.datsTrans.datsLen = datsTX_Len;
					datsRcv_respond.datsTrans.datsLen = 0;
					devRemoteDataReqMethod.keepTxUntilCmp_IF = 1; //死磕
					devRemoteDataReqMethod.datsTxKeep_Period = REMOTE_DATAREQ_TIMEOUT / 8; //死磕周期，除次比 单个超时周期内 发 8 次
					devRunning_Status = status_dataTransRequestDatsSend;
					
//#if(DEBUG_LOGOUT_EN == 1)
//					{ //输出打印，谨记 用后注释，否则占用大量代码空间
//						memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
//						sprintf(log_buf, ">>>>>>>>standby dataTX_buf_tab3:[%02X-%02X-%02X].\n", (int)datsSend_request.datsTrans.dats[20], (int)datsSend_request.datsTrans.dats[21], (int)datsSend_request.datsTrans.dats[22]);
//						PrintString1_logOut(log_buf);
//					}			
//#endif	
				}
			}
		}break;
		
		/*心跳_网关在线*/
		case ZIGB_FRAMEHEAD_HEARTBEAT:{
		
			
			
		}break;
		
		/*心跳_网关离线*///internet离线，不是zigb网络离线
		case ZIGB_FRAMEHEAD_HBOFFLINE:{
		
			
			
		}break;
		
#if(ZIGB_DATATRANS_WORKMODE == DATATRANS_WORKMODE_KEEPACESS) /*宏判头*///定时类通讯模式判断
		/*定时询访_网关在线*/
		case DTMODEKEEPACESS_FRAMEHEAD_ONLINE:{
		
			stt_agingDataSet_bitHold code 	agingCmd_Temp = {0};
			stt_devOpreatDataPonit xdata 	dev_dataPointTemp = {0};
			
			bit frameCodeCheck_PASS = 0; //校验码检查通过标志
			bit frameMacCheck_PASS  = 0; //mac地址待检查通过标志
			
//#if(DEBUG_LOGOUT_EN == 1)
//			{ //输出打印，谨记 用后注释，否则占用大量代码空间
//				memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
//				sprintf(log_buf, "periodKeepAcess respondRcv, cmd[%02X], dataLen[%02d].\n", (int)datsParam[8], (int)datsParam[1]);
//				PrintString1_logOut(log_buf);
//			}			
//#endif	
			
			if(datsParam[datsParam[1] - 1] == frame_Check(&datsParam[1], datsParam[1] - 2))frameCodeCheck_PASS = 1;
			if(!memcmp(&datsParam[2], &MAC_ID[1], 5))frameMacCheck_PASS = 1;
			
			if(frameCodeCheck_PASS && frameMacCheck_PASS){ 
		
				memcpy(&dev_dataPointTemp, &datsParam[15], sizeof(stt_devOpreatDataPonit)); //数据结构化
				
//#if(DEBUG_LOGOUT_EN == 1)
//				{ //输出打印，谨记 用后注释，否则占用大量代码空间
//					memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
//					sprintf(log_buf, "agingCmd[%02X], swAging[%d], val[%02X].\n", (int)datsParam[8],
//																				  (int)dev_dataPointTemp.devAgingOpreat_agingReference.agingCmd_swOpreat, 
//																				  (int)dev_dataPointTemp.devStatus_Reference.statusRef_swStatus);
//					PrintString1_logOut(log_buf);
//				}			
//#endif	
			
				switch(datsParam[8]){ //帧命令
				
					case DTMODEKEEPACESS_FRAMECMD_ASR:{
						
						static bit local_ftyRecover_standbyFLG = 0; //恢复出厂设置操作预动作标志
				
						/*非时效性命令判断*///数据不一致时，更新缓存后执行动作即可（非时效则每次获取数据时都要与本地数据作比较）
//						{ //普通开关操作，无需时效
//						
//							if((status_Relay & 0x07) != dev_dataPointTemp.devStatus_Reference.statusRef_swStatus){ 
//							
//								swCommand_fromUsr.objRelay = dev_dataPointTemp.devStatus_Reference.statusRef_swStatus;
//								swCommand_fromUsr.actMethod = relay_OnOff;
//								
//#if(DEBUG_LOGOUT_EN == 1)
//								{ //输出打印，谨记 用后注释，否则占用大量代码空间
//									
//									memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
//									sprintf(log_buf, ">>>>>>>>relayStatus reales:%02X, currentVal:%02X.\n", (int)status_Relay & 0x07, (int)dev_dataPointTemp.devStatus_Reference.statusRef_swStatus);
//									PrintString1_logOut(log_buf);
//								}			
//#endif	
//							}
//						}
						
//						{ //开关定时操作，无需时效
//						
//							timing_Dats xdata timDatsTemp_CalibrateTab[TIMEER_TABLENGTH] = {0};
//							
//							datsTiming_read_eeprom(timDatsTemp_CalibrateTab);
//							if(memcmp(timDatsTemp_CalibrateTab, dev_dataPointTemp.devData_timer, sizeof(timing_Dats) * TIMEER_TABLENGTH)){ //数据不一致则更新eeprom
//							
//								coverEEPROM_write_n(EEPROM_ADDR_swTimeTab, dev_dataPointTemp.devData_timer, sizeof(timing_Dats) * TIMEER_TABLENGTH);
//							}
//						}
						
//						{ //绿色模式操作，无需时效
//						
//							if(delayPeriod_closeLoop != dev_dataPointTemp.devData_greenMode){
//							
//								
//							}
//						}
						
//						{ //夜间模式操作，无需时效
//							
//							timing_Dats xdata nightDatsTemp_CalibrateTab[2];
//							
//							datsTimNight_read_eeprom(nightDatsTemp_CalibrateTab);
//							if(memcmp(nightDatsTemp_CalibrateTab, dev_dataPointTemp.devData_nightMode, sizeof(timing_Dats) * 2)){ //数据不一致则更新eeprom
//							
//								coverEEPROM_write_n(EEPROM_ADDR_TimeTabNightMode, dev_dataPointTemp.devData_nightMode, sizeof(timing_Dats) * 2);
//							}	
//						}
						
//						{ //背光灯设置操作，无需时效
//						
//							if(tipsInsert_swLedBKG_ON != dev_dataPointTemp.devData_bkLight[0]){ //开色
//							
//								
//							}
//							
//							if(tipsInsert_swLedBKG_OFF != dev_dataPointTemp.devData_bkLight[1]){ //关色
//							
//								
//							}
//						}
						
						/*时效性命令判断*///更新时效操作后，清空时效操作位（时效是为了节约性能不用每次查询时都作比较）
						if(memcmp(&agingCmd_Temp, &dev_dataPointTemp.devAgingOpreat_agingReference, sizeof(stt_agingDataSet_bitHold))){ //一旦有时效指令位置 1 ，只要有时效占位，就原位回发
							
							heartBeatCount = PERIOD_HEARTBEAT_ASR; //有时效控制，强行提前心跳立即回码
							
							memcpy(&dev_agingCmd_rcvPassive, &dev_dataPointTemp.devAgingOpreat_agingReference, sizeof(stt_agingDataSet_bitHold)); //本地被动时效操作缓存同步更新，用于原位回发
						
							/*时效操作解析*/		

							if(dev_dataPointTemp.devAgingOpreat_agingReference.agingCmd_swOpreat){ //开关状态操作，需要时效
								
								if((status_Relay & 0x07) != dev_dataPointTemp.devStatus_Reference.statusRef_swStatus){ 
								
									swCommand_fromUsr.objRelay = dev_dataPointTemp.devStatus_Reference.statusRef_swStatus;
									swCommand_fromUsr.actMethod = relay_OnOff;
									
									devActionPush_IF.push_IF = 1; //推送使能
#if(DEBUG_LOGOUT_EN == 1)
									{ //输出打印，谨记 用后注释，否则占用大量代码空间
										
										memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
										sprintf(log_buf, ">>>>>>>>relayStatus reales:%02X, currentVal:%02X.\n", (int)status_Relay & 0x07, (int)dev_dataPointTemp.devStatus_Reference.statusRef_swStatus);
										PrintString1_logOut(log_buf);
									}			
#endif	
								}
							}
							
							if(dev_dataPointTemp.devAgingOpreat_agingReference.agingCmd_delaySetOpreat){ //延时设置操作，需要时效
			
								if(dev_dataPointTemp.devData_delayer){ //延时时间大于0就是开
								
									ifDelay_sw_running_FLAG |= (1 << 1); //延时标志更新，启动
									delayPeriod_onoff 		= dev_dataPointTemp.devData_delayer; //延时时间
									delayUp_act		  		= dev_dataPointTemp.devData_delayUpStatus; //延时到达时，开关响应状态
									delayCnt_onoff			= 0; //延时计数清零
#if(DEBUG_LOGOUT_EN == 1)
									{ //输出打印，谨记 用后注释，否则占用大量代码空间
										
										memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
										sprintf(log_buf, ">>>>>>>>delayPeriod:[%d], delayUpAct:[%02X].\n", (int)delayPeriod_onoff, (int)delayUp_act);
										PrintString1_logOut(log_buf);
									}			
#endif	
									
								}else{
								
									ifDelay_sw_running_FLAG &= ~(1 << 0); //延时标志更新
									delayPeriod_onoff 		= 0; 
									delayCnt_onoff			= 0; 
								}
							}
							
							if(dev_dataPointTemp.devAgingOpreat_agingReference.agingCmd_devResetOpreat){ //出厂设置复位动作，需要时效
								
								local_ftyRecover_standbyFLG = 1; //接收到恢复出厂动作后，将恢复出厂设置动作进行就绪态记录，等待时效标志置零后再进行实际动作					
#if(DEBUG_LOGOUT_EN == 1)
								{ //输出打印，谨记 用后注释，否则占用大量代码空间
						
									memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
									sprintf(log_buf, ">>>>>>>>factory recover standBy!.\n");
									PrintString1_logOut(log_buf);
								}			
#endif		
							}
							
							if(dev_dataPointTemp.devAgingOpreat_agingReference.agingCmd_devLock){ //设备锁设置操作，需要时效
							
								u8 deviceLock_IF = 0; //操作字节缓存
								
#if(DEBUG_LOGOUT_EN == 1)
								{ //输出打印，谨记 用后注释，否则占用大量代码空间
									
									memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
									sprintf(log_buf, ">>>>>>>>agingCmd devLock coming, lockIf:[%d].\n", (int)dev_dataPointTemp.devStatus_Reference.statusRef_devLock);
									PrintString1_logOut(log_buf);
								}			
#endif	
								
								if(dev_dataPointTemp.devStatus_Reference.statusRef_devLock){ //数据放在状态里
								
									deviceLock_flag = deviceLock_IF = 1; //运行缓存更新
									
								}else{
								
									deviceLock_flag = deviceLock_IF = 0; //运行缓存更新
								}
								coverEEPROM_write_n(EEPROM_ADDR_deviceLockFLAG, &deviceLock_IF, 1); //直接更新eeprom数据
							}
							
							if(dev_dataPointTemp.devAgingOpreat_agingReference.agingCmd_timerSetOpreat){ //定时设置操作，需要时效
								
								u8 loop = 0;
#if(DEBUG_LOGOUT_EN == 1)
								{ //输出打印，谨记 用后注释，否则占用大量代码空间
									
									memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
									sprintf(log_buf, ">>>>>>>>agingCmd timer coming, dataTab1:[%02X-%02X-%02X].\n", (int)dev_dataPointTemp.devData_timer[0],
																													(int)dev_dataPointTemp.devData_timer[1],
																													(int)dev_dataPointTemp.devData_timer[2]);
									PrintString1_logOut(log_buf);
								}			
#endif	
								for(loop = 0; loop < TIMEER_TABLENGTH; loop ++){ //运行缓存更新
								
									if(dev_dataPointTemp.devData_timer[loop * 3] == 0x80){	/*一次性定时判断*///周占位为空，而定时器被打开，说明是一次性
									
										swTim_oneShoot_FLAG |= (1 << loop);	//一次性定时标志开启
										dev_dataPointTemp.devData_timer[loop * 3] |= (1 << (systemTime_current.time_Week - 1)); //强行进行当前周占位，当次执行完毕后清除
										
									}else{
									
										swTim_oneShoot_FLAG &= ~(1 << loop); //一次性定时标志关闭
									}
								}
								coverEEPROM_write_n(EEPROM_ADDR_swTimeTab, dev_dataPointTemp.devData_timer, sizeof(timing_Dats) * TIMEER_TABLENGTH); //直接更新eeprom数据
								itrf_datsTiming_read_eeprom(); //普通开关定时表更新<<<运行缓存更新
							}
							
							if(dev_dataPointTemp.devAgingOpreat_agingReference.agingCmd_greenModeSetOpreat){ //绿色模式设置操作，需要时效
								
#if(DEBUG_LOGOUT_EN == 1)
								{ //输出打印，谨记 用后注释，否则占用大量代码空间
									
									memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
									sprintf(log_buf, ">>>>>>>>agingCmd greenMode coming, timeSet:%d.\n", (int)dev_dataPointTemp.devData_greenMode);
									PrintString1_logOut(log_buf);
								}			
#endif	
	
								(dev_dataPointTemp.devData_greenMode)?(ifDelay_sw_running_FLAG |= (1 << 0)):(ifDelay_sw_running_FLAG &= ~(1 << 0)); //更新运行缓存
								delayPeriod_closeLoop = dev_dataPointTemp.devData_greenMode;
								coverEEPROM_write_n(EEPROM_ADDR_swDelayFLAG, &ifDelay_sw_running_FLAG, 1); //直接更新eeprom数据
								coverEEPROM_write_n(EEPROM_ADDR_periodCloseLoop, &delayPeriod_closeLoop, 1);
							}
							
							if(dev_dataPointTemp.devAgingOpreat_agingReference.agingCmd_nightModeSetOpreat){ //夜间模式设置操作，需要时效
								
								u8 dataTemp[sizeof(timing_Dats) * 2] = {0};
#if(DEBUG_LOGOUT_EN == 1)
								{ //输出打印，谨记 用后注释，否则占用大量代码空间
									
									memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
									sprintf(log_buf, ">>>>>>>>agingCmd nightMode coming with dats:[%02X %02X %02X", (int)dev_dataPointTemp.devData_nightMode[0],
																													(int)dev_dataPointTemp.devData_nightMode[1],
																													(int)dev_dataPointTemp.devData_nightMode[2]);
									PrintString1_logOut(log_buf);
																													
									memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
									sprintf(log_buf, " %02X %02X %02X].\n", 	(int)dev_dataPointTemp.devData_nightMode[3],
																				(int)dev_dataPointTemp.devData_nightMode[4],
																				(int)dev_dataPointTemp.devData_nightMode[5]);
									PrintString1_logOut(log_buf);
								}			
#endif	
								(dev_dataPointTemp.devData_nightMode[0])?(dataTemp[0] |= 0x7f):(dataTemp[0] &= ~0x7f); //全天夜间
								(dev_dataPointTemp.devData_nightMode[1])?(dataTemp[0] |= 0x80):(dataTemp[0] &= ~0x80); //时段夜间
								dataTemp[1] = dev_dataPointTemp.devData_nightMode[2]; //字节下标2 低5位 时
								dataTemp[2] = dev_dataPointTemp.devData_nightMode[3]; //字节下标3 全8位 分
								dataTemp[4] = dev_dataPointTemp.devData_nightMode[4]; //字节下标4 低5位 时
								dataTemp[5] = dev_dataPointTemp.devData_nightMode[5]; //字节下标5 全8位 分
								
								coverEEPROM_write_n(EEPROM_ADDR_TimeTabNightMode, dataTemp, sizeof(timing_Dats) * 2); //直接更新eeprom数据
								itrf_datsTimNight_read_eeprom(); //夜间模式定时表更新<<<运行缓存更新
							}
							
							if(dev_dataPointTemp.devAgingOpreat_agingReference.agingCmd_bkLightSetOpreat){ //背光灯设置操作，需要时效
								
#if(DEBUG_LOGOUT_EN == 1)
								{ //输出打印，谨记 用后注释，否则占用大量代码空间
									
									memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
									sprintf(log_buf, ">>>>>>>>agingCmd bkLight coming, on-Isrt:%02d, off-Isrt:%02d.\n", (int)dev_dataPointTemp.devData_bkLight[0], (int)dev_dataPointTemp.devData_bkLight[1]);
									PrintString1_logOut(log_buf);
								}			
#endif	
							
								tipsInsert_swLedBKG_ON 	= dev_dataPointTemp.devData_bkLight[0]; //更新运行缓存
								tipsInsert_swLedBKG_OFF = dev_dataPointTemp.devData_bkLight[1]; //直接更新eeprom数据
								coverEEPROM_write_n(EEPROM_ADDR_ledSWBackGround, dev_dataPointTemp.devData_bkLight, 2);
							}
							
							if(dev_dataPointTemp.devAgingOpreat_agingReference.agingCmd_horsingLight){ //跑马灯设置操作，需要时效
							
#if(DEBUG_LOGOUT_EN == 1)
								{ //输出打印，谨记 用后注释，否则占用大量代码空间
									
									memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
									sprintf(log_buf, ">>>>>>>>agingCmd horsingLight coming, opreatData:%02X.\n", (int)dev_dataPointTemp.devStatus_Reference.statusRef_horsingLight);
									PrintString1_logOut(log_buf);
								}	
#endif	
								
								ifHorsingLight_running_FLAG = dev_dataPointTemp.devStatus_Reference.statusRef_horsingLight;
								if(ifHorsingLight_running_FLAG)counter_ifTipsFree = 0;
								else tips_statusChangeToNormal();
							}
							
							if(dev_dataPointTemp.devAgingOpreat_agingReference.agingCmd_switchBitBindSetOpreat){ //开关位互控绑定操作设置操作，需要时效
							
								u8 xdata loop = 0;
#if(DEBUG_LOGOUT_EN == 1)
								{ //输出打印，谨记 用后注释，否则占用大量代码空间
									
									memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
									sprintf(log_buf, ">>>>>>>>agingCmd switchBindSet coming, opreatBitHold:%02X bindData:%02X %02X %02X.\n", (int)dev_dataPointTemp.devAgingOpreat_agingReference.agingCmd_switchBitBindSetOpreat,
																																			 (int)dev_dataPointTemp.devData_switchBitBind[0], 
																																			 (int)dev_dataPointTemp.devData_switchBitBind[1], 
																																			 (int)dev_dataPointTemp.devData_switchBitBind[2]);
									PrintString1_logOut(log_buf);
								}	
#endif	
								
								for(loop = 0; loop < 3; loop ++){
								
									if(dev_dataPointTemp.devAgingOpreat_agingReference.agingCmd_switchBitBindSetOpreat & (1 << loop)){
									
										coverEEPROM_write_n(EEPROM_ADDR_portCtrlEachOther + loop, &dev_dataPointTemp.devData_switchBitBind[loop], 1);
										CTRLEATHER_PORT[loop] = dev_dataPointTemp.devData_switchBitBind[loop];
									}
								}
								
								reConnectAfterDatsReq_IF = 1; //即刻注册互控通讯簇端口
							}
						
						}else{
						
							memset(&dev_agingCmd_rcvPassive, 0, sizeof(stt_agingDataSet_bitHold)); //本地被动时效操作缓存清零
							
							if(local_ftyRecover_standbyFLG){ //当时效标志置零后才执行恢复出厂动作，否则上位机会一直往下发复位
								
#if(DEBUG_LOGOUT_EN == 1)
								{ //输出打印，谨记 用后注释，否则占用大量代码空间
									
									memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
									sprintf(log_buf, ">>>>>>>>factory recover doing now!.\n");
									PrintString1_logOut(log_buf);
								}			
#endif	
								local_ftyRecover_standbyFLG = 0;
								Factory_recover();
							}
						}
						
					}break;
						
					case DTMODEKEEPACESS_FRAMECMD_PST:{
						
						if(memcmp(&dev_agingCmd_sndInitative, &dev_dataPointTemp.devAgingOpreat_agingReference, sizeof(stt_agingDataSet_bitHold))){ //本地主动时效操作缓存同步更新，时效占位与本地不一致 询访命令就一直为主动
						
							dtModeKeepAcess_currentCmd = DTMODEKEEPACESS_FRAMECMD_PST;
							
						}else{
						
							heartBeatPeriod	= PERIOD_HEARTBEAT_ASR; //切换为被动询访，心跳周期改为被动
							memset(&dev_agingCmd_sndInitative, 0, sizeof(stt_agingDataSet_bitHold)); //本地主动时效操作缓存清零
							dtModeKeepAcess_currentCmd = DTMODEKEEPACESS_FRAMECMD_ASR;
						}
						
					}break;
						
					default:{}break;
				}
			}
			
		}break;
		
		/*定时询访_网关离线*///internet离线，不是zigb网络离线
		case DTMODEKEEPACESS_FRAMEHEAD_OFFLINE:{
			
			periodDataTrans_momentHang(6); //internet离线情况下，周期通讯就没用了，通信频次下降到 6s/次
#if(DEBUG_LOGOUT_EN == 1)
			{ //输出打印，谨记 用后注释，否则占用大量代码空间
				
				memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
				sprintf(log_buf, ">>>>>>>>internet offline.\n");
				PrintString1_logOut(log_buf);
			}	
#endif
		}break;
		
#endif /*宏判尾*///定时类通讯模式判断
		
		default:{}break;
	}
}

/*恢复出厂预置动作*/
void fun_factoryRecoverOpreat(void){

	devStatus_switch.statusChange_standBy = status_devFactoryRecoverStandBy;
	devStatus_switch.statusChange_IF = 1;
	
	factoryRecover_HoldTimeCount = 6;
	factoryRecover_standBy_FLG = 1;
	tips_statusChangeToFactoryRecover(6);
}

/*zigbee主线程*///动作阻塞大于200ms的函数都设为状态机运行，其它小于200ms函数，阻塞维持，否则状态机复杂度加大
void thread_dataTrans(void){
	
	u8 code cmd_datsComing[2] = {0x44, 0x81}; //远端数据帧指令
	u8 code cmd_nwkOpenNote[2] = {0x45, 0xCB}; //网络开放通知

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
			
			{/*初始化时间赋值*///仅开机赋值一次，不做时区调整
				static bit FLG_timeSetInit = 1;
				
				if(FLG_timeSetInit){
				
					FLG_timeSetInit = 0;
					zigB_sysTimeSet(1533810700UL - 946713600UL, 0); //zigbee时间戳从unix纪元946713600<2000/01/01 00:00:00>开始计算
					
					dev_currentPanid = ZigB_getPanIDCurrent(); //开机后获取一次PINID
#if(DEBUG_LOGOUT_EN == 1)
					{ //输出打印，谨记 用后注释，否则占用大量代码空间
						
						memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
						sprintf(log_buf, "currentPain get is :%04X.\n", dev_currentPanid);
						PrintString1_logOut(log_buf);
					}	
#endif
				}
			}
			
			if(devTips_status == status_tipsNwkFind)tips_statusChangeToNormal(); //tips复原(网络已加入，恢复正常tips)
			
			{/*网关主机丢失检测*///即时更新tips
			
				if(!timeCounter_coordinatorLost_detecting)devTips_nwkZigb = nwkZigb_outLine; //网关丢失tips与掉线tips判为一类
			}
			
			//--------------------------------主状态业务：数据持续发送机制执行（无视回码）----------------------------//主要用于针对互控业务
			{
				
				u16 code DTREQ_EXATTR_ONCEPERIOD = 251; //单次发送间隔时间定义 单位：ms
				u16 idata constandLoop_reserve = datsSend_requestEx[0].constant_Loop + datsSend_requestEx[1].constant_Loop + datsSend_requestEx[2].constant_Loop;
				
				if(constandLoop_reserve){
				
					if(!dtReqEx_counter){
					
						u16 idata current_Insert = constandLoop_reserve % 3; //次序轮流
						
#if(DEBUG_LOGOUT_EN == 1)
						if((constandLoop_reserve % 3) == 0){ //输出打印，谨记 用后注释，否则占用大量代码空间(3个loop打印一次)
							
							memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
							sprintf(log_buf, ">>>dtCtrlEach loopParam:%d %d %d.\n", (int)datsSend_requestEx[0].constant_Loop,
																					(int)datsSend_requestEx[1].constant_Loop,
																					(int)datsSend_requestEx[2].constant_Loop);
							PrintString1_logOut(log_buf);
						}	
#endif
						
						while(!datsSend_requestEx[current_Insert].constant_Loop){ //次序轮流前提下，筛选可用
						
							current_Insert ++;
							current_Insert %= 3;
						}
						
						datsSend_requestEx[current_Insert].constant_Loop --;
						
						datsSend_requestEx[current_Insert].dats[1] = datsSend_requestEx[current_Insert].constant_Loop; //实时更新持续发送剩余次数值
						dataSendRemote_straightforward(datsSend_requestEx[current_Insert].nwkAddr, datsSend_requestEx[current_Insert].portPoint, datsSend_requestEx[current_Insert].dats, datsSend_requestEx[current_Insert].datsLen);
						
						dtReqEx_counter = DTREQ_EXATTR_ONCEPERIOD;
					}
				}
			}
	
			//--------------------------------主状态业务：心跳--------------------------------------------------------//
			if(heartBeatCycle_FLG && !heartBeatHang_timeCnt){ //心跳触发标志 及 挂起时间 判断
				
				u8 xdata frame_dataLen = 0; //待发送数据帧长度
			
				heartBeatCycle_FLG = 0;
				heartBeat_cmdFLG = !heartBeat_cmdFLG;
				
				memset(paramTX_temp, 0, sizeof(u8) * dataLen_zigbDatsTrans); //清缓存
				
#if(ZIGB_DATATRANS_WORKMODE == DATATRANS_WORKMODE_HEARTBEAT) //原即时通讯机制心跳
				
				frame_dataLen = 14;
				paramTX_temp[0] = ZIGB_FRAMEHEAD_HEARTBEAT;
				paramTX_temp[1] = frame_dataLen ;
				(heartBeat_cmdFLG)?(paramTX_temp[2] = FRAME_HEARTBEAT_cmdOdd):(paramTX_temp[2] = FRAME_HEARTBEAT_cmdEven);
				memcpy(&paramTX_temp[4], &MAC_ID[1], 5);
				
				if(heartBeat_cmdFLG){ //奇包
				
					
				
				}else{ //偶包
				
					
				}
#elif(ZIGB_DATATRANS_WORKMODE == DATATRANS_WORKMODE_KEEPACESS)	//新周期询访机制询访周期
				
				//状态填装-实时值
				dev_currentDataPoint.devStatus_Reference.statusRef_swStatus 	= status_Relay; //周期询访本地数据点 开关状态更新
				if(devActionPush_IF.push_IF){ //推送数据加载
				
					devActionPush_IF.push_IF = 0;
					dev_currentDataPoint.devStatus_Reference.statusRef_swPush = 0;
					dev_currentDataPoint.devStatus_Reference.statusRef_swPush |= devActionPush_IF.dats_Push;
				}
				dev_currentDataPoint.devStatus_Reference.statusRef_timer 		= ifTim_sw_running_FLAG; //周期询访本地数据点 定时器状态更新
				dev_currentDataPoint.devStatus_Reference.statusRef_devLock		= deviceLock_flag;
				dev_currentDataPoint.devStatus_Reference.statusRef_delay		= (ifDelay_sw_running_FLAG & 0x02) >> 1;
				dev_currentDataPoint.devStatus_Reference.statusRef_greenMode	= (ifDelay_sw_running_FLAG & 0x01) >> 0;
				dev_currentDataPoint.devStatus_Reference.statusRef_nightMode	= ifNightMode_sw_running_FLAG;
				dev_currentDataPoint.devStatus_Reference.statusRef_horsingLight	= ifHorsingLight_running_FLAG;
				
				//属性值填装-实时值
				{
					u8 xdata loop = 0;
					EEPROM_read_n(EEPROM_ADDR_swTimeTab, &dev_currentDataPoint.devData_timer, sizeof(timing_Dats) * TIMEER_TABLENGTH); //定时数据
					for(loop = 0; loop < TIMEER_TABLENGTH; loop ++){ //一次性周占位恢复
					
						if(swTim_oneShoot_FLAG & (1 << loop))dev_currentDataPoint.devData_timer[3 * loop] = 0x80; //针对一次性定时回码周占位清空
					}
				}
				dev_currentDataPoint.devData_delayer 		= delayPeriod_onoff - (delayCnt_onoff / 60); //延时数据
				dev_currentDataPoint.devData_delayUpStatus	= delayUp_act; //延时响应状态数据
				dev_currentDataPoint.devData_greenMode 		= delayPeriod_closeLoop; //绿色模式状态数据
				{ //夜间模式数据特殊转换
					
					timing_Dats xdata nightDatsTemp_CalibrateTab[2] = {0};
					
					datsTimNight_read_eeprom(nightDatsTemp_CalibrateTab);
					((nightDatsTemp_CalibrateTab[0].Week_Num & 0x7F) == 0x7F)?(dev_currentDataPoint.devData_nightMode[0] = 1):(dev_currentDataPoint.devData_nightMode[0] = 0);
					(nightDatsTemp_CalibrateTab[0].if_Timing)?(dev_currentDataPoint.devData_nightMode[1] = 1):(dev_currentDataPoint.devData_nightMode[1] = 0);
					dev_currentDataPoint.devData_nightMode[2] = nightDatsTemp_CalibrateTab[0].Hour;
					dev_currentDataPoint.devData_nightMode[3] = nightDatsTemp_CalibrateTab[0].Minute;
					dev_currentDataPoint.devData_nightMode[4] = nightDatsTemp_CalibrateTab[1].Hour;
					dev_currentDataPoint.devData_nightMode[5] = nightDatsTemp_CalibrateTab[1].Minute;
					
#if(DEBUG_LOGOUT_EN == 1)
//					{ //输出打印，谨记 用后注释，否则占用大量代码空间<<<夜间模式裸数据打印
//						
//						memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
//						sprintf(log_buf, ">>>nightMode dats upload: [%02X %02X %02X", 	(int)dev_currentDataPoint.devData_nightMode[0],
//																						(int)dev_currentDataPoint.devData_nightMode[1],
//																						(int)dev_currentDataPoint.devData_nightMode[2]);
//						PrintString1_logOut(log_buf);
//																						
//						memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
//						sprintf(log_buf, " %02X %02X %02X].\n", 	(int)dev_currentDataPoint.devData_nightMode[3],
//																	(int)dev_currentDataPoint.devData_nightMode[4],
//																	(int)dev_currentDataPoint.devData_nightMode[5]);
//						PrintString1_logOut(log_buf);
//					}	

//					{ //输出打印，谨记 用后注释，否则占用大量代码空间<<<定时运行标志打印
//					
//						memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
//						sprintf(log_buf, "ifTim_sw_running_FLAG:%d\n", 	(int)ifTim_sw_running_FLAG);
//						PrintString1_logOut(log_buf);
//					}
#endif	
				}
				EEPROM_read_n(EEPROM_ADDR_ledSWBackGround, &dev_currentDataPoint.devData_bkLight, 2); //背景灯数据
				dev_currentDataPoint.devData_devReset = 0;
				EEPROM_read_n(EEPROM_ADDR_portCtrlEachOther, dev_currentDataPoint.devData_switchBitBind, clusterNum_usr); //互控绑定数据
				
				//时效操作占位指令填装
				switch(dtModeKeepAcess_currentCmd){
				
					case DTMODEKEEPACESS_FRAMECMD_ASR:{
					
						memcpy(&dev_currentDataPoint.devAgingOpreat_agingReference, &dev_agingCmd_rcvPassive, sizeof(stt_agingDataSet_bitHold));
					
					}break;
						
					case DTMODEKEEPACESS_FRAMECMD_PST:{
					
						memcpy(&dev_currentDataPoint.devAgingOpreat_agingReference, &dev_agingCmd_sndInitative, sizeof(stt_agingDataSet_bitHold));
					
					}break;
						
					default:{}break;
				}
				
				//数据帧总数据填装
				frame_dataLen 					= 0;
				paramTX_temp[frame_dataLen ++] 	= DTMODEKEEPACESS_FRAMEHEAD_ONLINE; //帧头
				paramTX_temp[frame_dataLen ++] 	= 0;  //帧长暂填0，最后更新
				memcpy(&paramTX_temp[frame_dataLen], &MAC_ID[1], 5); //MAC
				frame_dataLen += 6; //空出1Byte MAC
				paramTX_temp[frame_dataLen ++] 	= dtModeKeepAcess_currentCmd; //命令
				paramTX_temp[frame_dataLen ++] 	= SWITCH_TYPE; //开关信息
				paramTX_temp[frame_dataLen ++] 	= (u8)((dev_currentPanid & 0xFF00) >> 8); //PANID填装
				paramTX_temp[frame_dataLen ++] 	= (u8)((dev_currentPanid & 0x00FF) >> 0); 
				paramTX_temp[frame_dataLen ++] 	= DEVICE_VERSION_NUM; //设备版本号填装
				paramTX_temp[frame_dataLen ++]	= sysTimeZone_H; //时区_时
				paramTX_temp[frame_dataLen ++]	= sysTimeZone_M; //时区_分
				memcpy(&paramTX_temp[frame_dataLen], &dev_currentDataPoint, sizeof(stt_devOpreatDataPonit)); //直接数据指针对齐,数据点向数据帧待发缓存强怼
				frame_dataLen += sizeof(stt_devOpreatDataPonit);
				frame_dataLen ++;
				paramTX_temp[1]					= frame_dataLen;
				paramTX_temp[frame_dataLen - 1]	= frame_Check(&paramTX_temp[1], frame_dataLen - 2); //校验码最后算
				
//#if(DEBUG_LOGOUT_EN == 1)
//				{ //输出打印，谨记 用后注释，否则占用大量代码空间
//					u8 xdata log_buf[64];
//					
//					sprintf(log_buf, "package_num:[%02d], check_num[%02X].\n", (int)frame_dataLen, (int)paramTX_temp[frame_dataLen - 1]);
//					PrintString1_logOut(log_buf);
//				}			
//#endif	
			
#endif				
				datsSend_request.nwkAddr = 0; //仅对网关发送，进行数据转发
				datsSend_request.portPoint = ZIGB_ENDPOINT_CTRLNORMAL; //常规数据转发专用端口
				memset(datsSend_request.datsTrans.dats, 0, DATBASE_LENGTH * sizeof(u8));
				memcpy(datsSend_request.datsTrans.dats, paramTX_temp, frame_dataLen );
				datsSend_request.datsTrans.datsLen = frame_dataLen ;
				datsRcv_respond.datsTrans.datsLen = 0;
				devRemoteDataReqMethod.keepTxUntilCmp_IF = 1; //死磕
				devRemoteDataReqMethod.datsTxKeep_Period = REMOTE_DATAREQ_TIMEOUT / 4; //死磕周期，除次比 单个超时周期内 发 4次
				devRunning_Status = status_dataTransRequestDatsSend;
				
				return; //越过本次调度，占用数据发送状态更改权，先到先得，其它需要更改数据发送权的业务，发送状态保持，等待先行业务数据发送完毕
			}
			
			//------------------------------主状态业务：本地开关受集群控制状态位周期性轮询<包括有互控和场景>---------//
#if(COLONYINFO_QUERYPERIOD_EN == ENABLE) /*宏判头*///集群控制信息周期查询使能
			if(!colonyCtrlGet_queryCounter && !colonyCtrlGetHang_timeCnt){ //周期询查 及 挂起时间判断
			
				colonyCtrlGet_queryCounter = COLONYCTRLGET_QUERYPERIOD;
				
				/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>frame reference<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
				/*--------------------------------------------------------------------------------------------------------------------------*/
				/*	frame_data[0]		|	frame_data[1]		|	frame_data[2...6]	|	frame_data[7]			|	frame_data[8...10]	|
				/*--------------------------------------------------------------------------------------------------------------------------*/
				/*	命令				|	数据长度			|	本机MAC地址			|	场景说明(暂无说明)		|	互控说明(当前组号)	|		
				/*--------------------------------------------------------------------------------------------------------------------------*/
				memset(paramTX_temp, 0, sizeof(u8) * dataLen_zigbDatsTrans); //清缓存
				paramTX_temp[0] = ZIGB_SYSCMD_COLONYPARAM_REQPERIOD; //命令
				paramTX_temp[1] = clusterNum_usr + 5 + 1; //数据长度说明
				memcpy(&paramTX_temp[2], &MAC_ID[1], 5); //MAC地址填装
				paramTX_temp[7] = 0; //场景说明装载(无说明，0填充)
				memcpy(&paramTX_temp[8], CTRLEATHER_PORT, clusterNum_usr); //互控说明装载(说明端口号)
				
				datsSend_request.nwkAddr = 0; //仅对网关发送
				datsSend_request.portPoint = ZIGB_ENDPOINT_CTRLSYSZIGB; //zigb系统交互专用端口
				memset(datsSend_request.datsTrans.dats, 0, DATBASE_LENGTH * sizeof(u8));
				memcpy(datsSend_request.datsTrans.dats, paramTX_temp, paramTX_temp[1] + 2); 
				datsSend_request.datsTrans.datsLen = paramTX_temp[1] + 2;
				datsRcv_respond.datsTrans.datsLen = 0; //不需要远端应答
				devRunning_Status = status_dataTransRequestDatsSend;
				
				return; //越过本次调度，占用数据发送状态更改权，先到先得，其它需要更改数据发送权的业务，发送状态保持，等待先行业务数据发送完毕
			}
#endif /*宏判尾*///集群控制信息周期查询使能
			
			//--------------------------------主状态业务：数据推送---------------------------------------------------//	
			if(devActionPush_IF.push_IF){
				
				const bit dataFromRemote_IF = 1; //远程推送
				const bit specialCmd_IF = 0; //非特殊占位
				
				u8 xdata datsTX_Len = 0;
				
				devActionPush_IF.push_IF = 0;
				
#if(ZIGB_DATATRANS_WORKMODE == DATATRANS_WORKMODE_HEARTBEAT) //原即时通讯机制心跳
				
				memset(paramTX_temp, 0, sizeof(u8) * dataLen_zigbDatsTrans); //清缓存
				
				paramTX_temp[11] = devActionPush_IF.dats_Push; //推送信息填装

#if(DEBUG_LOGOUT_EN == 1)
				{ //输出打印，谨记 用后注释，否则占用大量代码空间
					u8 xdata log_buf[64];
					
					sprintf(log_buf, "swData push:%02X.\n", (int)devActionPush_IF.dats_Push);
					PrintString1_logOut(log_buf);
				}			
#endif	
				datsTX_Len = dtasTX_loadBasic_CUST(dataFromRemote_IF,
												   paramTX_temp,
												   33,
												   FRAME_TYPE_StoM_RCVsuccess,
												   FRAME_MtoZIGBCMD_cmdControl,
												   specialCmd_IF);
			
				datsSend_request.nwkAddr = 0; //仅对网关发送，进行数据转发
				datsSend_request.portPoint = ZIGB_ENDPOINT_CTRLNORMAL; //常规数据转发专用端口
				memset(datsSend_request.datsTrans.dats, 0, DATBASE_LENGTH * sizeof(u8));
				memcpy(datsSend_request.datsTrans.dats, paramTX_temp, datsTX_Len);
				datsSend_request.datsTrans.datsLen = datsTX_Len;
				datsRcv_respond.datsTrans.datsLen = 0; //无需远端应答
				devRunning_Status = status_dataTransRequestDatsSend; //直接切换（不做预备动作）
				
#elif(ZIGB_DATATRANS_WORKMODE == DATATRANS_WORKMODE_KEEPACESS) //周期询访机制无需推送，只需触发主动时效命令即可
				
				dev_currentDataPoint.devStatus_Reference.statusRef_swPush = (devActionPush_IF.dats_Push & 0xE0) >> 5; //属性值填装
				dev_agingCmd_sndInitative.agingCmd_swOpreat = 1; //对应主动上传时效占位置一
				dtModeKeepAcess_currentCmd = DTMODEKEEPACESS_FRAMECMD_PST;
				
				heartBeatPeriod	= PERIOD_HEARTBEAT_PST; //主动询访切换，心跳周期改为主动
				heartBeatCount  = heartBeatPeriod; 
#endif

				return; //越过本次调度，占用数据发送状态更改权，先到先得，其它需要更改数据发送权的业务，发送状态保持，等待先行业务数据发送完毕
			}
			
			//--------------------------------主状态业务：互控同步---------------------------------------------------//
			if(EACHCTRL_realesFLG){ //广播互控值
			
				if(devRunning_Status == status_passiveDataRcv){
				
					u8 idata loop;
					
					for(loop = 0; loop < clusterNum_usr; loop ++){ //三个开关位分别判定
					
						if(EACHCTRL_realesFLG & (1 << loop)){ //互控有效位判断
						
							EACHCTRL_realesFLG &= ~(1 << loop); //互控有效位清零
							
							paramTX_temp[0] = (status_Relay >> loop) & 0x01; //开关状态填装
							
							if((CTRLEATHER_PORT[loop] > CTRLEATHER_PORT_NUMSTART) && CTRLEATHER_PORT[loop] < CTRLEATHER_PORT_NUMTAIL){ //是否为有效互控端口
								
								(paramTX_temp[0])?(colonyCtrlGet_statusLocalEaCtrl[loop] = STATUSLOCALEACTRL_VALMASKRESERVE_ON):(colonyCtrlGet_statusLocalEaCtrl[loop] = STATUSLOCALEACTRL_VALMASKRESERVE_OFF); //本地互控状态查询值更新
								localDataRecord_eaCtrl[loop] = paramTX_temp[0];  //本地互控记录值更新
								colonyCtrlGet_queryCounter = COLONYCTRLGET_QUERYPERIOD; //集群信息查询主动滞后，以防与主机集群信息未更新，导致与本地信息冲突
							
								/*无视回码，持续性发送*///非常规
								datsSend_requestEx[loop].nwkAddr = 0xffff; //间接组播（对互控专用端口进行广播）
								datsSend_requestEx[loop].portPoint = CTRLEATHER_PORT[loop]; //互控位对应绑定端口
								memset(datsSend_requestEx[loop].dats, 0, 10 * sizeof(u8));
								
								datsSend_requestEx[loop].dats[0] = paramTX_temp[0];
								datsSend_requestEx[loop].dats[1] = 0; //下标2为持续发送剩余次数，在发送时实时更新，初始赋值为0
								datsSend_requestEx[loop].datsLen = 2;
								datsSend_requestEx[loop].constant_Loop = 30; //无视回码，发30次
								
//								/*常规发送，收到协议栈回码响应就停止发送，且有超时*///常规
//								datsSend_request.nwkAddr = 0xffff; //间接组播（对互控专用端口进行广播）
//								datsSend_request.portPoint = CTRLEATHER_PORT[loop]; //互控位对应绑定端口
//								memset(datsSend_request.datsTrans.dats, 0, DATBASE_LENGTH * sizeof(u8));
//								memcpy(datsSend_request.datsTrans.dats, paramTX_temp, 1);
//								datsSend_request.datsTrans.datsLen = 1;
//								datsRcv_respond.datsTrans.datsLen = 0; //无需远端应答
//								
//								devRemoteDataReqMethod.keepTxUntilCmp_IF = 1; //死磕
//								devRemoteDataReqMethod.datsTxKeep_Period = REMOTE_DATAREQ_TIMEOUT / 10; //死磕周期，除次比 单个超时周期内 发 10 次
//								
//								EACHCTRL_reportFLG = 1; //向网关汇报
//								
//								devRunning_Status = status_dataTransRequestDatsSend; //直接切换（不做预备动作）
//								
//								return; //越过本次调度，占用数据发送状态更改权，先到先得，其它需要更改数据发送权的业务，发送状态保持，等待先行业务数据发送完毕
							}
						}
					}
				}	
			}

#if(COLONYINFO_QUERYPERIOD_EN == ENABLE) /*宏判头*///集群控制信息周期查询使能	
			if(EACHCTRL_reportFLG){ //向网关单播当前所有互控组号对应的开关状态值
			
				EACHCTRL_reportFLG = 0;
				
				/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>frame reference<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
				/*----------------------------------------------------------------------------------------------------------*/
				/*	frame_data[0]		|	frame_data[1]		|	frame_data[2/4/6]	|		frame_data[3/5/7]			|
				/*----------------------------------------------------------------------------------------------------------*/
				/*	命令				|	数据长度			|	本地互控端口号		|	本地互控端口号对应开关状态值	|
				/*----------------------------------------------------------------------------------------------------------*/
				{
					u8 code remote_responseFrame[3] = {ZIGB_SYSCMD_EACHCTRL_REPORT, 0x01, 0x00}; //远端响应帧<确保主机收到>
					
#if(DEBUG_LOGOUT_EN == 1)
					{ //输出打印，谨记 用后注释，否则占用大量代码空间
						u8 xdata log_buf[64];
						
						sprintf(log_buf, "current eaCtrl insrt[2] is: %02X.\n", (int)colonyCtrlGet_statusLocalEaCtrl[1]);
						PrintString1_logOut(log_buf);
					}			
#endif	
					
					memset(datsSend_request.datsTrans.dats, 0, DATBASE_LENGTH * sizeof(u8));
					datsSend_request.datsTrans.dats[0] 	= ZIGB_SYSCMD_EACHCTRL_REPORT;
					datsSend_request.datsTrans.dats[1] 	= 6;
					datsSend_request.datsTrans.dats[2] 	= CTRLEATHER_PORT[0];
					datsSend_request.datsTrans.dats[3] 	= colonyCtrlGet_statusLocalEaCtrl[0];
					datsSend_request.datsTrans.dats[4] 	= CTRLEATHER_PORT[1];
					datsSend_request.datsTrans.dats[5] 	= colonyCtrlGet_statusLocalEaCtrl[1];
					datsSend_request.datsTrans.dats[6] 	= CTRLEATHER_PORT[2];
					datsSend_request.datsTrans.dats[7] 	= colonyCtrlGet_statusLocalEaCtrl[2];
					datsSend_request.datsTrans.datsLen 	= 8;
					datsSend_request.nwkAddr 			= 0;
					datsSend_request.portPoint 			= ZIGB_ENDPOINT_CTRLSYSZIGB;
					
					memset(datsRcv_respond.datsTrans.dats, 0, DATBASE_LENGTH * sizeof(u8)); //需要远端响应
					memcpy(datsRcv_respond.datsTrans.dats, remote_responseFrame, 3); //远端响应帧加载
					datsRcv_respond.datsTrans.datsLen	= 3;
					datsRcv_respond.nwkAddr 			= 0;
					datsRcv_respond.portPoint 			= ZIGB_ENDPOINT_CTRLSYSZIGB;
					
					devRunning_Status = status_dataTransRequestDatsSend; //直接切换（不做预备动作）
				}
				
				return; //越过本次调度，占用数据发送状态更改权，先到先得，其它需要更改数据发送权的业务，发送状态保持，等待先行业务数据发送完毕
			}
#endif /*宏判尾*///集群控制信息周期查询使能
			
			//--------------------------------主状态业务：数据解析响应-----------------------------------------------//
			if(uartRX_toutFLG){ //数据接收(帧超时)
				
				uartRX_toutFLG = 0;
				
//				if(datsRcv_ZIGB.rcvDats[0] != ZIGB_FRAME_HEAD){ //帧头不对，打印输出
//				
//#if(DEBUG_LOGOUT_EN == 1)
//						{ //输出打印，谨记 用后注释，否则占用大量代码空间
//							u8 xdata log_buf[64];
//							
//							sprintf(log_buf, "err frameHead:%02X.\n", (int)datsRcv_ZIGB.rcvDats[0]);
//							PrintString1_logOut(log_buf);
//						}			
//#endif	
//				}
				
				/*Zigbee一级协议核对解析*/
				if((datsRcv_ZIGB.rcvDats[0] == ZIGB_FRAME_HEAD) &&
					!memcmp(&datsRcv_ZIGB.rcvDats[2], cmd_datsComing, 2)){ //远端数据到来指令
					
					u16 idata datsFrom_addr = ((u16)(datsRcv_ZIGB.rcvDats[9]) << 8) | ((u16)(datsRcv_ZIGB.rcvDats[8]) << 0); //数据发送方网络地址
					u8 	idata srcPoint =  datsRcv_ZIGB.rcvDats[10];	//源端
					u8 	idata dstPoint =  datsRcv_ZIGB.rcvDats[11];	//远端
						
					if(datsFrom_addr == ZIGB_NWKADDR_CORDINATER){ //数据来源短地址检测，是否来自网关主机
					
						timeCounter_coordinatorLost_detecting = COORDINATOR_LOST_PERIOD_CONFIRM; //网关主机失联确认检测计时重置
						if(devTips_nwkZigb != nwkZigb_nwkOpen)devTips_nwkZigb = nwkZigb_Normal; //zigbTips状态响应，只要接收到网关zigb数据，tips状态就切换至正常
					}
					
					memset(paramRX_temp, 0, sizeof(u8) * dataLen_zigbDatsTrans);
					memcpy(paramRX_temp, &(datsRcv_ZIGB.rcvDats[21]), datsRcv_ZIGB.rcvDats[20]);
						
					if(srcPoint > CTRLEATHER_PORT_NUMSTART && srcPoint < CTRLEATHER_PORT_NUMTAIL){ /*互控端口*/
						
						u8 idata statusRelay_temp = status_Relay; //当前开关状态缓存
						u8 idata localActLoop = 0;
						
						colonyCtrlGet_queryCounter = COLONYCTRLGET_QUERYPERIOD; //集群信息查询主动滞后，以防与主机集群信息未更新，导致与本地信息冲突
						
#if(DEBUG_LOGOUT_EN == 1)
						{ //输出打印，谨记 用后注释，否则占用大量代码空间
							memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
							sprintf(log_buf, "ctrl eachOther cmd coming, cluster:%02d, val:%02d, loop:%02d.\n", (int)srcPoint, (int)paramRX_temp[0], (int)paramRX_temp[1]);
							PrintString1_logOut(log_buf);
						}			
#endif	
						/*互控被动响应*/
						for(localActLoop = 0; localActLoop < clusterNum_usr; localActLoop ++){
						
							if((srcPoint == CTRLEATHER_PORT[localActLoop]) && (0 != CTRLEATHER_PORT[localActLoop])){ //开关位1 互控绑定判断
							
								if(paramRX_temp[1] > datsSend_requestEx[localActLoop].constant_Loop){ //loop值大于本地才有效
								
									statusRelay_temp &= ~(1 << localActLoop); //动作位缓存清零
									swCommand_fromUsr.objRelay = statusRelay_temp | paramRX_temp[0] << localActLoop; //bit对应 开关位动作加载
									swCommand_fromUsr.actMethod = relay_OnOff;
									(paramRX_temp[0])?(colonyCtrlGet_statusLocalEaCtrl[localActLoop] = STATUSLOCALEACTRL_VALMASKRESERVE_ON):(colonyCtrlGet_statusLocalEaCtrl[localActLoop] = STATUSLOCALEACTRL_VALMASKRESERVE_OFF); //本地互控轮询值更新
									
									datsSend_requestEx[localActLoop].constant_Loop = 0;  
									
									localDataRecord_eaCtrl[localActLoop] = paramRX_temp[0]; //本地互控记录值更新
								}
								
								break;
							}
						}
						
						devActionPush_IF.push_IF = 1; //推送使能
					
					}else{ /*非互控端口*///剩下就是系统专用的15个端口
					
						switch(srcPoint){
							
							/*场景集群端口*/
							case ZIGB_ENDPOINT_CTRLSECENARIO:{	
							
								dataParing_scenarioCtrl(paramRX_temp); //场景集群控制解析
								
							}break;
						
							/*常规控制转发端口*/
							case ZIGB_ENDPOINT_CTRLNORMAL:{	
							
								if(datsFrom_addr == ZIGB_NWKADDR_CORDINATER){ //来自协调器
								
									dataParing_Nomal(paramRX_temp, datsFrom_addr, srcPoint); //常规解析
								}
								
							}break;
							
							/*zigb系统交互端口*/
							case ZIGB_ENDPOINT_CTRLSYSZIGB:{	
							
								dataParing_zigbSysCtrl(paramRX_temp); //系统控制解析
								
							}break;
								
							default:{
							
								
								
							}break;
						}
					}
					
				}else
				if((datsRcv_ZIGB.rcvDats[0] == ZIGB_FRAME_HEAD) &&
				   !memcmp(&datsRcv_ZIGB.rcvDats[2], cmd_nwkOpenNote, 2)){ //网络开放通知
					
					tips_statusChangeToZigbNwkOpen(datsRcv_ZIGB.rcvDats[4]); //tips触发
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
		
		case status_devNwkHold:{
		
			//--------------------------------协状态：网络挂起-----------------------------------------------//
			devTips_nwkZigb = nwkZigb_hold;
			function_devNwkHold();
			
		}break;
		
		case status_devFactoryRecoverStandBy:{
		
			//--------------------------------协状态：恢复出厂预置-----------------------------------------------//
			devTips_nwkZigb = nwkZigb_outLine;
			if(!factoryRecover_HoldTimeCount){
			
				if(factoryRecover_standBy_FLG)Factory_recover();
			}
			
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
