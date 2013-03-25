
/*************************@INCLUDE_START************************/
#include "mcu.h"
#include "vectors.h"
#include "can.h"
/**************************@INCLUDE_END*************************/

#define COER0_OE                CAN0_COER0_OE
#define CTRLR0_INIT             CAN0_CTRLR0_INIT
#define CTRLR0_CCE              CAN0_CTRLR0_CCE
#define CTRLR0_EIE              CAN0_CTRLR0_EIE
#define CTRLR0_IE               CAN0_CTRLR0_IE
#define BTR0                    CAN0_BTR0
#define IF1ARB0                 CAN0_IF1ARB0
#define IF2ARB0                 CAN0_IF2ARB0
#define IF1CMSK0                CAN0_IF1CMSK0
#define IF1CREQ0                CAN0_IF1CREQ0
#define IF1MCTR0                CAN0_IF1MCTR0
#define IF1MSK0                 CAN0_IF1MSK0
#define	IF1DTA10                CAN0_IF1DTA10
#define	IF1DTA20                CAN0_IF1DTA20
#define	IF1DTB10                CAN0_IF1DTB10
#define	IF1DTB20                CAN0_IF1DTB20
#define	IF2DTA10                CAN0_IF2DTA10
#define	IF2DTA20                CAN0_IF2DTA20
#define	IF2DTB10                CAN0_IF2DTB10
#define	IF2DTB20                CAN0_IF2DTB20
#define ERRCNT0                 CAN0_ERRCNT0
#define INTR0                   CAN0_INTR0
#define STATR0                  CAN0_STATR0
#define CTRLR0_SIE              CAN0_CTRLR0_SIE
#define IF2CMSK0                CAN0_IF2CMSK0
#define IF2CREQ0                CAN0_IF2CREQ0
#define IF2MCTR0                CAN0_IF2MCTR0

// Number of message buffers
#define MESSAGE_BUFFER_COUNT	32

// Masks for IFxCMSKn register
#define READ					0x00
#define WRITE					0x80

#define MASK					0x40
#define ARB						0x20
#define CONTROL					0x10

#define CLEAR_IRQ_PENDING		0x08
#define CLEAR_NEWDAT			0x04	// for read access
#define SET_TXREQ				0x04	// for write access

#define DATA_A					0x02
#define DATA_B					0x01

// Masks for IFxARB2n register
#define MSGVAL					0x80000000

#define EXT_ID					0x40000000
#define STD_ID					0x00000000

#define DIR_TX					0x20000000
#define DIR_RX					0x00000000

// Masks for IFxMCTRn register
#define UMASK					0x1000
#define TXIE					0x0800
#define RXIE					0x0400
#define RMTEN					0x0200
#define EOB						0x0080

// Masks for STATRn register
#define EPASS					0x0020
#define EWARN					0x0040
#define BOFF					0x0080

/*********************@GLOBAL_VARIABLES_START*******************/
/**********************@GLOBAL_VARIABLES_END********************/
STRUCT_CAN_ERROR stcError;
STRUCT_CAN_MESSAGE* apstcCANMessages[MESSAGE_BUFFER_COUNT];

/*******************@FUNCTION_DECLARATION_START*****************/
/*******************@FUNCTION_DECLARATION_END*******************/


/*********************@FUNCTION_HEADER_START*********************
*@FUNCTION NAME:    CAN_Init()                                  *
*                                                               *
*@DESCRIPTION:      Init CAN0                                   *
*                                                               *
*@PARAMETER:        none                                        *
*                                                               *
*@RETURN:           none                                        *
*                                                               *
***********************@FUNCTION_HEADER_END*********************/
void CAN_Init(void)
{
	int i;
	
	COER0_OE = 1;               // enable CAN0 output
	PIER03_IE4 = 1;             // enable CAN0 input
	
	CTRLR0_INIT = 1;
	CTRLR0_CCE  = 1;		// Enable configuration change
	
	BTR0 = CAN_BITRATE;		// Set baudrate
			
		
	// Disable all message boxes
	// Set MSGVAL = 0 (disable message buffer)
	//IF1ARB20 = 0;
	IF1ARB0 = 0;
	// Transfer MSGVAL bit (in IF1ARB20)
	IF1CMSK0 = WRITE | ARB;
	// Iterate over all available message buffers
	for (i=0; i<MESSAGE_BUFFER_COUNT; i++)
	{
		// Start transfer by setting message number
		IF1CREQ0 = i;
		apstcCANMessages[i] = null;
	}
	
	CTRLR0_EIE  = 1;		// Enable error change interrupt (only BOFF->1 and EWARN->1 is signaled)
	CTRLR0_IE   = 1;		// Enable module interrupts
	
	CTRLR0_CCE  = 0;		// Disable configuration change
	CTRLR0_INIT = 0;		// Normal operation
}

