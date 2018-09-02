#include "Tips.h"

#include "Relay.h"
#include "dataManage.h"
#include "timerAct.h"

#include "eeprom.h"
#include "delay.h"

color_Attr xdata relay1_Tips 	= {0};
color_Attr xdata relay2_Tips 	= {0};
color_Attr xdata relay3_Tips 	= {0};
color_Attr xdata zigbNwk_Tips 	= {0};

u8 tipsInsert_swLedBKG_ON 		= 1;
u8 tipsInsert_swLedBKG_OFF 		= 0;
color_Attr code  tips_relayUnused= {31, 0,  0};

color_Attr code color_Tab[TIPS_SWBKCOLOR_TYPENUM] = {

	{ 0,  0,  0}, {20, 10, 31}, {31,  0,  0},
	{31,  0, 10}, { 8,  0, 16}, {0,  31,  0},
	{16, 31,  0}, {31, 10,  0}, {0,   0, 31},
	{ 0, 10, 31},
};

sound_Attr xdata devTips_beep  	= {0, 0, 0};

tips_Status devTips_status 		= status_Null;

u8  xdata counter_tipsAct 		= 0; //tips 调色灯调色单周期
u8  counter_ifTipsFree 			= TIPS_SWFREELOOP_TIME;

enum_beeps dev_statusBeeps	 	= beepsMode_null; //状态机状态：蜂鸣器状态指示

tips_nwkZigbStatus devTips_nwkZigb = nwkZigb_Normal; //zigbee网络状态指示灯

void tipLED_pinInit(void){

	P2M1 &= ~0xF7;
	P2M0 |= 0xF7;
	
	P0M1 &= ~0x0B;
	P0M0 |= 0x0B;
	
	P5M1 &= ~0x30;
	P5M0 |= 0x30;
	
	devTips_status = status_Normal;
	
	tipsLED_colorSet(obj_Relay1, 0, 0, 0);
	tipsLED_colorSet(obj_Relay2, 0, 0, 0);
	tipsLED_colorSet(obj_Relay3, 0, 0, 0);
	tipsLED_colorSet(obj_zigbNwk,0, 0, 0);
	
//	tipsLED_colorSet(obj_Relay1, 0, 0, 0);
//	tipsLED_colorSet(obj_Relay2, 0, 0, 0);
//	tipsLED_colorSet(obj_Relay3, 0, 0, 0);
//	tipsLED_colorSet(obj_zigbNwk,0, 0, 0);
}

void pinBeep_Init(void){

	P3M1	&= ~(0x04);
	P3M0	|= 0x04;
}

/*从内部eeprom更新tips背景灯色*/
void ledBKGColorSw_Reales(void){

	EEPROM_read_n(EEPROM_ADDR_ledSWBackGround, &tipsInsert_swLedBKG_ON, 1);
	EEPROM_read_n(EEPROM_ADDR_ledSWBackGround + 1, &tipsInsert_swLedBKG_OFF, 1);
	
	if(tipsInsert_swLedBKG_ON > TIPS_SWBKCOLOR_TYPENUM - 1)tipsInsert_swLedBKG_ON = 1;
	if(tipsInsert_swLedBKG_OFF > TIPS_SWBKCOLOR_TYPENUM - 1)tipsInsert_swLedBKG_OFF = 0;
}

/*触发非阻塞beeps_Tips*/
void beeps_usrActive(u8 tons, u8 time, u8 loop){ //音调 时间 次数

	if(!ifNightMode_sw_running_FLAG){ //非夜间模式声音有效
	
		devTips_beep.tips_Period = tons;
		devTips_beep.tips_time = time;
		devTips_beep.tips_loop = loop;
		dev_statusBeeps = beepsMode_standBy;
	}
}

/*led状态指示模式切换至正常模式*/
void tips_statusChangeToNormal(void){

	counter_ifTipsFree = TIPS_SWFREELOOP_TIME;
	devTips_status = status_Normal;
}

