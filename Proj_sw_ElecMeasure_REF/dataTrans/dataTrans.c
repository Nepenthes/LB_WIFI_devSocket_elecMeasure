#include "dataTrans.h"

#include "pars_Method.h"
#include "dataManage.h"
#include "wifi_LPT220.h"
#include "HYM8564.h"
#include "HW8012.h"
#include "Relay.h"
#include "Tips.h"

#include "string.h"
#include "stdio.h"

#include "USART.h"
#include "soft_uart.h"
#include "eeprom.h"
#include "delay.h"

//*********************MAC地址相关变量引用区*********************/
extern u8 xdata 		MAC_ID[6];

//*********************定时动作线程相关变量引用区****************/
extern	u8 				swTim_onShoot_FLAG;		//普通开关定时一次性标志——低四位标识四个定时器
extern	bit				ifTim_sw_running_FLAG;	//普通开关定时运行标志位

extern u8 				ifDelay_sw_running_FLAG;	//延时动作_是否运行标志位（bit 1延时开关运行使能标志，bit 0定时关闭运行使能标志）
extern u16				delayCnt_onoff;
extern u8				delayPeriod_onoff;
extern bit				delayUp_act;
extern u16				delayCnt_closeLoop;
extern u8				delayPeriod_closeLoop;

//*********************电量检测线程相关变量引用区****************/
extern	elec_Param 	xdata paramElec_Param;	//电参数变量
extern	float		xdata elec_Consum;

//*********************电量检测线程相关变量引用区****************/
extern	bit 			status_Relay;

/**********************本地文件变量定义区************************/
//串口接收超时标志
bit uartRX_toutFLG 						= 0;
//串口接收超时计数
bit rxTout_count_EN 					= 0;
u8  rxTout_count 						= 0;
//串口数据缓存
u8	datsRcv_length						= 0;
uartTout_datsRcv xdata datsRcv_ZIGB 	= {{0}, 0};

//当前接收上位机模式标识
dataTrans_obj 	dataTrans_objWIFI 		= DATATRANS_objFLAG_MOBILE;

//上位机下达开关命令标志
switch_Status	swStatus_fromUsr  		= swStatus_null;

//数据包响应回复长度——默认局域网数据长度
u8 xdata 		repeatTX_Len			= dataTransLength_objMOBILE;

//远端MACID缓存
u8	xdata		Dst_MACID_Temp[6] 		= {0};

//心跳包奇偶标志
bit 			heartBeat_Type 			= 0;	//奇偶包标志，奇数包1，偶书包0
//心跳包周期计数值
u16	xdata		heartBeat_Cnt			= 0;
//心跳包计数周期
const 	u32 	heartBeat_Period 		= 8000; 	//心跳包周期设置毫秒系数值 * 50us（定时器内计数）

