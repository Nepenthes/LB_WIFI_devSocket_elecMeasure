#include "appTimer.h"

#include "STC15Fxxxx.H"

#include "stdio.h"
#include "string.h"

#include "dataTrans.h"
#include "Relay.h"
#include "HW8012.h"
#include "wifi_LPT220.h"
#include "Tips.h"
#include "dataManage.h"
#include "eeprom.h"

//*********************数据传输变量引用区***************************/
extern bit 				rxTout_count_EN;	
extern u8  				rxTout_count;	//串口接收超时计数
extern bit 				uartRX_toutFLG;
extern u8 				datsRcv_length;
extern uartTout_datsRcv xdata datsRcv_ZIGB;

//*********************WIFI模块功能引脚按键线程变量引用区*********/
extern u16 xdata		funKey_WIFI_cnt;

//*********************数据传输相关变量引用区*********************/
extern u16 xdata		heartBeat_Cnt;

//*********************继电器相关变量引用区*********************/
extern bit 				status_Relay;

//*********************定时延时业务相关变量引用区*****************/
extern switch_Status	swStatus_fromTim;
extern u8 				ifDelay_sw_running_FLAG;	//延时动作_是否运行标志位（bit 1延时开关运行使能标志，bit 0定时关闭运行使能标志）
extern u16				delayCnt_onoff;
extern u8				delayPeriod_onoff;
extern bit				delayUp_act;
extern u16				delayCnt_closeLoop;
extern u8				delayPeriod_closeLoop;
extern bit 				greenModeStart_IF;

//***************用户按键线程变量引用区*****************/
extern bit		 		usrKeyCount_EN;
extern u16	idata 		usrKeyCount;

//*********************Tips变量引用区****************************/
extern enum_segTips dev_segTips;
extern enum_ledTips dev_ledTips;

void appTimer0_Init(void){	//50us 中断@24.000M

	AUXR |= 0x80;		
	TMOD &= 0xF0;		
	TL0   = 0x50;		
	TH0   = 0xFB;	
	TF0   = 0;	
	ET0	  = 1;	//开中断
	PT0   = 0;	//高优先级中断
	
	TR0   = 1;		
}

