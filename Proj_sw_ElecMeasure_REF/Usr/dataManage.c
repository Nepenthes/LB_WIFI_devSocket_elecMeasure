#include "dataManage.h"

#include "wifi_LPT220.h"
#include "Tips.h"

#include "soft_uart.h"
#include "eeprom.h"
#include "delay.h"

#include "string.h"
#include "stdio.h"

/********************�����ļ�����������******************/
unsigned char xdata MAC_ID[6] = {0};

/*------------------------------------------------------------------*/
void software_Reset(void){
	
	beeps(2);	//��ʾ��
	
	((void(code *)(void))0x0000)();
	
	//����״̬�ָ��������Զ���ʼ��
}

void MAC_ID_Relaes(void){

	unsigned char code *id_ptr 	= ROMADDR_ROM_STC_ID;
	
	memcpy(MAC_ID, id_ptr - 6, 6); //˳����ǰ����ǰ����ֻȡ����λ
}

void Factory_recover(void){

	u8 xdata log_info[30] 		= {0};
	u8 		 eeprom_buffer[20]	= {0};
	u8 code	 IP_default[4]		= {47,52,5,108};		//Ĭ����۷�����
	
	beeps(7);	//��ʾ��
	
	WIFI_hwRestoreFactory();
	
	EEPROM_SectorErase(EEPROM_ADDR_START);		//�״�����EEPROM����
	delayMs(10);
	
	eeprom_buffer[0] = BIRTHDAY_FLAG;			//���ձ��
	EEPROM_write_n(EEPROM_ADDR_BirthdayMark, &eeprom_buffer[0], 1);
	delayMs(10);
	
	eeprom_buffer[0] = 0;						//�豸���⿪
	EEPROM_write_n(EEPROM_ADDR_deviceLockFLAG, &eeprom_buffer[0], 1);
	delayMs(10);	
	
	memset(eeprom_buffer, 0, 12 * sizeof(u8));	//��ͨ���ض�ʱʱ�̱�����
	EEPROM_write_n(EEPROM_ADDR_swTimeTab, &eeprom_buffer[0], 13);
	delayMs(10);
	
	memcpy(eeprom_buffer, IP_default, 4);		//������IPģ������Ĭ��
	EEPROM_write_n(EEPROM_ADDR_serverIP_record, &eeprom_buffer[0], 4);
	delayMs(10);
}

void birthDay_Judge(void){
	
	u8 		 eeprom_buffer[20]	= {0};
	u8 code	 IP_default[4]		= {47,52,5,108};		//Ĭ����۷�����
	
	delayMs(10);
	
#if(IF_REBORN != 1)		//����ǰ������ա���ת�����ã��޸�ͷ�ļ�IF_REBORN ֵ����
	
	EEPROM_read_n(EEPROM_ADDR_BirthdayMark, &eeprom_buffer[0], 1);
	delayMs(10);
	
	if(BIRTHDAY_FLAG != eeprom_buffer[0]){
	
		EEPROM_SectorErase(EEPROM_ADDR_START);		//�״�����EEPROM����
		delayMs(10);
		
		eeprom_buffer[0] = BIRTHDAY_FLAG;			//���ձ��
		EEPROM_write_n(EEPROM_ADDR_BirthdayMark, &eeprom_buffer[0], 1);
		delayMs(10);
		
		eeprom_buffer[0] = 0;						//�豸���⿪
		EEPROM_write_n(EEPROM_ADDR_deviceLockFLAG, &eeprom_buffer[0], 1);
		delayMs(10);		
		
		memset(eeprom_buffer, 0, 12 * sizeof(u8));	//��ͨ���ض�ʱʱ�̱�����
		EEPROM_write_n(EEPROM_ADDR_swTimeTab, &eeprom_buffer[0], 13);
		delayMs(10);
		
		memcpy(eeprom_buffer, IP_default, 4);		//������IPģ������Ĭ��
		EEPROM_write_n(EEPROM_ADDR_serverIP_record, &eeprom_buffer[0], 4);
		delayMs(10);
		
	}else{

		
	}

#else
	
	eeprom_buffer[0] = 0;		//���±�ǳ������Ǳ��
	coverEEPROM_write_n(EEPROM_ADDR_BirthdayMark, &eeprom_buffer[0], 1);
	
	delayMs(10);
	
	eeprom_buffer[0] = 0;
	EEPROM_read_n(EEPROM_ADDR_BirthdayMark, &eeprom_buffer[0], 1);
	
#endif
}