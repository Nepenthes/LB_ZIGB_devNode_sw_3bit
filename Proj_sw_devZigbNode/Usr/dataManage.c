#include "dataManage.h"

#include "STC15Fxxxx.H"

#include "Relay.h"

#include "eeprom.h"
#include "delay.h"
#include "USART.h"

#include "stdio.h"
#include "string.h"

#include "Tips.h"

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_FANS)
 u8 SWITCH_TYPE = SWITCH_TYPE_FANS;
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
 u8 SWITCH_TYPE = SWITCH_TYPE_dIMMER;
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
 u8 SWITCH_TYPE = SWITCH_TYPE_SOCKETS;
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)
 u8 SWITCH_TYPE = SWITCH_TYPE_INFRARED;
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SCENARIO)
 u8 SWITCH_TYPE = SWITCH_TYPE_SCENARIO;
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_HEATER)
 u8 SWITCH_TYPE = SWITCH_TYPE_HEATER;
#else
 u8 SWITCH_TYPE = SWITCH_TYPE_CURTAIN;
#endif

u8 DEV_actReserve = 0x01;

//u8 CTRLEATHER_PORT[clusterNum_usr] = {0x1A, 0x1B, 0x1C};
u8 CTRLEATHER_PORT[clusterNum_usr] = {0, 0, 0};
u16 idata mutualCtrlDevList[clusterNum_usr][MUTUALCTRL_DEV_NUM_MAX - 1] = {0}; //互控组内设备列表,<必须使用idata 或 data 内存，xdata内存有缺陷,会导致第一个8 bit内存无故被清零>

u16 dev_currentPanid = 0;

#if(DEBUG_LOGOUT_EN == 1)	
 u8 xdata log_buf[LOGBUFF_LEN] = {0};
#endif		

/********************本地文件变量创建区******************/
unsigned char xdata MAC_ID[6] 		= {0}; 
unsigned char xdata MAC_ID_DST[6] 	= {1,1,1,1,1,1};  //远端MAC地址默认全是1，全是0的话影响服务器解析

#if(DATASAVE_INTLESS_ENABLEIF)
 u8 xdata loopInsert_relayStatusRealTime_record = 0; //继电器状态实时记录游标
#endif

//设备锁标志
bit	deviceLock_flag	= false;

//zigb网络存在标志
bit zigbNwk_exist_FLG = 0;

/*MAC更新*/
void MAC_ID_Relaes(void){
	
	u8 code *id_ptr = ROMADDR_ROM_STC_ID;

	memcpy(MAC_ID, id_ptr - 5, 6); //起点在前，向后读，只取后六位
	
#if(DEBUG_LOGOUT_EN == 1)	
	{ //输出打印，谨记 用后注释，否则占用大量代码空间

		memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
		
		sprintf(log_buf, "mac_reales:%02X %02X %02X ", (int)MAC_ID[0], (int)MAC_ID[1], (int)MAC_ID[2]);
		PrintString1_logOut(log_buf);
		sprintf(log_buf, "%02X %02X %02X.\n", (int)MAC_ID[3], (int)MAC_ID[4], (int)MAC_ID[5]);
		PrintString1_logOut(log_buf);
	}
#endif

//	memcpy(MAC_ID, id_ptr - 5, 6); 
//	memcpy(MAC_ID, MACID_test, 6); 
}

void portCtrlEachOther_Reales(void){

	EEPROM_read_n(EEPROM_ADDR_portCtrlEachOther, CTRLEATHER_PORT, 3);
}

void devLockInfo_Reales(void){

	u8 xdata deviceLock_IF = 0;

	EEPROM_read_n(EEPROM_ADDR_deviceLockFLAG, &deviceLock_IF, 1);
	
	(deviceLock_IF)?(deviceLock_flag = 1):(deviceLock_flag = 0);
}

/*获取当前开关类型对应有效操作位*/
u8 switchTypeReserve_GET(void){

	u8 act_Reserve = 0x07;

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
	act_Reserve = 0x01;
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)
	act_Reserve = 0x01;
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
	act_Reserve = 0x07;
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_FANS)
	act_Reserve = 0x07;
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SCENARIO)
	act_Reserve = 0x07;