/*********************@FUNCTION_HEADER_START*********************
*@FUNCTION NAME:    CAN_SetupTransmitter()                      *
*                                                               *
*@DESCRIPTION:      Sets up a CAN message buffer for trans-     *
*                   mission. The transmission must be startet   *
*                   by CAN_UpdateTransmitter.                   *
*                                                               *
*@PARAMETER:        ucMessageNr - zero based message buffer     *
*                                 number (0 - 31)               *
*                   pstcID - pointer to CAN ID structure        *
*                                                               *
*@RETURN:           none                                        *
*                                                               *
***********************@FUNCTION_HEADER_END*********************/

void CAN_SetupTransmitter(unsigned char ucMessageNr, STRUCT_CAN_ID* pstcID)
{
	ucMessageNr = ucMessageNr & (MESSAGE_BUFFER_COUNT - 1);

	// Set ID
	if (pstcID->bExtID == true)
	{	
		IF1ARB0 = DIR_TX | EXT_ID | (pstcID->ulID & 0x1FFFFFFF);
		//IF1ARB10 = pstcID->ulID & 0xFFFF;
		//IF1ARB20 = DIR_TX | EXT_ID | ((pstcID->ulID >> 16) & 0x1FFF);
	}
	else
	{
		IF1ARB0 = DIR_TX | STD_ID | ((pstcID->ulID & 0x7FF) << 18);

		//IF1ARB10 = 0;
		//IF1ARB20 = DIR_TX | STD_ID | ((pstcID->ulID & 0x7FF) << 2);
	}
	// Set MSGVAL = 1 (enable message)
	//IF1ARB20 |= MSGVAL;
	IF1ARB0 |= MSGVAL;
	// Init message control register
	IF1MCTR0 = EOB;	
	
	// Transfer settings
	IF1CMSK0 = WRITE | ARB | CONTROL;
	// Start transfer by setting message number
	IF1CREQ0 = ucMessageNr + 1;
}

/*********************@FUNCTION_HEADER_START*********************
*@FUNCTION NAME:    CAN_UpdateTransmitter()                     *
*                                                               *
*@DESCRIPTION:      Updates the data of a transmit message      *
*                   buffer. The transmission can be started     *
*                   immediately (TX_CTRL_TRANSMIT) or after     *
*                   the receptin of a remote frame              *
*                   (TX_CTRL_REMOTE) - or both option.          *
*                                                               *
*@PARAMETER:        ucMessageNr - zero based message buffer     *
*                                 number (0 - 31)               *
*                   pstcData - pointer to data structure        *
*                   ucTxCtrl - transmission control (TX_CTRL_?) *
*                                                               *
*@RETURN:           none                                        *
*                                                               *
***********************@FUNCTION_HEADER_END*********************/

void CAN_UpdateTransmitter(unsigned char ucMessageNr, STRUCT_CAN_DATA* pstcData, unsigned char ucTxCtrl)
{
	unsigned int usData;
	unsigned char  ucDLC;

	ucMessageNr = ucMessageNr & (MESSAGE_BUFFER_COUNT - 1);

	// Set DLC and data
	ucDLC = pstcData->ucDLC;
	if (ucDLC > 8)
		ucDLC = 8;
	usData = 0;
	switch (ucDLC)
	{
		case 8:	usData = pstcData->aucData[7] << 8;
		case 7: IF1DTB20 = usData | pstcData->aucData[6];
		case 6:	usData = pstcData->aucData[5] << 8;
		case 5: IF1DTB10 = usData | pstcData->aucData[4];
		case 4:	usData = pstcData->aucData[3] << 8;
		case 3: IF1DTA20 = usData | pstcData->aucData[2];
		case 2:	usData = pstcData->aucData[1] << 8;
		case 1: IF1DTA10 = usData | pstcData->aucData[0];
		default:
			break;
	}
	
	// Set DLC
	IF1MCTR0 = EOB | ucDLC;
	// Common transfer settings (with transmission request)
	IF1CMSK0 = WRITE | CONTROL | DATA_A | DATA_B;
	
	// If transmission request is to set
	if ((ucTxCtrl & TX_CTRL_TRANSMIT) != 0)
	{
		// Add transmission request to command mask register
		IF1CMSK0 |= SET_TXREQ;
	}
	
	// If remote frame processing is to enable
	if ((ucTxCtrl & TX_CTRL_REMOTE) != 0)
	{
		// Enable remote frame processing
		IF1MCTR0 |= RMTEN;
	}
		
	// Start transfer by setting message number
	IF1CREQ0 = ucMessageNr + 1;
}

