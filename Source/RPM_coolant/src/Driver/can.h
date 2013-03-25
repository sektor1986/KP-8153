#include "base_types.h"

#ifndef __CAN_H__
#define __CAN_H__

// CAN bitrate settings for CLKP2 = 16 MHz
#define CAN_BITRATE_20_k		0x7FDF
#define CAN_BITRATE_50_k		0x6BCF
#define CAN_BITRATE_100_k		0x5CC7
#define CAN_BITRATE_125_k		0x49C7
#define CAN_BITRATE_250_k		0x49C3
#define CAN_BITRATE_500_k		0x49C1
#define CAN_BITRATE_800_k		0x6BC0
#define CAN_BITRATE_1000_k		0x49C0

#define CAN_BITRATE             CAN_BITRATE_250_k   // <<< set the bitrate to be used (CAN_BITRATE_???)

// CAN transmission control
#define TX_CTRL_TRANSMIT		0x01	// immediately transmit message
#define TX_CTRL_REMOTE			0x02	// transmit after reception of a remote frame

// CAN error states (STRUCT_CAN_ERROR.ucState)
#define CAN_ERROR_ACTIVE			0
#define CAN_ERROR_WARNING_LEVEL		1
#define CAN_ERROR_PASSIVE			2
#define CAN_ERROR_BUS_OFF			3

typedef struct _STRUCT_CAN_ID
{
	unsigned long ulID;
	bool          bExtID;
} STRUCT_CAN_ID;

typedef struct _STRUCT_CAN_DATA
{
	unsigned char ucDLC;
	unsigned char aucData[8];
} STRUCT_CAN_DATA;

typedef struct _STRUCT_CAN_MESSAGE
{
	STRUCT_CAN_ID   stcID;
	STRUCT_CAN_DATA stcData;
	volatile bool bNewRxData;
	volatile bool bOverflow;
} STRUCT_CAN_MESSAGE;

typedef struct _STRUCT_CAN_ERROR
{
    unsigned char ucState;
    unsigned char ucTxErrorCounter;
    unsigned char ucRxErrorCounter;
} STRUCT_CAN_ERROR;

void CAN_Init(void);
void CAN_SetupTransmitter(unsigned char ucMessageNr, STRUCT_CAN_ID* pstcID);
void CAN_UpdateTransmitter(unsigned char ucMessageNr, STRUCT_CAN_DATA* pstcData, unsigned char ucTxCtrl);
void CAN_SetupReceiver(unsigned char ucMessageNr, STRUCT_CAN_ID* pstcID, unsigned long ulIDMask, STRUCT_CAN_MESSAGE* pstcMessage);
STRUCT_CAN_ERROR CAN_GetErrorState(void);
__interrupt void ISR_CAN0(void);

#endif	// __CAN_H__
