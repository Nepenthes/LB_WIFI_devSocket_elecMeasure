#include "dataManage.h"

#include "wifi_LPT220.h"
#include "Tips.h"

#include "soft_uart.h"
#include "eeprom.h"
#include "delay.h"

#include "string.h"
#include "stdio.h"

/********************本地文件变量创建区******************/
unsigned char xdata MAC_ID[6] = {0};

/*------------------------------------------------------------------*/
void software_Reset(void){
	
	beeps(2);	//提示音
	
	((void(code *)(void))0x0000)();
	
	//无需状态恢复，重启自动初始化
}

void MAC_ID_Relaes(void){

	unsigned char code *id_ptr 	= ROMADDR_ROM_STC_ID;
	
	memcpy(MAC_ID, id_ptr - 6, 6); //顺序向前，往前读，只取后六位
}

void Factory_recover(void){

	u8 xdata log_info[30] 		= {0};
	u8 		 eeprom_buffer[20]	= {0};
	u8 code	 IP_default[4]		= {47,52,5,108};		//默认香港服务器
	
	beeps(7);	//提示音
	
	WIFI_hwRestoreFactory();
	
	EEPROM_SectorErase(EEPROM_ADDR_START);		//首次启动EEPROM擦除
	delayMs(10);
	
	eeprom_buffer[0] = BIRTHDAY_FLAG;			//生日标记
	EEPROM_write_n(EEPROM_ADDR_BirthdayMark, &eeprom_buffer[0], 1);
	delayMs(10);
	
	eeprom_buffer[0] = 0;						//设备锁解开
	EEPROM_write_n(EEPROM_ADDR_deviceLockFLAG, &eeprom_buffer[0], 1);
	delayMs(10);	
	
	memset(eeprom_buffer, 0, 12 * sizeof(u8));	//普通开关定时时刻表清零
	EEPROM_write_n(EEPROM_ADDR_swTimeTab, &eeprom_buffer[0], 13);
	delayMs(10);
	
	memcpy(eeprom_buffer, IP_default, 4);		//服务器IP模板设置默认
	EEPROM_write_n(EEPROM_ADDR_serverIP_record, &eeprom_buffer[0], 4);
	delayMs(10);
}

void birthDay_Judge(void){
	
	u8 		 eeprom_buffer[20]	= {0};
	u8 code	 IP_default[4]		= {47,52,5,108};		//默认香港服务器
	
	delayMs(10);
	
#if(IF_REBORN != 1)		//出厂前配置清空——转世设置，修改头文件IF_REBORN 值即可
	
	EEPROM_read_n(EEPROM_ADDR_BirthdayMark, &eeprom_buffer[0], 1);
	delayMs(10);
	
	if(BIRTHDAY_FLAG != eeprom_buffer[0]){
	
		EEPROM_SectorErase(EEPROM_ADDR_START);		//首次启动EEPROM擦除
		delayMs(10);
		
		eeprom_buffer[0] = BIRTHDAY_FLAG;			//生日标记
		EEPROM_write_n(EEPROM_ADDR_BirthdayMark, &eeprom_buffer[0], 1);
		delayMs(10);
		
		eeprom_buffer[0] = 0;						//设备锁解开
		EEPROM_write_n(EEPROM_ADDR_deviceLockFLAG, &eeprom_buffer[0], 1);
		delayMs(10);		
		
		memset(eeprom_buffer, 0, 12 * sizeof(u8));	//普通开关定时时刻表清零
		EEPROM_write_n(EEPROM_ADDR_swTimeTab, &eeprom_buffer[0], 13);
		delayMs(10);
		
		memcpy(eeprom_buffer, IP_default, 4);		//服务器IP模板设置默认
		EEPROM_write_n(EEPROM_ADDR_serverIP_record, &eeprom_buffer[0], 4);
		delayMs(10);
		
	}else{

		
	}

#else
	
	eeprom_buffer[0] = 0;		//重新标记出厂覆盖标记
	coverEEPROM_write_n(EEPROM_ADDR_BirthdayMark, &eeprom_buffer[0], 1);
	
	delayMs(10);
	
	eeprom_buffer[0] = 0;
	EEPROM_read_n(EEPROM_ADDR_BirthdayMark, &eeprom_buffer[0], 1);
	
#endif
}