#include "driver_I2C_HXD019D.h"

#include "DS18B20.h"

#include "Tips.h"
#include "dataManage.h" 
#include "dataTrans.h"
#include "delay.h"

#include "string.h"
#include "stdio.h"

#include "eeprom.h"

#include "Relay.h"

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED) //ǿ���豸����Ϊ����ת����ʱ����������Ч

u8 IR_currentOpreatRes = 0; //�����������ظ�ֵ
u8 IR_currentOpreatinsert = 0; //����ת��������Ż���

u16 xdata infraredAct_timeCounter = 0; //���������ؼ�ʱ��һ�����ڳ�ʱ��ʱ

static enumInfrared_status xdata theadStatus_infrared = infraredSMStatus_null;

static u8 xdata IR_dataTransBuff[232] = {0}; //����оƬ�·����ݻ���
static u8 xdata IR_opToutLoop = 0; //����ת��������ʱ����

void infrared_pinInit(void){

	P3M1 |= 0x20;	//P35 -infrared_REST
	P3M0 &= ~0x20;
	
	hxd019d_pinREST = !hxd019d_resetLevel;
	
	P3M1 &= ~0x10;	//P34 -infrared_BUSY
	P3M0 &= ~0x10;
	
	hxd019d_pinBUSY = 1; //��ʼ������
}

static void ir_Delay35us(void)		//@24.000MHz -25uS-45uS
{
	unsigned char i, j;

	i = 1;
	j = 206;
	do
	{
		while (--j);
	} while (--i);
}

static void ir_Delay21ms(void)		//@24.000MHz -18mS-25mS
{
	unsigned char i, j, k;

	_nop_();
	_nop_();
	i = 2;
	j = 235;
	k = 63;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}

static u8 GetACKSign(void) 
{
	u8 ACKSign;

	SetSDAInput();
	ir_Delay35us();
	ir_Delay35us();
	ir_Delay35us();
	ir_Delay35us();

	SetSCLHigh();
	ir_Delay35us();
	ir_Delay35us();
	ir_Delay35us();
	ir_Delay35us();

	ACKSign = GetDINStatus();			

	ir_Delay35us();
	ir_Delay35us();
	ir_Delay35us();
	ir_Delay35us();

	SetSCLLow();
	ir_Delay35us();
	ir_Delay35us();
	ir_Delay35us();
	ir_Delay35us();

	return ACKSign;
}

static void SendACKSign(void)
{
	SetSDAOutput();
	ir_Delay35us();
	SetSDALow();			///hxd;��ӵ�SDA_Low,û����֤��
	ir_Delay35us();

	SetSCLHigh();			//��9�ε�CLK�ĸ�
	ir_Delay35us();			//;1mS
	
	SetSCLLow(); 
}

static void I2COpen(void)
{	
	SetSDAOutput(); 
	SetSCLOutput(); 

	SetSCLHigh();  
	SetSDAHigh();
}

static void I2CClose(void)   
{
	SetSDAOutput(); 
	SetSCLOutput(); 

	SetSCLHigh();
	SetSDAHigh();
}

static void I2CStart(void) 
{
	SetSDAOutput();
	SetSCLOutput();
	SetSCLHigh();
	SetSDAHigh();
	ir_Delay35us();
	ir_Delay35us();

	SetSDALow();
	ir_Delay35us();

	ir_Delay35us();
	ir_Delay35us();

	ir_Delay35us();
	ir_Delay35us();

	ir_Delay35us();

	ir_Delay35us();
	ir_Delay35us();

	ir_Delay35us();
	ir_Delay35us();

	SetSCLLow();  
	ir_Delay35us();
	ir_Delay35us();
}

static void I2CStop(void)
{
	SetSDAOutput();
	SetSCLOutput();
	SetSCLLow();
	SetSDALow();
	ir_Delay35us();

	SetSCLHigh();
	ir_Delay35us();

	SetSDAHigh();
	ir_Delay35us();  
}

static void I2CWriteData( char bData )
{
	u8 Data_Bit,ACKSign/*,TmpDat*/;
	char i;	//kal_int8 i;

	SetSDAOutput();
	SetSCLLow();
	ir_Delay35us();
    
	for(i=7;i>=0;i--)
	{
		ir_Delay35us();
		
		Data_Bit=(bData>>i)& 0x01;

		if(Data_Bit)
		SetSDAHigh();
		else
		SetSDALow();

		ir_Delay35us();
		SetSCLHigh();
		ir_Delay35us();
		SetSCLLow();		
	}
	
	ACKSign=GetACKSign();
}

