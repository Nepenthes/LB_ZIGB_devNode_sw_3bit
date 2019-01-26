#include "Tips.h"

#include "USART.h"

#include "string.h"
#include "stdio.h"

#include "driver_I2C_HXD019D.h"

#include "Relay.h"
#include "dataManage.h"
#include "timerAct.h"
#include "appTimer.h"
#include "usrKin.h"

#include "eeprom.h"
#include "delay.h"

color_Attr code color_Tab[TIPS_SWBKCOLOR_TYPENUM] = { //选色表

	{ 0,  0,  0}, {20, 10, 31}, {31,  0,  0}, //黑0、白1、红2
	{31,  0, 10}, { 8,  0, 16}, {0,  31,  0}, //粉3、紫4、绿5
	{16, 31,  0}, {31, 10,  0}, {0,   0, 31}, //橙6、靛7、蓝8
	{ 0, 10, 31}, //冰蓝9
};

color_Attr xdata relay1_Tips 	= {0};
color_Attr xdata relay2_Tips 	= {0};
color_Attr xdata relay3_Tips 	= {0};
color_Attr xdata zigbNwk_Tips 	= {0};

bkLightColorInsert_paramAttr xdata devBackgroundLight_param = {0}; //背光灯索引参数缓存

color_Attr code  tips_relayUnused= {0, 0, 0}; //无效继电器背景灯颜色

bit	idata ifHorsingLight_running_FLAG = 1;	//跑马灯运行标志位  默认开

tips_Status devTips_status 		= status_Null; //系统状态指示
tips_nwkZigbStatus devTips_nwkZigb = nwkZigb_Normal; //zigbee网络状态指示灯

u16 xdata counter_tipsAct 		= 0; //tips 调色灯调色单周期
u8  xdata counter_ifTipsFree 	= TIPS_SWFREELOOP_TIME; //触摸空闲释放计时变量
bit idata countEN_ifTipsFree	= 0; //触摸空闲释放计时使能
u8  xdata tipsTimeCount_zigNwkOpen	= 0; //zigb 网络开放tips时间计时变量
u8	xdata tipsTimeCount_touchReset	= 0; //触摸IC复位tips计时变量
u8	xdata tipsTimeCount_factoryRecover = 0; //恢复出厂tips倒计时变量

sound_Attr xdata devTips_beep  	= {0, 0, 0};
enum_beeps xdata dev_statusBeeps= beepsMode_null; //状态机状态：蜂鸣器状态指示

void tipLED_pinInit(void){

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
	P5M1 &= ~(0x10);
	P5M0 |= 0x10;
	
	P2M1 &= ~(0x20);
	P2M0 |= 0x20;
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)
	P2M1 &= ~(0x0C);
	P2M0 |= 0x0C;
	
#else
	P2M1 &= ~0xF7;
	P2M0 |= 0xF7;
	
	P0M1 &= ~0x0B;
	P0M0 |= 0x0B;
	
	P5M1 &= ~0x30;
	P5M0 |= 0x30;
	
#endif
	
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

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)
	P2M1 &= ~0x01;
	P2M0 |= 0x01;
	
#else
	P3M1 &= ~(0x04);
	P3M0 |= 0x04;
	
#endif
}

/*从内部eeprom更新tips背景灯色*/
void ledBKGColorSw_Reales(void){

//	EEPROM_read_n(EEPROM_ADDR_ledSWBackGround, &tipsInsert_swLedBKG_ON, 1);
//	EEPROM_read_n(EEPROM_ADDR_ledSWBackGround + 1, &tipsInsert_swLedBKG_OFF, 1);
//	
//	if(tipsInsert_swLedBKG_ON > TIPS_SWBKCOLOR_TYPENUM - 1)tipsInsert_swLedBKG_ON = TIPSBKCOLOR_DEFAULT_ON;
//	if(tipsInsert_swLedBKG_OFF > TIPS_SWBKCOLOR_TYPENUM - 1)tipsInsert_swLedBKG_OFF = TIPSBKCOLOR_DEFAULT_OFF;
//	
//	coverEEPROM_write_n(EEPROM_ADDR_ledSWBackGround, &tipsInsert_swLedBKG_ON, 1);
//	coverEEPROM_write_n(EEPROM_ADDR_ledSWBackGround + 1, &tipsInsert_swLedBKG_OFF, 1);
	
	EEPROM_read_n(EEPROM_ADDR_ledSWBackGround, (u8 *)&devBackgroundLight_param, sizeof(bkLightColorInsert_paramAttr)); //参数读取
	
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_FANS) //参数预处理
	if(devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear0 > (TIPS_SWBKCOLOR_TYPENUM - 1))devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear0 = 0; //黑
	if(devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear1 > (TIPS_SWBKCOLOR_TYPENUM - 1))devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear1 = 5; //绿
	if(devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear2 > (TIPS_SWBKCOLOR_TYPENUM - 1))devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear2 = 8; //蓝
	if(devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear3 > (TIPS_SWBKCOLOR_TYPENUM - 1))devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear3 = 2; //红
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER) //参数预处理
	if(devBackgroundLight_param.dimmer_BKlight_Param.dimmer_BKlightColorInsert_press > 			 (TIPS_SWBKCOLOR_TYPENUM - 1))devBackgroundLight_param.dimmer_BKlight_Param.dimmer_BKlightColorInsert_press = 0; //黑
	if(devBackgroundLight_param.dimmer_BKlight_Param.dimmer_BKlightColorInsert_brightness0 > 	 (TIPS_SWBKCOLOR_TYPENUM - 1))devBackgroundLight_param.dimmer_BKlight_Param.dimmer_BKlightColorInsert_brightness0 = 5; //绿
	if(devBackgroundLight_param.dimmer_BKlight_Param.dimmer_BKlightColorInsert_brightness1to99 > (TIPS_SWBKCOLOR_TYPENUM - 1))devBackgroundLight_param.dimmer_BKlight_Param.dimmer_BKlightColorInsert_brightness1to99 = 8; //蓝
	if(devBackgroundLight_param.dimmer_BKlight_Param.dimmer_BKlightColorInsert_brightness100 > 	 (TIPS_SWBKCOLOR_TYPENUM - 1))devBackgroundLight_param.dimmer_BKlight_Param.dimmer_BKlightColorInsert_brightness100 = 2; //红
	
