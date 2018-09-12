#include "wifi_LPT220.h"

#include "dataTrans.h"
#include "pars_Method.h"
#include "dataManage.h"
#include "eeprom.h"
#include "Tips.h"
#include "HW8012.h"

#include "USART.h"
#include "delay.h"
#include "soft_uart.h"

#include "string.h"
#include "stdio.h"
#include "ctype.h"
#include "stdlib.h"

//*******************串口变量引用区***************************/
extern COMx_Define 		COM1;

//*******************数据传输变量引用区***********************/
extern bit				deviceLock_flag;		//设备锁标志

/********************本地文件变量创建区******************/
u16 xdata funKey_WIFI_cnt 		= 0;	//wifi模块物理按键连接引脚模拟按键时长计数值

/*/------------------------------------------------------/*/
#define cmdAT_packLenth 7
datsAttr_wifiInit code wifiCMD_dats[cmdAT_packLenth] = {		//顺序禁止更改！！！
	
//	{{"\rAT+E\r\n"},								{"AT+E", "AT+E"},						{4,4},		150},
//	{{"\rAT+WMODE=AP\r\n"},							{"+ok\r\n",	"ok"},						{5,2},		150},
//	{{"\rAT+WMODE=STA\r\n"},						{"+ok\r\n",	"ok"},						{5,2},		150},
//	{{"\rAT+Z\r\n"},								{"+ok\r\n",	"ok"},						{5,2},		200},
//	
//	{{"\rAT+NETP=UDP,SERVER,8866,192.168.5.1\r\n"},	{"+ok\r\n",	"ok"},						{5,2},		200},
//	
//	{{"\rAT+NETP=UDP,SERVER,8866,255.255.255.255\r\n"},{"+ok\r\n", "ok"},					{5,2},		200},	//测试专用log_socketA
//	{{"\rAT+SOCKB=UDPS,8866,255.255.255.255\r\n"},	{"+ok\r\n", "ok"},						{5,2},		200},	//测试专用log_socketB

	{{"AT+E\r\n"},									{"AT+E", "AT+E"},						{4,4},		150},
	{{"AT+WMODE=AP\r\n"},							{"+ok\r\n",	"ok"},						{5,2},		200},
	{{"AT+WMODE=STA\r\n"},							{"+ok\r\n",	"ok"},						{5,2},		150},
	{{"AT+Z\r\n"},									{"+ok\r\n",	"ok"},						{5,2},		200},
	
	{{"AT+NETP=UDP,SERVER,8866,192.168.5.1\r\n"},	{"+ok\r\n",	"ok"},						{5,2},		200},
	
	{{"AT+NETP=UDP,SERVER,8866,255.255.255.255\r\n"},{"+ok\r\n", "ok"},						{5,2},		200},	//测试专用log_socketA
	{{"AT+SOCKB=UDPS,8866,255.255.255.255\r\n"},	{"+ok\r\n", "ok"},						{5,2},		200},	//测试专用log_socketB
};

/*********************WIFI模块控制引脚初始化******************/
void WIFI_pinInit(void){

	//RST_P26 推挽输出
	P2M1 &= 0xBF;
	P2M0 |= 0x40; 
	
	//Rload_P12 推挽输出
	P1M1 &= 0xFB;
	P1M0 |= 0x04; 
	
	//Rload_P13 高阻输入
	P1M1 |= 0x08;
	P1M0 &= 0xF7;
	
	//TX推挽输出
	P3M1 &= 0xFD;	
	P3M0 |= 0x02;	
	
	//RX高阻输入
	P3M1 |= 0x01;
	P3M0 &= 0xFE;
	
	WIFI_funcPIN_nRst	 = WIFI_pinDISEN;
	WIFI_funcPIN_nReload = WIFI_pinDISEN;
}

/*********************WIFI模块硬件控制smartlink******************/
void WIFI_hwSmartLink(void){
	
	//重新解开设备锁
	{
		u8 deviceLock_IF = 0;
		
		deviceLock_flag  = 0;
		coverEEPROM_write_n(EEPROM_ADDR_deviceLockFLAG, &deviceLock_IF, 1);
	}
	
	WIFI_funcPIN_nReload = WIFI_pinEN;
	funKey_WIFI_cnt = 1000;	//1000ms
	
	beeps(3);	//提示
	delayMs(1500);
}

