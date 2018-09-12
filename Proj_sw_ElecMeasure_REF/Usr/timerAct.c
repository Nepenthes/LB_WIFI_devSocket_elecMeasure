#include "timerAct.h"

#include "dataManage.h"
#include "HYM8564.h"
#include "Relay.h"
#include "dataTrans.h"

#include "string.h"
#include "stdio.h"

#include "eeprom.h"

///**************�������������������***********************/
extern float xdata 		elec_Consum;

/****************�����ļ�����������*************************/
switch_Status	swStatus_fromTim  				= swStatus_null;	//��ʱ�����¿��������־����ʱʱ�̵���ʱ������Ӧ����״̬

u8 				swTim_onShoot_FLAG				= 0;	//��ͨ���ض�ʱһ���Ա�־��������λ��ʶ�ĸ���ʱ��
bit				ifTim_sw_running_FLAG			= 0;	//��ͨ���ض�ʱ���б�־λ

u8 				ifDelay_sw_running_FLAG			= 0;	//��ʱ����_�Ƿ����б�־λ��bit 1��ʱ��������ʹ�ܱ�־��bit 0ѭ����ʱ�ر�����ʹ�ܱ�־��
u16				delayCnt_onoff					= 0;	//��ʱ������ʱ����
u8				delayPeriod_onoff				= 0;	//��ʱ��������
bit				delayUp_act						= 0;	//��ʱ�������嶯��
u16				delayCnt_closeLoop				= 0;	//��ɫģʽ��ʱ����
u8				delayPeriod_closeLoop			= 0;	//��ɫģʽ��������

/*-----------------------------------------------------------------------------------------------*/
void datsTiming_read_eeprom(timing_Dats timDats_tab[4]){

	u8 dats_Temp[12];
	u8 loop;
	
	EEPROM_read_n(EEPROM_ADDR_swTimeTab, dats_Temp, 12);
	
	for(loop = 0; loop < 4; loop ++){
	
		timDats_tab[loop].Week_Num	= (dats_Temp[loop * 3 + 0] & 0x7f) >> 0;	/*��ռλ����*///����λ
		timDats_tab[loop].if_Timing = (dats_Temp[loop * 3 + 0] & 0x80) >> 7;	/*�Ƿ�����ʱ������*///��һλ
		timDats_tab[loop].Status_Act= (dats_Temp[loop * 3 + 1] & 0xe0) >> 5;	/*��ʱ��Ӧ״̬����*///����λ
		timDats_tab[loop].Hour		= (dats_Temp[loop * 3 + 1] & 0x1f) >> 0;	/*��ʱʱ��_Сʱ*///����λ
		timDats_tab[loop].Minute	= (dats_Temp[loop * 3 + 2] & 0xff) >> 0;	/*��ʱʱ��_��*///ȫ��λ
	}
}

/*��ռλ�ж�*///�жϵ�ǰ��ֵ�Ƿ���ռλ�ֽ���
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
	
	stt_Time xdata		valTime_Local					= {0};	//��ǰʱ�仺��
	
	timing_Dats xdata	timDatsTemp_CalibrateTab[4] 	= {0};	/*��ʱ��ʼʱ�̱���*///��ʼʱ�̼�����
	
	timeRead(&valTime_Local);	//��ǰʱ���ȡ
	
//	{ //����log����-��ǰʱ�����
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
	
