#include "HW8012.h"

#include "dataTrans.h"
#include "USART.h"

#include "stdio.h"
#include "string.h"

#include "soft_uart.h"

#include "pars_Method.h"
#include "Tips.h"
#include "dataManage.h"
#include "eeprom.h"

#include "delay.h"

/*脉冲计数值*/
float xdata pinFP_volXcurCNT = 1.0F; //电流电压脉冲计数值
float xdata pinFP_powerCNT 	 = 1.0F; //功率脉冲计数值
float xdata pinFP_stdby_powerCNT = 1.0F; //功率脉冲预测计数值

/*频率定值*/
float xdata pinFP_volXcur 	 = 1.0F; //电流电压 检测频率值
float xdata pinFP_power		 = 1.0F; //功率 检测频率值 5秒周期
float xdata pinFP_powerStdby = 1.0F; //功率 预检测频率值 1秒周期

elec_Param 	xdata paramElec_Param = {0.001f, 0.001f, 0.001f};	//电参数变量
float		xdata elec_Consum	  = 0.00000001f;

void intMeasure_Init(void){
	
	/*U/I select IO*///推挽输出
	P0M1	&= 0xFD;
	P0M0	|= 0x02;
	
	/*频率信号输出 IO*///高阻输入
	P3M1	|= 0xC0;
	P3M0	&= 0x3F;

	INT_CLKO |=  (1 << 4);  //(EX2 = 1)使能INT2中断
	INT_CLKO |=  (1 << 5);	//(EX3 = 1)使能INT3中断
	
	PX1 = PX0 = 0;
	
	pin_funFP_Select = 1;
}

/********************* INT2中断函数 *************************/
void Ext_INT2 (void) interrupt INT2_VECTOR{	//进中断时已经清除标志

	pinFP_powerCNT += 1.0F;
	pinFP_stdby_powerCNT += 1.0F;
}

void Ext_INT3 (void) interrupt INT3_VECTOR{	//进中断时已经清除标志

	pinFP_volXcurCNT += 1.0F;
}

//float elecConsum_save_read(void){

//	unsigned long xdata temp = 0UL;
//	u8 xdata valTemp_save[4] = {0};
//	
//	EEPROM_read_n(EEPROM_ADDR_elecValSaveMark, valTemp_save, 4);
//	temp |= (u32)valTemp_save[0] << 0;
//	temp |= (u32)valTemp_save[1] << 8;
//	temp |= (u32)valTemp_save[2] << 16;
//	temp |= (u32)valTemp_save[3] << 24;
//	
//	return ((float)temp / 100000000.00F);
//}

//void floatVal_save(float val, bit reCord_IF){

//	unsigned long xdata temp = 0UL;
//	u8 xdata valTemp_save[4] = {0};
//	
//	float xdata dataBase = elecConsum_save_read();
////	float xdata dataBase = 0.1234F;
//	
//	if(reCord_IF)dataBase = val;
//	else{
//	
//		dataBase += val;
//	}
//	
//	temp = (unsigned long)(dataBase * 100000000.00F);
//	valTemp_save[0] = (u8)((temp & 0x000000ffUL) >>  0);
//	valTemp_save[1] = (u8)((temp & 0x0000ff00UL) >>  8);
//	valTemp_save[2] = (u8)((temp & 0x00ff0000UL) >> 16);
//	valTemp_save[3] = (u8)((temp & 0xff000000UL) >> 24);
//	
//	coverEEPROM_write_n(EEPROM_ADDR_elecValSaveMark, valTemp_save, 4);
//}

void elecParamLog_thread(void){

	const 	u16 log_Period 	= 600;
	static	u16 log_Cnt		= 0;
	
	u8 xdata log_Info[80];
	
	if(NULL != memmem(rxBuff_WIFI, COM_RX1_Lenth, "signal switch", 13)){
	
		rxBuff_WIFI_Clr();
		pin_funFP_Select = !pin_funFP_Select;
		memset(log_Info, 0, sizeof(u8) * 40);
		sprintf(log_Info, "set %d ok!\n", (int)pin_funFP_Select);
		uartObjWIFI_Send_String(log_Info, strlen(log_Info));
	}
	
	if(log_Cnt < log_Period)log_Cnt ++;
	else{
	
		log_Cnt = 0;
		
//		paramElec_Param.eVoltage = paramElec_Param.eCurrent = paramElec_Param.ePower = elec_Consum = 123.45600;
		
		/*数据输出测试*///8051性能有限，sprintf浮点数输出极限为2个单位，故分次打印输出，效果一样
		memset(log_Info, 0, sizeof(u8) * 80);
		if(pin_funFP_Select){
		
			sprintf(log_Info, " fpU/I: %.02f Hz(电压).\n fpP: %.02f Hz.\n",  pinFP_volXcur,
																			 pinFP_power);
			uartObjWIFI_Send_String(log_Info, strlen(log_Info));	
			
			memset(log_Info, 0, sizeof(u8) * 80);
			sprintf(log_Info, " val_U: %.02fV.\n", paramElec_Param.eVoltage);
			
			uartObjWIFI_Send_String(log_Info, strlen(log_Info));	
			
		}else{
		
			sprintf(log_Info, " fpU/I: %.02f Hz(电流).\n fpP: %.02f Hz.\n",  pinFP_volXcur,
																			 pinFP_power);
			uartObjWIFI_Send_String(log_Info, strlen(log_Info));
			
			memset(log_Info, 0, sizeof(u8) * 80);
			sprintf(log_Info, " val_I: %.02fmA\n", paramElec_Param.eCurrent);
			
			uartObjWIFI_Send_String(log_Info, strlen(log_Info));
		}															
		
		{
		
//			float xdata elecConsum_temp = 0.0F;
//			
//			floatVal_save(elec_Consum, 0);
//			elecConsum_temp = elecConsum_save_read();
			
			memset(log_Info, 0, sizeof(u8) * 80);
			sprintf(log_Info, " val_P: %.02fW.\n val_elec_Sum: %.08fkWh.\n\n",	paramElec_Param.ePower,
																				elec_Consum);
			
			uartObjWIFI_Send_String(log_Info, strlen(log_Info));
		}
	}
}