/*********************@FUNCTION_HEADER_START*********************
*@FUNCTION NAME:    CAN_SetupReceiver()                         *
*                                                               *
*@DESCRIPTION:      Sets up a message buffer for receiving      *
*                                                               *
*@PARAMETER:        ucMessageNr - zero based message buffer     *
*                                 number (0 - 31)               *
*                   pstcID - pointer to CAN ID structure        *
*                   ulIDMask - ID mask (bit = 0 -> mask)        *
*                   pstcMessage - pointer to CAN message        *
*                                 structure to for received     *
*                                 data.                         *
*                                                               *
*@RETURN:           none                                        *
*                                                               *
***********************@FUNCTION_HEADER_END*********************/

void CAN_SetupReceiver(unsigned char ucMessageNr, STRUCT_CAN_ID* pstcID, unsigned long ulIDMask, STRUCT_CAN_MESSAGE* pstcMessage)
{
	ucMessageNr = ucMessageNr & (MESSAGE_BUFFER_COUNT - 1);

	// Save pointer to message structure
	apstcCANMessages[ucMessageNr] = pstcMessage;

	// Set ID and mask
	if (pstcID->bExtID == true)
	{	
		IF1ARB0 = DIR_RX | EXT_ID | (pstcID->ulID & 0x1FFFFFFF);
		//IF1ARB10 = pstcID->ulID & 0xFFFF;
		//IF1ARB20 = DIR_RX | EXT_ID | ((pstcID->ulID >> 16) & 0x1FFF);
		IF1MSK0 = ulIDMask & 0x1FFFFFFF;
		//IF1MSK10 = ulIDMask & 0xFFFF;
		//IF1MSK20 = (ulIDMask >> 16) & 0x1FFF;
	}
	else
	{
		IF1ARB0 = DIR_TX | STD_ID | ((pstcID->ulID & 0x7FF) << 18);
		//IF1ARB10 = 0;
		//IF1ARB20 = DIR_TX | STD_ID | ((pstcID->ulID & 0x7FF) << 2);
		
		IF1MSK0 = (ulIDMask & 0x7FF) << 18;
		//IF1MSK10 = 0;
		//IF1MSK20 = (ulIDMask & 0x7FF) << 2;
	}
	// Set MSGVAL = 1 (enable message)
	IF1ARB0 |= MSGVAL;
	
	// Set Control register (use mask, enable receive interrupt)
	IF1MCTR0 = UMASK | RXIE;		

	
	
	// Transfer settings
	IF1CMSK0 = WRITE | ARB | MASK | CONTROL;
	// Start transfer by setting message number
	IF1CREQ0 = ucMessageNr + 1;
}

STRUCT_CAN_ERROR CAN_GetErrorState(void)
{
  unsigned int usERRCNT = ERRCNT0;
  
  stcError.ucTxErrorCounter =  usERRCNT       & 0x00FF;
  stcError.ucRxErrorCounter = (usERRCNT >> 8) & 0x007F;
    
  return stcError;
}

/*********************@FUNCTION_HEADER_START*********************
*@FUNCTION NAME:    ISR_CAN0()                                  *
*                                                               *
*@DESCRIPTION:      Interrupt service routine for CAN0. Handles *
*                   transmission, reception and status change   *
*                   interrupts.                                 *
*                                                               *
*@PARAMETER:        none                                        *
*                                                               *
*@RETURN:           none                                        *
*                                                               *
***********************@FUNCTION_HEADER_END*********************/

