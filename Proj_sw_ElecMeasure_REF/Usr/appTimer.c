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

//*********************���ݴ������������***************************/
extern bit 				rxTout_count_EN;	
extern u8  				rxTout_count;	//���ڽ��ճ�ʱ����
extern bit 				uartRX_toutFLG;
extern u8 				datsRcv_length;
extern uartTout_datsRcv xdata datsRcv_ZIGB;

//*********************WIFIģ�鹦�����Ű����̱߳���������*********/
extern u16 xdata		funKey_WIFI_cnt;

//*********************���ݴ�����ر���������*********************/
extern u16 xdata		heartBeat_Cnt;

//*********************�̵�����ر���������*********************/
extern bit 				status_Relay;

//*********************��ʱ��ʱҵ����ر���������*****************/
extern switch_Status	swStatus_fromTim;
extern u8 				ifDelay_sw_running_FLAG;	//��ʱ����_�Ƿ����б�־λ��bit 1��ʱ��������ʹ�ܱ�־��bit 0��ʱ�ر�����ʹ�ܱ�־��
extern u16				delayCnt_onoff;
extern u8				delayPeriod_onoff;
extern bit				delayUp_act;
extern u16				delayCnt_closeLoop;
extern u8				delayPeriod_closeLoop;
extern bit 				greenModeStart_IF;

//***************�û������̱߳���������*****************/
extern bit		 		usrKeyCount_EN;
extern u16	idata 		usrKeyCount;

//*********************Tips����������****************************/
extern enum_segTips dev_segTips;
extern enum_ledTips dev_ledTips;

void appTimer0_Init(void){	//50us �ж�@24.000M

	AUXR |= 0x80;		
	TMOD &= 0xF0;		
	TL0   = 0x50;		
	TH0   = 0xFB;	
	TF0   = 0;	
	ET0	  = 1;	//���ж�
	PT0   = 0;	//�����ȼ��ж�
	
	TR0   = 1;		
}

