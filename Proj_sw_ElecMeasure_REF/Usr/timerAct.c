#include "timerAct.h"

#include "dataManage.h"
#include "HYM8564.h"
#include "Relay.h"
#include "dataTrans.h"

#include "string.h"
#include "stdio.h"

#include "eeprom.h"

///**************电参数测量变量声明区***********************/
extern float xdata 		elec_Consum;

/****************本地文件变量定义区*************************/
switch_Status	swStatus_fromTim  				= swStatus_null;	//定时器更新开关命令标志，定时时刻到达时开关响应动作状态

u8 				swTim_onShoot_FLAG				= 0;	//普通开关定时一次性标志——低四位标识四个定时器
bit				ifTim_sw_running_FLAG			= 0;	//普通开关定时运行标志位

u8 				ifDelay_sw_running_FLAG			= 0;	//延时动作_是否运行标志位（bit 1延时开关运行使能标志，bit 0循环定时关闭运行使能标志）
u16				delayCnt_onoff					= 0;	//延时动作计时计数
u8				delayPeriod_onoff				= 0;	//延时动作周期
bit				delayUp_act						= 0;	//延时动作具体动作
u16				delayCnt_closeLoop				= 0;	//绿色模式计时计数
u8				delayPeriod_closeLoop			= 0;	//绿色模式动作周期

/*-----------------------------------------------------------------------------------------------*/
void datsTiming_read_eeprom(timing_Dats timDats_tab[4]){

	u8 dats_Temp[12];
	u8 loop;
	
	EEPROM_read_n(EEPROM_ADDR_swTimeTab, dats_Temp, 12);
	
	for(loop = 0; loop < 4; loop ++){
	
		timDats_tab[loop].Week_Num	= (dats_Temp[loop * 3 + 0] & 0x7f) >> 0;	/*周占位数据*///低七位
		timDats_tab[loop].if_Timing = (dats_Temp[loop * 3 + 0] & 0x80) >> 7;	/*是否开启定时器数据*///高一位
		timDats_tab[loop].Status_Act= (dats_Temp[loop * 3 + 1] & 0xe0) >> 5;	/*定时响应状态数据*///高三位
		timDats_tab[loop].Hour		= (dats_Temp[loop * 3 + 1] & 0x1f) >> 0;	/*定时时刻_小时*///低五位
		timDats_tab[loop].Minute	= (dats_Temp[loop * 3 + 2] & 0xff) >> 0;	/*定时时刻_分*///全八位
	}
}

/*周占位判断*///判断当前周值是否在占位字节中
bit weekend_judge(u8 weekNum, u8 HoldNum){

	u8 loop;
	
	weekNum --;
	for(loop = 0; loop < 7; loop ++){
	
		if(HoldNum & (1 << loop)){
			
			if(loop == weekNum)return 1;
		}
	}
	
	return 0;
}