/*********************WIFI模块硬件控制恢复出厂设置******************/
void WIFI_hwRestoreFactory(void){

	WIFI_funcPIN_nReload = WIFI_pinEN;
	funKey_WIFI_cnt = 5000; //5000ms
	
	beeps(4);		//提示，语句下置，与上面动作同步进行
	delayMs(6000);
	
	((void(code *)(void))0x0000)();
}

/**********************************退透传*******************************/
bit WIFI_ENTM_OUT(unsigned char repeatLoop){
	
	u8 loopOut = repeatLoop;
	
	while(SBUF != 'a' && (-- loopOut)){
	
		uartObjWIFI_Send_Byte('+');
		delayMs(200);
	}
	
	if(!loopOut)return 0;
	else loopOut = repeatLoop;
	
	while(SBUF != 'A' && (-- loopOut)){
	
		uartObjWIFI_Send_Byte('a');
		delayMs(100);
	}
	
	if(!loopOut)return 0;
	else return 1;
	
	return 0;
}

/*********************单条AT指令下达及验证响应******************/
bit cmdAT_validation(char *cmd, char *reply[2], unsigned char replyLen[2], unsigned int timeWait, unsigned char repeatLoop){

	unsigned char loop 	= 0,
				  loopa	= 0;
	
	u16 Local_Delay = 0;		
	
	uartRX_toutFLG = 0;
	for(loop = 0; loop < repeatLoop; loop ++){
		
		Local_Delay = timeWait;	
	
		uartObjWIFI_Send_String(cmd, strlen(cmd));
		
		while(Local_Delay --){
		
			delayMs(2);
			
			if(uartRX_toutFLG){
			
				uartRX_toutFLG = 0;
				for(loopa = 0; loopa < 2; loopa ++){
				
					if(memmem(datsRcv_ZIGB.rcvDats, datsRcv_ZIGB.rcvDatsLen, reply[loopa], replyLen[loopa])){
						
						return 1;
					}	
				}	
			}		
		}
	}	
	
	return 0;
}

/*********************单条AT指令下达及验证响应——形参结构体打包******************/
bit cmdPackAT_validation(datsAttr_wifiInit cmdPack, unsigned char repeatLoop){

	return cmdAT_validation(cmdPack.wifiInit_reqCMD,
							cmdPack.wifiInit_REPLY,
							cmdPack.REPLY_DATLENTAB,
							cmdPack.timeTab_waitAnsr,
							repeatLoop);
}