//#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS) //参数预处理，无特殊，随默认
//#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED) //参数预处理，无特殊，随默认
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SCENARIO) //参数预处理
	if(devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig > (TIPS_SWBKCOLOR_TYPENUM - 1))devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig = 8; //蓝
	if(devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig > (TIPS_SWBKCOLOR_TYPENUM - 1))devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig = 5; //绿

#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_HEATER) //参数预处理，热水器为强制背光
	devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_open = 2; //红
	devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_close = 0; //黑
	devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_closeDelay30 = 5; //绿
	devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_closeDelay60 = 8; //蓝

#else
	switch(SWITCH_TYPE){
	
		case SWITCH_TYPE_CURTAIN:{
		
			if(devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press > (TIPS_SWBKCOLOR_TYPENUM - 1))devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press = 8; //蓝
			if(devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce > (TIPS_SWBKCOLOR_TYPENUM - 1))devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce = 5; //绿
		
		}break;
		
		case SWITCH_TYPE_SWBIT1:
		case SWITCH_TYPE_SWBIT2:
		case SWITCH_TYPE_SWBIT3:{
			
			if(devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open > (TIPS_SWBKCOLOR_TYPENUM - 1))devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open = 8; //蓝
			if(devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close > (TIPS_SWBKCOLOR_TYPENUM - 1))devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close = 5; //绿
			
		}break;
		
		default:{}break;
	}

#endif
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

void thread_tipsGetDark(u8 funSet){ //占位清色值

	if((funSet & 0x01) >> 0)relay1_Tips.colorGray_R = relay1_Tips.colorGray_G = relay1_Tips.colorGray_B = 0;
	if((funSet & 0x02) >> 1)relay2_Tips.colorGray_R = relay2_Tips.colorGray_G = relay2_Tips.colorGray_B = 0;
	if((funSet & 0x04) >> 2)relay3_Tips.colorGray_R = relay3_Tips.colorGray_G = relay3_Tips.colorGray_B = 0;
	if((funSet & 0x08) >> 3)zigbNwk_Tips.colorGray_R = zigbNwk_Tips.colorGray_G = zigbNwk_Tips.colorGray_B = 0;
}

/*led状态指示模式切换至正常模式*/
void tips_statusChangeToNormal(void){

	counter_ifTipsFree = TIPS_SWFREELOOP_TIME;
	devTips_status = status_Normal;
	if(!countEN_ifTipsFree)countEN_ifTipsFree = 1; //触摸释放计时使能
	if(tipsTimeCount_zigNwkOpen)devTips_nwkZigb = nwkZigb_nwkOpen;
}

/*led状态指示模式切换至zigb网络开放*/
void tips_statusChangeToZigbNwkOpen(u8 timeopen){

	beeps_usrActive(3, 40, 2);
	tipsTimeCount_zigNwkOpen = timeopen;
	devTips_status = status_tipsNwkOpen;
	devTips_nwkZigb = nwkZigb_nwkOpen;
	thread_tipsGetDark(0x0F);
}

/*led状态指示模式切换至zigb网络加入请求*/
void tips_statusChangeToZigbNwkFind(void){

	beeps_usrActive(3, 40, 2);
	devTips_status = status_tipsNwkFind;
	thread_tipsGetDark(0x0F);
}

/*led状态指示模式切换至触摸IC复位模式*/
void tips_statusChangeToTouchReset(u8 timeHold){

	tipsTimeCount_touchReset = timeHold;
	devTips_status = status_touchReset;
	thread_tipsGetDark(0x0F);
	beeps_usrActive(3, 40, 2);
}