#else
	switch(SWITCH_TYPE){
	
		case SWITCH_TYPE_HEATER:
		case SWITCH_TYPE_SWBIT1:{
		
			act_Reserve = 0x02;
		
		}break;
		
		case SWITCH_TYPE_SWBIT2:{
		
			act_Reserve = 0x05;
		
		}break;
		
		case SWITCH_TYPE_SWBIT3:
		case SWITCH_TYPE_CURTAIN:{
		
			act_Reserve = 0x07;
		
		}break;
	}
	
#endif
	
	return act_Reserve;
}

void statusSave_zigbNwk_nwkExistIF(bit nwkExistIF){
	
	u8 idata dataTemp = 0;

	zigbNwk_exist_FLG = nwkExistIF;
	
	(nwkExistIF)?(dataTemp = DATASAVE_MASK_ZIGBNWK_EXIST):(dataTemp = DATASAVE_MASK_ZIGBNWK_EXISTNOT);
	
	coverEEPROM_write_n(EEPROM_ADDR_zigbNwkExistIF, &dataTemp, 1);
	
#if(DEBUG_LOGOUT_EN == 1)	
	{ //输出打印，谨记 用后注释，否则占用大量代码空间

		memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
		
		sprintf(log_buf, "zigbNwk exsitFLG reales:%d.\n", (int)zigbNwk_exist_FLG);
		PrintString1_logOut(log_buf);
	}
#endif

}

bit statusGet_zigbNwk_nwkExistIF(void){

	u8 idata dataTemp = 0;
	
	EEPROM_read_n(EEPROM_ADDR_zigbNwkExistIF, &dataTemp, 1);
	
	if(dataTemp == DATASAVE_MASK_ZIGBNWK_EXIST)return 1;
	else return 0;
}

void zigbNwkExist_detectReales(void){

	zigbNwk_exist_FLG = statusGet_zigbNwk_nwkExistIF();
	
#if(DEBUG_LOGOUT_EN == 1)	
	{ //输出打印，谨记 用后注释，否则占用大量代码空间

		memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
		
		sprintf(log_buf, "zigbNwk exsitFLG reales:%d.\n", (int)zigbNwk_exist_FLG);
		PrintString1_logOut(log_buf);
	}
#endif
}

void mutualCtrlSysParam_checkAndStore(u8 mutualCtrlGroup_insert, u16 devAddr){

	u8 xdata 	loop = 0;
	bit idata 	devAddrExist_IF = 0;
	
	if(mutualCtrlGroup_insert > (clusterNum_usr - 1))return;
	
	for(loop = 0; loop < (MUTUALCTRL_DEV_NUM_MAX - 1); loop ++){
		
#if(DEBUG_LOGOUT_EN == 1)	
		{ //输出打印，谨记 用后注释，否则占用大量代码空间

			memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
			
			sprintf(log_buf, "addr_t:%04X, addr_s:%04X.\n", (int)devAddr, (int)mutualCtrlDevList[mutualCtrlGroup_insert][loop]);
			PrintString1_logOut(log_buf);
		}
#endif
	
		if(devAddr == mutualCtrlDevList[mutualCtrlGroup_insert][loop]){
		
			devAddrExist_IF = 1;
			break;
		}
	}
	
	if(devAddrExist_IF){
	
#if(DEBUG_LOGOUT_EN == 1)	
		{ //输出打印，谨记 用后注释，否则占用大量代码空间

			memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
			
			sprintf(log_buf, "devAddr:%04X exist in mutualGroup:%d.\n", (int)devAddr, (int)mutualCtrlGroup_insert);
			PrintString1_logOut(log_buf);
		}
#endif
	
	}else{
		
		memcpy((u8 *)&(mutualCtrlDevList[mutualCtrlGroup_insert][0]), (u8 *)&(mutualCtrlDevList[mutualCtrlGroup_insert][1]), sizeof(u16) * (MUTUALCTRL_DEV_NUM_MAX - 2));
		mutualCtrlDevList[mutualCtrlGroup_insert][MUTUALCTRL_DEV_NUM_MAX - 2] = devAddr;
		coverEEPROM_write_n(EEPROM_ADDR_mutualCtrlAddrs, (u8 *)mutualCtrlDevList, sizeof(u16) * clusterNum_usr * (MUTUALCTRL_DEV_NUM_MAX - 1));
	
#if(DEBUG_LOGOUT_EN == 1)	
		{ //输出打印，谨记 用后注释，否则占用大量代码空间

			memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
			
			sprintf(log_buf, "devAddr:%04X add to mutualGroup:%d.\n", (int)devAddr, (int)mutualCtrlGroup_insert);
			PrintString1_logOut(log_buf);
		}
		
//		{ //输出打印，谨记 用后注释，否则占用大量代码空间

//			memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
//			
//			sprintf(log_buf, "devAddrList[0]:%04X %04X.\n", (int)mutualCtrlDevList[mutualCtrlGroup_insert][0],
//															(int)mutualCtrlDevList[mutualCtrlGroup_insert][1]);
//			PrintString1_logOut(log_buf);
//		}
#endif
	}
}

