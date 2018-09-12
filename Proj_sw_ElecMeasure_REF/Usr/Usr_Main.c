#include "STC15Fxxxx.H"

#include "HW8012.h"
#include "appTimer.h"
#include "wifi_LPT220.h"
#include "dataTrans.h"
#include "Relay.h"
#include "HYM8564.h"
#include "dataManage.h"
#include "timerAct.h"
#include "usrKin.h"

#include "soft_uart.h"
#include "delay.h" 

#include "Tips.h"

void usr_Test(void){

//	P33 =! P33;
//	delayMs(500);
	
//	if(!P24)P25 = 1;else P25 = 0;
//	HYM8564_Test();
	
	beeps(7);
//	P32 = !P32;
//	delayMs(1);
}

void bsp_Init(void){
	
	MAC_ID_Relaes		();
	SYS_Data_Relaes		();

	appTimer0_Init		();
	intMeasure_Init		();
	
	iicHYM8564_pinInit	();
	pinLed_Init			();
	pinBeep_Init		();
	WIFI_pinInit		();	
	usrKin_pinInit		();
	
	uartObjWIFI_Init	();
	
	relay_pinInit		();
	
	AUXR 	&= 0xfd;
	EA 		=  1;
	
//	while(1)usr_Test();
	
	dev_ledTips = ledMode_init;
	dev_segTips = segMode_init;
	
	while(!WIFI_configInit ());
}

int main(void){
	
//	WIFI_pinInit		();	
//	uartObjWIFI_Init	();
//	
//	while(1){
//	
//		WIFI_funcPIN_nReload = WIFI_pinDISEN;
//		WIFI_funcPIN_nRst	= WIFI_pinEN;	//硬件复位一次
//		delayMs(100);
//		WIFI_funcPIN_nRst	= WIFI_pinDISEN;
//		delayMs(3000);

////		while(WIFI_tipsPIN_nReady)delayMs(100);
//		
//		WIFI_funcPIN_nReload = WIFI_pinEN;
//		delayMs(1000);
//		WIFI_funcPIN_nReload = WIFI_pinDISEN;
////		delayMs(10000);
//		
//		uartObjWIFI_Send_Byte('+');
//		delayMs(200);
//		uartObjWIFI_Send_Byte('+');
//		delayMs(200);
//		uartObjWIFI_Send_Byte('+');
//		delayMs(200);
//		uartObjWIFI_Send_Byte('+');
//		delayMs(200);
//		uartObjWIFI_Send_Byte('+');
//		delayMs(200);
//		uartObjWIFI_Send_Byte('+');
//		delayMs(200);
//		uartObjWIFI_Send_Byte('+');
//		delayMs(200);
//		uartObjWIFI_Send_Byte('+');
//		delayMs(200);
//	}

	birthDay_Judge();
	bsp_Init();
	dev_segTips = segMode_initCmp;
	dev_ledTips = ledMode_relayOpenIF;
	beeps(10);
	
	while(1){
		
		thread_Timing();
		thread_Relay();
		
#if(TEST_LOG == 1)
		elecParamLog_thread();
#else
		thread_dataTrans();
		thread_LocalStaus_Release();
#endif
		UsrKEYScan(fun_kShort, fun_kLongA, fun_kLongB);
	}
}