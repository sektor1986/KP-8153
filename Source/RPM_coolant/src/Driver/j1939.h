/******************************************************************************
 * $Id$ / $Rev$ / $Date$
 * $URL$
 *****************************************************************************/
/*               (C) Fujitsu Microelectronics Europe GmbH                    */
/*                                                                           */
/* The following software deliverable is intended for and must only be       */
/* used in an evaluation laboratory environment.                             */
/* It is provided without charge and therefore provided on an as-is          */
/* basis, subject to alterations.                                            */
/*                                                                           */
/* The software deliverable is to be used exclusively in connection          */
/* with FME products.                                                        */
/* In the event the software deliverable includes the use of open            */
/* source components, the provisions of the governing open source            */
/* license agreement shall apply with respect to such software               */
/* deliverable.                                                              */
/* FME does not warrant that the deliverables do not infringe any            */
/* third party intellectual property right (IPR). In the event that          */
/* the deliverables infringe a third party IPR it is the sole                */
/* responsibility of the customer to obtain necessary licenses to            */
/* continue the usage of the deliverable.                                    */
/*                                                                           */
/* To the maximum extent permitted by applicable law FME disclaims all       */
/* warranties, whether express or implied, in particular, but not            */
/* limited to, warranties of merchantability and fitness for a               */
/* particular purpose for which the deliverable is not designated.           */
/*                                                                           */
/* To the maximum extent permitted by applicable law, FME’s liability        */
/* is restricted to intention and gross negligence.                          */
/* FME is not liable for consequential damages.                              */
/*                                                                           */
/* (Disclaimer V1.2)                                                         */
/*****************************************************************************/
/** \file J1939.h
 **
 **
 ** History:
 *****************************************************************************/
#ifndef __J1939_H__
#define __J1939_H__

#include "can.h"

#define CAN_COUNT_PARAM			2

#define RPM						0
#define COOLANT_TEMP			1

typedef struct stc_can_control
{
	STRUCT_CAN_ID CanId;
	unsigned long CanMask;
	STRUCT_CAN_MESSAGE CanMessage;	
	unsigned long CounterPasses;
	unsigned long TimeUpdate;
	bool Available;	
} stc_can_cantrol_t;

typedef struct J1939_REC
{	
  /* j1939 attribute */
  unsigned long  ID; 			/* ID J1939         */
  unsigned long  mask; 		/* msk                  */
  unsigned long  timeUpdate;  /* time update params   */
} J1939_INFO;

void J1939_init(void);
extern stc_can_cantrol_t J1939CtrBufer[CAN_COUNT_PARAM];

#endif	// __J1939_H__
