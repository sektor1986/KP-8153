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
/** \file timer.c
 **
 ** Offers n 16 bit timers with a 1 ms resolution counting with main clock
 ** timer (16FX) or the main/sub osc stabilization wait timer (FR)
 ** (and its interrupt). Which timer will be used is determined by the 
 ** MCU_SERIES (defined in mcu.h).
 **
 ** After Timer_Init is called the application can start SW timers by
 ** calling Timer_Start. The timer ID which must be used to identifiy added
 ** SW timer must be defined in timer.h, starting from 0. The number of 
 ** SW timers must be set with TIMER_COUNT (last timer ID + 1).
 **
 ** The module serves the appropriate timer interrupt in the ISR_MainClockTimer
 ** Timer_Isr_MainClockTimer. This ISR will handle all enabled SW timers.
 ** 
 ** The function Timer_IsElapsed can be used to check, whether a SW timer
 ** is elapsed or not. Use Timer_Remaining to get the remaining time until
 ** a timer elapses. Timer_Wait can be used for synchronously wait for 
 ** a desired duration, including hardware watchdog clearing (if desired).
 **
 ** The timer module also supports callback functions to prevent polling 
 ** the timer status. To prevent long interrupt duration due to multiple
 ** callback functions, the function Timer_Main must be called cyclically
 ** within the application main loop. The callback functions are called from
 ** the Timer_Main function (in task level, outside interrupt handler).
 **
 ** History:
 **   - 2009-10-08  1.05  JWa  portation to new coding rules and file style
 **   - 2009-03-27  1.04  JWa  ISR_MainClockTimer added to timer.h
 **   - 2009-07-08  1.03  JWa  Timer_Disable added
 **   - 2008-03-06  1.02  JWa  correction on timer interval for 91460 series
 **   - 2008-02-29  1.01  JWa  Timer_Remaining added
 **   - 2008-02-20  1.00  JWa  Initial version
 **
 *****************************************************************************/

#ifndef __TIMER_H__
#define __TIMER_H__

/*****************************************************************************/
/* Include files                                                             */
/*****************************************************************************/

#include "base_types.h"


/*****************************************************************************/
/* Global pre-processor symbols/macros ('#define')                           */
/*****************************************************************************/
// Timer IDs (must start at 0 and be incremented by 1)
#define TIMER_ID_MAIN                   0
#define TIMER_ID_BUTTON                 1
#define TIMER_1S                        2
#define TIMER_60S                       3
#define TIMER_ID_J1939                  4

// Number of timer IDs (= last timer ID + 1)
#define TIMER_COUNT						5

/*****************************************************************************/
/* Global type definitions ('typedef')                                       */
/*****************************************************************************/

/*****************************************************************************/
/* Global variable declarations ('extern', definition in C source)           */
/*****************************************************************************/

/*****************************************************************************/
/* Global function prototypes ('extern', definition in C source)             */
/*****************************************************************************/
void Timer_Init(void);
void Timer_Disable(void);
void Timer_Start(uint8_t u8TimerID, uint16_t u16TimeMs, boolean_t bReload, func_ptr_t pfnCallback);
void Timer_Main(void);
uint16_t Timer_Remaining(uint8_t u8TimerID);
boolean_t Timer_IsElapsed(uint8_t u8TimerID, boolean_t bClearElapsedFlag);
void Timer_Wait(uint8_t u8TimerID, uint16_t u16TimeMs, boolean_t bServeHardwareWatchdog);
__interrupt void Timer_Isr_MainClockTimer(void);

#endif	/* __TIMER_H__ */