static void writeI2C(char *data2, u8 count)		//hxd;ͨ��д��
{
	u8 i;
	u8 j = 0;
	char iBuffer;

	I2COpen();
	ir_Delay35us();

	SetSCLLow();
	ir_Delay35us();
	ir_Delay35us();
	SetSCLHigh();
	
	ir_Delay21ms(); //20ms	//14

	I2CStart();
	ir_Delay35us();
	
	for(i = 0; i < count; i++)	//count=7,��ֵ
	{
		iBuffer = data2[i];
		I2CWriteData(iBuffer);
		ir_Delay35us();
	}

	ir_Delay35us();

	I2CStop();
	ir_Delay35us();

	I2CClose();
	ir_Delay35us();
}

static void Learn_start(void)
{
		
	I2COpen();
	ir_Delay35us();

	SetSCLLow();
	ir_Delay35us();
	ir_Delay35us();
	SetSCLHigh();
	
	ir_Delay21ms(); //20ms	//14

	I2CStart();
	ir_Delay35us();
	
	I2CWriteData(0x30);
	ir_Delay35us();

	I2CWriteData(0x20);
	ir_Delay35us();
	
	I2CWriteData(0x50);
	ir_Delay35us();

	ir_Delay35us();	

	I2CStop();
	ir_Delay35us();

	I2CClose();
	ir_Delay35us();
}

static void I2CReadData(u8* pbData)
{
//	u8 Data_Bit; u8 ACKSign;
	u8 readdata = 0;
	u8 i=8;

	SetSDAInput(); 
	while (i--)
	{
		readdata<<=1;

		SetSCLHigh(); 
		ir_Delay35us();

		readdata |= GetDINStatus();  //������ʱ���������ԣ�readdata |= GetDINStatus()?0x01:0x00;

		SetSCLLow();
		ir_Delay35us();
		ir_Delay35us();		//hxd;��delay
	}
	SetSCLLow(); 
	ir_Delay35us();

	*pbData = readdata;

	SendACKSign();
	
	ir_Delay35us();
	ir_Delay35us();		//hxd;1G���ϵ�ϵͳҪ��,test

}

static void readI2C(char* readtempbuf)     //u8
{
	u8 bValue;
	u8 i=0;
	u8 checksum;

	I2COpen();
	ir_Delay35us();

	SetSCLLow();
	ir_Delay35us();
	ir_Delay35us();
	SetSCLHigh();
	ir_Delay21ms(); //20ms	

	I2CStart();
	ir_Delay35us();

	//----------------------------------------
	//write
	I2CWriteData(0x30);
	ir_Delay35us();
	//address point
	I2CWriteData(0x62);
	ir_Delay35us();

	//---------------------------------------
	//read
	I2CStart();
	ir_Delay35us();

	I2CWriteData(0x31);
	ir_Delay35us();

	I2CReadData(&bValue);	//wjs;read:FCS(1B)
	ir_Delay35us();			//wjs;1G���ϵ�ϵͳҪ��	
	
	if(bValue != 0x00)
	{
		I2CStop();
		ir_Delay35us();
		I2CClose();
		ir_Delay35us();
	//	kal_prompt_trace(MOD_TST, "remote_study_type_error");
	}

	i = 0;
	readtempbuf[i] = bValue;
//	kal_prompt_trace(MOD_TST, "remote_I2C_data[%d] = %d",i,readtempbuf[i]);
	checksum = 0xc3;

	for(i = 1; i < 230; i++)			//wjs;read:learndata(109B)+120=230
	{
		I2CReadData(&bValue);
		ir_Delay35us();
		readtempbuf[i] = bValue;
	//	kal_prompt_trace(MOD_TST, "remote_I2C_data[%d] = %d",i,readtempbuf[i]);
		checksum += bValue;
	}
	
	I2CReadData(&bValue);		//wjs;read:CK(1B)	?????
	ir_Delay35us();
	//	kal_prompt_trace(MOD_MMI, "remote_read_checksum = %d",bValue);
	//	kal_prompt_trace(MOD_MMI, "remote_count_checksum = %d",checksum);

	I2CStop();
	ir_Delay35us();
	I2CClose();
	ir_Delay35us();
//��ʱ��У
//	if(bValue != checksum)
//	{
//	//	kal_prompt_trace(MOD_MMI, "remote_study_checksum_error");
//	return 0;lg,
//	}
//	else
//	{
//	//	kal_prompt_trace(MOD_MMI, "remote_study_checksum_ok");
//		return 1;
//	}
}