void thread_tipsGetDark(u8 funSet){ //占位清色值

	if((funSet & 0x01) >> 0)relay1_Tips.colorGray_R = relay1_Tips.colorGray_G = relay1_Tips.colorGray_B = 0;
	if((funSet & 0x02) >> 1)relay2_Tips.colorGray_R = relay2_Tips.colorGray_G = relay2_Tips.colorGray_B = 0;
	if((funSet & 0x04) >> 2)relay3_Tips.colorGray_R = relay3_Tips.colorGray_G = relay3_Tips.colorGray_B = 0;
	if((funSet & 0x08) >> 3)zigbNwk_Tips.colorGray_R = zigbNwk_Tips.colorGray_G = zigbNwk_Tips.colorGray_B = 0;
}

/*tips主线程*/
void thread_Tips(void){
	
	if(ifNightMode_sw_running_FLAG){ //夜间模式，背景灯tips模式强制切换
	
		devTips_status = status_Night;
		
	}else{ //非夜间模式，其它花样切换开启
		
		if(devTips_status == status_Night)tips_statusChangeToNormal(); //当前若为夜间模式则切回正常模式
	
		if(!counter_ifTipsFree &&  //指定时间没有硬件操作，led状态指示切换至空闲模式
		   (devTips_status == status_Normal) ){ //正常模式下才可以切，其他模式不能
		    
			thread_tipsGetDark(0x0F);
			devTips_status = status_keyFree;
		}
	}
	
	switch(devTips_status){
	
		case status_sysStandBy:{
		
			
			
		}break;
			
		case status_keyFree:{
		
			tips_sysStandBy();
		
		}break;
		
		case status_Night:
		case status_Normal:{
			
			static u16 tips_Counter = 0;
			u16 code tips_Period = 200;

			/*继电器状态指示*/
			(DEV_actReserve & 0x01)?((status_Relay & 0x01)?(tipsLED_colorSet(obj_Relay1, color_Tab[tipsInsert_swLedBKG_ON].colorGray_R, color_Tab[tipsInsert_swLedBKG_ON].colorGray_G, color_Tab[tipsInsert_swLedBKG_ON].colorGray_B)):(tipsLED_colorSet(obj_Relay1, color_Tab[tipsInsert_swLedBKG_OFF].colorGray_R, color_Tab[tipsInsert_swLedBKG_OFF].colorGray_G, color_Tab[tipsInsert_swLedBKG_OFF].colorGray_B))):(tipsLED_colorSet(obj_Relay1, tips_relayUnused.colorGray_R, tips_relayUnused.colorGray_G, tips_relayUnused.colorGray_B));
			(DEV_actReserve & 0x02)?((status_Relay & 0x02)?(tipsLED_colorSet(obj_Relay2, color_Tab[tipsInsert_swLedBKG_ON].colorGray_R, color_Tab[tipsInsert_swLedBKG_ON].colorGray_G, color_Tab[tipsInsert_swLedBKG_ON].colorGray_B)):(tipsLED_colorSet(obj_Relay2, color_Tab[tipsInsert_swLedBKG_OFF].colorGray_R, color_Tab[tipsInsert_swLedBKG_OFF].colorGray_G, color_Tab[tipsInsert_swLedBKG_OFF].colorGray_B))):(tipsLED_colorSet(obj_Relay2, tips_relayUnused.colorGray_R, tips_relayUnused.colorGray_G, tips_relayUnused.colorGray_B));			
			(DEV_actReserve & 0x04)?((status_Relay & 0x04)?(tipsLED_colorSet(obj_Relay3, color_Tab[tipsInsert_swLedBKG_ON].colorGray_R, color_Tab[tipsInsert_swLedBKG_ON].colorGray_G, color_Tab[tipsInsert_swLedBKG_ON].colorGray_B)):(tipsLED_colorSet(obj_Relay3, color_Tab[tipsInsert_swLedBKG_OFF].colorGray_R, color_Tab[tipsInsert_swLedBKG_OFF].colorGray_G, color_Tab[tipsInsert_swLedBKG_OFF].colorGray_B))):(tipsLED_colorSet(obj_Relay3, tips_relayUnused.colorGray_R, tips_relayUnused.colorGray_G, tips_relayUnused.colorGray_B));	
			
			/*zigb网络状态指示*/
			{
				static bit tips_Type = 0;
				
				if(tips_Counter < tips_Period)tips_Counter ++;
				else{
				
					tips_Type = !tips_Type;
					tips_Counter = 0;
					
					switch(devTips_nwkZigb){
					
						case nwkZigb_Normal:{
						
							(tips_Type)?(tipsLED_colorSet(obj_zigbNwk, 0, 20, 0)):(tipsLED_colorSet(obj_zigbNwk, 0, 20, 0)); //常绿
						
						}break;
						
						case nwkZigb_outLine:{
						
							(tips_Type)?(tipsLED_colorSet(obj_zigbNwk, 20, 0, 0)):(tipsLED_colorSet(obj_zigbNwk, 20, 0, 0)); //常红
						
						}break;
						
						case nwkZigb_reConfig:{
						
							(tips_Type)?(tipsLED_colorSet(obj_zigbNwk, 30, 10, 0)):(tipsLED_colorSet(obj_zigbNwk, 30, 10, 0)); //常黄
							
						}break;
						
						case nwkZigb_nwkREQ:{
						
							(tips_Type)?(tipsLED_colorSet(obj_zigbNwk, 20, 0, 0)):(tipsLED_colorSet(obj_zigbNwk, 0, 0, 0)); //红闪
						
						}break;
						
						default:break;
					}
				}
			}
			
		}break;
		
		case status_tipsNwkOpen:{
			
			thread_tipsGetDark(0x0F);
			
			tips_specified(1);
		
		}break;
		
		case status_tipsNwkFind:{
			
			thread_tipsGetDark(0x0F);
		
			tips_specified(0);
		
		}break;
			
		default:{}break;
	}
}

