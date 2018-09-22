#include "Relay.h"

#include "eeprom.h"
#include "dataManage.h"
#include "Tips.h"

//*********************数据传输相关变量引用区*************************/
extern switch_Status	swStatus_fromUsr; //上位机直接下达的开关命令

//*********************定时检测进程相关变量引用区*********************/
extern switch_Status	swStatus_fromTim;  //定时时刻到达时开关响应动作状态

//*********************定时延时业务相关变量引用区*****************/
extern u16				delayCnt_closeLoop;
extern bit 				greenModeStart_IF;

/**********************本地文件变量定义区*****************************/
status_ifSave			relayStatus_ifSave		= statusSave_disable;	//开关记忆使能变量
bit 					status_Relay 			= false;

/*开关初始化*/
void relay_pinInit(void){
	
	u8 idata statusTemp = 0;

	//推挽
	P3M1	&= 0xF7;
	P3M0	|= 0x08;
	
	pinRELAY = 0;
	
	if(relayStatus_ifSave == statusSave_enable){
		
		EEPROM_read_n(EEPROM_ADDR_relayStatus, &statusTemp, 1);
		status_Relay = statusTemp;
		pinRELAY	 = status_Relay;
		
	}else{
	
		statusTemp 	 = 0;
		coverEEPROM_write_n(EEPROM_ADDR_relayStatus, &statusTemp, 1);
		pinRELAY	 = status_Relay;
	}
}

/*开关动作*/
void relay_Act(rly_methodType act_Method){
	
	u8 statusTemp = 0;
	
	switch(act_Method){
	
		case relay_flip:{
		
			status_Relay = !status_Relay;
			
		}break;
		
		case relay_on:{
		
			status_Relay = actRelay_ON;
			
		}break;
		
		case relay_off:{
		
			status_Relay = actRelay_OFF;

		}break;
	}
	
	if(status_Relay){ //seg提示灯
	
		tipsSeg_INTFLG = 1;
		dev_segTips = segMode_touchOpen;
		
	}else{
	
		dev_segTips = segMode_touchClose;
		tipsSeg_INTFLG = 1;
	}
	
	dev_ledTips = ledMode_relayOpenIF; //led提示灯
	dev_beeps = beepsMode_Touch;
	
	pinRELAY = status_Relay;
	
	if(relayStatus_ifSave == statusSave_enable){	//开关状态记忆
	
		statusTemp = status_Relay;
		coverEEPROM_write_n(EEPROM_ADDR_relayStatus, &statusTemp, 1);
	}
}

/*继电器主线程*/
void thread_Relay(void){
	
	switch(swStatus_fromUsr){	//上位机下达开关业务逻辑
	
		case swStatus_on:{
		
			swStatus_fromUsr = swStatus_null;
 			relay_Act(relay_on);
			
		}break;
		
		case swStatus_off:{
		
			swStatus_fromUsr = swStatus_null;
			relay_Act(relay_off);
			
		}break;
		
		default:break;
	}
	
	switch(swStatus_fromTim){	//定时时刻表触发开关业务逻辑
	
		case swStatus_on:{
		
			swStatus_fromTim = swStatus_null;
			relay_Act(relay_on);
			
		}break;
		
		case swStatus_off:{
		
			swStatus_fromTim = swStatus_null;
			relay_Act(relay_off);
			
		}break;
		
		default:break;
	}
	
	if(!greenModeStart_IF){ //绿色模式计时是否已经开启

		if(status_Relay == actRelay_ON){ //开关一旦打开立刻更新绿色模式时间计数值
		
			greenModeStart_IF = 1;
			delayCnt_closeLoop = 0;
		} 
	}
}