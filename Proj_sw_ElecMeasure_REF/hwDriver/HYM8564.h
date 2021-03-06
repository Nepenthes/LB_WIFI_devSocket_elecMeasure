#ifndef __HYM8564_H_
#define __HYM8564_H_

#include "STC15Fxxxx.H"

#define IIC_hym8564_SDA	P16
#define IIC_hym8564_SCL	P17

#define IIC_Delay_GainFac	2

#define IICA_SetSDA	(IIC_hym8564_SDA = 1)
#define IICA_ClrSDA	(IIC_hym8564_SDA = 0)
#define IICA_GetSDA	(IIC_hym8564_SDA)

#define IICA_SetSCL	(IIC_hym8564_SCL = 1)
#define IICA_ClrSCL	(IIC_hym8564_SCL = 0)

#define HYM8564_ADD_RD	0xa3
#define HYM8564_ADD_WR	0xa2

#define registerADDR_HYM8564_YEAR		0x08
#define registerADDR_HYM8564_MONTH		0x07
#define registerADDR_HYM8564_WEEK		0x06
#define registerADDR_HYM8564_DAY		0x05
#define registerADDR_HYM8564_HOUR		0x04
#define registerADDR_HYM8564_MINUTE		0x03
#define registerADDR_HYM8564_SECOND		0x02

typedef struct{

	u8 time_Year;
	u8 time_Month;
	u8 time_Week;
	u8 time_Day;
	u8 time_Hour;
	u8 time_Minute;
	u8 time_Second;
}stt_Time;

void iicHYM8564_pinInit(void);
void timeSet(stt_Time timeDats);
void timeRead(stt_Time *timeDats);
void time_Logout(stt_Time code timeDats);

void HYM8564_Test(void);

#endif