void infraredOpreatAct_learnningStart(u8 opInsert){ //��Ӧ�������
	
	hxd019d_pinREST = !hxd019d_resetLevel; //��ֹӲ����λ

	if(theadStatus_infrared != infraredSMStatus_learnning){
	
		theadStatus_infrared = infraredSMStatus_learnningSTBY;
		infraredAct_timeCounter = IR_opStatusLearnningSTBY_TOUT;
		IR_opToutLoop = IR_opStatusLearnningSTBY_TOUTLOOP;
		
		IR_currentOpreatinsert = opInsert;
		
		Learn_start();
	}
}

void infraredOpreatAct_remoteControlStart(u8 opInsert){ //��Ӧ�������
	
	hxd019d_pinREST = !hxd019d_resetLevel; //��ֹӲ����λ

	theadStatus_infrared = infraredSMStatus_sigSendSTBY;
	
	infraredAct_timeCounter = IR_opStatusSigSendSTBY_TOUT; //���ƾ����ȴ�ʱ���趨
	
	infrared_eeprom_dataRead(opInsert, (u8 *)IR_dataTransBuff, 232); //�������ݻ����ȡ
	IR_currentOpreatinsert = opInsert;
	
#if(DEBUG_LOGOUT_EN == 1)
	{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�
		
		memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
		sprintf(log_buf, ">>>infrared data read[%02X] [%02X] [%02X].\n", (int)IR_dataTransBuff[7], (int)IR_dataTransBuff[8], (int)IR_dataTransBuff[9]);
		PrintString1_logOut(log_buf);
	}	
#endif
}

void infraredOpreatAct_Stop(void){

	hxd019d_pinREST = hxd019d_resetLevel; //Ӳ����λ����
	
	theadStatus_infrared = infraredSMStatus_free;
	infraredAct_timeCounter = 0;
	IR_opToutLoop = 0;
	
	memset(IR_dataTransBuff, 0, sizeof(u8) * 232);
	IR_currentOpreatinsert = 0;
	
	infraredAct_timeCounter = IR_resetOpreatTimeKeep; //��λ���ŵ͵�ƽ����ʱ��
}

enumInfrared_status infraredStatus_GET(void){

	return theadStatus_infrared;
}

