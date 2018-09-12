#ifndef __DATATRANS_H_
#define __DATATRANS_H_

#include "STC15Fxxxx.H"
#include "USART.h"

#define BAUD_WIFI	 115200UL   //���ڲ�����->WIFIģ��ͨѶ

#define rxBuff_WIFI	 RX1_Buffer	 

#define FRAME_HEAD_HEARTBEAT	0xAA
#define FRAME_TAIL_HEARTBEAT	0xAB
#define FRAME_HEAD_MOBILE		0xAA
#define FRAME_HEAD_SERVER		0xCC

#define dataTransLength_objMOBILE			33
#define dataTransLength_objSERVER			45
#define dataHeartBeatLength_objSERVER		20

#define FRAME_TYPE_MtoS_CMD					0xA0	/*��������*///�ֻ�������
#define FRAME_TYPE_StoM_RCVsuccess			0x0A	/*��������*///�������ֻ�
#define FRAME_TYPE_StoM_RCVfail				0x0C
#define FRAME_TYPE_StoM_upLoad				0x0D
#define FRAME_TYPE_StoM_reaptSMK			0x0E

#define FRAME_HEARTBEAT_cmdOdd				0x22	/*����*///����������
#define FRAME_HEARTBEAT_cmdEven				0x23	/*����*///ż��������

#define FRAME_MtoSCMD_cmdControl			0x10	/*����*///����
#define FRAME_MtoSCMD_cmdConfigSearch		0x09	/*����*///��������
#define FRAME_MtoSCMD_cmdQuery				0x11	/*����*///���ò�ѯ
#define FRAME_MtoSCMD_cmdInterface			0x15	/*����*///���ý���
#define FRAME_MtoSCMD_cmdReset				0x16	/*����*///��λ
#define FRAME_MtoSCMD_cmdLockON				0x17	/*����*///����
#define FRAME_MtoSCMD_cmdLockOFF			0x18	/*����*///����
#define FRAME_MtoSCMD_cmdswTimQuery			0x19	/*����*///��ͨ���ض�ʱ��ѯ
#define FRAME_MtoSCMD_cmdConfigAP			0x50	/*����*///AP����
#define FRAME_MtoSCMD_cmdBeepsON			0x1A	/*����*///����ʾ��
#define FRAME_MtoSCMD_cmdBeepsOFF			0x1B	/*����*///����ʾ��
#define FRAME_MtoSCMD_cmdftRecoverRQ		0x22	/*����*///��ѯ�Ƿ�֧�ָֻ�����
#define FRAME_MtoSCMD_cmdRecoverFactory		0x1F	/*����*///�ָ�����
#define FRAME_MtoSCMD_cmdCfg_swTim			0x14	/*����*///��ͨ���ض�ʱ

#define	cmdConfigTim_normalSwConfig			0xA0	/*����1*///��ͨ���ض�ʱ��ʶ-����2
#define cmdConfigTim_onoffDelaySwConfig		0xA1	/*����1*///��ʱ���ر�ʶ-����2
#define cmdConfigTim_closeLoopSwConfig		0xA2	/*����1*///ѭ���رձ�ʶ-����2

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