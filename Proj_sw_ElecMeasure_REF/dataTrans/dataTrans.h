#ifndef __DATATRANS_H_
#define __DATATRANS_H_

#include "STC15Fxxxx.H"
#include "USART.h"

#define BAUD_WIFI	 115200UL   //串口波特率->WIFI模块通讯

#define rxBuff_WIFI	 RX1_Buffer	 

#define FRAME_HEAD_HEARTBEAT	0xAA
#define FRAME_TAIL_HEARTBEAT	0xAB
#define FRAME_HEAD_MOBILE		0xAA
#define FRAME_HEAD_SERVER		0xCC

#define dataTransLength_objMOBILE			33
#define dataTransLength_objSERVER			45
#define dataHeartBeatLength_objSERVER		20

#define FRAME_TYPE_MtoS_CMD					0xA0	/*数据类型*///手机至开关
#define FRAME_TYPE_StoM_RCVsuccess			0x0A	/*数据类型*///开关至手机
#define FRAME_TYPE_StoM_RCVfail				0x0C
#define FRAME_TYPE_StoM_upLoad				0x0D
#define FRAME_TYPE_StoM_reaptSMK			0x0E

#define FRAME_HEARTBEAT_cmdOdd				0x22	/*命令*///奇数心跳包
#define FRAME_HEARTBEAT_cmdEven				0x23	/*命令*///偶数心跳包

#define FRAME_MtoSCMD_cmdControl			0x10	/*命令*///控制
#define FRAME_MtoSCMD_cmdConfigSearch		0x09	/*命令*///配置搜索
#define FRAME_MtoSCMD_cmdQuery				0x11	/*命令*///配置查询
#define FRAME_MtoSCMD_cmdInterface			0x15	/*命令*///配置交互
#define FRAME_MtoSCMD_cmdReset				0x16	/*命令*///复位
#define FRAME_MtoSCMD_cmdLockON				0x17	/*命令*///上锁
#define FRAME_MtoSCMD_cmdLockOFF			0x18	/*命令*///解锁
#define FRAME_MtoSCMD_cmdswTimQuery			0x19	/*命令*///普通开关定时查询
#define FRAME_MtoSCMD_cmdConfigAP			0x50	/*命令*///AP配置
#define FRAME_MtoSCMD_cmdBeepsON			0x1A	/*命令*///开提示音
#define FRAME_MtoSCMD_cmdBeepsOFF			0x1B	/*命令*///关提示音
#define FRAME_MtoSCMD_cmdftRecoverRQ		0x22	/*命令*///查询是否支持恢复出厂
#define FRAME_MtoSCMD_cmdRecoverFactory		0x1F	/*命令*///恢复出厂
#define FRAME_MtoSCMD_cmdCfg_swTim			0x14	/*命令*///普通开关定时

#define	cmdConfigTim_normalSwConfig			0xA0	/*数据1*///普通开关定时辨识-数据2
#define cmdConfigTim_onoffDelaySwConfig		0xA1	/*数据1*///延时开关辨识-数据2
#define cmdConfigTim_closeLoopSwConfig		0xA2	/*数据1*///循环关闭辨识-数据2

typedef struct{

	u8 rcvDats[COM_TX1_Lenth];
	u8 rcvDatsLen;
}uartTout_datsRcv;

typedef enum{

	DATATRANS_objFLAG_SERVER = 0,
	DATATRANS_objFLAG_MOBILE,
	HEARTBEAT_objFLAG_SERVER,
	dataTrans_obj_NULL,
}dataTrans_obj;

extern uartTout_datsRcv xdata datsRcv_ZIGB;
extern bit 				uartRX_toutFLG;

void uartObjWIFI_Init(void);
void rxBuff_WIFI_Clr(void);
void uartObjWIFI_Send_Byte(u8 dat);
void uartObjWIFI_Send_String(char *s,unsigned char ucLength);

void thread_dataTrans(void);
void thread_LocalStaus_Release(void);

#endif