/*����ת�������߳�*/
void thread_infraredSM(void){
	
#if(DEBUG_LOGOUT_EN == 1)
	static xdata status_Local = infraredSMStatus_free;
	
	if(status_Local != theadStatus_infrared){
	
		status_Local = theadStatus_infrared;
		
		{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�
			
			memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
			sprintf(log_buf, "infrared change, currentStatus:%d.\n", (int)status_Local);
			PrintString1_logOut(log_buf);
		}	
	}
#endif
	
	if(!couter_ds18b20Temperature_dtPeriod){ //�¶�ֵ���£�ds18b20 �¶Ȳɼ�ҵ��ϲ�������ת��
	
		couter_ds18b20Temperature_dtPeriod = DS18B20_TEMPRATRUE_DTPERIOD;
		temperatureCurrent_VAL = Ds18b20ReadTemp();
		
#if(DEBUG_LOGOUT_EN == 1)
		{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�
			
			memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
			sprintf(log_buf, "current temprature reales:%.2f��.\n", (float)temperatureCurrent_VAL / 100.0F);
			PrintString1_logOut(log_buf);
		}	
#endif
	}
	
	if(!dev_agingCmd_sndInitative.agingCmd_infrareOpreat)IR_currentOpreatRes = 0; //�����������Ϸ�ʱЧΪ0���򽫺��⵱ǰ�����������ֵ����

	switch(theadStatus_infrared){
	
		case infraredSMStatus_free:{ //����
		
			
		
		}break;
		
		case infraredSMStatus_learnningSTBY:{ //ѧϰ����
		
			if(IR_opToutLoop){ //����̬��ʱ����
			
				if(!hxd019d_pinBUSY){ //busyΪ�Ϳ���ѧϰ
				
					theadStatus_infrared = infraredSMStatus_learnning; //�л���ѧϰ̬
					infraredAct_timeCounter = IR_opStatusLearnning_TOUT; //ѧϰ̬��ʱʱ���趨
					
				}else{
				
					if(!infraredAct_timeCounter){ //����̬��ʱ
					
						Learn_start();
						infraredAct_timeCounter = IR_opStatusLearnningSTBY_TOUT;
						IR_opToutLoop --;
					}
				}
			
			}else{ //����̬��ʱ�����ﵽ�޶�ֵ
			
				theadStatus_infrared = infraredSMStatus_free;
			}
			
		}break;
			
		case infraredSMStatus_learnning:{ //ѧϰ
		
			if(infraredAct_timeCounter){ //ѧϰ̬��ʱʱ�䷶Χ�ڣ�δ��ʱ���ȴ�busy�ź�
			
				if(hxd019d_pinBUSY){
				
					u8 xdata check_Num = 0,
							 loopTemp = 0;
					
					//��������
					readI2C((char *)IR_dataTransBuff);
					
					//���ݴ���
					for(loopTemp = 1; loopTemp < 230; loopTemp ++)check_Num += IR_dataTransBuff[loopTemp];
					check_Num += 0x03;
					check_Num += 0x30;
					for(loopTemp = 229; loopTemp > 0; loopTemp --)IR_dataTransBuff[loopTemp + 1] = IR_dataTransBuff[loopTemp];
					IR_dataTransBuff[0] = 0x30;
					IR_dataTransBuff[1] = 0x03;
					IR_dataTransBuff[231] = check_Num;
					
					//���ݴ洢
					infrared_eeprom_dataSave(IR_currentOpreatinsert, (u8 *)IR_dataTransBuff, 232);
					theadStatus_infrared = infraredSMStatus_free; //�����ɹ����ָ�����̬
					beeps_usrActive(3, 50, 1);
					
					//ͨѶ�ظ�
					IR_currentOpreatRes = IR_OPREATRES_LEARNNING; //�������ֵ��װ
					dev_agingCmd_sndInitative.agingCmd_infrareOpreat = 1; //��Ӧ�����ϴ�ʱЧռλ��һ
					devActionPush_IF.push_IF = 1; //����ʹ��
#if(DEBUG_LOGOUT_EN == 1)
					{ //�����ӡ������ �ú�ע�ͣ�����ռ�ô�������ռ�
						
						memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
						sprintf(log_buf, ">>>infrared learnning sig got data[%02X] [%02X] [%02X].\n", (int)IR_dataTransBuff[7], (int)IR_dataTransBuff[8], (int)IR_dataTransBuff[9]);
						PrintString1_logOut(log_buf);
					}	
#endif
				}
			
			}else{ //ѧϰ̬��ʱ
			
				beeps_usrActive(3, 253, 2);
				infraredOpreatAct_Stop(); //������ֹ����
			}
			
		}break;
		
		case infraredSMStatus_sigSendSTBY:{ //���ƾ���
		
			if(!infraredAct_timeCounter)theadStatus_infrared = infraredSMStatus_sigSend; //���ƾ����ȴ�ʱ�䣬�ȴ�Ӳ���ȶ�
			
		}break;
			
		case infraredSMStatus_sigSend:{ //����
		
			//���Ʋ���
			writeI2C((char *)IR_dataTransBuff, 232);
			theadStatus_infrared = infraredSMStatus_free;
			
			//ͨѶ�ظ�
			IR_currentOpreatRes = IR_OPREATRES_CONTROL; //�������ֵ��װ
			dev_agingCmd_sndInitative.agingCmd_infrareOpreat = 1; //��Ӧ�����ϴ�ʱЧռλ��һ
			devActionPush_IF.push_IF = 1; //����ʹ��
		
		}break;
			
		case infraredSMStatus_opStop:{ //������ֹ
		
			if(!infraredAct_timeCounter){
			
				theadStatus_infrared = infraredSMStatus_free;
				hxd019d_pinREST = !hxd019d_resetLevel;
			}
		}break;
		
		default:{
		
			theadStatus_infrared = infraredSMStatus_free;
		
		}break;
	}
}
#endif