/*led状态指示模式切换至恢复出厂模式*/
void tips_statusChangeToFactoryRecover(u8 timeHold){

	tipsTimeCount_factoryRecover = timeHold;
	devTips_status = status_sysStandBy;
	thread_tipsGetDark(0x0F);
	beeps_usrActive(3, 255, 3);
}

/*tips主线程*///状态机
void thread_Tips(void){
	
	if(ifNightMode_sw_running_FLAG){ //夜间模式，背景灯tips模式强制切换
	
		if(devTips_status == status_Normal || //其它系统级tips不受夜间模式影响
		   devTips_status == status_keyFree){
			
			devTips_status = status_Night;
		}
		
	}else{ //非夜间模式，其它花样切换开启
		
		if(devTips_status == status_Night)tips_statusChangeToNormal(); //当前若为夜间模式则切回正常模式
		
		if(devTips_status == status_keyFree){
		
			if(!countEN_ifTipsFree)devTips_status = status_Normal; //中断正在运行的跑马灯
		}
	
		if( !counter_ifTipsFree &&  //指定时间没有硬件操作，led状态指示切换至空闲模式
		    (devTips_status == status_Normal) &&  //正常模式下才可以切，其他模式不能
			countEN_ifTipsFree && //且空闲计时使能
			ifHorsingLight_running_FLAG ){ //标志位置位才可以切
		    
			thread_tipsGetDark(0x0F);
			devTips_status = status_keyFree;
		}
	}
	
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
	ifHorsingLight_running_FLAG = 0; //插座不存在跑马灯
	
	switch(devTips_status){
	
		case status_Night:
		case status_Normal:{
			
			/*zigb网络状态指示*/
			{
				static bit tips_Type = 0;
				static u8 tipsEn_Loop = 0;
				
				static u8 xdata socketTips_R = 0,
								socketTips_B = 0;
				
				(status_Relay)?(socketTips_R = 30):(socketTips_R = 0);
				
				if(!counter_tipsAct){
				
					tips_Type = !tips_Type;
					
					switch(devTips_nwkZigb){
					
						case nwkZigb_Normal:{
							
							socketTips_B = 5;
						
						}break;
						
						case nwkZigb_nwkOpen:{
						
							if(tipsEn_Loop < (3 * 2)){ //周期闪3
							
								tipsEn_Loop ++;
								counter_tipsAct = 150;
								(tips_Type)?(socketTips_B = 5):(socketTips_B = 0);
								
							}else{
							
								tipsEn_Loop = 0;
								counter_tipsAct = 2000;
								socketTips_B = 0;
							}
							
						}break;
						
						case nwkZigb_outLine:{
						
							socketTips_B = 0;
						
						}break;
						
						case nwkZigb_reConfig:{ //间接1000 常闪
						
							counter_tipsAct = 1000;
							if(zigbNwk_exist_FLG)(tips_Type)?(socketTips_B = 5):(socketTips_B = 0); //重连 --本地有网络记录，重连
							else socketTips_B = 0; //重连 --本地没有有网络记录，不存在重连
							
						}break;
						
						case nwkZigb_nwkREQ:{ //间接100 常闪
						
							counter_tipsAct = 100;
							(tips_Type)?(socketTips_B = 5):(socketTips_B = 0);
						
						}break;
						
						case nwkZigb_hold:{
						
							if(tipsEn_Loop < (5 * 2)){ //周期闪5
							
								tipsEn_Loop ++;
								counter_tipsAct = 100;
								(tips_Type)?(socketTips_B = 5):(socketTips_B = 0);
								
							}else{
							
								tipsEn_Loop = 0;
								counter_tipsAct = 2000;
								socketTips_B = 0;
							}
							
						}break;
						
						default:{
						
							devTips_nwkZigb = nwkZigb_Normal;
						
						}break;
					}
				}
				
				tipsLED_colorSet(obj_Relay1, socketTips_R, 0, socketTips_B);
			}
			
		}break;
		
		default:{ //插座没有前台tips
		
			tips_statusChangeToNormal();
			
		}break;
	}
	
	/*过期tips处理*/
	{
	
		if((devTips_nwkZigb == nwkZigb_nwkOpen) && !tipsTimeCount_zigNwkOpen)devTips_nwkZigb = nwkZigb_Normal; //网络开放后台tips过期恢复
	}
	
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)
	ifHorsingLight_running_FLAG = 0; //红外不存在跑马灯
	
	switch(devTips_status){
	
		case status_Night:
		case status_Normal:{
			
			/*zigb网络状态指示*/
			{
				static bit tips_Type = 0;
				static u8 tipsEn_Loop = 0;
				
				static u8 xdata socketTips_R = 0,
								socketTips_B = 0;
				
				(infraredStatus_GET() != infraredSMStatus_free)?(socketTips_R = 30):(socketTips_R = 0);
				
				if(!counter_tipsAct){
				
					tips_Type = !tips_Type;
					
					switch(devTips_nwkZigb){
					
						case nwkZigb_Normal:{
							
							socketTips_B = 5;
						
						}break;
						
						case nwkZigb_nwkOpen:{
						
							if(tipsEn_Loop < (3 * 2)){ //周期闪3
							
								tipsEn_Loop ++;
								counter_tipsAct = 150;
								(tips_Type)?(socketTips_B = 5):(socketTips_B = 0);
								
							}else{
							
								tipsEn_Loop = 0;
								counter_tipsAct = 2000;
								socketTips_B = 0;
							}
							
						}break;
						
						case nwkZigb_outLine:{
						
							socketTips_B = 0;
						
						}break;
						
						case nwkZigb_reConfig:{ //间接1000 常闪
							
							counter_tipsAct = 1000;
							if(zigbNwk_exist_FLG)(tips_Type)?(socketTips_B = 5):(socketTips_B = 0); //重连 --本地有网络记录，重连
							else socketTips_B = 0; //重连 --本地没有有网络记录，不存在重连
							
						}break;
						
						case nwkZigb_nwkREQ:{ //间接100 常闪
						
							counter_tipsAct = 100;
							(tips_Type)?(socketTips_B = 5):(socketTips_B = 0);
						
						}break;
						
						case nwkZigb_hold:{
						
							if(tipsEn_Loop < (5 * 2)){ //周期闪5
							
								tipsEn_Loop ++;
								counter_tipsAct = 100;
								(tips_Type)?(socketTips_B = 5):(socketTips_B = 0);
								
							}else{
							
								tipsEn_Loop = 0;
								counter_tipsAct = 2000;
								socketTips_B = 0;
							}
							
						}break;
						
						default:{
						
							devTips_nwkZigb = nwkZigb_Normal;
						
						}break;
					}
				}
				
				tipsLED_colorSet(obj_Relay1, socketTips_R, 0, socketTips_B);
			}
			
		}break;
		
		default:{ //红外没有前台tips
		
			tips_statusChangeToNormal();
			
		}break;
	}
	
	/*过期tips处理*/
	{
	
		if((devTips_nwkZigb == nwkZigb_nwkOpen) && !tipsTimeCount_zigNwkOpen)devTips_nwkZigb = nwkZigb_Normal; //网络开放后台tips过期恢复
	}
	
