#ifndef __HW8012_H_
#define __HW8012_H_

#include "STC15Fxxxx.H"

#define TEST_LOG 			0

#define pin_funFP_Select	P01

//#define coefficient_Cur		6.9370F	//电流系数
//#define coefficient_Vol		0.2667F	//电压系数
//#define coefficient_Pow		3.2510F	//功率系数

/*接口数据线都用472电阻*/
#define coefficient_Cur		7.142860F	//电流系数
#define coefficient_Vol		0.357143F	//电压系数
#define coefficient_Pow		4.411065F	//功率系数

typedef struct{

	float ePower;
	float eVoltage;
	float eCurrent;
}elec_Param;

extern elec_Param xdata paramElec_Param;
extern float	  xdata elec_Consum;

/*计数值*/
extern float xdata pinFP_volXcurCNT;
extern float xdata pinFP_powerCNT;
extern float xdata pinFP_stdby_powerCNT;

/*定值*/
extern float xdata pinFP_volXcur;
extern float xdata pinFP_power;
extern float xdata pinFP_powerStdby;

void intMeasure_Init(void);
void elecParamLog_thread(void);

void floatVal_save(float val, bit reCord_IF);
float elecConsum_save_read(void);

#endif