void timer0_Rountine (void) interrupt TIMER0_VECTOR{	
	
	u32 code fpDetect_Period 		= 100000UL;	//��Ƶ�������5��  100000 * 50us = 5s
	static xdata u32 fpDetect_Cnt 	= 0UL;
	
	u16 code  fpDetectPeriod_stdBy	= 1000;		//���ؼ��tips_standBy
	static xdata u16 fpDetectCount_stdBy = 0;

	u8 code   period_1ms 			= 20;		//1msר��
	static u8 count_1ms 			= 0;
	
	u16 code period_1second 		= 20000;	//1��ר��
	static xdata u16 count_1second 	= 0;	
	
	u16 code tipsInit_Period 		= 1000;		//��ʱ����������
	static xdata u16 tipsInit_Cnt 	= 0;	
	
	u8 code   period_beep 			= 2;		//beepר��
	static u8 count_beep 			= 0;
	
	//*******************beep��ʱ����ҵ��**************************/
	if(count_beep < period_beep)count_beep ++;
	else{
	
		count_beep = 0;
		
		switch(dev_beeps){ //״̬��
			
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
	
	//*******************tips��ˮ�Ƽ�ʱ����ҵ��**************************/
	if(tipsInit_Cnt < tipsInit_Period)tipsInit_Cnt ++;
	else{
	
		tipsInit_Cnt  = 0;
		
		switch(dev_segTips){ //״̬��
		
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
			
				powerTips(paramElec_Param.ePower);	//����Tips��ʾ
				
			}break;
				
			default:{
			
				segTips_allDark();
				
			}break;
		}
	}
	
	//*******************tipsָʾ��ҵ��**************************/
	switch(dev_ledTips){ //״̬��
	
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
	
	//*******************��׼1�����ҵ��***************************/
	if(count_1second < period_1second)count_1second ++;
	else{
		
		count_1second = 0;
	
		/*��ʱ��ʱҵ�񣬵��㶯��*/
		if(ifDelay_sw_running_FLAG & (1 << 1)){
		
			if(delayCnt_onoff < ((u16)delayPeriod_onoff * 60))delayCnt_onoff ++;
			else
			if(delayCnt_onoff){ //��ʱֵȷ��
			
				delayCnt_onoff = 0;
				
				ifDelay_sw_running_FLAG &= ~(1 << 1);	//һ���Ա�־���
				
				(delayUp_act)?(swStatus_fromTim = swStatus_on):(swStatus_fromTim = swStatus_off);	//���ض���
			}
		}
		
		/*��ɫģʽ��ʱҵ��ѭ���ر�*/
		if((ifDelay_sw_running_FLAG & (1 << 0)) && status_Relay){
		
			if(delayCnt_closeLoop < ((u16)delayPeriod_closeLoop * 60))delayCnt_closeLoop ++;
			else{
				
				delayCnt_closeLoop = 0;
				greenModeStart_IF = 0;
			
				swStatus_fromTim = swStatus_off;
			}
		}
	}
	
	//*******************��׼1�������ҵ��**************************/
	if(count_1ms < period_1ms)count_1ms ++;
	else{
	
		count_1ms = 0;
		
		/*���������ڼ���*/
		heartBeat_Cnt++;
		
		/*�û�������ʱ����*/
		if(usrKeyCount_EN)usrKeyCount ++;
		
		/*WIFIģ�鹦�����Ű�������ҵ����*/
		if(funKey_WIFI_cnt)funKey_WIFI_cnt --;
		else{
		
			WIFI_funcPIN_nReload = WIFI_pinDISEN;	//WIFI�ⲿ���ſ���ҵ��
		}
		
		/*HLW8012��ƵTips-standBy*///Ԥ���
		if(fpDetectCount_stdBy < fpDetectPeriod_stdBy)fpDetectCount_stdBy ++;
		else{
			
			fpDetectCount_stdBy = 0;
			
			pinFP_powerStdby = pinFP_stdby_powerCNT / 1.0F;
			pinFP_stdby_powerCNT = 0;
			if(status_Relay == actRelay_ON){
				
				if((pinFP_powerStdby - pinFP_power) > 10.0F){ //����
				
					dev_segTips = segMode_elecDetectStandby;
					tipsSeg_INTFLG = 1;
				}
				else
				if((pinFP_power - pinFP_powerStdby) > 10.0F){ //���
				
					dev_segTips = segMode_null;
					tipsSeg_INTFLG = 1;
				}else
				if((pinFP_powerStdby - pinFP_power) < 3.0F && (pinFP_powerStdby - pinFP_power) > -3.0F){ //�޲�
				
					dev_segTips = segMode_elecDetect;
					tipsSeg_INTFLG = 1;
				}
			}
		}
	}
	
	//***************���ڽ��ճ�ʱʱ������*******************************//
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
	
	//*******************HLW8012��Ƶҵ��****************************	
	if(fpDetect_Cnt <= fpDetect_Period)fpDetect_Cnt ++;
	else{
	
		fpDetect_Cnt 	 = 0;
		
		pinFP_volXcur	 = pinFP_volXcurCNT / 5.0F;	//ֵȷ��
		pinFP_power		 = pinFP_powerCNT / 5.0F;
		
//		if(pinFP_power > 5.0){
//		
//			if(status_Relay == actRelay_ON){
//			
//				dev_segTips = segMode_elecDetect;
//				tipsSeg_INTFLG = 1;
//			}
//		}
		
		if(pin_funFP_Select){	//��ѹ
		
			paramElec_Param.eVoltage = pinFP_volXcur * (coefficient_Vol - (0.00000082 * pinFP_volXcur));
			
		}else{	//����
		
//			paramElec_Param.eCurrent = (float)pinFP_volXcur * coefficient_Cur;
			
			paramElec_Param.eCurrent = pinFP_volXcur * (coefficient_Cur + (0.00000246 * pinFP_volXcur));
		}
		
//		/*��������*///���Դ����
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
		
		//���⸡����ȫ��Ϊ�����ʱ���ַǷ�ֵ
//		if(!pinFP_powerCNT) pinFP_powerCNT = 1;
//		if(!pinFP_power) pinFP_power = 1; 
//		if(paramElec_Param.ePower < 0.1) paramElec_Param.ePower = 0.1;

		/*ʵ��ҵ���߼�*/
		paramElec_Param.ePower = (float)pinFP_power * (coefficient_Pow - (0.00000001 * pinFP_power));	//����
		
		{
			float powerFP_Temp = pinFP_power;
			
			if(powerFP_Temp < 0.1F)powerFP_Temp = 0.1F;
			elec_Consum	+= 1.00F * ((float)pinFP_powerCNT * paramElec_Param.ePower / (1000.00F * 3600.00F * (float)powerFP_Temp)); //�õ���
		}
		
		pinFP_volXcurCNT = 0.0;	//��������
		pinFP_powerCNT	 = 0.0;
	}
}