#else
	switch(devTips_status){
	
		case status_sysStandBy:{
		
			tips_sysStandBy();
			
		}break;
			
		case status_keyFree:{
		
			tips_sysButtonReales();
		
		}break;
		
		case status_Night:
		case status_Normal:{
			
			static u16 tipsNwk_Counter = 0;
			u16 code tipsNwk_Period = 500;
			u8 relayStatus_tipsTemp = status_Relay;
			
			if(SWITCH_TYPE == SWITCH_TYPE_SWBIT1){ //随继电器
			
				relayStatus_tipsTemp |= status_Relay & 0x01; //显存1加载
				relayStatus_tipsTemp = relayStatus_tipsTemp << 1; //显存1处理

			}else
			if(SWITCH_TYPE == SWITCH_TYPE_SWBIT2){ //随继电器

				relayStatus_tipsTemp |= status_Relay & 0x02; //显存2加载
				relayStatus_tipsTemp = relayStatus_tipsTemp << 1; //显存2处理
				relayStatus_tipsTemp |= status_Relay & 0x01; //显存1加载

			}else
			if(SWITCH_TYPE == SWITCH_TYPE_SWBIT3){ //随继电器

				relayStatus_tipsTemp = status_Relay; //直接加载，不处理
				
			}else
			if(SWITCH_TYPE == SWITCH_TYPE_CURTAIN || SWITCH_TYPE == SWITCH_TYPE_FANS){ //随继电器
				
				relayStatus_tipsTemp = status_Relay;
				
			}else
			if(SWITCH_TYPE == SWITCH_TYPE_dIMMER){ //随触摸
				
				relayStatus_tipsTemp = touchPadScan_oneShoot();
			}

			/*继电器状态指示*/
			switch(SWITCH_TYPE){
				
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_FANS)				
				case SWITCH_TYPE_FANS:{
				
					switch(relayStatus_tipsTemp){ //非占位指示
							
						case 0x01:{
						
							tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear1].colorGray_R, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear1].colorGray_G, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear1].colorGray_B);
							tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear1].colorGray_R, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear1].colorGray_G, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear1].colorGray_B);
							tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear1].colorGray_R, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear1].colorGray_G, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear1].colorGray_B);
						
						}break;
						
						case 0x02:{
						
							tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear2].colorGray_R, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear2].colorGray_G, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear2].colorGray_B);
							tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear2].colorGray_R, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear2].colorGray_G, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear2].colorGray_B);
							tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear2].colorGray_R, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear2].colorGray_G, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear2].colorGray_B);
							
						}break;
							
						case 0x03:{
						
							tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear3].colorGray_R, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear3].colorGray_G, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear3].colorGray_B);
							tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear3].colorGray_R, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear3].colorGray_G, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear3].colorGray_B);
							tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear3].colorGray_R, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear3].colorGray_G, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear3].colorGray_B);
							
						}break;
						
						case 0x00:
						default:{
						
							tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear0].colorGray_R, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear0].colorGray_G, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear0].colorGray_B);
							tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear0].colorGray_R, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear0].colorGray_G, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear0].colorGray_B);
							tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear0].colorGray_R, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear0].colorGray_G, 
														 color_Tab[devBackgroundLight_param.fans_BKlight_Param.fans_BKlightColorInsert_gear0].colorGray_B);
							
						}break;
					}
					
				}break;
				
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)		
				case SWITCH_TYPE_dIMMER:{
					
					u8 idata TIPSBKCOLOR_USRDEF_PRESS 	= devBackgroundLight_param.dimmer_BKlight_Param.dimmer_BKlightColorInsert_press; //按下色缓存
					u8 idata TIPSBKCOLOR_USRDEF_UP 		= devBackgroundLight_param.dimmer_BKlight_Param.dimmer_BKlightColorInsert_brightness0; //弹起色缓存
					
					if(!status_Relay)TIPSBKCOLOR_USRDEF_UP = devBackgroundLight_param.dimmer_BKlight_Param.dimmer_BKlightColorInsert_brightness0; //亮度为0时弹起色
					else if(status_Relay == 100)TIPSBKCOLOR_USRDEF_UP = devBackgroundLight_param.dimmer_BKlight_Param.dimmer_BKlightColorInsert_brightness100; //亮度为100时弹起色
					else TIPSBKCOLOR_USRDEF_UP = devBackgroundLight_param.dimmer_BKlight_Param.dimmer_BKlightColorInsert_brightness1to99; //亮度为1-99时弹起色
				
					switch(relayStatus_tipsTemp){ //非占位指示
					
						case 0x01:{
						
							tipsLED_colorSet(obj_Relay1, color_Tab[TIPSBKCOLOR_USRDEF_PRESS].colorGray_R, color_Tab[TIPSBKCOLOR_USRDEF_PRESS].colorGray_G, color_Tab[TIPSBKCOLOR_USRDEF_PRESS].colorGray_B);
							tipsLED_colorSet(obj_Relay2, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_R, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_G, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_B);
							tipsLED_colorSet(obj_Relay3, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_R, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_G, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_B);
						
						}break;
						
						case 0x02:{
						
							tipsLED_colorSet(obj_Relay1, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_R, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_G, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_B);
							tipsLED_colorSet(obj_Relay2, color_Tab[TIPSBKCOLOR_USRDEF_PRESS].colorGray_R, color_Tab[TIPSBKCOLOR_USRDEF_PRESS].colorGray_G, color_Tab[TIPSBKCOLOR_USRDEF_PRESS].colorGray_B);
							tipsLED_colorSet(obj_Relay3, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_R, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_G, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_B);
						
						}break;
							
						case 0x04:{
						
							tipsLED_colorSet(obj_Relay1, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_R, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_G, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_B);
							tipsLED_colorSet(obj_Relay2, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_R, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_G, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_B);
							tipsLED_colorSet(obj_Relay3, color_Tab[TIPSBKCOLOR_USRDEF_PRESS].colorGray_R, color_Tab[TIPSBKCOLOR_USRDEF_PRESS].colorGray_G, color_Tab[TIPSBKCOLOR_USRDEF_PRESS].colorGray_B);
						
						}break;
						
						default:{
					
							tipsLED_colorSet(obj_Relay1, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_R, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_G, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_B);
							tipsLED_colorSet(obj_Relay2, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_R, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_G, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_B);
							tipsLED_colorSet(obj_Relay3, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_R, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_G, color_Tab[TIPSBKCOLOR_USRDEF_UP].colorGray_B);
							
						}break;
					}
				
				}break;
				
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SCENARIO)
				case SWITCH_TYPE_SCENARIO:{
				
					u16 code tipsCounter_scenarioKeepTrigForce = 500; //场景触发强制间隔时，tips闪烁相关周期
					static bit tipsFlash_flg = 0;
					
					if(!counter_tipsAct){
					
						counter_tipsAct = tipsCounter_scenarioKeepTrigForce;
						tipsFlash_flg = !tipsFlash_flg;
					}
				
					switch(scenario_ActParam.scenarioKey_currentTrig){
					
						case scenarioKey_current_S1:{
						
							tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_R, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_G, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_B);
							tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_R, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_G, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_B);
							
							if(scenario_ActParam.scenarioKeepTrig_timeCounter && scenario_ActParam.scenarioKeepTrig_timeCounter != COUNTER_DISENABLE_MASK_SPECIALVAL_U8){
							
								(tipsFlash_flg)?\
									(tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_R, 
																  color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_G, 
																  color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_B)):\
									(tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_R, 
																  color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_G, 
																  color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_B));
								
							}else{
							
								tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_R, 
															 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_G, 
															 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_B);
							}
						
						}break;
							
						case scenarioKey_current_S2:{
						
							tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_R, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_G, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_B);
							tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_R, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_G, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_B);
							
							if(scenario_ActParam.scenarioKeepTrig_timeCounter && scenario_ActParam.scenarioKeepTrig_timeCounter != COUNTER_DISENABLE_MASK_SPECIALVAL_U8){
							
								(tipsFlash_flg)?\
									(tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_R, 
																  color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_G, 
																  color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_B)):\
									(tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_R, 
																  color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_G, 
																  color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_B));
								
							}else{
							
								tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_R, 
															 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_G, 
															 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_B);
							}
						
						}break;
						
						case scenarioKey_current_S3:{
						
							tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_R, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_G, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_B);
							tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_R, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_G, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_B);
							
							if(scenario_ActParam.scenarioKeepTrig_timeCounter && scenario_ActParam.scenarioKeepTrig_timeCounter != COUNTER_DISENABLE_MASK_SPECIALVAL_U8){
							
								(tipsFlash_flg)?\
									(tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_R, 
																  color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_G, 
																  color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_B)):\
									(tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_R, 
																  color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_G, 
																  color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_B));
								
							}else{
							
								tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_R, 
															 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_G, 
															 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_trig].colorGray_B);
							}
						
						}break;
						
						default:{
						
							tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_R, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_G, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_B);
							tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_R, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_G, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_B);
							tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_R, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_G, 
														 color_Tab[devBackgroundLight_param.scenario_BKlight_Param.scenario_BKlightColorInsert_notrig].colorGray_B);
							
						}break;
					}
					
				}break;
				
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_HEATER)	
				case SWITCH_TYPE_HEATER:{
					
					tipsLED_colorSet(obj_Relay1, 0, 0, 0);
					tipsLED_colorSet(obj_Relay3, 0, 0, 0);
				
					switch(heater_ActParam.heater_currentActMode){
					
						case heaterActMode_swClose:{
						
							tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_close].colorGray_R, 
														 color_Tab[devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_close].colorGray_G, 
														 color_Tab[devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_close].colorGray_B);
						
						}break;
							
						case heaterActMode_swKeepOpen:{
						
							tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_open].colorGray_R, 
														 color_Tab[devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_open].colorGray_G, 
														 color_Tab[devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_open].colorGray_B);
							
						}break;
							
						case heaterActMode_swCloseDelay30min:{
						
							tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_closeDelay30].colorGray_R, 
														 color_Tab[devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_closeDelay30].colorGray_G, 
														 color_Tab[devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_closeDelay30].colorGray_B);
						
						}break;
							
						case heaterActMode_swCloseDelay60min:{
						
							tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_closeDelay60].colorGray_R, 
														 color_Tab[devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_closeDelay60].colorGray_G, 
														 color_Tab[devBackgroundLight_param.heater_BKlight_Param.heater_BKlightColorInsert_closeDelay60].colorGray_B);
						
						}break;
							
						default:{}break;
					}
						
				}break;
				
