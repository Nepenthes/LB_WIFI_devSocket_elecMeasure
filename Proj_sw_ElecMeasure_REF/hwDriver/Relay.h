#ifndef __RELAY_H_
#define __RELAY_H_

#include "STC15Fxxxx.H"

#define pinRELAY		P33

#define actRelay_ON		1
#define actRelay_OFF	0

typedef enum{

	swStatus_on = 0,
	swStatus_off,
	swStatus_null,
}switch_Status;

typedef enum{

	statusSave_enable = 1,
	statusSave_disable,
}status_ifSave;

typedef enum{

	relay_flip = 1,
	relay_on,
	relay_off,
}rly_methodType;

void relay_pinInit(void);
void relay_Act(rly_methodType act_Method);
void thread_Relay(void);

#endif