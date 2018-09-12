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

//*********************MAC��ַ��ر���������*********************/
extern u8 xdata 		MAC_ID[6];

//*********************��ʱ�����߳���ر���������****************/
extern	u8 				swTim_onShoot_FLAG;		//��ͨ���ض�ʱһ���Ա�־��������λ��ʶ�ĸ���ʱ��
extern	bit				ifTim_sw_running_FLAG;	//��ͨ���ض�ʱ���б�־λ

extern u8 				ifDelay_sw_running_FLAG;	//��ʱ����_�Ƿ����б�־λ��bit 1��ʱ��������ʹ�ܱ�־��bit 0��ʱ�ر�����ʹ�ܱ�־��
extern u16				delayCnt_onoff;
extern u8				delayPeriod_onoff;
extern bit				delayUp_act;
extern u16				delayCnt_closeLoop;
extern u8				delayPeriod_closeLoop;

//*********************��������߳���ر���������****************/
extern	elec_Param 	xdata paramElec_Param;	//���������
extern	float		xdata elec_Consum;

//*********************��������߳���ر���������****************/
extern	bit 			status_Relay;

/**********************�����ļ�����������************************/
//���ڽ��ճ�ʱ��־
bit uartRX_toutFLG 						= 0;
//���ڽ��ճ�ʱ����
bit rxTout_count_EN 					= 0;
u8  rxTout_count 						= 0;
//�������ݻ���
u8	datsRcv_length						= 0;
uartTout_datsRcv xdata datsRcv_ZIGB 	= {{0}, 0};

//��ǰ������λ��ģʽ��ʶ
dataTrans_obj 	dataTrans_objWIFI 		= DATATRANS_objFLAG_MOBILE;

//��λ���´￪�������־
switch_Status	swStatus_fromUsr  		= swStatus_null;

//���ݰ���Ӧ�ظ����ȡ���Ĭ�Ͼ��������ݳ���
u8 xdata 		repeatTX_Len			= dataTransLength_objMOBILE;

//Զ��MACID����
u8	xdata		Dst_MACID_Temp[6] 		= {0};

//��������ż��־
bit 			heartBeat_Type 			= 0;	//��ż����־��������1��ż���0
//���������ڼ���ֵ
u16	xdata		heartBeat_Cnt			= 0;
//��������������
const 	u32 	heartBeat_Period 		= 8000; 	//�������������ú���ϵ��ֵ * 50us����ʱ���ڼ�����

/*--------------------------------------------------------------*/
void uartObjWIFI_Init(void){
	
	EA = 0;

	PS = 1;
	SCON = (SCON & 0x3f) | UART_8bit_BRTx;

{
	u32 j = (MAIN_Fosc / 4) / BAUD_WIFI;	//��1T����
		j = 65536UL - j;
	
	TH2 = (u8)(j>>8);
	TL2 = (u8)j;
}
	AUXR &= ~(1<<4);	//Timer stop
	AUXR |= 0x01;		//S1 BRT Use Timer2;
	AUXR &= ~(1<<3);	//Timer2 set As Timer
	AUXR |=  (1<<2);	//Timer2 set as 1T mode

	IE2  &= ~(1<<2);	//��ֹ�ж�
	AUXR &= ~(1<<3);	//��ʱ
	AUXR |=  (1<<4);	//Timer run enable

	ES 	  = 1;
	REN   = 1;
	P_SW1 = (P_SW1 & 0x3f) | (UART1_SW_P30_P31 & 0xc0);
	
	memset(TX1_Buffer, 0, sizeof(char) * COM_TX1_Lenth);

	EA = 1;
	
	PrintString1("i'm UART1 for wifi data translate !!!");
}

/*----------------------------
���ʹ�������
----------------------------*/
void uartObjWIFI_Send_Byte(u8 dat)	//����1
{
	TX1_write2buff(dat);
}

void uartObjWIFI_Send_String(char *s,unsigned char ucLength){	 //����1
	
	uart1_datsSend(s, ucLength);
}

void rxBuff_WIFI_Clr(void){

	memset(rxBuff_WIFI, 0xff, sizeof(char) * COM_RX1_Lenth);
	COM1.RX_Cnt = 0;
}

/********************* �Զ���У��*****************************/
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