#else			
				case SWITCH_TYPE_CURTAIN:{
				
					switch(relayStatus_tipsTemp){ //非占位指示
					
						case 0x01:{
						
							tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_R, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_G, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_B);
							tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_R, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_G, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_B);
							tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press].colorGray_R, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press].colorGray_G, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press].colorGray_B);
						
						}break;
							
						case 0x04:{
						
							tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press].colorGray_R, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press].colorGray_G, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press].colorGray_B);
							tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_R, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_G, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_B);
							tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_R, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_G, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_B);
						
						}break;
						
						default:{
						
							tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_R, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_G, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_B);
							tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press].colorGray_R, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press].colorGray_G, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press].colorGray_B);
							tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_R, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_G, 
														 color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].colorGray_B);
						
						}break;
					}
				
				}break;
				
				case SWITCH_TYPE_SWBIT1:
				case SWITCH_TYPE_SWBIT2:
				case SWITCH_TYPE_SWBIT3:
				default:{ //占位指示
					
					(DEV_actReserve & 0x01)?\
						((relayStatus_tipsTemp & 0x01)?\
							(tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open].colorGray_R, 
														  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open].colorGray_G, 
														  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open].colorGray_B)):\
							(tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close].colorGray_R, 
														  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close].colorGray_G, 
														  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close].colorGray_B))):\
						(tipsLED_colorSet(obj_Relay1, tips_relayUnused.colorGray_R, tips_relayUnused.colorGray_G, tips_relayUnused.colorGray_B));
					
					(DEV_actReserve & 0x02)?\
						((relayStatus_tipsTemp & 0x02)?\
							(tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open].colorGray_R, 
														  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open].colorGray_G, 
														  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open].colorGray_B)):\
							(tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close].colorGray_R, 
														  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close].colorGray_G, 
														  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close].colorGray_B))):\
						(tipsLED_colorSet(obj_Relay2, tips_relayUnused.colorGray_R, tips_relayUnused.colorGray_G, tips_relayUnused.colorGray_B));	
					
					(DEV_actReserve & 0x04)?\
						((relayStatus_tipsTemp & 0x04)?\
							(tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open].colorGray_R, 
														  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open].colorGray_G, 
														  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open].colorGray_B)):\
							(tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close].colorGray_R, 
														  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close].colorGray_G, 
														  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close].colorGray_B))):\
						(tipsLED_colorSet(obj_Relay3, tips_relayUnused.colorGray_R, tips_relayUnused.colorGray_G, tips_relayUnused.colorGray_B));	
					
				}break;
				