/*********************WIFI模块网络参数配置初始化******************/
bit WIFI_configInit(void){
	
	u8 xdata serverIP_temp[4] 	= {0};
	u8 xdata ATCMD_temp[40]		= {0};
	
	u16 data tOut_cnt	= 5000;
	
	WIFI_funcPIN_nRst	= WIFI_pinEN;	//硬件复位一次
	delayMs(300);
	WIFI_funcPIN_nRst	= WIFI_pinDISEN;
	delayMs(3000);
//	WIFI_funcPIN_nReload = WIFI_pinEN;
//	delayMs(1000);
//	WIFI_funcPIN_nReload = WIFI_pinDISEN;
	
	
	while(WIFI_tipsPIN_nReady && (tOut_cnt --))delayMs(1);
	if(!tOut_cnt)return 0;

	if(!WIFI_ENTM_OUT(15))return 0;								//退透传
	
	if(!cmdPackAT_validation(wifiCMD_dats[0], 5))return 0;		//消回传
	delayMs(10);
	
#if(TEST_LOG == 1)
	
	if(!cmdPackAT_validation(wifiCMD_dats[1], 5))return 0;
	
	sprintf(ATCMD_temp, "AT+WAP=11BGN,LANBON_swTestLog,AUTO\r\n");
	if(!cmdAT_validation(ATCMD_temp,						//AP_SSID配置
						 wifiCMD_dats[4].wifiInit_REPLY,
						 wifiCMD_dats[4].REPLY_DATLENTAB,
						 wifiCMD_dats[4].timeTab_waitAnsr,
						 5))return 0; 
	delayMs(10);
	
	memset(ATCMD_temp, 0, 40 * sizeof(u8));
//	sprintf(ATCMD_temp, "AT+WAKEY=WPA2PSK,AES,88888888\r\n");
	sprintf(ATCMD_temp, "AT+WAKEY=OPEN,NONE\r\n");	
	if(!cmdAT_validation(ATCMD_temp,						//AP_密码配置
						 wifiCMD_dats[4].wifiInit_REPLY,
						 wifiCMD_dats[4].REPLY_DATLENTAB,
						 wifiCMD_dats[4].timeTab_waitAnsr,
						 5))return 0; 
	delayMs(10);
	
	memset(ATCMD_temp, 0, 40 * sizeof(u8));
	if(!cmdPackAT_validation(wifiCMD_dats[5], 5))return 0;	//socketA_log配置
	delayMs(10);
	if(!cmdPackAT_validation(wifiCMD_dats[6], 5))return 0;	//socketB_log配置
	delayMs(10);
	
#else
		
	if(!cmdPackAT_validation(wifiCMD_dats[2], 5))return 0;
		
	if(!cmdPackAT_validation(wifiCMD_dats[4], 5))return 0;	//socket_A配置
	
	EEPROM_read_n(EEPROM_ADDR_serverIP_record, &serverIP_temp[0], 4);
	sprintf(ATCMD_temp, "AT+SOCKB=UDP,80,%d.%d.%d.%d\r\n", 	(int)serverIP_temp[0], 
															(int)serverIP_temp[1], 
															(int)serverIP_temp[2], 
															(int)serverIP_temp[3]);
	delayMs(10);
														   
	if(!cmdAT_validation(ATCMD_temp,						//socket_B配置
						 wifiCMD_dats[4].wifiInit_REPLY,
						 wifiCMD_dats[4].REPLY_DATLENTAB,
						 wifiCMD_dats[4].timeTab_waitAnsr,
						 5))return 0; 
	delayMs(10);
	
#endif

//	if(!cmdPackAT_validation(wifiCMD_dats[3], 5))return 0;		//wifi重启
	
	uartObjWIFI_Send_String(wifiCMD_dats[3].wifiInit_reqCMD, strlen(wifiCMD_dats[3].wifiInit_reqCMD));	//重启进透传
	delayMs(200);
	uartObjWIFI_Send_String(wifiCMD_dats[3].wifiInit_reqCMD, strlen(wifiCMD_dats[3].wifiInit_reqCMD));	
	
	while(WIFI_tipsPIN_nReady && (tOut_cnt --))delayMs(1);
	if(!tOut_cnt)return 0;
		
	return 1;
}

/*********************UDP_IP更变******************/
	bit WIFI_serverUDP_CHG(unsigned char ip[4]){

	unsigned char idata ipNem_temp[40] 		= {0},
				  idata ipRecord_temp[4]	= {0};
				  
	EEPROM_read_n(EEPROM_ADDR_serverIP_record, ipRecord_temp, 4);
	if(!memcmp(ipRecord_temp, ip, 4))return 1;
	else{
		
		coverEEPROM_write_n(EEPROM_ADDR_serverIP_record, ip, 4);
	
		sprintf(ipNem_temp, "AT+SOCKB=UDP,80,%d.%d.%d.%d\r\n", (int)ip[0], (int)ip[1], (int)ip[2], (int)ip[3]);
		
		if(!WIFI_ENTM_OUT(10))return 0;
		if(!cmdPackAT_validation(wifiCMD_dats[0], 5))return 0;
		
		if(!cmdAT_validation(ipNem_temp,						//socket_B配置
							 wifiCMD_dats[4].wifiInit_REPLY,
							 wifiCMD_dats[4].REPLY_DATLENTAB,
							 wifiCMD_dats[4].timeTab_waitAnsr,
							 5))return 0; 
		
//		if(!cmdPackAT_validation(wifiCMD_dats[3], 5))return 0;
		uartObjWIFI_Send_String(wifiCMD_dats[3].wifiInit_reqCMD, strlen(wifiCMD_dats[3].wifiInit_reqCMD));	//重启进透传
		delayMs(200);
		uartObjWIFI_Send_String(wifiCMD_dats[3].wifiInit_reqCMD, strlen(wifiCMD_dats[3].wifiInit_reqCMD));	
		
		return 1;
	}
}