void tipsLED_colorSet(tipsLED_Obj obj, u8 gray_R, u8 gray_G, u8 gray_B){

	if((devTips_status == status_Normal) ||
	   (devTips_status == status_Night)){
		   
		if(devTips_status == status_Night){	//夜间模式，背光亮度调整
			
			u8 code bright_coef = 4;
		
			gray_R /= bright_coef;
			gray_G /= bright_coef;
			gray_B /= bright_coef;
		}
	
		switch(obj){
		
			case obj_Relay1:{
			
				relay1_Tips.colorGray_R = gray_R;
				relay1_Tips.colorGray_G = gray_G;
				relay1_Tips.colorGray_B = gray_B;
				
			}break;
			
			case obj_Relay2:{
			
				relay2_Tips.colorGray_R = gray_R;
				relay2_Tips.colorGray_G = gray_G;
				relay2_Tips.colorGray_B = gray_B;
				
			}break;
			
			case obj_Relay3:{
			
				relay3_Tips.colorGray_R = gray_R;
				relay3_Tips.colorGray_G = gray_G;
				relay3_Tips.colorGray_B = gray_B;
				
			}break;
					
			case obj_zigbNwk:{
			
				zigbNwk_Tips.colorGray_R = gray_R;
				zigbNwk_Tips.colorGray_G = gray_G;
				zigbNwk_Tips.colorGray_B = gray_B;
				
			}break;
			
			default:break;
		}
	}
}