__interrupt void ISR_CAN0(void)
{
	unsigned int usINTR0 = INTR0;
			
	if (usINTR0 == 0x8000)
	{
      // Status change interrupt -> cleared by reading STATR0
	  unsigned int usSTATR = STATR0;
	  bool bEnableSIE = true;
	  
      // If BOFF flag is set
	  if ((usSTATR & BOFF) != 0)
	    stcError.ucState = CAN_ERROR_BUS_OFF;
      // If EPASS flag is set
	  else if ((usSTATR & EPASS) != 0)
	    stcError.ucState = CAN_ERROR_PASSIVE;
      // If EWARN flag is set
	  else if ((usSTATR & EWARN) != 0)
	    stcError.ucState = CAN_ERROR_WARNING_LEVEL;
	  else
	  {
        stcError.ucState = CAN_ERROR_ACTIVE;
        bEnableSIE = false;
      }
      
      if (bEnableSIE == true)
      {
        // Enable error interrupt (everthing is signaled)
        CTRLR0_SIE = 1;
      }
      else
      {
        // Disable status change interrupt (-> only error are signaled)
      	CTRLR0_SIE = 0;
      }
	}
	else
	{
		unsigned char ucMessageNr = usINTR0 - 1;
				
		// Message buffer interrupt
		// -> read message and clear INTPD
		IF2CMSK0 = READ | MASK | ARB | CONTROL | CLEAR_IRQ_PENDING | CLEAR_NEWDAT | DATA_A | DATA_B;
		IF2CREQ0 = usINTR0;		// Set message number
				
		// If there is no receiver object for this message buffer -> return
		if (apstcCANMessages[ucMessageNr] == null)
			return;
			
		// Check, if the message structure is empty
		if (apstcCANMessages[ucMessageNr]->bNewRxData == false)
		{
			// Save message data in message structure
			// If received ID is extended (29 bit)
			if ((IF2ARB0 & EXT_ID) != 0)
			{
				apstcCANMessages[ucMessageNr]->stcID.ulID  = (unsigned long)(IF2ARB0 & 0x1FFFFFFF);
				//apstcCANMessages[ucMessageNr]->stcID.ulID  = (unsigned long)(IF2ARB20 & 0x1FFF) << 16;
				//apstcCANMessages[ucMessageNr]->stcID.ulID |=  IF2ARB10;
				apstcCANMessages[ucMessageNr]->stcID.bExtID = true;
			}
			else
			{
				apstcCANMessages[ucMessageNr]->stcID.ulID  = (IF2ARB0 & 0x1FFFFFFF) >> 18;
				//apstcCANMessages[ucMessageNr]->stcID.ulID  = (IF2ARB20 & 0x1FFF) >> 2;
				apstcCANMessages[ucMessageNr]->stcID.bExtID = false;
			}
			// Save DLC
			apstcCANMessages[ucMessageNr]->stcData.ucDLC = IF2MCTR0 & 0x000F;
			switch (apstcCANMessages[ucMessageNr]->stcData.ucDLC)
			{
				case 8:	apstcCANMessages[ucMessageNr]->stcData.aucData[7] = IF2DTB20 >> 8;
				case 7: apstcCANMessages[ucMessageNr]->stcData.aucData[6] = IF2DTB20;
				case 6:	apstcCANMessages[ucMessageNr]->stcData.aucData[5] = IF2DTB10 >> 8;
				case 5: apstcCANMessages[ucMessageNr]->stcData.aucData[4] = IF2DTB10;
				case 4:	apstcCANMessages[ucMessageNr]->stcData.aucData[3] = IF2DTA20 >> 8;
				case 3: apstcCANMessages[ucMessageNr]->stcData.aucData[2] = IF2DTA20;
				case 2:	apstcCANMessages[ucMessageNr]->stcData.aucData[1] = IF2DTA10 >> 8;
				case 1: apstcCANMessages[ucMessageNr]->stcData.aucData[0] = IF2DTA10; 
				default:
					break;
			}
			
			// Set new data flag
			apstcCANMessages[ucMessageNr]->bNewRxData = true;
		}
		else
		{
			// Set overflow flag
			apstcCANMessages[ucMessageNr]->bOverflow = true;
		}
	}
}
/********************@FUNCTION_DECLARATION_END******************/
