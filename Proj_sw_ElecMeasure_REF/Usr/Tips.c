#include "Tips.h"

#include "GPIO.h"
#include "delay.h"

enum_segTips dev_segTips = segMode_init; //状态机状态：seg状态指示
enum_ledTips dev_ledTips = ledMode_init;  //状态机状态：led状态指示
enum_beeps   dev_beeps	 = beepsMode_null; //状态机状态：蜂鸣器状态指示
bit tipsBeep_INTFLG = 0; //蜂鸣器状态中断切换标志（当前状态正在进行时被中断切换，再次回到当前状态时使状态及内部变量回到初始值）
bit tipsSeg_INTFLG = 0; //数码管状态中断切换标志 （当前状态正在进行时被中断切换，再次回到当前状态时使状态及内部变量回到初始值）
bit beeps_EN = 1; //蜂鸣器使能

#define spect_size 15
u8 code spect_len[spect_size] 	= {2,2,2,4,4,4,7,7,7,6,6,3,3};
u8 code spect[spect_size][8]	= {

	{3,1},//0
	{7,2},//1
	{3,3},//2
	{1,2,4,5},//3
	{5,4,3,2},//4
	{1,2,6,7},//5
	{5,5,6,4,3,1,5},//6
	{7,7,5,6,1,2,4},//7
	{6,6,2,3,1,4,7},//8
	{1,2,5,6,3,4},//9
	{5,4,3,3,4,3},//10
	{5,2,2},//11
	{7,4,4},//12
};

void pinLed_Init(void){

	//开漏
	P2M1	|= 0x20;
	P2M0	|= 0x20;
	
	//推挽
	P2M1	&= ~(0xAF);
	P2M0	|=   0xAF;
}

void pinBeep_Init(void){

	P3M1	&= ~(0x04);
	P3M0	|= 0x04;
	
	beepTips_s(3, 3, 8);
	delayMs(50);
	beepTips_s(3, 3, 8);
}

void beeps_delay(u16 cnt){

	while(-- cnt){
		
		NOP7();
	}
}

void powerTips(float tips_val){

	P2 |= 0x8F;
	if(tips_val > 15.0F) P20 = 0;
	if(tips_val > 100.0F)P21 = 0;
	if(tips_val > 200.0F)P22 = 0;
	if(tips_val > 300.0F)P23 = 0;
	if(tips_val > 400.0F)P27 = 0;
}

void seg_tipsShow(u8 dats){

	(dats & 0x01)?(P20 = 0):(P20 = 1);
	(dats & 0x02)?(P21 = 0):(P21 = 1);
	(dats & 0x04)?(P22 = 0):(P22 = 1);
	(dats & 0x08)?(P23 = 0):(P23 = 1);
	(dats & 0x10)?(P27 = 0):(P27 = 1);
}

void segTips_Init(void){

	static u8 tips_data = 0;
	static u8 loop = 0;
	static bit tips_flg = 0;
	
	if(loop > 6){loop = 0; tips_flg = !tips_flg;}
	else{
	
		loop ++;
		
		if(!tips_flg)tips_data |= 1;

		seg_tipsShow(tips_data);
		
		tips_data <<= 1;
	}
}

void segTips_InitCmp(void){

	static xdata u8 loop = 0;
	static bit tips_flg = 0;
	
	if(loop > 16){
	
		loop = 0;
		seg_tipsShow(0x00);
		dev_segTips = segMode_elecDetect;
	}
	else{
	
		loop ++;
		
		if((loop % 2) == 0)tips_flg = !tips_flg;
		(tips_flg)?(seg_tipsShow(0x00)):(seg_tipsShow(0xFF));
	}
}

void segTips_detectStandBy(void){

	static u8 tips_data = 0;
	static u8 loop = 0;
	static bit tips_flg = 0;
	
	if(tipsSeg_INTFLG){
	
		tipsSeg_INTFLG = 0;
		
		loop = 0;
		tips_flg = 0;
		tips_data = 0;
	}
	
	if(loop > 6){loop = 0; tips_flg = !tips_flg;}
	else{
	
		loop ++;
		
		seg_tipsShow(tips_data);
		
		if(!tips_flg){
		
			tips_data <<= 1;
			tips_data |= 1;
			
		}else{
			
			tips_data >>= 1;
		}
	}
}

void segTips_touchOpen(void){

	static u8 tips_data = 0;
	static u8 loop = 0;
	
	if(tipsSeg_INTFLG){
	
		tipsSeg_INTFLG = 0;
		
		loop = 0;
		tips_data = 0;
	}
	
	if(loop > 6){
		
		loop = 0; 
		dev_segTips = segMode_elecDetect;
		seg_tipsShow(0);
		
	}
	else{
		
		loop ++;
		
		seg_tipsShow(tips_data);
		tips_data <<= 1;
		tips_data |= 1;
	}
}

void segTips_touchClose(void){

	static u8 tips_data = 0x1f;
	static u8 loop = 0;
	
	if(tipsSeg_INTFLG){
	
		tipsSeg_INTFLG = 0;
		
		loop = 0;
		tips_data = 0x1f;
	}
	
	if(loop > 6){
		
		loop = 0; 
		seg_tipsShow(0);
	}
	else{
		
		loop ++;
		
		seg_tipsShow(tips_data);
		tips_data >>= 1;
	}
}

void segTips_allDark(void){

	seg_tipsShow(0);
}

void beepTips(u8 tones, u16 time, u8 vol){	//音调， 时长， 音量

	const u8 vol_fa = 10;
	u16 tones_base  = tones * 50 + 100;
	u16 cycle		= time * 1000 / tones_base;
	u16 loop;
	
	for(loop = 0; loop < cycle; loop ++){
	
		if(beeps_EN)PIN_BEEP = 0;
		beeps_delay(tones_base / vol_fa * vol);
		PIN_BEEP = 1;
		beeps_delay(tones_base / vol_fa * (vol_fa - vol));
	}
}

void beepTips_s(u8 tones, u16 time, u8 vol){	//音调， 时长*100， 音量

	u8 loop = time;
	
	for(loop = time; loop != 0; loop --)beepTips(tones, 50, vol);
}

void beeps(u8 num){

	u8 loop;
	
	for(loop = 0; loop < spect_len[num]; loop ++)
		beepTips_s(spect[num][loop], 4, 8);
}