#endif
			}
			
			/*过期tips处理*/
			{
			
				if((devTips_nwkZigb == nwkZigb_nwkOpen) && !tipsTimeCount_zigNwkOpen)devTips_nwkZigb = nwkZigb_Normal; //网络开放后台tips过期恢复
			}
			
			/*zigb网络状态指示*/
			{
				static bit tips_Type = 0;
				
				if(tipsNwk_Counter < tipsNwk_Period)tipsNwk_Counter ++;
				else{
				
					tips_Type = !tips_Type;
					tipsNwk_Counter = 0;
					
					switch(devTips_nwkZigb){
					
						case nwkZigb_Normal:{
						
							(tips_Type)?(tipsLED_colorSet(obj_zigbNwk, 0, 30, 0)):(tipsLED_colorSet(obj_zigbNwk, 0, 30, 0)); //常绿
						
						}break;
						
						case nwkZigb_nwkOpen:{
						
							(tips_Type)?(tipsLED_colorSet(obj_zigbNwk, 0, 30, 0)):(tipsLED_colorSet(obj_zigbNwk, 0, 0, 0)); //绿闪
							
						}break;
						
						case nwkZigb_outLine:{
						
							(tips_Type)?(tipsLED_colorSet(obj_zigbNwk, 30, 0, 0)):(tipsLED_colorSet(obj_zigbNwk, 30, 0, 0)); //常红
						
						}break;
						
						case nwkZigb_reConfig:{
							
							if(zigbNwk_exist_FLG){ //网络存在
							
								if(devZigbNwk_startUp_delayCounter)(tips_Type)?(tipsLED_colorSet(obj_zigbNwk, 30, 0, 0)):(tipsLED_colorSet(obj_zigbNwk, 0, 0, 0)); //红闪
								else tipsLED_colorSet(obj_zigbNwk, 30, 10, 0); //常黄 --有网络记录，重连中
							
							}else{ //网络不存在
								
								tipsLED_colorSet(obj_zigbNwk, 0, 0, 0); //常黑 --本身没有网络记录
							}
							
						}break;
						
						case nwkZigb_nwkREQ:{
						
							(tips_Type)?(tipsLED_colorSet(obj_zigbNwk, 30, 10, 0)):(tipsLED_colorSet(obj_zigbNwk, 0, 0, 0)); //黄闪
						
						}break;
						
						case nwkZigb_hold:{
						
							(tips_Type)?(tipsLED_colorSet(obj_zigbNwk, 30, 0, 0)):(tipsLED_colorSet(obj_zigbNwk, 0, 0, 0)); //红闪
							
						}break;
						
						default:{
						
							devTips_nwkZigb = nwkZigb_Normal;
						
						}break;
					}
				}
			}
			
		}break;
		
		case status_tipsNwkOpen:{
			
			if(tipsTimeCount_zigNwkOpen)tips_specified(1);
			else{

				tips_statusChangeToNormal();
#if(DEBUG_LOGOUT_EN == 1)
				{ //输出打印，谨记 用后注释，否则占用大量代码空间
					memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
					sprintf(log_buf, "zigbNwk Close.\n");
					PrintString1_logOut(log_buf);
				}			
#endif	
			}
			
		}break;
		
		case status_tipsNwkFind:{
		
			tips_specified(0);
		
		}break;
		
		case status_devHold:{
		
			tips_warning();
			
		}break;
		
		case status_touchReset:{
		
			if(tipsTimeCount_touchReset)tips_sysTouchReset();
			else{
			
				tips_statusChangeToNormal();
#if(DEBUG_LOGOUT_EN == 1)
				{ //输出打印，谨记 用后注释，否则占用大量代码空间
					memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
					sprintf(log_buf, "touch reset complete.\n");
					PrintString1_logOut(log_buf);
				}			
#endif	
			}
			
		}break;
			
		default:{
		
			devTips_status = status_Normal;
			
		}break;
	}