void thread_Timing(void){

	u8 loop = 0;
	
	stt_Time xdata		valTime_Local					= {0};	//当前时间缓存
	
	timing_Dats xdata	timDatsTemp_CalibrateTab[4] 	= {0};	/*定时起始时刻表缓存*///起始时刻及属性
	
	timeRead(&valTime_Local);	//当前时间获取
	
//	{ //调试log代码-当前时间输出
//		
//		u8 code log_period = 200;
//		static u8 log_Count = 0;
//		
//		if(log_Count < log_period)log_Count ++;
//		else{
//		
//			log_Count = 0;
//			
//			time_Logout(valTime_Local);
//		}
//	}
	
//	{ //调试log代码-当前电量信息输出
//		
//		u8 xdata log_dats[80] = {0};
//		u8 code log_period = 200;
//		static u8 log_Count = 0;
//		
//		if(log_Count < log_period)log_Count ++;
//		else{
//		
//			log_Count = 0;
//			
//			sprintf(log_dats, 
//					"当前小时电量累计：%.08f\n", 
//					elec_Consum);
//			/*log调试打印*///普通定时定时信息
//			uartObjWIFI_Send_String(log_dats, strlen(log_dats));
//		}
//	}
	
	/*延时业务及自动循环业务掉电存储数据读取*///开机读一次更新即可
	{
		
		static bit read_FLG = 0;
		
		if(!read_FLG){
		
			read_FLG = 1;
			
			EEPROM_read_n(EEPROM_ADDR_swDelayFLAG, &ifDelay_sw_running_FLAG, 1);
			EEPROM_read_n(EEPROM_ADDR_periodCloseLoop, &delayPeriod_closeLoop, 1);
		}
	}
	
	/*用电量定时清除*///整点清除=====================================================================================================<<<
	if((valTime_Local.time_Minute	== 0)&&		//时刻比对,整点
	   (valTime_Local.time_Second	<= 30)){	//时刻比对时间限在前30秒
		 
			elec_Consum = 0.0f;
	   }
	
	/*普通开关定时*///四段数据=======================================================================================================<<<
	datsTiming_read_eeprom(timDatsTemp_CalibrateTab);	/*普通开关*///时刻表读取
	
	/*判断是否所有普通开关定时都为关*/
	if((timDatsTemp_CalibrateTab[0].if_Timing == 0) &&	//全关，置标志位
	   (timDatsTemp_CalibrateTab[1].if_Timing == 0) &&
	   (timDatsTemp_CalibrateTab[2].if_Timing == 0) &&
	   (timDatsTemp_CalibrateTab[3].if_Timing == 0)
	  ){
	  
		ifTim_sw_running_FLAG = 0; 
		  
	}else{	//非全关，置标志位，并执行定时逻辑
		
		ifTim_sw_running_FLAG = 1; 
	
		for(loop = 0; loop < 4; loop ++){
			
			if(weekend_judge(valTime_Local.time_Week, timDatsTemp_CalibrateTab[loop].Week_Num)){	//周占位比对，成功才进行下一步
			
				if(timCount_ENABLE == timDatsTemp_CalibrateTab[loop].if_Timing){	//是否开启定时
					
//					{ //调试log代码-当前有效定时信息输出
//						
//						u8 xdata log_dats[80] = {0};
//						u8 code log_period = 200;
//						static u8 log_Count = 0;
//						
//						if(log_Count < log_period)log_Count ++;
//						else{
//						
//							log_Count = 0;
//							
//							sprintf(log_dats, 
//									"有效定时：%d号, 定_时:%d, 定_分:%d \n", 
//									(int)loop, 
//									(int)timDatsTemp_CalibrateTab[loop].Hour, 
//									(int)timDatsTemp_CalibrateTab[loop].Minute);
//							/*log调试打印*///普通定时定时信息
//							uartObjWIFI_Send_String(log_dats, strlen(log_dats));
//						}
//					}
					
					if(((u16)valTime_Local.time_Hour * 60 + (u16)valTime_Local.time_Minute) ==  \
					   ((u16)timDatsTemp_CalibrateTab[loop].Hour * 60 + (u16)timDatsTemp_CalibrateTab[loop].Minute) && //时刻比对
					   (valTime_Local.time_Second <= 5)){	 //时刻比对时间限在前5秒
						   
//						uartObjWIFI_Send_String("time_UP!!!", 11);
						
						//一次性定时判断
						if(swTim_onShoot_FLAG & (1 << loop)){	//是否为一次性定时，是则清空本段定时信息
							
							u8 code dats_Temp = 0;
							
							swTim_onShoot_FLAG &= ~(1 << loop);
							coverEEPROM_write_n(EEPROM_ADDR_swTimeTab + loop * 3, &dats_Temp, 1); //定时信息清空
						}
					   
						//普通开关动作响应
						if(timDatsTemp_CalibrateTab[loop].Status_Act & 0x01){	/*开启*/
						
							swStatus_fromTim = swStatus_on;
							
						}else{		/*关闭*/
						
							swStatus_fromTim = swStatus_off;
						}
					}else
					if(((u16)valTime_Local.time_Hour * 60 + (u16)valTime_Local.time_Minute) >	//当前时间大于定时时间，直接清除一次性标志
					   ((u16)timDatsTemp_CalibrateTab[loop].Hour * 60 + (u16)timDatsTemp_CalibrateTab[loop].Minute)){
						   
						//一次性定时判断
						if(swTim_onShoot_FLAG & (1 << loop)){	//是否为一次性定时，是则清空本段定时信息
							
							u8 code dats_Temp = 0;
							
							swTim_onShoot_FLAG &= ~(1 << loop);
							coverEEPROM_write_n(EEPROM_ADDR_swTimeTab + loop * 3, &dats_Temp, 1); //定时信息清空
						}
					}
				}
			}
		}
	}
}
