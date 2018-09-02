#include "usrKin.h"
#include "Tips.h"

#include "wifi_LPT220.h"
#include "relay.h"

#include "soft_uart.h"
#include "delay.h"

#include "stdio.h"
#include "string.h"

//*********************�̵���������ر�������******************/
extern status_ifSave	relayStatus_ifSave;
extern bit 				status_Relay;

/**********************�����ļ�����������**********************/
bit			usrKeyCount_EN	= 0;
u16	idata 	usrKeyCount		= 0;

void usrKin_pinInit(void){
	
	P2M1 &= ~(0x10);
	P2M0 &= ~(0x10);

//	P2M1 |= 0x10;
//	P2M0 &= ~(0x10);
	
//	P2M1 |= 0x10;
//	P2M0 |= 0x10;
}

bit UsrKEYScan_oneShoot(void){

	return Usr_key;
}

void UsrKEYScan(funKey_Callback funCB_Short, funKey_Callback funCB_LongA, funKey_Callback funCB_LongB){
	
	code	u16	keyCfrmLoop_Short 	= 20,	//�̰�����ʱ��
				keyCfrmLoop_LongA 	= 8000,	//����Aʱ��
				keyCfrmLoop_LongB 	= 20000,//����Bʱ��
				keyCfrmLoop_MAX	 	= 60000;//��ʱ�ⶥ
	
	static	bit LongA_FLG = 0;
	static	bit LongB_FLG = 0;
	
	static	bit keyPress_FLG = 0;

	if(!UsrKEYScan_oneShoot()){	//ѡ����ʾ
		
		keyPress_FLG = 1;
	
		if(!usrKeyCount_EN) usrKeyCount_EN= 1;	//��ʱ
		
		if((usrKeyCount >= keyCfrmLoop_LongA) && (usrKeyCount <= keyCfrmLoop_LongB) && !LongA_FLG){
			
			funCB_LongA();
			
			LongA_FLG = 1;
		}	
		
		if((usrKeyCount >= keyCfrmLoop_LongB) && (usrKeyCount <= keyCfrmLoop_MAX) && !LongB_FLG){
			
			funCB_LongB();
			
			LongB_FLG = 1;
		}
		
	}else{	//ѡ����Ӧ
		
		if(LongA_FLG || LongB_FLG)dev_ledTips = ledMode_relayOpenIF; //�����ͷź�led_tips�ָ�
		
		usrKeyCount_EN = 0;
		
		if(keyPress_FLG){
		
			keyPress_FLG = 0;
			
			if(usrKeyCount < keyCfrmLoop_LongA && usrKeyCount > keyCfrmLoop_Short)funCB_Short();
			
			usrKeyCount = 0;
			LongA_FLG 	= 0;
			LongB_FLG 	= 0;
		}
	}
}

void fun_kLongA(void){

	dev_ledTips = ledMode_smartConfig;
	WIFI_hwSmartLink();
}

void fun_kLongB(void){

	dev_ledTips = ledMode_factory;
	WIFI_hwRestoreFactory();
}

void fun_kShort(void){

	relay_Act(relay_flip);
}