void timer0_Rountine (void) interrupt TIMER0_VECTOR{	
	
	u32 code fpDetect_Period 		= 100000UL;	//测频检测周期5秒  100000 * 50us = 5s
	static xdata u32 fpDetect_Cnt 	= 0UL;
	
	u16 code  fpDetectPeriod_stdBy	= 1000;		//负载检测tips_standBy
	static xdata u16 fpDetectCount_stdBy = 0;

	u8 code   period_1ms 			= 20;		//1ms专用
	static u8 count_1ms 			= 0;
	
	u16 code period_1second 		= 20000;	//1秒专用
	static xdata u16 count_1second 	= 0;	
	
	u16 code tipsInit_Period 		= 1000;		//延时检测计数周期
	static xdata u16 tipsInit_Cnt 	= 0;	
	
	u8 code   period_beep 			= 2;		//beep专用
	static u8 count_beep 			= 0;
	
	//*******************beep计时计数业务**************************/
	if(count_beep < period_beep)count_beep ++;
	else{
	
		count_beep = 0;
		
		switch(dev_beeps){ //状态机
			
			case beepsMode_Touch:{
				
				u16 code tips_Period = 20 * 300 / 2;
				static u16 xdata tips_Count = 0;
				
				if(tipsBeep_INTFLG){
				
					tipsBeep_INTFLG = 0;
					tips_Count = 0;
				}
				
				if(tips_Count < tips_Period){
				
					tips_Count ++;
					if(beeps_EN)PIN_BEEP = !PIN_BEEP;
					
				}
				else
				{
				
					tips_Count = 0;
					
					PIN_BEEP = 1;
					dev_beeps = beepsMode_null;
				}
				
			}break;
		
			default:{
			
				PIN_BEEP = 1;
				
			}break;
		}
	}
	
	//*******************tips流水灯计时计数业务**************************/
	if(tipsInit_Cnt < tipsInit_Period)tipsInit_Cnt ++;
	else{
	
		tipsInit_Cnt  = 0;
		
		switch(dev_segTips){ //状态机
		
			case segMode_init:{
			
				segTips_Init();
				
			}break;
			
			case segMode_initCmp:{
			
				segTips_InitCmp();
				
			}break;
			
			case segMode_touchOpen:{
			
				segTips_touchOpen();
				
			}break;
			
			case segMode_touchClose:{
			
				segTips_touchClose();
				
			}break;
			
			case segMode_elecDetectStandby:{
			
				segTips_detectStandBy();
				
			}break;
			
			case segMode_elecDetect:{
			
				powerTips(paramElec_Param.ePower);	//功率Tips显示
				
			}break;
				
			default:{
			
				segTips_allDark();
				
			}break;
		}
	}
	
	//*******************tips指示灯业务**************************/
	switch(dev_ledTips){ //状态机
	
		case ledMode_init:{
		
			PIN_LED = tipsLED_statusON;
			
		}break;
		
		case ledMode_relayOpenIF:{
		
			(status_Relay)?(PIN_LED = tipsLED_statusON):(PIN_LED = tipsLED_statusOFF);
		
		}break;	
		
		case ledMode_smartConfig:{
		
			u16 code tips_Period 	= 20 * 150;
			static u16 xdata tips_Count = 0;
			u8  code loop_Period 	= 60;
			static u8 tips_Loop 	= 60;
			
			if(tips_Count < tips_Period)tips_Count ++;
			else{

				tips_Count = 0;
				
				if(tips_Loop){
				
					tips_Loop --;
					PIN_LED = !PIN_LED;
					
				}else{
				
					PIN_LED 	= tipsLED_statusOFF;
					tips_Loop 	= loop_Period;
					dev_ledTips = ledMode_relayOpenIF;
				}
			}
			
		}break;
		
		case ledMode_factory:{
		
			u16 code tips_Period = 20 * 6000;
			static u16 xdata tips_Count = 0;
			
			PIN_LED = tipsLED_statusON;
			if(tips_Count < tips_Period)tips_Count ++;
			else{
			
				tips_Count = 0;
				
				PIN_LED = tipsLED_statusOFF;
				dev_ledTips = ledMode_relayOpenIF;
			}
			
		}break;
		
		default:{
		
			PIN_LED = tipsLED_statusOFF;
		
		}break;
	}
	
	//*******************标准1秒计数业务***************************/
	if(count_1second < period_1second)count_1second ++;
	else{
		
		count_1second = 0;
	
		/*延时计时业务，到点动作*/
		if(ifDelay_sw_running_FLAG & (1 << 1)){
		
			if(delayCnt_onoff < ((u16)delayPeriod_onoff * 60))delayCnt_onoff ++;
			else
			if(delayCnt_onoff){ //计时值确认
			
				delayCnt_onoff = 0;
				
				ifDelay_sw_running_FLAG &= ~(1 << 1);	//一次性标志清除
				
				(delayUp_act)?(swStatus_fromTim = swStatus_on):(swStatus_fromTim = swStatus_off);	//开关动作
			}
		}
		
		/*绿色模式计时业务，循环关闭*/
		if((ifDelay_sw_running_FLAG & (1 << 0)) && status_Relay){
		
			if(delayCnt_closeLoop < ((u16)delayPeriod_closeLoop * 60))delayCnt_closeLoop ++;
			else{
				
				delayCnt_closeLoop = 0;
				greenModeStart_IF = 0;
			
				swStatus_fromTim = swStatus_off;
			}
		}
	}
	
	//*******************标准1毫秒计数业务**************************/
	if(count_1ms < period_1ms)count_1ms ++;
	else{
	
		count_1ms = 0;
		
		/*心跳包周期计数*/
		heartBeat_Cnt++;
		
		/*用户按键超时计数*/
		if(usrKeyCount_EN)usrKeyCount ++;
		
		/*WIFI模块功能引脚按键计数业务区*/
		if(funKey_WIFI_cnt)funKey_WIFI_cnt --;
		else{
		
			WIFI_funcPIN_nReload = WIFI_pinDISEN;	//WIFI外部引脚控制业务
		}
		
		/*HLW8012测频Tips-standBy*///预检测
		if(fpDetectCount_stdBy < fpDetectPeriod_stdBy)fpDetectCount_stdBy ++;
		else{
			
			fpDetectCount_stdBy = 0;
			
			pinFP_powerStdby = pinFP_stdby_powerCNT / 1.0F;
			pinFP_stdby_powerCNT = 0;
			if(status_Relay == actRelay_ON){
				
				if((pinFP_powerStdby - pinFP_power) > 10.0F){ //升差
				
					dev_segTips = segMode_elecDetectStandby;
					tipsSeg_INTFLG = 1;
				}
				else
				if((pinFP_power - pinFP_powerStdby) > 10.0F){ //落差
				
					dev_segTips = segMode_null;
					tipsSeg_INTFLG = 1;
				}else
				if((pinFP_powerStdby - pinFP_power) < 3.0F && (pinFP_powerStdby - pinFP_power) > -3.0F){ //无差
				
					dev_segTips = segMode_elecDetect;
					tipsSeg_INTFLG = 1;
				}
			}
		}
	}
	
	//***************串口接收超时时长计数*******************************//
	if(rxTout_count_EN){
	
		if(rxTout_count < TimeOutSet1)rxTout_count ++;
		else{
			
			if(!uartRX_toutFLG){
			
				uartRX_toutFLG = 1;
				
				memset(datsRcv_ZIGB.rcvDats, 0xff, sizeof(char) * COM_RX1_Lenth);
				memcpy(datsRcv_ZIGB.rcvDats, RX1_Buffer, datsRcv_length);
				datsRcv_ZIGB.rcvDatsLen = datsRcv_length;
			}
			rxTout_count_EN = 0;
		}
	}
	
	//*******************HLW8012测频业务****************************	
	if(fpDetect_Cnt <= fpDetect_Period)fpDetect_Cnt ++;
	else{
	
		fpDetect_Cnt 	 = 0;
		
		pinFP_volXcur	 = pinFP_volXcurCNT / 5.0F;	//值确认
		pinFP_power		 = pinFP_powerCNT / 5.0F;
		
//		if(pinFP_power > 5.0){
//		
//			if(status_Relay == actRelay_ON){
//			
//				dev_segTips = segMode_elecDetect;
//				tipsSeg_INTFLG = 1;
//			}
//		}
		
		if(pin_funFP_Select){	//电压
		
			paramElec_Param.eVoltage = pinFP_volXcur * (coefficient_Vol - (0.00000082 * pinFP_volXcur));
			
		}else{	//电流
		
//			paramElec_Param.eCurrent = (float)pinFP_volXcur * coefficient_Cur;
			
			paramElec_Param.eCurrent = pinFP_volXcur * (coefficient_Cur + (0.00000246 * pinFP_volXcur));
		}
		
//		/*测试数据*///测试代码段
//		{
//		
//			static float xdata test_elec_Consum = 0.0,
//						 xdata test_ePower	  = 0.0;
//			
//			test_elec_Consum += 0.0001;
//			test_ePower		 += 0.0001;
//			
//			elec_Consum				= test_elec_Consum;
//			paramElec_Param.ePower 	= test_ePower;
//		}
		
		//避免浮点数全部为零计算时出现非法值
//		if(!pinFP_powerCNT) pinFP_powerCNT = 1;
//		if(!pinFP_power) pinFP_power = 1; 
//		if(paramElec_Param.ePower < 0.1) paramElec_Param.ePower = 0.1;

		/*实际业务逻辑*/
		paramElec_Param.ePower = (float)pinFP_power * (coefficient_Pow - (0.00000001 * pinFP_power));	//功率
		
		{
			float powerFP_Temp = pinFP_power;
			
			if(powerFP_Temp < 0.1F)powerFP_Temp = 0.1F;
			elec_Consum	+= 1.00F * ((float)pinFP_powerCNT * paramElec_Param.ePower / (1000.00F * 3600.00F * (float)powerFP_Temp)); //用电量
		}
		
		pinFP_volXcurCNT = 0.0;	//脉冲清零
		pinFP_powerCNT	 = 0.0;
	}
}