//	{ //����log����-��ǰ������Ϣ���
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
//					"��ǰСʱ�����ۼƣ�%.08f\n", 
//					elec_Consum);
//			/*log���Դ�ӡ*///��ͨ��ʱ��ʱ��Ϣ
//			uartObjWIFI_Send_String(log_dats, strlen(log_dats));
//		}
//	}
	
	/*��ʱҵ���Զ�ѭ��ҵ�����洢���ݶ�ȡ*///������һ�θ��¼���
	{
		
		static bit read_FLG = 0;
		
		if(!read_FLG){
		
			read_FLG = 1;
			
			EEPROM_read_n(EEPROM_ADDR_swDelayFLAG, &ifDelay_sw_running_FLAG, 1);
			EEPROM_read_n(EEPROM_ADDR_periodCloseLoop, &delayPeriod_closeLoop, 1);
		}
	}
	
	/*�õ�����ʱ���*///�������=====================================================================================================<<<
	if((valTime_Local.time_Minute	== 0)&&		//ʱ�̱ȶ�,����
	   (valTime_Local.time_Second	<= 30)){	//ʱ�̱ȶ�ʱ������ǰ30��
		 
			elec_Consum = 0.0f;
	   }
	
	/*��ͨ���ض�ʱ*///�Ķ�����=======================================================================================================<<<
	datsTiming_read_eeprom(timDatsTemp_CalibrateTab);	/*��ͨ����*///ʱ�̱��ȡ
	
	/*�ж��Ƿ�������ͨ���ض�ʱ��Ϊ��*/
	if((timDatsTemp_CalibrateTab[0].if_Timing == 0) &&	//ȫ�أ��ñ�־λ
	   (timDatsTemp_CalibrateTab[1].if_Timing == 0) &&
	   (timDatsTemp_CalibrateTab[2].if_Timing == 0) &&
	   (timDatsTemp_CalibrateTab[3].if_Timing == 0)
	  ){
	  
		ifTim_sw_running_FLAG = 0; 
		  
	}else{	//��ȫ�أ��ñ�־λ����ִ�ж�ʱ�߼�
		
		ifTim_sw_running_FLAG = 1; 
	
		for(loop = 0; loop < 4; loop ++){
			
			if(weekend_judge(valTime_Local.time_Week, timDatsTemp_CalibrateTab[loop].Week_Num)){	//��ռλ�ȶԣ��ɹ��Ž�����һ��
			
				if(timCount_ENABLE == timDatsTemp_CalibrateTab[loop].if_Timing){	//�Ƿ�����ʱ
					
//					{ //����log����-��ǰ��Ч��ʱ��Ϣ���
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
//									"��Ч��ʱ��%d��, ��_ʱ:%d, ��_��:%d \n", 
//									(int)loop, 
//									(int)timDatsTemp_CalibrateTab[loop].Hour, 
//									(int)timDatsTemp_CalibrateTab[loop].Minute);
//							/*log���Դ�ӡ*///��ͨ��ʱ��ʱ��Ϣ
//							uartObjWIFI_Send_String(log_dats, strlen(log_dats));
//						}
//					}
					
					if(((u16)valTime_Local.time_Hour * 60 + (u16)valTime_Local.time_Minute) ==  \
					   ((u16)timDatsTemp_CalibrateTab[loop].Hour * 60 + (u16)timDatsTemp_CalibrateTab[loop].Minute) && //ʱ�̱ȶ�
					   (valTime_Local.time_Second <= 5)){	 //ʱ�̱ȶ�ʱ������ǰ5��
						   
//						uartObjWIFI_Send_String("time_UP!!!", 11);
						
						//һ���Զ�ʱ�ж�
						if(swTim_onShoot_FLAG & (1 << loop)){	//�Ƿ�Ϊһ���Զ�ʱ��������ձ��ζ�ʱ��Ϣ
							
							u8 code dats_Temp = 0;
							
							swTim_onShoot_FLAG &= ~(1 << loop);
							coverEEPROM_write_n(EEPROM_ADDR_swTimeTab + loop * 3, &dats_Temp, 1); //��ʱ��Ϣ���
						}
					   
						//��ͨ���ض�����Ӧ
						if(timDatsTemp_CalibrateTab[loop].Status_Act & 0x01){	/*����*/
						
							swStatus_fromTim = swStatus_on;
							
						}else{		/*�ر�*/
						
							swStatus_fromTim = swStatus_off;
						}
					}else
					if(((u16)valTime_Local.time_Hour * 60 + (u16)valTime_Local.time_Minute) >	//��ǰʱ����ڶ�ʱʱ�䣬ֱ�����һ���Ա�־
					   ((u16)timDatsTemp_CalibrateTab[loop].Hour * 60 + (u16)timDatsTemp_CalibrateTab[loop].Minute)){
						   
						//һ���Զ�ʱ�ж�
						if(swTim_onShoot_FLAG & (1 << loop)){	//�Ƿ�Ϊһ���Զ�ʱ��������ձ��ζ�ʱ��Ϣ
							
							u8 code dats_Temp = 0;
							
							swTim_onShoot_FLAG &= ~(1 << loop);
							coverEEPROM_write_n(EEPROM_ADDR_swTimeTab + loop * 3, &dats_Temp, 1); //��ʱ��Ϣ���
						}
					}
				}
			}
		}
	}
}
