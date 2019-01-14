#include "DS18B20.h"

#include "dataManage.h" 
#include "dataTrans.h"
#include "USART.h"
#include "delay.h"

#include "string.h"
#include "stdio.h"

#include "eeprom.h"

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED) ////强制设备类型为红外转发器时，此驱动生效，红外转发器才有DSPORT18b20硬件

u8 couter_ds18b20Temperature_dtPeriod = DS18B20_TEMPRATRUE_DTPERIOD;

u16 temperatureCurrent_VAL = 0; //当前温度值 --高八位为整数部分，低八位为小数部分

void ds18b20_pinInit(void){ //上拉准双向

	P2M1 &= ~0x10;
	P2M0 &= ~0x10;
}

static void localDelay15us(void)		//@24.000MHz
{
	unsigned char i;

	i = 118;
	while (--i);
}


static void delay15us(u16 temp)	
{
	for(temp;temp>0;temp--)
	{
		localDelay15us();
	}
}
 
static bit RST(void)	 
{
	bit ret=1;
 	DSPORT=0;
	delay15us(39);
	DSPORT=1;
	delay15us(4);
	ret=DSPORT;	
	
	delay15us(27);
	DSPORT=1;
    return ret ;
}

static void writebit(bit dat_bit)
{
	DSPORT=0;
	NOP10();
	DSPORT=dat_bit;
	delay15us(4);
	DSPORT=1;	
}

static void writebyte(u8 dat_byte)
{
	u16 i;
	for(i=0;i<8;i++)
	{
		 writebit((dat_byte&0x01));
		 dat_byte>>=1;
	}
}

static bit readbit(void)
{
	bit dat_bit;
	
	DSPORT=0;
	NOP10();
	DSPORT=1;
	NOP40();
	dat_bit=DSPORT;
	delay15us(3);
	DSPORT=1;
	return dat_bit;
}

static u8 readbyte(void)
{
	u8 dat_byte=0;
	u8 i,j;
	
	for(i=0;i<8;i++)
	{
		j= readbit();
		dat_byte=(j<<7)|(dat_byte>>1);
	}
	
	return dat_byte;
}

static void sweap(void)
{
	RST();
	NOP10();
	writebyte(0xcc);
	writebyte(0x44);
}

u16 Ds18b20ReadTemp(void)
{
	u16 a,b,t,temp;
	bit flag;
	float ftemp;
	
	sweap();
	
	RST();
	NOP10();
	
	writebyte(0xcc);
	writebyte(0xbe);
	a=readbyte();
	b=readbyte();
	
	if(b&0xfc)
	{
		 temp=b;
		 temp= temp<<8;
		 temp|=a;
		 temp=((~temp)+1);
		 ftemp=temp*0.0625*100+0.5;
		 t=ftemp;
		 flag=1;
	}
	else
	{
	   	ftemp=((b*256)+a)*0.0625;
	    t=ftemp*100+0.5;
		flag=0;
	}
	
	return t;
}
#endif