void tips_breath(void){

	static u8 	localTips_Count = COLORGRAY_MAX - 1; //步骤中期切换初始值
	static bit 	count_FLG = 1;
	static u8 	tipsStep = 0;

	u8 code speed = 3;
	u8 code step_period = 3;
	
//	if(!localTips_Count && !count_FLG)(tipsStep >= step_period)?(tipsStep = 0):(tipsStep ++); //步骤初始期期 切换到下一步骤
	if(!localTips_Count)count_FLG = 1;
	else 
	if(localTips_Count >= (COLORGRAY_MAX - 1)){
		
		count_FLG = 0;
		
		localTips_Count = COLORGRAY_MAX - 2; //快速脱离当前状态
		(tipsStep >= step_period)?(tipsStep = 0):(tipsStep ++);  //步骤中期 切换到下一步骤
	}
	
	if(!counter_tipsAct){
	
		(!count_FLG)?(counter_tipsAct = speed * (localTips_Count --)):(counter_tipsAct = speed * (localTips_Count ++)); //更新单周期时间
	}	
	
	switch(tipsStep){
	
		case 0:{
		
			zigbNwk_Tips.colorGray_G = 31 - localTips_Count;
			
		}break;
		
		case 2:{
		
			relay3_Tips.colorGray_R = 31 - localTips_Count;
			
		}break;
		
		default:break;
	}
}

void tips_fadeOut(void){

	static u8 	localTips_Count = 0; //步骤初期切换初始值
	static bit 	count_FLG = 1;
	static u8 	tipsStep = 0;
	static u8 	pwmType_A = 0,
				pwmType_B = 0,
				pwmType_C = 0,
				pwmType_D = 0;
	
	u8 code speed = 1;
	u8 code step_period = 1;
	
	if(!localTips_Count && !count_FLG)(tipsStep >= step_period)?(tipsStep = 0):(tipsStep ++); //步骤初始期期 切换到下一步骤
	if(!localTips_Count)count_FLG = 1;
	else 
	if(localTips_Count > (COLORGRAY_MAX * 4)){
	
		count_FLG = 0;
		
//		localTips_Count = COLORGRAY_MAX - 2; //快速脱离当前状态
//		(tipsStep >= step_period)?(tipsStep = 0):(tipsStep ++);  //步骤中期 切换到下一步骤
	}
	
	if(!counter_tipsAct){
	
		(!count_FLG)?(counter_tipsAct = speed * ((localTips_Count --) % COLORGRAY_MAX)):(counter_tipsAct = speed * ((localTips_Count ++) % COLORGRAY_MAX)); //更新单周期时间
		
		if(localTips_Count >= 00 && localTips_Count< 32)pwmType_A = localTips_Count - 0;
		if(localTips_Count >= 32 && localTips_Count< 64)pwmType_B = localTips_Count - 32;
		if(localTips_Count >= 64 && localTips_Count< 96)pwmType_C = localTips_Count - 64;
		if(localTips_Count >= 96 && localTips_Count< 128)pwmType_D = localTips_Count - 96;
	}
	
	switch(tipsStep){
	
		case 0:{
			
			if(count_FLG){ 
				
				zigbNwk_Tips.colorGray_B = pwmType_A;
				relay3_Tips.colorGray_B = pwmType_B;
				relay2_Tips.colorGray_B = pwmType_C;
				relay1_Tips.colorGray_B = pwmType_D;
				
				zigbNwk_Tips.colorGray_R = 31 - pwmType_A;
				relay3_Tips.colorGray_R = 31 - pwmType_B;
				relay2_Tips.colorGray_R = 31 - pwmType_C;
				relay1_Tips.colorGray_R = 31 - pwmType_D;
				
			}else{ 
				
				zigbNwk_Tips.colorGray_R = 31 - pwmType_D;
				relay3_Tips.colorGray_R = 31 - pwmType_C;
				relay2_Tips.colorGray_R = 31 - pwmType_B;
				relay1_Tips.colorGray_R = 31 - pwmType_A;
				
				zigbNwk_Tips.colorGray_B = pwmType_D;
				relay3_Tips.colorGray_B = pwmType_C;
				relay2_Tips.colorGray_B = pwmType_B;
				relay1_Tips.colorGray_B = pwmType_A;
			}
			
		}break;
			
		case 1:{
			
			if(count_FLG){ 
				
				zigbNwk_Tips.colorGray_G = pwmType_A / 4;
				relay3_Tips.colorGray_G = pwmType_B / 4;
				relay2_Tips.colorGray_G = pwmType_C / 4;
				relay1_Tips.colorGray_G = pwmType_D / 4;
				
				zigbNwk_Tips.colorGray_R = 31 - pwmType_A;
				relay3_Tips.colorGray_R = 31 - pwmType_B;
				relay2_Tips.colorGray_R = 31 - pwmType_C;
				relay1_Tips.colorGray_R = 31 - pwmType_D;
				
			}else{ 
				
				zigbNwk_Tips.colorGray_R = 31 - pwmType_D;
				relay3_Tips.colorGray_R = 31 - pwmType_C;
				relay2_Tips.colorGray_R = 31 - pwmType_B;
				relay1_Tips.colorGray_R = 31 - pwmType_A;
				
				zigbNwk_Tips.colorGray_G = pwmType_D / 4;
				relay3_Tips.colorGray_G = pwmType_C / 4;
				relay2_Tips.colorGray_G = pwmType_B / 4;
				relay1_Tips.colorGray_G = pwmType_A / 4;
			}
			
		}break;
			
		default:break;
	}
}