/*�����ݷ�װ���������ݰ�����ǰ�����ã��Զ������ϴ����ݴ���ʱ�Ķ���������ݷ�װ*///����У�鱻��ǰ������
void dtasTX_loadBasic_AUTO(u8 dats_Tx[45],		//���Ͱ����ݻ��������Ϣ��װ
						   u8 frame_Type,
						   u8 frame_CMD,
						   bit ifSpecial_CMD){	
						  
	switch(repeatTX_Len){
	
		/*���������ݰ���װ*///45�ֽ�
		case dataTransLength_objSERVER:{
			
			u8 xdata datsTemp[32] = {0};
		
			dats_Tx[0] 	= FRAME_HEAD_SERVER;
			
			memcpy(&datsTemp[0], &dats_Tx[1], 32);
			memcpy(&dats_Tx[13], &datsTemp[0], 32);	//֡ͷ�������ճ�12���ֽ�
			memcpy(&dats_Tx[1],  &Dst_MACID_Temp[0], 6);	//Զ��MACID���
			memcpy(&dats_Tx[8],  &MAC_ID[1], 5);	/*dats_Tx[7] ��ʱ��0*///Զ��MACID���
			
			dats_Tx[1 + 12] = dataTransLength_objSERVER;
			dats_Tx[2 + 12] = frame_Type;
			dats_Tx[3 + 12] = frame_CMD;
			
			if(!ifSpecial_CMD)dats_Tx[10 + 12] = SWITCH_TYPE_FP;	//�����������
			
			memcpy(&dats_Tx[5 + 12], &MAC_ID[1], 5);	//MAC���
								  
			dats_Tx[4 + 12] = frame_Check(&dats_Tx[5 + 12], 28);	
			
		}break;
		
		/*���������ݰ���װ*///33�ֽ�
		case dataTransLength_objMOBILE:{
		
			dats_Tx[0] 	= FRAME_HEAD_MOBILE;
			
			dats_Tx[1] 	= dataTransLength_objMOBILE;
			dats_Tx[2] 	= frame_Type;
			dats_Tx[3] 	= frame_CMD;
			
			if(!ifSpecial_CMD)dats_Tx[10] = SWITCH_TYPE_FP;	//�����������
			
			memcpy(&dats_Tx[5], &MAC_ID[1], 5);	//MAC���
								  
			dats_Tx[4] 	= frame_Check(&dats_Tx[5], 28);
		}break;
		
		default:break;
	}	
}
						   
/*�����ݷ�װ���������ݰ�����ǰ�����ã��Զ������������ݷ�װ*///����У�鱻��ǰ������
void dtasTX_loadBasic_CUST(dataTrans_obj obj_custom,	//���Ͱ����ݻ��������Ϣ��װ
						   u8 dats_Tx[45],		
						   u8 frame_Type,
						   u8 frame_CMD,
						   bit ifSpecial_CMD){	
						  
	switch(obj_custom){
	
		/*���������ݰ���װ*///45�ֽ�
		case DATATRANS_objFLAG_SERVER:{
			
			u8 xdata datsTemp[32] = {0};
		
			dats_Tx[0] 	= FRAME_HEAD_SERVER;
			
			memcpy(&datsTemp[0], &dats_Tx[1], 32);
			memcpy(&dats_Tx[13], &datsTemp[0], 32);	//֡ͷ�������ճ�12���ֽ�
			memcpy(&dats_Tx[1],  &Dst_MACID_Temp[0], 6);	//Զ��MACID���
			memcpy(&dats_Tx[8],  &MAC_ID[1], 5);	/*dats_Tx[7] ��ʱ��0*///Զ��MACID���
			
			dats_Tx[1 + 12] = dataTransLength_objSERVER;
			dats_Tx[2 + 12] = frame_Type;
			dats_Tx[3 + 12] = frame_CMD;
			
			if(!ifSpecial_CMD)dats_Tx[10 + 12] = SWITCH_TYPE_FP;	//�����������
			
			memcpy(&dats_Tx[5 + 12], &MAC_ID[1], 5);	//MAC���
								  
			dats_Tx[4 + 12] = frame_Check(&dats_Tx[5 + 12], 28);	
			
		}break;
		
		/*���������ݰ���װ*///33�ֽ�	--����45����33��װ
		case DATATRANS_objFLAG_MOBILE:
		
		default:{
		
			dats_Tx[0] 	= FRAME_HEAD_MOBILE;
			
			dats_Tx[1] 	= dataTransLength_objMOBILE;
			dats_Tx[2] 	= frame_Type;
			dats_Tx[3] 	= frame_CMD;
			
			if(!ifSpecial_CMD)dats_Tx[10] = SWITCH_TYPE_FP;	//�����������
			
			memcpy(&dats_Tx[5], &MAC_ID[1], 5);	//MAC���
								  
			dats_Tx[4] 	= frame_Check(&dats_Tx[5], 28);
		}break;
	}	
}

