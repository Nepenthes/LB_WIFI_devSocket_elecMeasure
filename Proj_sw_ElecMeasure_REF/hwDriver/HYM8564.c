#include "HYM8564.h"

#include "dataTrans.h"

#include "delay.h"
#include "soft_uart.h"

#include "stdio.h"
#include "string.h"

void Delay1us(void){		//@24.00MHz

	NOP10();
}

/****************IICʱ��������ʱ******************/
void DelayXus(unsigned int x){
	
	u16 pdata loop = IIC_Delay_GainFac * x;	//��ʱ����
	
	while(-- loop){
	
		Delay1us();
	}
}

void iicHYM8564_pinInit(void){

	P1M1 &= 0x3F;
	P1M0 &= 0x3F;
}

void IIC_HYM8564_Start(void){ 

	IICA_ClrSCL;DelayXus(1);
	IICA_SetSDA;DelayXus(1);
	IICA_SetSCL;DelayXus(1);
	IICA_ClrSDA;DelayXus(1);
	IICA_ClrSCL;DelayXus(1);
}

void IIC_HYM8564_Stop(void){       
  
	IICA_ClrSDA;DelayXus(1);
	IICA_SetSCL;DelayXus(1);
	IICA_SetSDA;DelayXus(1);
	IICA_ClrSCL;DelayXus(5);
	IICA_SetSCL;
}

unsigned char IIC_HYM8564_RecvAck(void){

	unsigned char ack = 0;
	unsigned int a = 0;
	
	IICA_ClrSCL;DelayXus(1);
	IICA_SetSCL;DelayXus(1);
	
	while((IICA_GetSDA) && (a < 2000))a++;
	
	ack = IICA_GetSDA; DelayXus(1);
	
	IICA_ClrSCL;
	
	return ack;
}

void IIC_HYM8564_SendByte(unsigned char dat){

	int i;
	
	IICA_ClrSCL;DelayXus(1);
	
	for (i = 7; i >= 0; i--){
	
		IICA_ClrSCL;DelayXus(1);
		IICA_ClrSDA;DelayXus(1);
		
		if (((dat >> i) & 0x01) != 0){
		
			IICA_SetSDA;
			DelayXus(1);
		}else{
		
			IICA_ClrSDA;
			DelayXus(1);
		}       
		IICA_SetSCL;
		DelayXus(1);
	}
	
	IICA_ClrSCL;
	DelayXus(1);
	return;
}

unsigned char IIC_HYM8564_ReadByte(void){  

    int i;
    unsigned char recv = 0;
	
    IICA_ClrSCL;DelayXus(1);
	
    for(i = 7; i >= 0; i --) {  
    
        recv <<= 1;
		IICA_SetSCL;DelayXus(1);
		
   		if(IICA_GetSDA)recv |= 0x01;	
		else{
		
			recv &= 0xfe;
		}
			
		IICA_ClrSCL;
		DelayXus(1);
    }
	
    return recv;
}

unsigned char read_HYM8564_byte(unsigned char add){

	unsigned char dat	= 0,
				  temp	= 0;

	IIC_HYM8564_Start();
	IIC_HYM8564_SendByte(HYM8564_ADD_WR);

	temp = IIC_HYM8564_RecvAck();

	IIC_HYM8564_SendByte(add);

	temp = IIC_HYM8564_RecvAck();

	IIC_HYM8564_Start();
	IIC_HYM8564_SendByte(HYM8564_ADD_RD);

	temp = IIC_HYM8564_RecvAck();
	dat	 = IIC_HYM8564_ReadByte();

	IIC_HYM8564_Stop();

	return(dat);
}

void write_HYM8564_byte(unsigned char add,unsigned char dat){

	unsigned char temp = 0;
	
	IIC_HYM8564_Start();
	IIC_HYM8564_SendByte(HYM8564_ADD_WR);
	
	temp = IIC_HYM8564_RecvAck();
	
	IIC_HYM8564_SendByte(add);
	
	temp = IIC_HYM8564_RecvAck();
	
	IIC_HYM8564_SendByte(dat);
	
	temp = IIC_HYM8564_RecvAck();
	
	IIC_HYM8564_Stop();
}

u8 DtoBCD(u8 num){
	
  return ((num / 10) << 4) + num % 10;
}

u8 BCDtoD(u8 num){
	
  return ((num & 0x0f) + ((num >> 4) * 10));
}

void timeSet(stt_Time timeDats){

	write_HYM8564_byte(registerADDR_HYM8564_YEAR, 	DtoBCD(timeDats.time_Year));
	write_HYM8564_byte(registerADDR_HYM8564_MONTH, 	DtoBCD(timeDats.time_Month));
	/*������жϣ������ܻᱻ���0*///��ֵΪ0�򱣳�ԭ��
	if(timeDats.time_Week)write_HYM8564_byte(registerADDR_HYM8564_WEEK, DtoBCD(timeDats.time_Week));		
	write_HYM8564_byte(registerADDR_HYM8564_DAY, 	DtoBCD(timeDats.time_Day));
	write_HYM8564_byte(registerADDR_HYM8564_HOUR, 	DtoBCD(timeDats.time_Hour));
	write_HYM8564_byte(registerADDR_HYM8564_MINUTE, DtoBCD(timeDats.time_Minute));
	if(timeDats.time_Second)write_HYM8564_byte(registerADDR_HYM8564_SECOND, DtoBCD(timeDats.time_Second)); //��ֵΪ0�򱣳�
}

void timeRead(stt_Time *timeDats){

	timeDats->time_Year 	= BCDtoD(read_HYM8564_byte(registerADDR_HYM8564_YEAR));
	timeDats->time_Month	= BCDtoD(read_HYM8564_byte(registerADDR_HYM8564_MONTH));
	timeDats->time_Week 	= BCDtoD(read_HYM8564_byte(registerADDR_HYM8564_WEEK));
	timeDats->time_Day 		= BCDtoD(read_HYM8564_byte(registerADDR_HYM8564_DAY));
	timeDats->time_Hour 	= BCDtoD(read_HYM8564_byte(registerADDR_HYM8564_HOUR));
	timeDats->time_Minute 	= BCDtoD(read_HYM8564_byte(registerADDR_HYM8564_MINUTE));
	timeDats->time_Second 	= BCDtoD(read_HYM8564_byte(registerADDR_HYM8564_SECOND));
}

void time_Logout(stt_Time code timeDats){

	u8 xdata Log[80] 	= {0};
	
	sprintf(Log, 
	"\n>>===ʱ���===<<\n    20%d/%02d/%02d-W%d\n        %02d:%02d:%02d\n", 
			(int)timeDats.time_Year,
			(int)timeDats.time_Month,
			(int)timeDats.time_Day,
			(int)timeDats.time_Week,
			(int)timeDats.time_Hour,
			(int)timeDats.time_Minute,
			(int)timeDats.time_Second);
			
//	LogDats(Log, strlen(Log));
	uartObjWIFI_Send_String(Log, strlen(Log));
}

void HYM8564_Test(void){
	
	static u8 adjust_CNT = 0;

	stt_Time code timeSet_Tab1 = {18, 9, 1, 10, 17, 41, 55};
	
	stt_Time xdata 	valTime_Local = {0};
	
	timeSet(timeSet_Tab1);
	
	while(1){
	
		timeRead(&valTime_Local);
		
		time_Logout(valTime_Local);
		
		delayMs(1500);
		
		if(adjust_CNT < 8)adjust_CNT ++;
		else{
		
			adjust_CNT = 0;
			
			timeSet(timeSet_Tab1);
		}
	}
}