#endif
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

void tips_warning(void){

	static u8 xdata tipsStep = 0;
	
	switch(tipsStep){
	
		case 0:{
		
			counter_tipsAct = 1000;
			tipsStep = 1;
		
		}break;
		
		case 1:{
		
			if(counter_tipsAct){
				
				if(counter_tipsAct % 199 > 99){
				
					zigbNwk_Tips.colorGray_R = \
					relay3_Tips.colorGray_R = \
					relay2_Tips.colorGray_R = \
					relay1_Tips.colorGray_R = 31;
				
				}else{
				
					zigbNwk_Tips.colorGray_R = \
					relay3_Tips.colorGray_R = \
					relay2_Tips.colorGray_R = \
					relay1_Tips.colorGray_R = 0;
				}
			
			}else{
			
				counter_tipsAct = 600;
				thread_tipsGetDark(0x0F);
				tipsStep = 2;
			}
		
		}break;
		
		
		case 2:{
		
			if(!counter_tipsAct)tipsStep = 0;
			
		}break;
		
		default:{
		
			tipsStep = 0;
		
		}break;
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
	
	u8 code speed = 3;
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

void tips_specified(u8 tips_Type){ //tips类别

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
						
							if(localTips_Count % 15 > 10){
							
								zigbNwk_Tips.colorGray_R = 31;
								zigbNwk_Tips.colorGray_G = 10;
								
							}else{
							
								zigbNwk_Tips.colorGray_R = 0;
								zigbNwk_Tips.colorGray_G = 0;
							}
						
						}else{
						
							if(localTips_Count > 45){zigbNwk_Tips.colorGray_R = 31; zigbNwk_Tips.colorGray_G = 10;}
							if(localTips_Count > 50){relay3_Tips.colorGray_R = 31; relay3_Tips.colorGray_G = 10;}
							if(localTips_Count > 60){relay2_Tips.colorGray_R = 31; relay2_Tips.colorGray_G = 10;}
							if(localTips_Count > 70){relay1_Tips.colorGray_R = 31; relay1_Tips.colorGray_G = 10;}
						}
						
					}else{ 
						
						zigbNwk_Tips.colorGray_R = pwmType_D;
						relay3_Tips.colorGray_R = pwmType_C;
						relay2_Tips.colorGray_R = pwmType_B;
						relay1_Tips.colorGray_R = pwmType_A;
						
						zigbNwk_Tips.colorGray_G = pwmType_D / 3;
						relay3_Tips.colorGray_G = pwmType_C / 3;
						relay2_Tips.colorGray_G = pwmType_B / 3;
						relay1_Tips.colorGray_G = pwmType_A / 3;
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
						
//							if(localTips_Count % 15 > 10)zigbNwk_Tips.colorGray_B = 31;else zigbNwk_Tips.colorGray_B = 0;
							if(localTips_Count % 15 > 10)zigbNwk_Tips.colorGray_G = 31;else zigbNwk_Tips.colorGray_G = 0;
						
						}else{
						
//							if(localTips_Count > 45)relay1_Tips.colorGray_B = 31;
//							if(localTips_Count > 50)relay2_Tips.colorGray_B = 31;
//							if(localTips_Count > 60)relay3_Tips.colorGray_B = 31;
//							if(localTips_Count > 70)zigbNwk_Tips.colorGray_B = 31;
							
							if(localTips_Count > 45)relay1_Tips.colorGray_G = 31;
							if(localTips_Count > 50)relay2_Tips.colorGray_G = 31;
							if(localTips_Count > 60)relay3_Tips.colorGray_G = 31;
							if(localTips_Count > 70)zigbNwk_Tips.colorGray_G = 31;
						}
						
					}else{ 
						
//						relay1_Tips.colorGray_B = pwmType_D;
//						relay2_Tips.colorGray_B = pwmType_C;
//						relay3_Tips.colorGray_B = pwmType_B;
//						zigbNwk_Tips.colorGray_B = pwmType_A;
						
						relay1_Tips.colorGray_G = pwmType_D;
						relay2_Tips.colorGray_G = pwmType_C;
						relay3_Tips.colorGray_G = pwmType_B;
						zigbNwk_Tips.colorGray_G = pwmType_A;
					}
					
				}break;
					
				default:break;
			}
			
		}break;
		
		default:break;
	}
}

void tips_sysTouchReset(void){
	
	static bit tipsStep = 0;

	if(counter_tipsAct){
	
		if(tipsStep){
		
			zigbNwk_Tips.colorGray_R = 0;
			relay1_Tips.colorGray_R = 0;
			relay2_Tips.colorGray_R = 0;
			relay3_Tips.colorGray_R = 0;
			
		}else{
		
			zigbNwk_Tips.colorGray_R = 31;
			relay1_Tips.colorGray_R = 31;
			relay2_Tips.colorGray_R = 31;
			relay3_Tips.colorGray_R = 31;
		}
		
	}else{
	
		counter_tipsAct = 400;
		tipsStep = !tipsStep;
	}
}

void tips_sysStandBy(void){

	zigbNwk_Tips.colorGray_R = 31;
	relay1_Tips.colorGray_R = 31;
	relay2_Tips.colorGray_R = 31;
	relay3_Tips.colorGray_R = 31;
}

void tips_sysButtonReales(void){

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
		
		counter_tipsAct = 12; //更新单周期时间(@50us中断，中断频率为网关主机两倍)
	
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


