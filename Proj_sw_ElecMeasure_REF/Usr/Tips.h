#ifndef __TIPS_H_
#define __TIPS_H_

#include "STC15Fxxxx.H"

#define PIN_BEEP	P32
#define PIN_LED		P25

#define tipsLED_statusON	0
#define tipsLED_statusOFF	1

typedef enum seg_tipsMode{

	segMode_null = 0,
	segMode_init,
	segMode_initCmp,
	segMode_touchOpen,
	segMode_touchClose,
	segMode_elecDetectStandby,
	segMode_elecDetect,
}enum_segTips;

typedef enum led_tipsMode{

	ledMode_null = 0,
	ledMode_init,
    ledMode_relayOpenIF,
	ledMode_smartConfig,
	ledMode_factory
}enum_ledTips;

typedef enum beepsMode{

	beepsMode_null = 0,
	beepsMode_Touch,
}enum_beeps;

extern enum_segTips dev_segTips;
extern enum_ledTips dev_ledTips;
extern enum_beeps   dev_beeps;
extern bit tipsBeep_INTFLG;
extern bit tipsSeg_INTFLG;
extern bit beeps_EN;

void segTips_Init(void);
void segTips_InitCmp(void);
void segTips_detectStandBy(void);
void segTips_touchOpen(void);
void segTips_touchClose(void);
void segTips_allDark(void);

void pinLed_Init(void);
void pinBeep_Init(void);
void powerTips(float tips_val);

void beepTips_s(u8 tones, u16 time, u8 vol);
void beeps(u8 num);

#endif