void mutualCtrlSysParam_dataReset(u8 opreatBit){

	u8 idata loop = 0;
	
	for(loop = 0; loop < clusterNum_usr; loop ++){
	
		if(opreatBit & (1 << loop))memset(mutualCtrlDevList[loop], 0xff, sizeof(u16) * (MUTUALCTRL_DEV_NUM_MAX - 1));
	}
	
	coverEEPROM_write_n(EEPROM_ADDR_mutualCtrlAddrs, (u8 *)mutualCtrlDevList, sizeof(u16) * clusterNum_usr * (MUTUALCTRL_DEV_NUM_MAX - 1));
}

void mutualCtrlSysParam_dataRecover(void){

	EEPROM_read_n(EEPROM_ADDR_mutualCtrlAddrs, (u8 *)mutualCtrlDevList, sizeof(u16) * clusterNum_usr * (MUTUALCTRL_DEV_NUM_MAX - 1));
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
	
	coverEEPROM_write_n(EEPROM_ADDR_START_USRDATA, datsTemp, EEPROM_USE_OF_NUMBER); //首次启动EEPROM擦除
	datsTemp[0] = BIRTHDAY_FLAG;
	coverEEPROM_write_n(EEPROM_ADDR_BirthdayMark, &datsTemp[0], 1);	//打出生标记
	
	memset(datsTemp, 0xff, sizeof(bkLightColorInsert_paramAttr)); //背光灯参数出厂化 --出厂化0xff填满，促使背光灯初始化时参数恢复至默认值
	coverEEPROM_write_n(EEPROM_ADDR_ledSWBackGround, datsTemp, sizeof(bkLightColorInsert_paramAttr));
	
	datsTemp[0] = 0;
	coverEEPROM_write_n(EEPROM_ADDR_deviceLockFLAG, &datsTemp[0], 1); //重新解锁
	
#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_FANS)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_dIMMER)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SOCKETS)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_SCENARIO)
#elif(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_HEATER)
#else
	datsTemp[0] = CURTAIN_ORBITAL_PERIOD_INITTIME; //窗帘轨道时间 赋初始值
	coverEEPROM_write_n(EEPROM_ADDR_curtainOrbitalPeriod, &datsTemp[0], 1);
	
#endif	

	datsTemp[0] = 0;
	coverEEPROM_write_n(EEPROM_ADDR_portCtrlEachOther, &datsTemp[0], clusterNum_usr);
	
	memset(CTRLEATHER_PORT, 0, clusterNum_usr); //运行缓存清空
	
	delayMs(10);
	
	((void(code *)(void))0x0000)(); //重启
}

void birthDay_Judge(void){

	u8 xdata datsTemp = 0;
	
	EEPROM_read_n(EEPROM_ADDR_BirthdayMark, &datsTemp, 1);
	if(datsTemp != BIRTHDAY_FLAG){
	
		Factory_recover(); //首次启动EEPROM擦除
	}
}

