#ifndef __DATATMANAGE_H_
#define __DATATMANAGE_H_

#define 	IF_REBORN						 0			/*是否重新覆盖生日标记*/// 1：出厂前转世	0：正常启动
#define		BIRTHDAY_FLAG					 0xA1		//产品出生标记

#define 	SWITCH_TYPE_FP	 				 0x07		//开关类型，电量检测插座

#define 	ROMADDR_ROM_STC_ID		 		 0x3ff8		//STC单片机 全球ID地址

#define 	EEPROM_ADDR_START	 			 0x0000		//起始地址

#define 	EEPROM_USE_OF_NUMBER 			 0x0080	

#define		EEPROM_ADDR_elecValSaveMark 	 0x0050		
	
#define		EEPROM_ADDR_BirthdayMark         0x0001		//01H - 01H 是否首次启动					01_Byte
#define  	EEPROM_ADDR_relayStatus          0x0002		//02H - 02H 开关状态存储					01_Byte
#define  	EEPROM_ADDR_timeZone_H           0x0003		//03H - 03H 时区——时						01_Byte
#define  	EEPROM_ADDR_timeZone_M           0x0004		//04H - 04H 时区——分						01_Byte
#define  	EEPROM_ADDR_deviceLockFLAG       0x0005		//05H - 05H 设备锁状态位					01_Byte
#define  	EEPROM_ADDR_swTimeTab          	 0x0010		//10H - 1CH 4组普通定时数据，每组3字节		12_Byte	
#define  	EEPROM_ADDR_swDelayFLAG			 0x0020		//20H - 20H 开关延时标志位集合				01_Byte
#define 	EEPROM_ADDR_periodCloseLoop		 0x0021		//21H - 21H	循环关闭时间间隔				01_Byte
#define		EEPROM_ADDR_serverIP_record    	 0x0030   	//30H - 33H 服务器IP存储					04_Byte
#define		EEPROM_ADDR_unDefine04	         0x0000		
#define		EEPROM_ADDR_unDefine05           0x0000
#define		EEPROM_ADDR_unDefine06           0x0000
#define		EEPROM_ADDR_unDefine07           0x0000
#define		EEPROM_ADDR_unDefine08           0x0000
#define		EEPROM_ADDR_unDefine11           0x0000
#define		EEPROM_ADDR_unDefine12           0x0000
#define		EEPROM_ADDR_unDefine13           0x0000

void MAC_ID_Relaes(void);
void birthDay_Judge(void);
void software_Reset(void);
void Factory_recover(void);

#endif