void tips_specified(u8 tips_Type){ //tips类别，次数统计

	static u8 	localTips_Count = 0; //步骤初期切换初始值
	static bit 	count_FLG = 1;
	static u8 	tipsStep = 0;
	static u8 	pwmType_A = 0,
				pwmType_B = 0,
				pwmType_C = 0,
				pwmType_D = 0;
	
	u8 code speed_Mol = 5,
	        speed_Den = 4;
	u8 code step_period = 0;
	
	if(!localTips_Count && !count_FLG){
	
		if(tipsStep < step_period)tipsStep ++; //步骤初始期期 切换到下一步骤
		else{
		
			tipsStep = 0;
			
			pwmType_A = pwmType_B = pwmType_C = pwmType_D = 0;
		}
	}
	if(!localTips_Count)count_FLG = 1;
	else 
	if(localTips_Count > 80){
	
		count_FLG = 0;
		
//		localTips_Count = COLORGRAY_MAX - 2; //快速脱离当前状态
//		(tipsStep >= step_period)?(tipsStep = 0):(tipsStep ++);  //步骤中期 切换到下一步骤
	}
	
	if(!counter_tipsAct){
	
		(!count_FLG)?(counter_tipsAct = ((localTips_Count --) % COLORGRAY_MAX) / speed_Mol * speed_Den):(counter_tipsAct = ((localTips_Count ++) % COLORGRAY_MAX) / speed_Mol * speed_Den); //更新单周期时间
		
		if(localTips_Count >= 00 && localTips_Count < 32)pwmType_A = localTips_Count - 0; 
		if(localTips_Count >= 16 && localTips_Count < 48)pwmType_B = localTips_Count - 16;
		if(localTips_Count >= 32 && localTips_Count < 64)pwmType_C = localTips_Count - 32;
		if(localTips_Count >= 48 && localTips_Count < 80)pwmType_D = localTips_Count - 48;
	}
	
	switch(tips_Type){
	
		case 0:{
		
			switch(tipsStep){
			
				case 0:{
					
					if(count_FLG){ 
						
						if(localTips_Count < 46){
						
							if(localTips_Count % 15 > 10)zigbNwk_Tips.colorGray_B = 31;else zigbNwk_Tips.colorGray_B = 0;
						
						}else{
						
							if(localTips_Count > 45)zigbNwk_Tips.colorGray_B = 31;
							if(localTips_Count > 50)relay3_Tips.colorGray_B = 31;
							if(localTips_Count > 60)relay2_Tips.colorGray_B = 31;
							if(localTips_Count > 70)relay1_Tips.colorGray_B = 31;
						}
						
					}else{ 
						
						zigbNwk_Tips.colorGray_B = pwmType_D;
						relay3_Tips.colorGray_B = pwmType_C;
						relay2_Tips.colorGray_B = pwmType_B;
						relay1_Tips.colorGray_B = pwmType_A;
					}
					
				}break;
					
				default:break;
			}
			
		}break;
		
		case 1:{
		
			switch(tipsStep){
			
				case 0:{
					
					if(count_FLG){ 
						
						if(localTips_Count < 46){
						
							if(localTips_Count % 15 > 10)zigbNwk_Tips.colorGray_B = 31;else zigbNwk_Tips.colorGray_B = 0;
						
						}else{
						
							if(localTips_Count > 45)relay1_Tips.colorGray_B = 31;
							if(localTips_Count > 50)relay2_Tips.colorGray_B = 31;
							if(localTips_Count > 60)relay3_Tips.colorGray_B = 31;
							if(localTips_Count > 70)zigbNwk_Tips.colorGray_B = 31;
						}
						
					}else{ 
						
						relay1_Tips.colorGray_B = pwmType_D;
						relay2_Tips.colorGray_B = pwmType_C;
						relay3_Tips.colorGray_B = pwmType_B;
						zigbNwk_Tips.colorGray_B = pwmType_A;
					}
					
				}break;
					
				default:break;
			}
			
		}break;
		
		default:break;
	}
}

