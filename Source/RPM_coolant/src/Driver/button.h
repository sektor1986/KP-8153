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
/** \file button.h
 **
 ** Basic functions for buttons using GPIO, featuring debouncing (no interrupts)
 ** The timer module is used (TIMER_ID_BUTTON). It must be initialized before
 ** and the Timer_Main function must be called cyclically (for callback function
 ** support).
 **
 ** History:
 **   - 2009-10-09  0.01  JWa  First version
 **   - 2009-11-16  0.02  JWa  Button_GetCurrentButtonState() added
 *****************************************************************************/

#ifndef __BUTTON_H__
#define __BUTTON_H__

/*****************************************************************************/
/* Include files                                                             */
/*****************************************************************************/
#include "base_types.h"

/*****************************************************************************/
/* Global pre-processor symbols/macros ('#define')                           */
/*****************************************************************************/
/** Button ID for button 1 */
#define BUTTON_ID_B1        ((uint16_t)0xA)
/** Button ID for button 2 */
#define BUTTON_ID_B2        ((uint16_t)0xB)        
/** Button ID for button 3 */
#define BUTTON_ID_B3        ((uint16_t)0xC)
/** Button ID for button 4 */
#define BUTTON_ID_B4        ((uint16_t)0xD)

/**
 ** Assignement of buttons to GPIO pins (GPIO port and appropriate pin number).
 ** Create a list as follows: { {port number, pin mask, button ID, idle state}, {port number, pin mask, button ID, idle state}, ... } */
#define BUTTON_ASSIGNMENT       {{13, 0x20, BUTTON_ID_B1, StateHigh},\
                                 {13, 0x10, BUTTON_ID_B2, StateHigh},\
                                 {13, 0x40, BUTTON_ID_B3, StateHigh}}
                                    
/** Set the scan interval for reading the pin state in milliseconds (20 ... 100) */
#define BUTTON_SCAN_INTERVAL_MS     20

/** Set debounce count i.e. number of scan intervals which must have the same state (1 ... 50) */
#define BUTTON_DEBOUNCE_COUNT       3

/*****************************************************************************/
/* Global type definitions ('typedef')                                       */
/*****************************************************************************/
typedef enum en_button_state
{
    StateInvalid,
    StateLow,
    StateHigh,
    StateLong
} en_button_state_t;

/**
 ** function pointer type to button callback function
 ** \param u16ButtonId  ID of button that changed its state (BUTTON_ID_???)
 ** \param enState      new state of button
 ** \return none */
typedef void (*button_callback_func_ptr_t)(uint16_t u16ButtonId, en_button_state_t enState);

/*****************************************************************************/
/* Global variable declarations ('extern', definition in C source)           */
/*****************************************************************************/

/*****************************************************************************/
/* Global function prototypes ('extern', definition in C source)             */
/*****************************************************************************/

extern void Button_Init(button_callback_func_ptr_t pfnCallback);
extern en_button_state_t Button_GetCurrentButtonState(uint16_t u16ButtonId);
#endif /* __BUTTON_H__ */