/********************* UART1(WIIF)�жϺ���_�Զ����ع�************************/
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

/***********�������ݼ�����������������**************/
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
		
		heartBeat_Cnt 	= heartBeat_Period - 100;	//��������ż����������״̬������100ms����ֹģ��ճ��
		heartBeat_Type	= 0;	//ż����
			
		Local_ifTim_sw_running_FLAG 		= ifTim_sw_running_FLAG;
		Local_status_Relay					= status_Relay;
		Local_deviceLock_flag				= deviceLock_flag;
		Local_ifDelay_sw_running_FLAG		= ifDelay_sw_running_FLAG;
	}
}

/********************* ���ݴ��估����************************/
void thread_dataTrans(void){
	
	static u8 timeZone_Hour 	= 0;
	static u8 timeZone_Minute	= 0;
	
	unsigned char xdata repeatTX_buff[45]	= {0};
	unsigned char xdata rxBuff_WIFI_temp[45]= {0};
		
	unsigned char frameLength				= 0;

	bit datsQualified_FLG					= 0;	
	bit Parsing_EN 							= 0;
	
	u8 xdata heartBeat_Pack[14] 			= {0};	
	
	static u8 elec_Consum_moment			= 0;	//�õ�����Ӧʱ�α�־������ֻ��ÿСʱ���õ���
	
	/*********************************���ݰ��ɼ�**********************************/
	if(uartRX_toutFLG){ //��ʱ��֡
	
		uartRX_toutFLG = 0;
		
		memset(rxBuff_WIFI_temp, 0, 45 * sizeof(u8));
		
		if((datsRcv_ZIGB.rcvDats[0] == FRAME_HEAD_SERVER) && 
		   (datsRcv_ZIGB.rcvDats[13] == dataTransLength_objSERVER) &&
		   (datsRcv_ZIGB.rcvDats[14] == FRAME_TYPE_MtoS_CMD) &&
		   (datsRcv_ZIGB.rcvDatsLen >= dataTransLength_objSERVER)){ /*��λ�����˸��ֽ�*/
			
			memcpy(&Dst_MACID_Temp[0], &datsRcv_ZIGB.rcvDats[7], 6);  //Զ��MACID�ݴ�	
			rxBuff_WIFI_temp[0] = datsRcv_ZIGB.rcvDats[0]; //֡ͷ��ֵ
			memcpy(&rxBuff_WIFI_temp[1], &datsRcv_ZIGB.rcvDats[13], 32);	//���ݼ��أ�֡���룬�����33�ֽ�
			
			//��Ӧ�ظ��������Ը���	
			dataTrans_objWIFI = DATATRANS_objFLAG_SERVER;
			repeatTX_Len	  = dataTransLength_objSERVER;
			
			frameLength = dataTransLength_objSERVER;
			datsQualified_FLG = 1;
				
		}else
		if((datsRcv_ZIGB.rcvDats[0] == FRAME_HEAD_MOBILE) && 
		   (datsRcv_ZIGB.rcvDats[1] == dataTransLength_objMOBILE) &&
		   (datsRcv_ZIGB.rcvDats[2] == FRAME_TYPE_MtoS_CMD) &&
		   (datsRcv_ZIGB.rcvDatsLen == dataTransLength_objMOBILE)){
		
			memcpy(rxBuff_WIFI_temp, datsRcv_ZIGB.rcvDats, dataTransLength_objMOBILE);	//���ݼ���
			
			//��Ӧ�ظ��������Ը���	
			dataTrans_objWIFI = DATATRANS_objFLAG_MOBILE;
			repeatTX_Len	  = dataTransLength_objMOBILE;
				
			frameLength = dataTransLength_objMOBILE;
			datsQualified_FLG = 1;
				
		}else
		if((datsRcv_ZIGB.rcvDats[0] == FRAME_HEAD_HEARTBEAT) && 
		   (datsRcv_ZIGB.rcvDats[1] == dataHeartBeatLength_objSERVER) &&
		   (datsRcv_ZIGB.rcvDatsLen == 14)){ /*�������صĻ���14�ֽ�*///����֡��Ϊ20���ҷ���20�ֽڣ���������Ȼ��14�ֽ�
			
			memcpy(rxBuff_WIFI_temp, datsRcv_ZIGB.rcvDats, 14);	//���ݼ���
			
			//��Ӧ�ظ��������Ը���	--������repeatTX_Len���£�����
			dataTrans_objWIFI = HEARTBEAT_objFLAG_SERVER;
			   
			frameLength = dataHeartBeatLength_objSERVER;
			datsQualified_FLG = 1;
			   
		}else{
		
			dataTrans_objWIFI = dataTrans_obj_NULL;
			datsQualified_FLG = 0;
		}
	}
	
	/*********************************���ݰ��������**********************************/
	if(datsQualified_FLG){
		
		bit frameCodeCheck_PASS = 0; //У������ͨ����־
		bit frameMacCheck_PASS  = 0; //mac��ַ�����ͨ����־
		
		datsQualified_FLG = 0;
	
		switch(frameLength){
		
			case dataTransLength_objSERVER:
			case dataTransLength_objMOBILE:{
			
				if(rxBuff_WIFI_temp[4] == frame_Check(&rxBuff_WIFI_temp[5], 28))frameCodeCheck_PASS = 1; //У������
				if(!memcmp(&rxBuff_WIFI_temp[5], &MAC_ID[1], 5))frameMacCheck_PASS  = 1; //mac��ַ���
				
				if(rxBuff_WIFI_temp[3] == FRAME_MtoSCMD_cmdCfg_swTim){ //���ý���У�������ָ��
				
					frameCodeCheck_PASS = 1;	
					
				}
				if(rxBuff_WIFI_temp[3] == FRAME_MtoSCMD_cmdConfigSearch){ //���ý���mac��ַ����ָ��
				
					frameMacCheck_PASS = 1;	
					
				}
				
				if(frameMacCheck_PASS & frameMacCheck_PASS){ //ָ����֤
					
					Parsing_EN = 1;	
					
				}else{
				
//					if(!frameMacCheck_PASS)uartObjWIFI_Send_String("fuck mac", 8);
//					if(!frameMacCheck_PASS)uartObjWIFI_Send_String("fuck code", 9);rxBuff_WIFI_temp
//					uartObjWIFI_Send_String(rxBuff_WIFI_temp, 33);
				}
				
			}break;
			
			case dataHeartBeatLength_objSERVER:{
			
				if(!memcmp(&rxBuff_WIFI_temp[3], &MAC_ID[1], 5)){	//MAC��ַ���رȶ�
					
					Parsing_EN = 1;	
				}
				
			}break;
			
			default:{
		
				Parsing_EN = 0;	
				
			}break;
		}
	}
	
	/*********************************���ݰ���ʼ������Ӧ**********************************/	
	if(Parsing_EN){	//��ʼ����
		
		Parsing_EN = 0;
		
		switch(dataTrans_objWIFI){
		
			//������
			case DATATRANS_objFLAG_SERVER:
			//������
			case DATATRANS_objFLAG_MOBILE:{
				
					bit specialCmd_FLAG = 0;	/*�������ݷ�װ��־*///����1�Ƿ�ռ�ã�ԭΪ�������ͣ�
					bit respond_FLAG	= 1;	/*�Ƿ���Ӧ�ظ���־*///���ݽ��պ��Ƿ������ظ���Ӧ
				
					u8 cmdParing_Temp = rxBuff_WIFI_temp[3];
				
					memset(repeatTX_buff, 0, sizeof(u8) * 45);

					switch(cmdParing_Temp){		//���������ʶ��

						/*����ָ��*/
						case FRAME_MtoSCMD_cmdControl:{		
							
							switch(rxBuff_WIFI_temp[11]){
							
								case 0x00:{		//����_��
								
									swStatus_fromUsr = swStatus_off;
									
								}break;
									
								case 0x01:{		//����_��
								
									swStatus_fromUsr = swStatus_on;

								}break;
								
								default:break;
							}
							
							repeatTX_buff[11]	= rxBuff_WIFI_temp[11];		//����״̬�������
							
						}break;

						/*��������ָ��*/
						case FRAME_MtoSCMD_cmdConfigSearch:{	
							
							u8 deviceLock_IF = 0;
											
							EEPROM_read_n(EEPROM_ADDR_deviceLockFLAG, &deviceLock_IF, 1);
							
							//�������
							if(!deviceLock_IF){	
								
								u8 xdata serverIP_temp[4] = {0};
								memcpy(&serverIP_temp[0], &rxBuff_WIFI_temp[6], 4);
							
								//��������ַͬ��
								if(WIFI_serverUDP_CHG(serverIP_temp)){
									

								}else{
								
								}

								//ʱ����Ϣͬ��
								timeZone_Hour 	= rxBuff_WIFI_temp[12];		//ʱ������
								timeZone_Minute	= rxBuff_WIFI_temp[13];
								coverEEPROM_write_n(EEPROM_ADDR_timeZone_H, &timeZone_Hour, 1);
								coverEEPROM_write_n(EEPROM_ADDR_timeZone_M, &timeZone_Minute, 1);
								
							}else{
							
								respond_FLAG = 0;
							}
							
						}break;
						
						/*��ѯ��¼ָ��*/
						case FRAME_MtoSCMD_cmdQuery:{	
							
							u8 deviceLock_IF = 0;
							
							EEPROM_read_n(EEPROM_ADDR_deviceLockFLAG, &deviceLock_IF, 1);

							if(deviceLock_IF){
								
								
							}else{
							

							}
							
						}break;
						
						case FRAME_MtoSCMD_cmdCfg_swTim:{	/*��ͨ���ض�ʱ����ָ��*/
							
							u8 loop = 0;
							
							//Уʱ
							stt_Time datsTime_temp = {0};
							
							datsTime_temp.time_Year		= rxBuff_WIFI_temp[26];
							datsTime_temp.time_Month 	= rxBuff_WIFI_temp[27];
							datsTime_temp.time_Day 		= rxBuff_WIFI_temp[28];
							datsTime_temp.time_Hour 	= rxBuff_WIFI_temp[29];
							elec_Consum_moment			= datsTime_temp.time_Hour;
							datsTime_temp.time_Minute 	= rxBuff_WIFI_temp[30];
							datsTime_temp.time_Week 	= rxBuff_WIFI_temp[31];
							
							timeSet(datsTime_temp);
							
							//��ʱ���ݴ�������,���ദ��
							switch(rxBuff_WIFI_temp[13]){
							
								case cmdConfigTim_normalSwConfig:{	/*��ͨ��ʱ*/
									
									for(loop = 0; loop < 4; loop ++){
									
										if(rxBuff_WIFI_temp[14 + loop * 3] == 0x80){	/*һ���Զ�ʱ�ж�*///��ռλΪ�գ�����ʱ�����򿪣�˵����һ����
										
											swTim_onShoot_FLAG 				|= (1 << loop);	//һ���Զ�ʱ��־����
											rxBuff_WIFI_temp[14 + loop * 3] |= (1 << (rxBuff_WIFI_temp[31] - 1)); //ǿ�н��е�ǰ��ռλ������ִ����Ϻ����
										}
									}
									coverEEPROM_write_n(EEPROM_ADDR_swTimeTab, &rxBuff_WIFI_temp[14], 12);	//��ʱ��

								}break;
								
								case cmdConfigTim_onoffDelaySwConfig:{	/*������ʱ*/
								
									if(rxBuff_WIFI_temp[14]){
									
										ifDelay_sw_running_FLAG |= (1 << 1);
										delayPeriod_onoff 		= rxBuff_WIFI_temp[14];
										
//										if(rxBuff_WIFI_temp[15] == 2)delayUp_act = 1; //2Ϊ��ʱ����1Ϊ��ʱ��
//										else delayUp_act = 0;
										
										delayUp_act		  		= rxBuff_WIFI_temp[15];
										
										delayCnt_onoff			= 0;
										
									}else{
									
										ifDelay_sw_running_FLAG &= ~(1 << 1); //������籣��
										delayPeriod_onoff 		= 0;
										delayCnt_onoff			= 0;
									}
									
								}break;
								
								case cmdConfigTim_closeLoopSwConfig:{	/*�Զ�ѭ���ر�*/
								
									if(rxBuff_WIFI_temp[14]){
									
										ifDelay_sw_running_FLAG |= (1 << 0);
										delayPeriod_closeLoop	= rxBuff_WIFI_temp[14];
										delayCnt_closeLoop		= 0;
									}else{
									
										ifDelay_sw_running_FLAG &= ~(1 << 0); //��Ҫ���籣��
										delayPeriod_closeLoop	= 0;
										delayCnt_closeLoop		= 0;
									}
									
									coverEEPROM_write_n(EEPROM_ADDR_swDelayFLAG, &ifDelay_sw_running_FLAG, 1);
									coverEEPROM_write_n(EEPROM_ADDR_periodCloseLoop, &delayPeriod_closeLoop, 1);
									
								}break;								
								
								default:break;
							}
							
							//������Ӧ���ظ�
							repeatTX_buff[12]	= rxBuff_WIFI_temp[12];
							repeatTX_buff[13]	= rxBuff_WIFI_temp[13];

						}break;
						
						/*���û���ָ��*/
						case FRAME_MtoSCMD_cmdInterface:{	
						
							
							
						}break;
						
						/*���ظ�λָ��*/
						case FRAME_MtoSCMD_cmdReset:{		
						
							
							
						}break;
						
						/*�豸����ָ����豸��Ϣ��Ӧʧ��*/
						case FRAME_MtoSCMD_cmdLockON:{		
							
							//���ݴ���������Ӧ
							{
								u8 deviceLock_IF = 1;
								
								deviceLock_flag  = 1;
								coverEEPROM_write_n(EEPROM_ADDR_deviceLockFLAG, &deviceLock_IF, 1);
							}							
						}break;
						
						/*�豸����ָ����豸��Ϣ��Ӧʹ��*/
						case FRAME_MtoSCMD_cmdLockOFF:{		
							
							//���ݴ���������Ӧ
							{
								u8 deviceLock_IF = 0;
								
								deviceLock_flag  = 0;
								coverEEPROM_write_n(EEPROM_ADDR_deviceLockFLAG, &deviceLock_IF, 1);
							}						
						}break;
						
						/*��ͨ���ض�ʱ���ò�ѯָ��*/
						case FRAME_MtoSCMD_cmdswTimQuery:{	
							
							repeatTX_buff[12]	= rxBuff_WIFI_temp[12];
							repeatTX_buff[13]	= rxBuff_WIFI_temp[13];
							
							//����ظ�
							switch(rxBuff_WIFI_temp[13]){
							
								case 0: /*��λ���ڶ�ʱ��ʱ���0����Э��*/
								case cmdConfigTim_normalSwConfig:{
								
									u8 loop = 0;
								
									//������Ӧ���ظ�
									EEPROM_read_n(EEPROM_ADDR_swTimeTab, &repeatTX_buff[14], 12);	//��ʱ��ظ���װ
									
									//�ظ����ݶ��δ������һ���Զ�ʱ���ݣ�
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
						
						/*APģʽ����ָ��*/
						case FRAME_MtoSCMD_cmdConfigAP:{	
						
							//��ָ����ã� ����
						}break;
						
						/*��ʾ��ʹ��ָ��*/
						case FRAME_MtoSCMD_cmdBeepsON:{		
						
							
						}break;
						
						/*��ʾ��ʧ��ָ��*/
						case FRAME_MtoSCMD_cmdBeepsOFF:{	
						
							
						}break;
						
						/*�ָ����������Ƿ�֧�ֲ�ѯ*/
						case FRAME_MtoSCMD_cmdftRecoverRQ:{	
						

						}
						
						/*�ָ���������ָ��*/
						case FRAME_MtoSCMD_cmdRecoverFactory:{	
						

						}break;
						
						default:break;
					}
					
					if(respond_FLAG){	//������Ӧ�ظ�ִ��
					
						dtasTX_loadBasic_AUTO( repeatTX_buff,				/*����ǰ���װ��*///���Ͱ�������Ϣ���
											   FRAME_TYPE_StoM_RCVsuccess,
											   cmdParing_Temp,
											   specialCmd_FLAG
											 );
						uartObjWIFI_Send_String(repeatTX_buff, repeatTX_Len);//���ݽ��ջظ���Ӧ	
					}
					
//					if((HEARTBEAT_objFLAG_SERVER != dataTrans_objWIFI) &&
//					   ((cmdParing_Temp != FRAME_MtoSCMD_cmdConfigSearch) && (DATATRANS_objFLAG_MOBILE == dataTrans_objWIFI)) \
//					)beeps(2);	//��ʾ��,����������,��������ʱ����
					
			}break;
			
			//���������ݽ���
			case HEARTBEAT_objFLAG_SERVER:{
			
				switch(rxBuff_WIFI_temp[2]){
				
					case FRAME_HEARTBEAT_cmdEven:{
					
						
					}break;
						
					case FRAME_HEARTBEAT_cmdOdd:{		//��������ʱ��У׼
					
						stt_Time datsTime_temp = {0};
						
//						u8 Y	= rxBuff_WIFI_temp[8];
//						u8 W 	= rxBuff_WIFI_temp[8];
						u8 M 	= rxBuff_WIFI_temp[9];
						u8 D 	= rxBuff_WIFI_temp[10];
//						u8 W 	= Y + (Y / 4) + 5 - 40 + (26 * (M + 1) / 10) + D - 1;	//���չ�ʽ
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
		
	/*************************************������ҵ�����**********************************************/
	if(heartBeat_Cnt < heartBeat_Period);
	else{
	
	  	heartBeat_Cnt = 0;
		
		memset(&heartBeat_Pack[0], 0, sizeof(u8) * dataHeartBeatLength_objSERVER);
		
		heartBeat_Pack[0]	= FRAME_HEAD_HEARTBEAT;
		heartBeat_Pack[1]	= dataHeartBeatLength_objSERVER;

		if(heartBeat_Type){	//��������
		
			heartBeat_Type = !heartBeat_Type;
			
			heartBeat_Pack[2]	= FRAME_HEARTBEAT_cmdOdd;
			memcpy(&heartBeat_Pack[3], &MAC_ID[1], 5);
			
			EEPROM_read_n(EEPROM_ADDR_timeZone_H, &timeZone_Hour, 1);
			EEPROM_read_n(EEPROM_ADDR_timeZone_M, &timeZone_Minute, 1);
			heartBeat_Pack[8] = timeZone_Hour;
			heartBeat_Pack[9] = timeZone_Minute;
			
//			//���Դ����
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
			u16 integer_prt = (u16)elec_Consum & 0x00FF;	//������������
			u16 decimal_prt = (u16)((elec_Consum - (float)integer_prt) * 10000.0F);	//����С������ǿתΪ��������
			
			heartBeat_Pack[10]	= (u8)((integer_prt & 0x00FF) >> 0);	//����������װ
			heartBeat_Pack[11]	= (u8)((decimal_prt & 0xFF00) >> 8);	//С��������װ
			heartBeat_Pack[12]	= (u8)((decimal_prt & 0x00FF) >> 0);	//С��������װ
			}
			
			{
			u16 integer_prt = (u16)paramElec_Param.ePower & 0xFFFF;	//������������
			u16 decimal_prt = (u16)((paramElec_Param.ePower - (float)integer_prt) * 10000.0F);	//����С������ǿתΪ��������
			
			heartBeat_Pack[13]	= (u8)((integer_prt & 0xFF00) >> 8);	//����������װ
			heartBeat_Pack[14]	= (u8)((integer_prt & 0x00FF) >> 0);	//����������װ
			heartBeat_Pack[15]	= (u8)((decimal_prt & 0xFF00) >> 8);	//С��������װ
			heartBeat_Pack[16]	= (u8)((decimal_prt & 0x00FF) >> 0);	//С��������װ
			}
			
			heartBeat_Pack[17]	= elec_Consum_moment;	//��Ӧʱ�α�־��װ
			
		}else{	//ż������
		
			heartBeat_Type = !heartBeat_Type;
			
			heartBeat_Pack[2]	= FRAME_HEARTBEAT_cmdEven;
			memcpy(&heartBeat_Pack[3], &MAC_ID[1], 5);
			
			heartBeat_Pack[8]	= deviceLock_flag;
			heartBeat_Pack[9]	= ifTim_sw_running_FLAG;
			heartBeat_Pack[10]	= status_Relay;
			heartBeat_Pack[11]	= (ifDelay_sw_running_FLAG & 0x01) >> 0; //��ɫģʽ
		}
		
		heartBeat_Pack[18]	= SWITCH_TYPE_FP;
		heartBeat_Pack[19]	= FRAME_TAIL_HEARTBEAT;

		uartObjWIFI_Send_String(heartBeat_Pack, dataHeartBeatLength_objSERVER);
	}
}