void tips_sysStandBy(void){

//	u8 code timUnit_period = 0;
//	static u8 timUnit_count = 0;
	
	static u8 localTips_Count = 0;
	u8 code localTips_Period = 128;
	static u8 tipsStep = 0;
	static u8 pwmType_A = 0,
		      pwmType_B = 0,
			  pwmType_C = 0,
			  pwmType_D = 0;
	
	if(!counter_tipsAct){
		
		counter_tipsAct = 6; //更新单周期时间
	
		if(localTips_Count > localTips_Period){
		
			localTips_Count = 0;
			(tipsStep > 7)?(tipsStep = 0):(tipsStep ++);
			pwmType_A = pwmType_B = pwmType_C = pwmType_D = 0;
		}
		else{
		
			localTips_Count ++;
			
			if(localTips_Count >= 00 && localTips_Count< 32)pwmType_A = localTips_Count - 0;
			if(localTips_Count >= 32 && localTips_Count< 64)pwmType_B = localTips_Count - 32;
			if(localTips_Count >= 64 && localTips_Count< 96)pwmType_C = localTips_Count - 64;
			if(localTips_Count >= 96 && localTips_Count< 128)pwmType_D = localTips_Count - 96;
		}
	}
	
	switch(tipsStep){
		
//		case 0:{
//		
//			zigbNwk_Tips.colorGray_R	= pwmType_A / 3;
//			relay3_Tips.colorGray_R 	= pwmType_B / 3;
//			relay2_Tips.colorGray_R 	= pwmType_C / 3;
//			relay1_Tips.colorGray_R 	= pwmType_D / 3;
//			
//			zigbNwk_Tips.colorGray_G	= pwmType_A / 5;
//			relay3_Tips.colorGray_G 	= pwmType_B / 5;
//			relay2_Tips.colorGray_G 	= pwmType_C / 5;
//			relay1_Tips.colorGray_G 	= pwmType_D / 5;
//			
//			zigbNwk_Tips.colorGray_B	= pwmType_A / 2;
//			relay3_Tips.colorGray_B 	= pwmType_B / 2;
//			relay2_Tips.colorGray_B 	= pwmType_C / 2;
//			relay1_Tips.colorGray_B 	= pwmType_D / 2;
//		
//		}break;
//		
//		case 1:{
//		
//			zigbNwk_Tips.colorGray_R	= 5 + (pwmType_A - 5);
//			relay3_Tips.colorGray_R 	= 5 + (pwmType_B - 5);
//			relay2_Tips.colorGray_R 	= 5 + (pwmType_C - 5);
//			relay1_Tips.colorGray_R 	= 5 + (pwmType_D - 5);
//			
//			zigbNwk_Tips.colorGray_G	= 3 - pwmType_A / 5;
//			relay3_Tips.colorGray_G 	= 3 - pwmType_B / 5;
//			relay2_Tips.colorGray_G 	= 3 - pwmType_C / 5;
//			relay1_Tips.colorGray_G 	= 3 - pwmType_D / 5;
//			
//			zigbNwk_Tips.colorGray_B	= 7 - pwmType_A / 2;
//			relay3_Tips.colorGray_B 	= 7 - pwmType_B / 2;
//			relay2_Tips.colorGray_B 	= 7 - pwmType_C / 2;
//			relay1_Tips.colorGray_B 	= 7 - pwmType_D / 2;
//		
//		}break;
//		
//		case 2:{
//			
//			zigbNwk_Tips.colorGray_G	= pwmType_A / 5;
//			relay3_Tips.colorGray_G 	= pwmType_B / 5;
//			relay2_Tips.colorGray_G 	= pwmType_C / 5;
//			relay1_Tips.colorGray_G 	= pwmType_D / 5;
//		
//		}break;
//			
//		case 3:{
//		
//			zigbNwk_Tips.colorGray_G	= 3 - pwmType_A / 5;
//			relay3_Tips.colorGray_G 	= 3 - pwmType_B / 5;
//			relay2_Tips.colorGray_G 	= 3 - pwmType_C / 5;
//			relay1_Tips.colorGray_G 	= 3 - pwmType_D / 5;
//			
//			zigbNwk_Tips.colorGray_B	= pwmType_A / 5;
//			relay3_Tips.colorGray_B 	= pwmType_B / 5;
//			relay2_Tips.colorGray_B 	= pwmType_C /5;
//			relay1_Tips.colorGray_B 	= pwmType_D / 5;
//		
//		}break;
//			
//		case 4:{
//			
//			zigbNwk_Tips.colorGray_R	= 15 - pwmType_A;
//			relay3_Tips.colorGray_R 	= 15 - pwmType_B;
//			relay2_Tips.colorGray_R 	= 15 - pwmType_C;
//			relay1_Tips.colorGray_R 	= 15 - pwmType_D;

//			zigbNwk_Tips.colorGray_B	= 3 + (pwmType_A - 3);
//			relay3_Tips.colorGray_B 	= 3 + (pwmType_B - 3);
//			relay2_Tips.colorGray_B 	= 3 + (pwmType_C - 3);
//			relay1_Tips.colorGray_B 	= 3 + (pwmType_D - 3);
//			
//		}break;
//			
//		case 5:{

//			zigbNwk_Tips.colorGray_B	= 15 - pwmType_A;
//			relay3_Tips.colorGray_B 	= 15 - pwmType_B;
//			relay2_Tips.colorGray_B 	= 15 - pwmType_C;
//			relay1_Tips.colorGray_B 	= 15 - pwmType_D;
//			
//			zigbNwk_Tips.colorGray_G	= pwmType_A;
//			relay3_Tips.colorGray_G 	= pwmType_B;
//			relay2_Tips.colorGray_G 	= pwmType_C;
//			relay1_Tips.colorGray_G 	= pwmType_D;
//			
//		}break;
//			
//		case 6:{
//		
//			zigbNwk_Tips.colorGray_B	= pwmType_A;
//			relay3_Tips.colorGray_B 	= pwmType_B;
//			relay2_Tips.colorGray_B 	= pwmType_C;
//			relay1_Tips.colorGray_B 	= pwmType_D;
//			
//			zigbNwk_Tips.colorGray_G	= 15 - pwmType_A + 3;
//			relay3_Tips.colorGray_G 	= 15 - pwmType_B + 3;
//			relay2_Tips.colorGray_G 	= 15 - pwmType_C + 3;
//			relay1_Tips.colorGray_G 	= 15 - pwmType_D + 3;
//		
//		}break;
//		
//		case 7:{
//		
//			zigbNwk_Tips.colorGray_B	= 15 - pwmType_A;
//			relay3_Tips.colorGray_B 	= 15 - pwmType_B;
//			relay2_Tips.colorGray_B 	= 15 - pwmType_C;
//			relay1_Tips.colorGray_B 	= 15 - pwmType_D;
//			
//			zigbNwk_Tips.colorGray_G	= 3 - pwmType_A / 5;
//			relay3_Tips.colorGray_G 	= 3 - pwmType_A / 5;
//			relay2_Tips.colorGray_G 	= 3 - pwmType_A / 5;
//			relay1_Tips.colorGray_G 	= 3 - pwmType_A / 5;
//			
//		}break;
	
		case 0:{ /*绿*///绿起
			
			zigbNwk_Tips.colorGray_G = pwmType_A / 5;
			relay3_Tips.colorGray_G = pwmType_B / 5;
			relay2_Tips.colorGray_G = pwmType_C / 5;
			relay1_Tips.colorGray_G = pwmType_D / 5;
			
		}break;
		
		case 1:{ /*黄*///红起
			
			zigbNwk_Tips.colorGray_R = pwmType_A;
			relay3_Tips.colorGray_R = pwmType_B;
			relay2_Tips.colorGray_R = pwmType_C;
			relay1_Tips.colorGray_R = pwmType_D;
			
		}break;
		
		case 2:{ /*红*///绿消
			
			zigbNwk_Tips.colorGray_G = 6 - pwmType_A / 5;
			relay3_Tips.colorGray_G = 6 - pwmType_B / 5;
			relay2_Tips.colorGray_G = 6 - pwmType_C / 5;
			relay1_Tips.colorGray_G = 6 - pwmType_D / 5;
			
		}break;
		
		case 3:{ /*粉*///蓝起
			
			zigbNwk_Tips.colorGray_B = pwmType_A / 2;
			relay3_Tips.colorGray_B = pwmType_B / 2;
			relay2_Tips.colorGray_B = pwmType_C / 2;
			relay1_Tips.colorGray_B = pwmType_D / 2;
			
		}break;
		
		case 4:{ /*蓝*///红消
		
			zigbNwk_Tips.colorGray_R = 31 - pwmType_A;
			relay3_Tips.colorGray_R = 31 - pwmType_B;
			relay2_Tips.colorGray_R = 31 - pwmType_C;
			relay1_Tips.colorGray_R = 31 - pwmType_D;
			
		}break;
		
		case 5:{ /*白*///绿起红起
		
			zigbNwk_Tips.colorGray_G = pwmType_A / 3;
			relay3_Tips.colorGray_G = pwmType_B / 3;
			relay2_Tips.colorGray_G = pwmType_C / 3;
			relay1_Tips.colorGray_G = pwmType_D / 3;
			
			zigbNwk_Tips.colorGray_R = pwmType_A / 2;
			relay3_Tips.colorGray_R = pwmType_B / 2;
			relay2_Tips.colorGray_R = pwmType_C / 2;
			relay1_Tips.colorGray_R = pwmType_D / 2;
			
		}break;
		
		case 6:{ /*dark*///全消
		
			zigbNwk_Tips.colorGray_B = 15 - pwmType_A / 2;
			relay3_Tips.colorGray_B = 15 - pwmType_B / 2;
			relay2_Tips.colorGray_B = 15 - pwmType_C / 2;
			relay1_Tips.colorGray_B = 15 - pwmType_D / 2;
			
			zigbNwk_Tips.colorGray_G = 10 - pwmType_A / 3;
			relay3_Tips.colorGray_G = 10 - pwmType_B / 3;
			relay2_Tips.colorGray_G = 10 - pwmType_C / 3;
			relay1_Tips.colorGray_G = 10 - pwmType_D / 3;
			
			zigbNwk_Tips.colorGray_R = 15 - pwmType_A / 2;
			relay3_Tips.colorGray_R = 15 - pwmType_B / 2;
			relay2_Tips.colorGray_R = 15 - pwmType_C / 2;
			relay1_Tips.colorGray_R = 15 - pwmType_D / 2;
			
		}break;
		
		default:{}break;
	}
}


