#include "timer.h"
#include "mcu.h"

typedef struct struct_timer
{
    volatile uint16_t   u16Timers;
    uint16_t            u16Duration;
    func_ptr_t          pfnCallback;
    volatile boolean_t  bElapsed;
} struct_timer_t;

static struct_timer_t m_astcTimers[TIMER_COUNT];
static volatile uint_fast8_t fu8CallbackFunctionTrigger;

/**
 ******************************************************************************
 ** Initializes the main clock timer to run with a duration of 1 ms (interrupt)
 **
 ** \param none
 **
 ** \return none
 *****************************************************************************/
void Timer_Init(void)
{
	fu8CallbackFunctionTrigger = 0;
	// Set duration to 1 ms
	MCTCR_MCTI = 0x04;
	// Enable interrupt request
	MCTCR_MCTIE = 1;	
}

/**
 ******************************************************************************
 ** Disables the main clock timer and clears all pending interrupt flags.
 **
 ** \param none
 **
 ** \return none
 *****************************************************************************/
void Timer_Disable(void)
{
	// Disable interrupt request
	MCTCR_MCTIE = 0;
	// Clear interrupt flag
	MCTCR_MCTIF = 0; 
 }
 
 /**
 ******************************************************************************
 ** Starts a 16 bit timer, indexed by an ID.
 ** To use callback the callback function, it is mandatory to
 ** cyclically call the Timer_Main() function. The callback
 ** functions are not called directly out of the ISR!
 **
 ** \param u8TimerID   TIMER_ID_? (defined in timer.h)
 ** \param u16TimeMs   duration in ms
 ** \param bReload     reload the timer automatically
 ** \param pfnCallback Callback function (when timer is elapsed)
 **
 ** \return none
 *****************************************************************************/
 void Timer_Start(uint8_t u8TimerID, uint16_t u16TimeMs, boolean_t bReload, func_ptr_t pfnCallback)
{
    if (u8TimerID < TIMER_COUNT)
    {
        m_astcTimers[u8TimerID].u16Timers   = u16TimeMs;
        m_astcTimers[u8TimerID].u16Duration = (bReload == TRUE) ? u16TimeMs : 0;
        m_astcTimers[u8TimerID].bElapsed    = FALSE;
        m_astcTimers[u8TimerID].pfnCallback = pfnCallback;
    }
}

/**
 ******************************************************************************
 ** Function must be called cyclically to use callback functions. If there
 ** are no callback functions in use, there is no need to call Timer_Main().
 **
 ** \param none
 **
 ** \return none
 *****************************************************************************/
void Timer_Main()
{
    int_fast16_t fi16TimerId;
    static uint_fast8_t fu8PrevTrigger = 0;
    uint_fast8_t fu8Trigger = fu8CallbackFunctionTrigger;

    // If no function is to call
    if (fu8PrevTrigger == fu8Trigger)
        return;
    else
        fu8PrevTrigger = fu8Trigger;
       
    // Look for callback functions to call 
    for (fi16TimerId = 0; fi16TimerId < TIMER_COUNT; fi16TimerId++)
    {
        // If this timer is elapsed
        if (m_astcTimers[fi16TimerId].bElapsed == TRUE)
        {
            // Clear "Elapsed" flag before a possible long function call
            // (so it may be reset while calling)
            m_astcTimers[fi16TimerId].bElapsed = FALSE;
            // Call function
            if (m_astcTimers[fi16TimerId].pfnCallback != NULL)
            {
                m_astcTimers[fi16TimerId].pfnCallback();
            }
        }
    }
}

/**
 ******************************************************************************
 ** Returns the remaining time of a timer.
 **
 ** \param u8TimerID TIMER_ID_? (defined in timer.h)
 **
 ** \return uint16_t remaining time in milliseconds
 *****************************************************************************/
uint16_t Timer_Remaining(uint8_t u8TimerID)
{
    if (u8TimerID < TIMER_COUNT)
    {
        return m_astcTimers[u8TimerID].u16Timers;
    }

    return 0;
}


/**
 ******************************************************************************
 ** Checks, wether a timer is elapsed or not.
 **
 ** \param u8TimerID         TIMER_ID_? (defined in timer.h)
 ** \param bClearElapsedFlag clear elapsed flag yes/no <=> TRUE/FALSE
 **
 ** \return boolean_t timer is elapsed yes/no <=> TRUE/FALSE
 *****************************************************************************/
boolean_t Timer_IsElapsed(uint8_t u8TimerID, boolean_t bClearElapsedFlag)
{
    boolean_t bElapsed = FALSE;
    
    if (u8TimerID < TIMER_COUNT)
    {
        bElapsed = m_astcTimers[u8TimerID].bElapsed;
        // Before clearing, check the mirrored bElapsed, to avoid
        // clearing the real flag just in the case it was set in the meantime
        if ((bClearElapsedFlag == TRUE) && (bElapsed == TRUE))
        {
            m_astcTimers[u8TimerID].bElapsed = FALSE;
        }
    } 

    return bElapsed;
}

/**
 ******************************************************************************
 ** Starts a timer and waits synchronously until it is elapsed.
 **
 ** \param u8TimerID               TIMER_ID_? (defined in timer.h)
 ** \param u16TimeMs               duration in ms
 ** \param bServeHardwareWatchdog  serve hardware watchdog or not (if any)
 **
 ** \return none
 *****************************************************************************/
void Timer_Wait(uint8_t u8TimerID, uint16_t u16TimeMs, boolean_t bServeHardwareWatchdog)
{
    Timer_Start(u8TimerID, u16TimeMs, FALSE, NULL);
    while (Timer_IsElapsed(u8TimerID, TRUE) == FALSE)
    {

    }
}

/**
 ******************************************************************************
 ** ISR for Main Clock Timer
 **
 ** \param none
 **
 ** \return none
 *****************************************************************************/
 __interrupt void Timer_Isr_MainClockTimer(void)
{
    int_fast16_t fi16TimerId;

    // Countdown all running timers
    for (fi16TimerId = 0; fi16TimerId < TIMER_COUNT; fi16TimerId++)
    {
        if (m_astcTimers[fi16TimerId].u16Timers > 0)
        {
            m_astcTimers[fi16TimerId].u16Timers--;
        }
        else
        {
            m_astcTimers[fi16TimerId].bElapsed = TRUE;
            fu8CallbackFunctionTrigger++;

            // Check, wether to reload the timer or not
            if (m_astcTimers[fi16TimerId].u16Duration != 0)
            {
                m_astcTimers[fi16TimerId].u16Timers = m_astcTimers[fi16TimerId].u16Duration;
            }
        }
    }

	// Clear interrupt flag
	MCTCR_MCTIF = 0;

}
