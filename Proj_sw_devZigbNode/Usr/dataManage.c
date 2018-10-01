#include "dataManage.h"

#include "STC15Fxxxx.H"

#include "eeprom.h"
#include "delay.h"

#include "stdio.h"
#include "string.h"

#include "Tips.h"

u8 SWITCH_TYPE = SWITCH_TYPE_SWBIT3;
u8 DEV_actReserve = 0x01;

//u8 CTRLEATHER_PORT[clusterNum_usr] = {0x1A, 0x1B, 0x1C};
u8 CTRLEATHER_PORT[clusterNum_usr] = {0, 0, 0};

/********************本地文件变量创建区******************/
unsigned char xdata MAC_ID[6] 		= {0}; 
unsigned char xdata MAC_ID_DST[6] 	= {1,1,1,1,1,1};  //远端MAC地址默认全是1，全是0的话影响服务器解析

//设备锁标志
bit	deviceLock_flag	= false;

/*MAC更新*/
void MAC_ID_Relaes(void){
	
	u8 code *id_ptr = ROMADDR_ROM_STC_ID;

	memcpy(MAC_ID, id_ptr - 6, 6); //顺序向前，往前读，只取后六位

//	memcpy(MAC_ID, id_ptr - 6, 6); 
//	memcpy(MAC_ID, MACID_test, 6); 
}

void portCtrlEachOther_Reales(void){

	EEPROM_read_n(EEPROM_ADDR_portCtrlEachOther, CTRLEATHER_PORT, 3);
}

void devLockInfo_Reales(void){

	u8 xdata deviceLock_IF = 0;

	coverEEPROM_write_n(EEPROM_ADDR_deviceLockFLAG, &deviceLock_IF, 1);
	
	(deviceLock_IF)?(deviceLock_flag = 1):(deviceLock_flag = 0);
}

/*获取当前开关类型对应有效操作位*/
u8 switchTypeReserve_GET(void){

	u8 act_Reserve = 0x07;

	if(SWITCH_TYPE == SWITCH_TYPE_SWBIT3){
		
		act_Reserve = 0x07;
		
	}else
	if(SWITCH_TYPE == SWITCH_TYPE_SWBIT2){
		
		act_Reserve = 0x03;
	
	}else
	if(SWITCH_TYPE == SWITCH_TYPE_SWBIT1){
	
		act_Reserve = 0x01;
	}
	
	return act_Reserve;
}

///*场景号对应EEPROM存储索引号查找*/
//u8 swScenarioNum_findFromEEPROM(bit vacancyFind_IF, u8 scenarioNum){ //是否为空位查找 否则指定场景号查找

//	u8 loop = 0;
//	u8 xdata datsTemp[SW_SCENCRAIO_LEN] = {0};
//	
//	EEPROM_read_n(EEPROM_ADDR_swScenarioNum, datsTemp, SW_SCENCRAIO_LEN);
//	for(loop = 0; loop < SW_SCENCRAIO_LEN; loop ++){
//	
//		if(vacancyFind_IF){ //查找空位
//		
//			if(0 == datsTemp[loop] || 0xff == datsTemp[loop])break;
//		
//		}else{	//查找指定场景编号
//		
//			if(scenarioNum == datsTemp[loop])break;
//		}
//	}
//	
//	if(loop < SW_SCENCRAIO_LEN){
//	
//		return loop; //索引可查 返回索引
//		
//	}else{
//	
//		return SW_SCENCRAIO_INSERTINVALID; //索引不可查 返回无效值
//	}
//}

///*场景存储*/
//bit swScenario_oprateSave(u8 scenarioNum, u8 swAct){

//	u8 datsTemp = 0;
//	u8 insert = 0;
//	
//	insert = swScenarioNum_findFromEEPROM(0, scenarioNum); //查场景编号
//	if(SW_SCENCRAIO_INSERTINVALID != insert){ //场景编号可查到则更改对应响应状态
//	
//		datsTemp = swAct;
//		coverEEPROM_write_n(EEPROM_ADDR_swScenarioAct + insert, &datsTemp, 1);
//		
//	}else{ //场景编号不可查到则新增
//		
//		insert = swScenarioNum_findFromEEPROM(1, 0); //查空位
//		if(SW_SCENCRAIO_INSERTINVALID != insert){ //有空位则新增
//		
//			datsTemp = swAct;
//			coverEEPROM_write_n(EEPROM_ADDR_swScenarioNum + insert, &datsTemp, 1);
//			datsTemp = scenarioNum;
//			coverEEPROM_write_n(EEPROM_ADDR_swScenarioAct + insert, &datsTemp, 1);
//		
//		}else{ //无空位返回失败
//		
//			return 0;
//		}
//	}
//	
//	return 1;
//}

///*场景删除*/
//bit swScenario_oprateDele(u8 scenarioNum){

//	u8 datsTemp = 0;
//	u8 insert = 0;
//	
//	insert = swScenarioNum_findFromEEPROM(0, scenarioNum); //查场景编号
//	
//	if(SW_SCENCRAIO_INSERTINVALID != insert){ //场景编号可查到则执行删除
//	
//		coverEEPROM_write_n(EEPROM_ADDR_swScenarioNum + insert, &datsTemp, 1);
//		coverEEPROM_write_n(EEPROM_ADDR_swScenarioAct + insert, &datsTemp, 1);
//		
//		return 1;
//		
//	}else{
//	
//		return 0;
//	}
//}

///*场景对应响应动作查询*/
//u8 swScenario_oprateCheck(u8 scenarioNum){

//	u8 datsTemp = 0;
//	u8 insert = 0;
//	
//	insert = swScenarioNum_findFromEEPROM(0, scenarioNum); //查场景编号
//	
//	if(SW_SCENCRAIO_INSERTINVALID != insert){ //场景编号可查到则执行删除
//		
//		EEPROM_read_n(EEPROM_ADDR_swScenarioAct, &datsTemp, 1); //读取场景对应动作并返回
//		return datsTemp;
//		
//	}else{
//	
//		return SW_SCENCRAIO_ACTINVALID; //场景号无效不可查 返回无效响应动作值
//	}
//}

void Factory_recover(void){
	
	u8 xdata datsTemp[EEPROM_USE_OF_NUMBER] = {0};
	
	coverEEPROM_write_n(EEPROM_ADDR_START, datsTemp, EEPROM_USE_OF_NUMBER); //首次启动EEPROM擦除
	datsTemp[0] = BIRTHDAY_FLAG;
	coverEEPROM_write_n(EEPROM_ADDR_BirthdayMark, &datsTemp[0], 1);	//打出生标记
	
	datsTemp[0] = TIPSBKCOLOR_DEFAULT_ON; //背光初始化
	coverEEPROM_write_n(EEPROM_ADDR_ledSWBackGround, &datsTemp[0], 1);
	datsTemp[0] = TIPSBKCOLOR_DEFAULT_OFF;
	coverEEPROM_write_n(EEPROM_ADDR_ledSWBackGround + 1, &datsTemp[0], 1);	
	
	datsTemp[0] = 0;
	coverEEPROM_write_n(EEPROM_ADDR_portCtrlEachOther, &datsTemp[0], clusterNum_usr);
	
	delayMs(10);
	
//	((void(code *)(void))0x0000)(); //重启
}

void birthDay_Judge(void){

	u8 xdata datsTemp = 0;
	
	EEPROM_read_n(EEPROM_ADDR_BirthdayMark, &datsTemp, 1);
	if(datsTemp != BIRTHDAY_FLAG){
	
		Factory_recover();//首次启动EEPROM擦除
	}
}