/*--------------------------------------------------------------*/
void uartObjWIFI_Init(void){
	
	EA = 0;

	PS = 1;
	SCON = (SCON & 0x3f) | UART_8bit_BRTx;

{
	u32 j = (MAIN_Fosc / 4) / BAUD_WIFI;	//按1T计算
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
	
	PrintString1("i'm UART1 for wifi data translate !!!");
}

/*----------------------------
发送串口数据
----------------------------*/
void uartObjWIFI_Send_Byte(u8 dat)	//串口1
{
	TX1_write2buff(dat);
}

void uartObjWIFI_Send_String(char *s,unsigned char ucLength){	 //串口1
	
	uart1_datsSend(s, ucLength);
}

void rxBuff_WIFI_Clr(void){

	memset(rxBuff_WIFI, 0xff, sizeof(char) * COM_RX1_Lenth);
	COM1.RX_Cnt = 0;
}

/********************* 自定义校验*****************************/
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

/*此数据封装必须在数据包发送前最后调用，自动根据上次数据传输时的对象进行数据封装*///避免校验被提前而出错
void dtasTX_loadBasic_AUTO(u8 dats_Tx[45],		//发送包数据缓存基本信息填装
						   u8 frame_Type,
						   u8 frame_CMD,
						   bit ifSpecial_CMD){	
						  
	switch(repeatTX_Len){
	
		/*服务器数据包填装*///45字节
		case dataTransLength_objSERVER:{
			
			u8 xdata datsTemp[32] = {0};
		
			dats_Tx[0] 	= FRAME_HEAD_SERVER;
			
			memcpy(&datsTemp[0], &dats_Tx[1], 32);
			memcpy(&dats_Tx[13], &datsTemp[0], 32);	//帧头后拉开空出12个字节
			memcpy(&dats_Tx[1],  &Dst_MACID_Temp[0], 6);	//远端MACID填充
			memcpy(&dats_Tx[8],  &MAC_ID[1], 5);	/*dats_Tx[7] 暂时填0*///远端MACID填充
			
			dats_Tx[1 + 12] = dataTransLength_objSERVER;
			dats_Tx[2 + 12] = frame_Type;
			dats_Tx[3 + 12] = frame_CMD;
			
			if(!ifSpecial_CMD)dats_Tx[10 + 12] = SWITCH_TYPE_FP;	//开关类型填充
			
			memcpy(&dats_Tx[5 + 12], &MAC_ID[1], 5);	//MAC填充
								  
			dats_Tx[4 + 12] = frame_Check(&dats_Tx[5 + 12], 28);	
			
		}break;
		
		/*局域网数据包填装*///33字节
		case dataTransLength_objMOBILE:{
		
			dats_Tx[0] 	= FRAME_HEAD_MOBILE;
			
			dats_Tx[1] 	= dataTransLength_objMOBILE;
			dats_Tx[2] 	= frame_Type;
			dats_Tx[3] 	= frame_CMD;
			
			if(!ifSpecial_CMD)dats_Tx[10] = SWITCH_TYPE_FP;	//开关类型填充
			
			memcpy(&dats_Tx[5], &MAC_ID[1], 5);	//MAC填充
								  
			dats_Tx[4] 	= frame_Check(&dats_Tx[5], 28);
		}break;
		
		default:break;
	}	
}
						   
/*此数据封装必须在数据包发送前最后调用，自定义对象进行数据封装*///避免校验被提前而出错
void dtasTX_loadBasic_CUST(dataTrans_obj obj_custom,	//发送包数据缓存基本信息填装
						   u8 dats_Tx[45],		
						   u8 frame_Type,
						   u8 frame_CMD,
						   bit ifSpecial_CMD){	
						  
	switch(obj_custom){
	
		/*服务器数据包填装*///45字节
		case DATATRANS_objFLAG_SERVER:{
			
			u8 xdata datsTemp[32] = {0};
		
			dats_Tx[0] 	= FRAME_HEAD_SERVER;
			
			memcpy(&datsTemp[0], &dats_Tx[1], 32);
			memcpy(&dats_Tx[13], &datsTemp[0], 32);	//帧头后拉开空出12个字节
			memcpy(&dats_Tx[1],  &Dst_MACID_Temp[0], 6);	//远端MACID填充
			memcpy(&dats_Tx[8],  &MAC_ID[1], 5);	/*dats_Tx[7] 暂时填0*///远端MACID填充
			
			dats_Tx[1 + 12] = dataTransLength_objSERVER;
			dats_Tx[2 + 12] = frame_Type;
			dats_Tx[3 + 12] = frame_CMD;
			
			if(!ifSpecial_CMD)dats_Tx[10 + 12] = SWITCH_TYPE_FP;	//开关类型填充
			
			memcpy(&dats_Tx[5 + 12], &MAC_ID[1], 5);	//MAC填充
								  
			dats_Tx[4 + 12] = frame_Check(&dats_Tx[5 + 12], 28);	
			
		}break;
		
		/*局域网数据包填装*///33字节	--除了45都是33填装
		case DATATRANS_objFLAG_MOBILE:
		
		default:{
		
			dats_Tx[0] 	= FRAME_HEAD_MOBILE;
			
			dats_Tx[1] 	= dataTransLength_objMOBILE;
			dats_Tx[2] 	= frame_Type;
			dats_Tx[3] 	= frame_CMD;
			
			if(!ifSpecial_CMD)dats_Tx[10] = SWITCH_TYPE_FP;	//开关类型填充
			
			memcpy(&dats_Tx[5], &MAC_ID[1], 5);	//MAC填充
								  
			dats_Tx[4] 	= frame_Check(&dats_Tx[5], 28);
		}break;
	}	
}

/********************* UART1(WIIF)中断函数_自定义重构************************/
void UART1_Rountine (void) interrupt UART1_VECTOR
{
	
	EA = 0;
	
	if(RI)
	{
		RI = 0;
		if(COM1.B_RX_OK == 0)
		{		
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
			if(++COM1.TX_read >= COM_TX1_Lenth)		COM1.TX_read = 0;
		}
		else	COM1.B_TX_busy = 0;
	}
	
	EA = 1;
}

/***********主动数据检测检测立马更新心跳包**************/
void thread_LocalStaus_Release(void){

	static bit				Local_ifTim_sw_running_FLAG			= 0;
	static bit 				Local_status_Relay 					= false;
	static bit				Local_deviceLock_flag  				= false;
	static u8				Local_ifDelay_sw_running_FLAG  		= 0;
	
	if((Local_ifTim_sw_running_FLAG 		!= ifTim_sw_running_FLAG) ||
	   (Local_status_Relay					!= status_Relay) ||
	   (Local_deviceLock_flag				!= deviceLock_flag) ||
	   (Local_ifDelay_sw_running_FLAG		!= ifDelay_sw_running_FLAG)
	){
		
		heartBeat_Cnt 	= heartBeat_Period - 100;	//立即更新偶数心跳——状态包，等100ms，防止模块粘包
		heartBeat_Type	= 0;	//偶数包
			
		Local_ifTim_sw_running_FLAG 		= ifTim_sw_running_FLAG;
		Local_status_Relay					= status_Relay;
		Local_deviceLock_flag				= deviceLock_flag;
		Local_ifDelay_sw_running_FLAG		= ifDelay_sw_running_FLAG;
	}
}

/********************* 数据传输及解析************************/
void thread_dataTrans(void){
	
	static u8 timeZone_Hour 	= 0;
	static u8 timeZone_Minute	= 0;
	
	unsigned char xdata repeatTX_buff[45]	= {0};
	unsigned char xdata rxBuff_WIFI_temp[45]= {0};
		
	unsigned char frameLength				= 0;

	bit datsQualified_FLG					= 0;	
	bit Parsing_EN 							= 0;
	
	u8 xdata heartBeat_Pack[14] 			= {0};	
	
	static u8 elec_Consum_moment			= 0;	//用电量对应时段标志，电量只是每小时的用电量
	
	/*********************************数据包采集**********************************/
	if(uartRX_toutFLG){ //超时断帧
	
		uartRX_toutFLG = 0;
		
		memset(rxBuff_WIFI_temp, 0, 45 * sizeof(u8));
		
		if((datsRcv_ZIGB.rcvDats[0] == FRAME_HEAD_SERVER) && 
		   (datsRcv_ZIGB.rcvDats[13] == dataTransLength_objSERVER) &&
		   (datsRcv_ZIGB.rcvDats[14] == FRAME_TYPE_MtoS_CMD) &&
		   (datsRcv_ZIGB.rcvDatsLen >= dataTransLength_objSERVER)){ /*上位机发了个字节*/
			
			memcpy(&Dst_MACID_Temp[0], &datsRcv_ZIGB.rcvDats[7], 6);  //远端MACID暂存	
			rxBuff_WIFI_temp[0] = datsRcv_ZIGB.rcvDats[0]; //帧头赋值
			memcpy(&rxBuff_WIFI_temp[1], &datsRcv_ZIGB.rcvDats[13], 32);	//数据加载，帧对齐，对齐成33字节
			
			//响应回复对象属性更新	
			dataTrans_objWIFI = DATATRANS_objFLAG_SERVER;
			repeatTX_Len	  = dataTransLength_objSERVER;
			
			frameLength = dataTransLength_objSERVER;
			datsQualified_FLG = 1;
				
		}else
		if((datsRcv_ZIGB.rcvDats[0] == FRAME_HEAD_MOBILE) && 
		   (datsRcv_ZIGB.rcvDats[1] == dataTransLength_objMOBILE) &&
		   (datsRcv_ZIGB.rcvDats[2] == FRAME_TYPE_MtoS_CMD) &&
		   (datsRcv_ZIGB.rcvDatsLen == dataTransLength_objMOBILE)){
		
			memcpy(rxBuff_WIFI_temp, datsRcv_ZIGB.rcvDats, dataTransLength_objMOBILE);	//数据加载
			
			//响应回复对象属性更新	
			dataTrans_objWIFI = DATATRANS_objFLAG_MOBILE;
			repeatTX_Len	  = dataTransLength_objMOBILE;
				
			frameLength = dataTransLength_objMOBILE;
			datsQualified_FLG = 1;
				
		}else
		if((datsRcv_ZIGB.rcvDats[0] == FRAME_HEAD_HEARTBEAT) && 
		   (datsRcv_ZIGB.rcvDats[1] == dataHeartBeatLength_objSERVER) &&
		   (datsRcv_ZIGB.rcvDatsLen == 14)){ /*服务器回的还是14字节*///但是帧长为20，我发的20字节，服务器依然回14字节
			
			memcpy(rxBuff_WIFI_temp, datsRcv_ZIGB.rcvDats, 14);	//数据加载
			
			//响应回复对象属性更新	--不进行repeatTX_Len更新！！！
			dataTrans_objWIFI = HEARTBEAT_objFLAG_SERVER;
			   
			frameLength = dataHeartBeatLength_objSERVER;
			datsQualified_FLG = 1;
			   
		}else{
		
			dataTrans_objWIFI = dataTrans_obj_NULL;
			datsQualified_FLG = 0;
		}
	}
	
	/*********************************数据包类型甄别**********************************/
	if(datsQualified_FLG){
		
		bit frameCodeCheck_PASS = 0; //校验码检查通过标志
		bit frameMacCheck_PASS  = 0; //mac地址待检查通过标志
		
		datsQualified_FLG = 0;
	
		switch(frameLength){
		
			case dataTransLength_objSERVER:
			case dataTransLength_objMOBILE:{
			
				if(rxBuff_WIFI_temp[4] == frame_Check(&rxBuff_WIFI_temp[5], 28))frameCodeCheck_PASS = 1; //校验码检查
				if(!memcmp(&rxBuff_WIFI_temp[5], &MAC_ID[1], 5))frameMacCheck_PASS  = 1; //mac地址检查
				
				if(rxBuff_WIFI_temp[3] == FRAME_MtoSCMD_cmdCfg_swTim){ //不用进行校验码检查的指令
				
					frameCodeCheck_PASS = 1;	
					
				}
				if(rxBuff_WIFI_temp[3] == FRAME_MtoSCMD_cmdConfigSearch){ //不用进行mac地址检查的指令
				
					frameMacCheck_PASS = 1;	
					
				}
				
				if(frameMacCheck_PASS & frameMacCheck_PASS){ //指令验证
					
					Parsing_EN = 1;	
					
				}else{
				
//					if(!frameMacCheck_PASS)uartObjWIFI_Send_String("fuck mac", 8);
//					if(!frameMacCheck_PASS)uartObjWIFI_Send_String("fuck code", 9);rxBuff_WIFI_temp
//					uartObjWIFI_Send_String(rxBuff_WIFI_temp, 33);
				}
				
			}break;
			
			case dataHeartBeatLength_objSERVER:{
			
				if(!memcmp(&rxBuff_WIFI_temp[3], &MAC_ID[1], 5)){	//MAC地址本地比对
					
					Parsing_EN = 1;	
				}
				
			}break;
			
			default:{
		
				Parsing_EN = 0;	
				
			}break;
		}
	}
	
	/*********************************数据包开始解析响应**********************************/	
	if(Parsing_EN){	//开始解析
		
		Parsing_EN = 0;
		
		switch(dataTrans_objWIFI){
		
			//服务器
			case DATATRANS_objFLAG_SERVER:
			//局域网
			case DATATRANS_objFLAG_MOBILE:{
				
					bit specialCmd_FLAG = 0;	/*特殊数据封装标志*///数据1是否占用（原为开关类型）
					bit respond_FLAG	= 1;	/*是否相应回复标志*///数据接收后是否主动回复响应
				
					u8 cmdParing_Temp = rxBuff_WIFI_temp[3];
				
					memset(repeatTX_buff, 0, sizeof(u8) * 45);

					switch(cmdParing_Temp){		//命令解析及识别

						/*控制指令*/
						case FRAME_MtoSCMD_cmdControl:{		
							
							switch(rxBuff_WIFI_temp[11]){
							
								case 0x00:{		//开关_开
								
									swStatus_fromUsr = swStatus_off;
									
								}break;
									
								case 0x01:{		//开关_关
								
									swStatus_fromUsr = swStatus_on;

								}break;
								
								default:break;
							}
							
							repeatTX_buff[11]	= rxBuff_WIFI_temp[11];		//开关状态更新填充
							
						}break;

						/*配置搜索指令*/
						case FRAME_MtoSCMD_cmdConfigSearch:{	
							
							u8 deviceLock_IF = 0;
											
							EEPROM_read_n(EEPROM_ADDR_deviceLockFLAG, &deviceLock_IF, 1);
							
							//上锁检测
							if(!deviceLock_IF){	
								
								u8 xdata serverIP_temp[4] = {0};
								memcpy(&serverIP_temp[0], &rxBuff_WIFI_temp[6], 4);
							
								//服务器地址同步
								if(WIFI_serverUDP_CHG(serverIP_temp)){
									

								}else{
								
								}

								//时间信息同步
								timeZone_Hour 	= rxBuff_WIFI_temp[12];		//时区更新
								timeZone_Minute	= rxBuff_WIFI_temp[13];
								coverEEPROM_write_n(EEPROM_ADDR_timeZone_H, &timeZone_Hour, 1);
								coverEEPROM_write_n(EEPROM_ADDR_timeZone_M, &timeZone_Minute, 1);
								
							}else{
							
								respond_FLAG = 0;
							}
							
						}break;
						
						/*查询登录指令*/
						case FRAME_MtoSCMD_cmdQuery:{	
							
							u8 deviceLock_IF = 0;
							
							EEPROM_read_n(EEPROM_ADDR_deviceLockFLAG, &deviceLock_IF, 1);

							if(deviceLock_IF){
								
								
							}else{
							

							}
							
						}break;
						
						case FRAME_MtoSCMD_cmdCfg_swTim:{	/*普通开关定时配置指令*/
							
							u8 loop = 0;
							
							//校时
							stt_Time datsTime_temp = {0};
							
							datsTime_temp.time_Year		= rxBuff_WIFI_temp[26];
							datsTime_temp.time_Month 	= rxBuff_WIFI_temp[27];
							datsTime_temp.time_Day 		= rxBuff_WIFI_temp[28];
							datsTime_temp.time_Hour 	= rxBuff_WIFI_temp[29];
							elec_Consum_moment			= datsTime_temp.time_Hour;
							datsTime_temp.time_Minute 	= rxBuff_WIFI_temp[30];
							datsTime_temp.time_Week 	= rxBuff_WIFI_temp[31];
							
							timeSet(datsTime_temp);
							
							//定时数据处理及更新,分类处理
							switch(rxBuff_WIFI_temp[13]){
							
								case cmdConfigTim_normalSwConfig:{	/*普通定时*/
									
									for(loop = 0; loop < 4; loop ++){
									
										if(rxBuff_WIFI_temp[14 + loop * 3] == 0x80){	/*一次性定时判断*///周占位为空，而定时器被打开，说明是一次性
										
											swTim_onShoot_FLAG 				|= (1 << loop);	//一次性定时标志开启
											rxBuff_WIFI_temp[14 + loop * 3] |= (1 << (rxBuff_WIFI_temp[31] - 1)); //强行进行当前周占位，当次执行完毕后清除
										}
									}
									coverEEPROM_write_n(EEPROM_ADDR_swTimeTab, &rxBuff_WIFI_temp[14], 12);	//定时表

								}break;
								
								case cmdConfigTim_onoffDelaySwConfig:{	/*开关延时*/
								
									if(rxBuff_WIFI_temp[14]){
									
										ifDelay_sw_running_FLAG |= (1 << 1);
										delayPeriod_onoff 		= rxBuff_WIFI_temp[14];
										
//										if(rxBuff_WIFI_temp[15] == 2)delayUp_act = 1; //2为延时开，1为延时关
//										else delayUp_act = 0;
										
										delayUp_act		  		= rxBuff_WIFI_temp[15];
										
										delayCnt_onoff			= 0;
										
									}else{
									
										ifDelay_sw_running_FLAG &= ~(1 << 1); //无需掉电保存
										delayPeriod_onoff 		= 0;
										delayCnt_onoff			= 0;
									}
									
								}break;
								
								case cmdConfigTim_closeLoopSwConfig:{	/*自动循环关闭*/
								
									if(rxBuff_WIFI_temp[14]){
									
										ifDelay_sw_running_FLAG |= (1 << 0);
										delayPeriod_closeLoop	= rxBuff_WIFI_temp[14];
										delayCnt_closeLoop		= 0;
									}else{
									
										ifDelay_sw_running_FLAG &= ~(1 << 0); //需要掉电保存
										delayPeriod_closeLoop	= 0;
										delayCnt_closeLoop		= 0;
									}
									
									coverEEPROM_write_n(EEPROM_ADDR_swDelayFLAG, &ifDelay_sw_running_FLAG, 1);
									coverEEPROM_write_n(EEPROM_ADDR_periodCloseLoop, &delayPeriod_closeLoop, 1);
									
								}break;								
								
								default:break;
							}
							
							//数据响应及回复
							repeatTX_buff[12]	= rxBuff_WIFI_temp[12];
							repeatTX_buff[13]	= rxBuff_WIFI_temp[13];

						}break;
						
						/*配置互控指令*/
						case FRAME_MtoSCMD_cmdInterface:{	
						
							
							
						}break;
						
						/*开关复位指令*/
						case FRAME_MtoSCMD_cmdReset:{		
						
							
							
						}break;
						
						/*设备锁定指令——设备信息响应失能*/
						case FRAME_MtoSCMD_cmdLockON:{		
							
							//数据处理及动作响应
							{
								u8 deviceLock_IF = 1;
								
								deviceLock_flag  = 1;
								coverEEPROM_write_n(EEPROM_ADDR_deviceLockFLAG, &deviceLock_IF, 1);
							}							
						}break;
						
						/*设备解锁指令——设备信息响应使能*/
						case FRAME_MtoSCMD_cmdLockOFF:{		
							
							//数据处理及动作响应
							{
								u8 deviceLock_IF = 0;
								
								deviceLock_flag  = 0;
								coverEEPROM_write_n(EEPROM_ADDR_deviceLockFLAG, &deviceLock_IF, 1);
							}						
						}break;
						
						/*普通开关定时配置查询指令*/
						case FRAME_MtoSCMD_cmdswTimQuery:{	
							
							repeatTX_buff[12]	= rxBuff_WIFI_temp[12];
							repeatTX_buff[13]	= rxBuff_WIFI_temp[13];
							
							//分类回复
							switch(rxBuff_WIFI_temp[13]){
							
								case 0: /*上位机在定时的时候给0，待协商*/
								case cmdConfigTim_normalSwConfig:{
								
									u8 loop = 0;
								
									//数据响应及回复
									EEPROM_read_n(EEPROM_ADDR_swTimeTab, &repeatTX_buff[14], 12);	//定时表回复填装
									
									//回复数据二次处理（针对一次性定时数据）
									for(loop = 0; loop < 4; loop ++){
									
										if(swTim_onShoot_FLAG & (1 << loop)){
											
											repeatTX_buff[14 + loop * 3] &= 0x80;
										}
									}
									
									specialCmd_FLAG = 1;
									
								}break;
								
								case cmdConfigTim_onoffDelaySwConfig:{
								
									if(!delayCnt_onoff)repeatTX_buff[14] = 0;
									else repeatTX_buff[14] = delayPeriod_onoff - (u8)(delayCnt_onoff / 60);
									repeatTX_buff[15] = delayUp_act;
//									if(delayUp_act)repeatTX_buff[15] = 2;
//									else repeatTX_buff[15] = 1;
									
								}break;
								
								case cmdConfigTim_closeLoopSwConfig:{
								
									repeatTX_buff[14] = delayPeriod_closeLoop;
									
								}break;
								
								default:break;
							}
						}break;
						
						/*AP模式配置指令*/
						case FRAME_MtoSCMD_cmdConfigAP:{	
						
							//此指令不常用， 保留
						}break;
						
						/*提示音使能指令*/
						case FRAME_MtoSCMD_cmdBeepsON:{		
						
							
						}break;
						
						/*提示音失能指令*/
						case FRAME_MtoSCMD_cmdBeepsOFF:{	
						
							
						}break;
						
						/*恢复出厂本地是否支持查询*/
						case FRAME_MtoSCMD_cmdftRecoverRQ:{	
						

						}
						
						/*恢复出厂设置指令*/
						case FRAME_MtoSCMD_cmdRecoverFactory:{	
						

						}break;
						
						default:break;
					}
					
					if(respond_FLAG){	//主动响应回复执行
					
						dtasTX_loadBasic_AUTO( repeatTX_buff,				/*发送前最后装载*///发送包基本信息填充
											   FRAME_TYPE_StoM_RCVsuccess,
											   cmdParing_Temp,
											   specialCmd_FLAG
											 );
						uartObjWIFI_Send_String(repeatTX_buff, repeatTX_Len);//数据接收回复响应	
					}
					
//					if((HEARTBEAT_objFLAG_SERVER != dataTrans_objWIFI) &&
//					   ((cmdParing_Temp != FRAME_MtoSCMD_cmdConfigSearch) && (DATATRANS_objFLAG_MOBILE == dataTrans_objWIFI)) \
//					)beeps(2);	//提示音,心跳包不响,搜索配置时不响
					
			}break;
			
			//心跳包数据解析
			case HEARTBEAT_objFLAG_SERVER:{
			
				switch(rxBuff_WIFI_temp[2]){
				
					case FRAME_HEARTBEAT_cmdEven:{
					
						
					}break;
						
					case FRAME_HEARTBEAT_cmdOdd:{		//奇数心跳时间校准
					
						stt_Time datsTime_temp = {0};
						
//						u8 Y	= rxBuff_WIFI_temp[8];
//						u8 W 	= rxBuff_WIFI_temp[8];
						u8 M 	= rxBuff_WIFI_temp[9];
						u8 D 	= rxBuff_WIFI_temp[10];
//						u8 W 	= Y + (Y / 4) + 5 - 40 + (26 * (M + 1) / 10) + D - 1;	//蔡勒公式
//						   W   %= 7; 
//						
//						W?(datsTime_temp.time_Week = W):(datsTime_temp.time_Week = 7);
						
						datsTime_temp.time_Year		= 18;
						datsTime_temp.time_Month 	= M;
						datsTime_temp.time_Day 		= D;
						datsTime_temp.time_Week		= rxBuff_WIFI_temp[8];
						datsTime_temp.time_Hour 	= rxBuff_WIFI_temp[11];
						datsTime_temp.time_Minute 	= rxBuff_WIFI_temp[12];
						datsTime_temp.time_Second 	= rxBuff_WIFI_temp[13];						
						
						timeSet(datsTime_temp);
						
					}break;
					
					default:break;
				}
				
			}break;
				
			default:break;
		}
	}
		
	/*************************************心跳包业务代码**********************************************/
	if(heartBeat_Cnt < heartBeat_Period);
	else{
	
	  	heartBeat_Cnt = 0;
		
		memset(&heartBeat_Pack[0], 0, sizeof(u8) * dataHeartBeatLength_objSERVER);
		
		heartBeat_Pack[0]	= FRAME_HEAD_HEARTBEAT;
		heartBeat_Pack[1]	= dataHeartBeatLength_objSERVER;

		if(heartBeat_Type){	//奇数心跳
		
			heartBeat_Type = !heartBeat_Type;
			
			heartBeat_Pack[2]	= FRAME_HEARTBEAT_cmdOdd;
			memcpy(&heartBeat_Pack[3], &MAC_ID[1], 5);
			
			EEPROM_read_n(EEPROM_ADDR_timeZone_H, &timeZone_Hour, 1);
			EEPROM_read_n(EEPROM_ADDR_timeZone_M, &timeZone_Minute, 1);
			heartBeat_Pack[8] = timeZone_Hour;
			heartBeat_Pack[9] = timeZone_Minute;
			
//			//测试代码段
//			{
//				static u8 test_timeZone_Hour = 0;
//				
//				if(test_timeZone_Hour < 24)test_timeZone_Hour ++;
//				else{
//				
//					test_timeZone_Hour = 0;
//				}
//				
//				heartBeat_Pack[8] = test_timeZone_Hour;
//				heartBeat_Pack[9] = 0;
//			}
			
			{
			u16 integer_prt = (u16)elec_Consum & 0x00FF;	//电量整数部分
			u16 decimal_prt = (u16)((elec_Consum - (float)integer_prt) * 10000.0F);	//电量小数部分强转为整数类型
			
			heartBeat_Pack[10]	= (u8)((integer_prt & 0x00FF) >> 0);	//整数部分填装
			heartBeat_Pack[11]	= (u8)((decimal_prt & 0xFF00) >> 8);	//小数部分填装
			heartBeat_Pack[12]	= (u8)((decimal_prt & 0x00FF) >> 0);	//小数部分填装
			}
			
			{
			u16 integer_prt = (u16)paramElec_Param.ePower & 0xFFFF;	//功率整数部分
			u16 decimal_prt = (u16)((paramElec_Param.ePower - (float)integer_prt) * 10000.0F);	//功率小数部分强转为整数类型
			
			heartBeat_Pack[13]	= (u8)((integer_prt & 0xFF00) >> 8);	//整数部分填装
			heartBeat_Pack[14]	= (u8)((integer_prt & 0x00FF) >> 0);	//整数部分填装
			heartBeat_Pack[15]	= (u8)((decimal_prt & 0xFF00) >> 8);	//小数部分填装
			heartBeat_Pack[16]	= (u8)((decimal_prt & 0x00FF) >> 0);	//小数部分填装
			}
			
			heartBeat_Pack[17]	= elec_Consum_moment;	//对应时段标志填装
			
		}else{	//偶数心跳
		
			heartBeat_Type = !heartBeat_Type;
			
			heartBeat_Pack[2]	= FRAME_HEARTBEAT_cmdEven;
			memcpy(&heartBeat_Pack[3], &MAC_ID[1], 5);
			
			heartBeat_Pack[8]	= deviceLock_flag;
			heartBeat_Pack[9]	= ifTim_sw_running_FLAG;
			heartBeat_Pack[10]	= status_Relay;
			heartBeat_Pack[11]	= (ifDelay_sw_running_FLAG & 0x01) >> 0; //绿色模式
		}
		
		heartBeat_Pack[18]	= SWITCH_TYPE_FP;
		heartBeat_Pack[19]	= FRAME_TAIL_HEARTBEAT;

		uartObjWIFI_Send_String(heartBeat_Pack, dataHeartBeatLength_objSERVER);
	}
}
