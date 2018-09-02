#ifndef __USRKIN_H_
#define __USRKIN_H_

#include "STC15Fxxxx.H"

sbit Usr_key = P2^4;

typedef void *funKey_Callback(void);

typedef enum{

	press_Null = 1,
	press_Short,
	press_Long,
}keyCfrm_Type;

void usrKin_pinInit(void);

bit UsrKEYScan_oneShoot(void);

void UsrKEYScan(funKey_Callback funCB_Short, funKey_Callback funCB_LongA, funKey_Callback funCB_LongB);

void fun_kLongA(void);
void fun_kLongB(void);
void fun_kShort(void);

#endif