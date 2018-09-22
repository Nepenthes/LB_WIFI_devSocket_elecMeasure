#include "Relay.h"

#include "eeprom.h"
#include "dataManage.h"
#include "Tips.h"

//*********************���ݴ�����ر���������*************************/
extern switch_Status	swStatus_fromUsr; //��λ��ֱ���´�Ŀ�������

//*********************��ʱ��������ر���������*********************/
extern switch_Status	swStatus_fromTim;  //��ʱʱ�̵���ʱ������Ӧ����״̬

//*********************��ʱ��ʱҵ����ر���������*****************/
extern u16				delayCnt_closeLoop;
extern bit 				greenModeStart_IF;

/**********************�����ļ�����������*****************************/
status_ifSave			relayStatus_ifSave		= statusSave_disable;	//���ؼ���ʹ�ܱ���
bit 					status_Relay 			= false;

/*���س�ʼ��*/
void relay_pinInit(void){
	
	u8 idata statusTemp = 0;

	//����
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

/*���ض���*/
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
	
	if(status_Relay){ //seg��ʾ��
	
		tipsSeg_INTFLG = 1;
		dev_segTips = segMode_touchOpen;
		
	}else{
	
		dev_segTips = segMode_touchClose;
		tipsSeg_INTFLG = 1;
	}
	
	dev_ledTips = ledMode_relayOpenIF; //led��ʾ��
	dev_beeps = beepsMode_Touch;
	
	pinRELAY = status_Relay;
	
	if(relayStatus_ifSave == statusSave_enable){	//����״̬����
	
		statusTemp = status_Relay;
		coverEEPROM_write_n(EEPROM_ADDR_relayStatus, &statusTemp, 1);
	}
}

/*�̵������߳�*/
void thread_Relay(void){
	
	switch(swStatus_fromUsr){	//��λ���´￪��ҵ���߼�
	
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
	
	switch(swStatus_fromTim){	//��ʱʱ�̱�������ҵ���߼�
	
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
	
	if(!greenModeStart_IF){ //��ɫģʽ��ʱ�Ƿ��Ѿ�����

		if(status_Relay == actRelay_ON){ //����һ�������̸�����ɫģʽʱ�����ֵ
		
			greenModeStart_IF = 1;
			delayCnt_closeLoop = 0;
		} 
	}
}