#if(SWITCH_TYPE_FORCEDEF == SWITCH_TYPE_INFRARED) //红外数据存储，一个扇区一条数据
void infrared_eeprom_dataSave(u8 insertNum, u8 dats[], u8 datsLen){
	
	//一个扇区为512 Bytes，一个红外指令为232 Bytes，故一个扇区可以存两个红外指令

	u16 code EEPROM_SECTOR_HALFSIZE = EEPROM_SECTOR_SIZE / 2;
	
	u8	xdata dataSaveTemp[EEPROM_SECTOR_SIZE] = {0}; //数据存储缓存
	u16 xdata EEadress_Insert = EEPROM_ADDR_START_IRDATA + ((u16)(insertNum / 2) * EEPROM_SECTOR_SIZE); //存储地址索引缓存
	
	EEPROM_read_n(EEadress_Insert, dataSaveTemp, EEPROM_SECTOR_SIZE); //整个扇区数据读到缓存
	EEPROM_SectorErase(EEadress_Insert); //缓存获取后擦除整个扇区
	
	if(insertNum % 2){ //奇数，对应扇区后半
	
		memset(&dataSaveTemp[EEPROM_SECTOR_HALFSIZE], 0, EEPROM_SECTOR_HALFSIZE); //数据存储至扇区后半
		memcpy(&dataSaveTemp[EEPROM_SECTOR_HALFSIZE], dats, datsLen);
		
	}else{ //偶数，对应扇区前半
	
		memset(&dataSaveTemp[0], 0, EEPROM_SECTOR_HALFSIZE); //数据存储至扇区前半
		memcpy(&dataSaveTemp[0], dats, datsLen);
	}
	
	EEPROM_write_n(EEadress_Insert, dataSaveTemp, EEPROM_SECTOR_SIZE);
}

void infrared_eeprom_dataRead(u8 insertNum, u8 dats[], u8 datsLen){
	
	//一个扇区为512 Bytes，一个红外指令为232 Bytes，故一个扇区可以存两个红外指令
	
	u16 code EEPROM_SECTOR_HALFSIZE = EEPROM_SECTOR_SIZE / 2;

	u16 xdata EEadress_Insert = EEPROM_ADDR_START_IRDATA + ((u16)(insertNum / 2) * EEPROM_SECTOR_SIZE); //存储地址索引缓存
	
	if(insertNum % 2){ //奇数，对应扇区后半
	
		EEadress_Insert += EEPROM_SECTOR_HALFSIZE; //存储索引更新至扇区后半
		
	}else{ //偶数，对应扇区前半
	
		EEadress_Insert = EEadress_Insert; //存储索引更新至扇区前半
	}
	
	EEPROM_read_n(EEadress_Insert, dats, datsLen);
}
#else
 #if(DATASAVE_INTLESS_ENABLEIF) //继电器状态EEPROM独立存储相关函数定义
void devParamDtaaSave_relayStatusRealTime(u8 currentRelayStatus){
	
	u8 xdata dataRead_temp[RECORDPERIOD_OPREATION_LOOP] = {0};
	
	if(loopInsert_relayStatusRealTime_record >= RECORDPERIOD_OPREATION_LOOP){
	
		loopInsert_relayStatusRealTime_record = 0;
		EEPROM_SectorErase(EEPROM_ADDR_START_STATUSRELAY); //擦扇区
	}
	
	dataRead_temp[loopInsert_relayStatusRealTime_record ++] = currentRelayStatus;
	
	EEPROM_write_n(EEPROM_ADDR_START_STATUSRELAY, dataRead_temp, loopInsert_relayStatusRealTime_record);
}

u8 devDataRecovery_relayStatus(void){

	u8 xdata dataRead_temp[RECORDPERIOD_OPREATION_LOOP] = {0};
	u8 xdata loop = 0;
	u8 xdata res = 0;
	
	EEPROM_read_n(EEPROM_ADDR_START_STATUSRELAY, dataRead_temp, RECORDPERIOD_OPREATION_LOOP);
	
	for(loop = 0; loop < RECORDPERIOD_OPREATION_LOOP; loop ++){
	
		if(dataRead_temp[loop] == 0xff){
		
			(!loop)?(res = 0):(res = dataRead_temp[loop - 1]);
  #if(DEBUG_LOGOUT_EN == 1)	
			{ //输出打印，谨记 用后注释，否则占用大量代码空间

				memset(log_buf, 0, LOGBUFF_LEN * sizeof(u8));
				
				sprintf(log_buf, "insert catch: %d, val:%02X.\n", (int)loop, (int)res);
				PrintString1_logOut(log_buf);
			}
  #endif
			break;
		}
	}
	
	EEPROM_SectorErase(EEPROM_ADDR_START_STATUSRELAY); //擦扇区
	
	return res;
}
 #endif

#endif