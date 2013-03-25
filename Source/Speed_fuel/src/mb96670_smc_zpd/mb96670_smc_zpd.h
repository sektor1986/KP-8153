/************************************************************************/
/*               (C) Fujitsu Microelectronics Europe GmbH               */
/*                                                                      */
/* The following software deliverable is intended for and must only be  */
/* used in an evaluation laboratory environment.                        */
/* It is provided without charge and therefore provided on an as-is     */
/* basis, subject to alterations.                                       */
/*                                                                      */
/* The software deliverable is to be used exclusively in connection     */
/* with FME products.                                                   */
/* In the event the software deliverable includes the use of open       */
/* source components, the provisions of the governing open source       */
/* license agreement shall apply with respect to such software          */
/* deliverable.                                                         */
/* FME does not warrant that the deliverables does not infringe any     */
/* third party intellectual property right (IPR). In the event that     */
/* the deliverables infringe a third party IPR it is the sole           */
/* responsibility of the customer to obtain necessary licenses to       */
/* continue the usage of the deliverable.                               */
/*                                                                      */
/* To the maximum extent permitted by applicable law FME disclaims all  */
/* warranties, whether express or implied, in particular, but not       */
/* limited to, warranties of merchantability and fitness for a          */
/* particular purpose for which the deliverable is not designated.      */
/*                                                                      */
/* To the maximum extent permitted by applicable law, FME’s liability   */
/* is restricted to intention and gross negligence.                     */
/* FME is not liable for consequential damages.                         */
/*                                                                      */
/* (V1.1)                                                               */
/************************************************************************/
/*----------------------------------------------------------------------------
  mb96300_smc_zpd.h
  
  Function prototypes, data structures and error codes for MB96300 stepper
  motor zero point detection software.
  
  The software uses the following ressources:
  - Stepper motor units (only the enabled SMC units)
  - Reload Timer 0 (ISR: ISR_RLT0, IRQ35)
  - Reload Timer 1 (no ISR)
  - ADC and configured channels (ISR: ISR_ADC, IRQ59)
  
  Integration:
  - add all files of folder mb96300_smc_zpd to your project
  - in vectors.c:
  	• include mb96300_smc_zpd.h
  	• set ISR_RLT0 to IRQ35
  	• set ISR_ADC to IRQ59
  	• set interrupt levels in ICR for IRQ35 and IRQ59
  	  (level ISR_ADC <= level ISR_RLT0)
  
  Usage of the software (see parameter description of functions below):
   - call SMC_ZPD_Init (set common settings for all stepper motors)
   - call SMC_ZPD_SetStepBlindingTime (to set "delay" time before sampling
     should start for each new electrical step)
   - call SMC_ZPD_SetStepSamplingTime (to set time when ADC sampling
     should be performed and to define the step width)
   - call SMC_ZPD_SetADCSampleCount (to set the number of samples to
     be recorded for each stepper motor unit during one step)
   - call SMC_ZPD_EnableSMC (for each desired stepper motor unit)
   - call SMC_ZPD_Start (to start zero point detection, runs via interrupts)
   - wait for callback function (set by SMC_ZPD_Init)
   - call SMC_ZPD_GetStallData (for each enabled stepper motor unit,
     to get stall data, e.g. stop quadrant, nr. of steps
    
  The ZPD can be stopped any time by SMC_ZPD_Stop.
    
  After stall detection or stop the enabled stepper motor units stop
  with output low.
  
  Before starting to drive the stepper motor(s), a connection check
  is executed. By measuring the voltage level on the pins SMC1Mn/SMC2Mn.
  The internal pull-up and external pull-down resistors
  realize a DC offset voltage on the stepper motor coil.
  If the measured voltage level is above/below ucMax/MinConnectionCheckLevel
  (in STRUCT_SMC_ZPD_SETTINGS), an error is detected.
  The stepper motor will not be used in this case. Please check the
  error state eStallError (STRUCT_STALL_DATA) after stall
  detection event occurs (also in error cases).
  
  The stall data structure (STRUCT_STALL_DATA) contains statistical
  information on min./max. and the last four form factor values
  of the last run. By running the ZPD with threshold = 0, the
  characteristics of the stepper motor that is used can be
  investigated. When setting a threshold value, the history
  buffer shows the last steps before stall and allows to better
  adjust the threshold value.  
  
  For developing purposes some test signals can be output to
  analyze the timing, stepping, sampling and stalling behaviour
  of each stepper motor. By calling the functions:
   - SMC_ZPD_SetTestSignal_StepBlindingTime
   - SMC_ZPD_SetTestSignal_StepSamplingTime
   - SMC_ZPD_SetTestSignal_ADCSampling
   - SMC_ZPD_SetTestSignal_StallDetected
  the test signal can be enabled/disabled.
    
  The stepping scheme which is used to drive the stepper
  motors is as follows (full steps, direction = 0/false):
  
          PWM1Pn | PWM1Mn | PWM2Pn | PWM2Mn
  Step 0:  H        L        Z        Z      ||
  Step 1:  Z        Z        H        L      ||
  Step 2:  L        H        Z        Z      ||
  Step 3:  Z        Z        L        H      \/

  The step numbers are the same that are given by the stall data
  structure (items ucCurrentStep and ucStallStep).

  31.07.09  1.00   JWa    small bugfix on reset of usStepCount

  29.07.09  1.00   JWa    Initial version (based on mb91460_smc_zpd v1.2.0.1)
-----------------------------------------------------------------------------*/

#ifndef __96670_SMC_ZPD_H__
#define __96670_SMC_ZPD_H__

#include "base_types.h"

// ----------------------------------------------------------------------------

// Error codes (return values of mb96300_smc_zpd functions)
typedef enum _ENUM_SMC_ZPD_ERRORS
{
	ZPD_ERROR_NO								= 0x00,	// no error
	ZPD_ERROR_NO_SMC_ENABLED					= 0x01,	// no stepper motor unit enabled
	ZPD_ERROR_ZPD_RUNNING						= 0x02,	// zero point detection is running
	ZPD_ERROR_ADC_SAMPLE_STEP_WIDTH_MISSMATCH	= 0x03,	// step width (sample part) is too short for nr. of AD samples
	ZPD_ERROR_PARAMETER_ZERO_VALUE				= 0x04,	// at least one parameter was 0 and must not be
	ZPD_ERROR_INVALID_SMC						= 0x05,	// the given stepper motor unit number is not supported
	ZPD_ERROR_ADC_SAMPLE_BUFFER_SIZE_TOO_LOW	= 0x06,	// ADC sample buffer size is too low
	ZPD_ERROR_ADC_SAMPLE_COUNT_TOO_LOW			= 0x07,	// ADC sample count (per SMC) is too low
	ZPD_ERROR_ADC_SAMPLE_COUNT_TOO_HIGH			= 0x08,	// ADC sample count (per SMC) is too high
	ZPD_ERROR_BLINDING_TIME_TOO_LOW				= 0x09,	// Blinding time is too low
	ZPD_ERROR_BLINDING_TIME_TOO_HIGH			= 0x0A,	// Blinding time is too high
	ZPD_ERROR_SAMPLING_TIME_TOO_LOW				= 0x0B,	// Sampling time is too low
	ZPD_ERROR_SAMPLING_TIME_TOO_HIGH			= 0x0C,	// Sampling time is too high

	ZPD_ERROR_CONNECTION_CHECK					= 0x10,	// connection error (voltage level sampled during connection check (aucConnectionCheck in STRUCT_STALL_DATA) is above ucMaxConnectionCheckLevel or below ucMinConnectionCheckLevel in STRUCT_SMC_ZPD_SETTINGS, motor is stopped immediately, stall data invalid)
	ZPD_ERROR_MAX_STEPS_REACHED					= 0x11	// emergency stop after usStepCount (STRUCT_STALL_DATA) reached usMaxSteps (STRUCT_SMC_ZPD_SETTINGS), stall data valid
} ENUM_SMC_ZPD_ERRORS;

// ----------------------------------------------------------------------------

typedef struct _STRUCT_SMC_ZPD_STALL_DATA
{
	unsigned long aulForm_History[4];		// form factor values of last 4 steps
	unsigned long ulForm_MaxDuringRun;		// max. form factor value that occurred during last run (or 0 if stalled after <5 steps)
	unsigned long ulForm_MinDuringRun;		// min. form factor value that occurred during last run (or 0xFFFFFFFF if stalled after <5 steps)
	ENUM_SMC_ZPD_ERRORS eStallError;		// indicates errors (ZPD_ERROR_??)
	unsigned char aucSMCZeroLine[2];		// idle voltage of measuring points 
	unsigned char aucConnectionCheck[2];	// measured value during connection check 
	unsigned short usStepCount;				// nr. of steps between start and stall
	unsigned char ucCurrentStep;			// current quadrant (0...3) where rotor is located
	unsigned char ucStallStep;				// quadrant (0...3) where stall event was detected
} STRUCT_SMC_ZPD_STALL_DATA;

typedef struct _STRUCT_SMC_ZPD_SETTINGS
{
	unsigned char aucADCChannel[4];	// ADC channel assignment for the four driving steps
	unsigned long ulThreshold;		// Threshold for stall detection (form factor must underflow this value to detect stall)
	unsigned short usMaxSteps;		// max. step count (full rotation range) -> used for emergency stop, if no stall is detected
	unsigned char ucMaxConnectionCheckLevel;	// max. ADC level that must be measured during connection check on SMC1Pn/SMC2Pn (int. pull-up resistor, SMC1Mn/SMC2Mn have int. pull-down resistor)
	unsigned char ucMinConnectionCheckLevel;	// min. ADC level that must be measured during connection check on SMC1Pn/SMC2Pn (int. pull-up resistor, SMC1Mn/SMC2Mn have int. pull-down resistor)
	unsigned char ucStartStep;		// start step 0...3 (i.e. start quadrant)
	bool bDirection;				// rotation direction
	unsigned char ucPostStallSteps;	// number of steps to be done after stall detection (inverted direction!)
} STRUCT_SMC_ZPD_SETTINGS;

typedef struct _STRUCT_SMC_ZPD_INFO
{
	unsigned long ulVersion;			// mb96300_smc_zpd version
	unsigned long aulParameter[10];		// compilation information of internal paramters
} STRUCT_SMC_ZPD_INFO;

typedef enum en_smc_mode
{   ///< Idle, no operation in progress (default after SmcZpd_Init)
    NormalDriving,  ///< normal driving i.e. micro-stepping is running
    Zpd             ///< ZPD operation is running
} en_smc_mode_t;

extern en_smc_mode_t        m_enSmcMode;

// Function prototypes

/*********************@FUNCTION_HEADER_START*********************
*@FUNCTION NAME:    SMC_ZPD_Init()                              *
*                                                               *
*@DESCRIPTION:      initialisation of reload timer 0 + 7, A/D   *
*                   converter, assignment of ADC sample buffer  *
*                   and callback function                       *
*                                                               *
*@PARAMETER:        pucADCSampleBuffer - ADC sample buffer (for *
*                                       adc samples of one step *
*                                       and for all enabled     *
*                                       stepper motor units)    *
*                                   size >= smc_units * samples *
*                   usADCSampleBufferSize - size of ADC sample  *
*                                           buffer in byte      *
*                   pCallbackFunction - pointer to callback fct.*
*                                       for stall event of all  *
*                                       enabled stepper motor   *
*                                       units                   *
*                                                               *
*@RETURN:           Error code (ENUM_SMC_ZPD_ERRORS)            *
*                                                               *
***********************@FUNCTION_HEADER_END*********************/
ENUM_SMC_ZPD_ERRORS SMC_ZPD_Init(unsigned char* pucADCSampleBuffer, unsigned short usADCSampleBufferSize, void (*pCallbackFunction)(void));

/*********************@FUNCTION_HEADER_START*********************
*@FUNCTION NAME:    SMC_ZPD_SetStepBlindingTime()               *
*                                                               *
*@DESCRIPTION:      sets the blinding time per electrical step. *
*                   The blinding time starts with each new el.  *
*                   step. During this time, no ADC sampling is  *
*                   performed. After this time is elapsed, the  *
*                   sampling time starts (with ADC sampling).   *
*                   blinding_time + sampling_time = step_time   *
*                                                               *
*@PARAMETER:        ulBlindingTimeUs - blinding time in µs      *
*                                                               *
*@RETURN:           Error code (ENUM_SMC_ZPD_ERRORS)            *
*                                                               *
***********************@FUNCTION_HEADER_END*********************/
ENUM_SMC_ZPD_ERRORS SMC_ZPD_SetStepBlindingTime(unsigned long ulBlindingTimeUs);

/*********************@FUNCTION_HEADER_START*********************
*@FUNCTION NAME:    SMC_ZPD_SetStepSamplingTime()               *
*                                                               *
*@DESCRIPTION:      sets the sampling time per electrical step. *
*                   The sampling time starts after the blinding *
*                   time is elapsed. During this time ADC       *
*                   sampling is performed.                      *
*                   blinding_time + sampling_time = step_time   *
* Attention: The sampling time must be long enough to sample    *
*            the configured number of ADC samples. SMC_ZPD_Start*
*            is checking this constraint.                       *
*                                                               *
*@PARAMETER:        ulSamplingTimeUs - sampling time in µs      *
*                                                               *
*@RETURN:           Error code (ENUM_SMC_ZPD_ERRORS)            *
*                                                               *
***********************@FUNCTION_HEADER_END*********************/
ENUM_SMC_ZPD_ERRORS SMC_ZPD_SetStepSamplingTime(unsigned long ulSamplingTimeUs);

/*********************@FUNCTION_HEADER_START*************************
*@FUNCTION NAME:    SMC_ZPD_SetADCSampleCount()                     *
*                                                                   *
*@DESCRIPTION:      sets the number of samples to record for one    *
*                   stepper motor unit during one electrical step   *
* Attention: The sampling time must be long enough to sample        *
*            the configured number of ADC samples. SMC_ZPD_Start is *
*            checking this constraint.                              *
*                                                                   *
*@PARAMETER:        usSamples - number of ADC samples               *
*                                                                   *
*@RETURN:           Error code (ENUM_SMC_ZPD_ERRORS)                *
*                                                                   *
***********************@FUNCTION_HEADER_END*************************/
ENUM_SMC_ZPD_ERRORS SMC_ZPD_SetADCSampleCount(unsigned short usSamples);

/*********************@FUNCTION_HEADER_START*********************
*@FUNCTION NAME:    SMC_ZPD_EnableSMC()                         *
*                                                               *
*@DESCRIPTION:      enables a stepper motor unit for the usage  *
*                   with the zero point detection and assigns   *
*                   analog channels for the four driving steps  *
*     Note: For stepper motor units which do not provide        *
*           analog channel connections with the stepper motor   *
*           unit pins, analog channels must be connected        *
*           externally. The application is responsible for the  *
*           correct initialization of these port pins to work as*
*           analog input.                                       *
*                                                               *
*@PARAMETER:        ucSMCNr - stepper motor unit number (0...5) *
*                   pstcSMCZPDSettings - SMC ZPD settings       *
*                                                               *
*@RETURN:           Error code (ENUM_SMC_ZPD_ERRORS)            *
*                                                               *
***********************@FUNCTION_HEADER_END*********************/
ENUM_SMC_ZPD_ERRORS SMC_ZPD_EnableSMC(unsigned char ucSMCNr, STRUCT_SMC_ZPD_SETTINGS* pstcSMCZPDSettings);

/*********************@FUNCTION_HEADER_START*********************
*@FUNCTION NAME:    SMC_ZPD_DisableSMC()                        *
*                                                               *
*@DESCRIPTION:      disables a stepper motor unit for the usage *
*                   with the zero point detection               *
*                                                               *
*@PARAMETER:        ucSMCNr - stepper motor unit number (0...5) *
*                                                               *
*@RETURN:           Error code (ENUM_SMC_ZPD_ERRORS)            *
*                                                               *
***********************@FUNCTION_HEADER_END*********************/
ENUM_SMC_ZPD_ERRORS SMC_ZPD_DisableSMC(unsigned char ucSMCNr);

/*********************@FUNCTION_HEADER_START*********************
*@FUNCTION NAME:    SMC_ZPD_Start()                             *
*                                                               *
*@DESCRIPTION:      starts the zero point detection run with    *
*                   currently enabled stepper motor units (must *
*                   be done by calling SMC_ZPD_EnableSMC before)*
*                   When all enabled stepper motor units are    *
*                   stalled, the callback function set by       *
*                   SMC_ZPD_Init is called.                     *
*                                                               *
*@PARAMETER:        none                                        *
*                                                               *
*@RETURN:           Error code (ENUM_SMC_ZPD_ERRORS)            *
*                                                               *
*                                                               *
***********************@FUNCTION_HEADER_END*********************/
ENUM_SMC_ZPD_ERRORS SMC_ZPD_Start(void);

/*********************@FUNCTION_HEADER_START*********************
*@FUNCTION NAME:    SMC_ZPD_Stop()                              *
*                                                               *
*@DESCRIPTION:      stops the zero point detection run and      *
*                   sets the enabled SMC pins to output low.    *
*                                                               *
*@PARAMETER:        none                                        *
*                                                               *
*@RETURN:           no                                          *
*                                                               *
*                                                               *
***********************@FUNCTION_HEADER_END*********************/
void SMC_ZPD_Stop(void);

/*********************@FUNCTION_HEADER_START*********************
*@FUNCTION NAME:    SMC_ZPD_GetStallData()                      *
*                                                               *
*@DESCRIPTION:      Gets latest stall data. Can be called after *
*                   a stall is detected.                        *
*                                                               *
*@PARAMETER:        ucSMCNr - stepper motor unit number (0...5) *
*                   pstcStallData - pointer to stall data       *
*                                   structure to be filled      *
*                                   with the latest stall       *
*                                   information                 *
*                                                               *
*@RETURN:           Error code (ENUM_SMC_ZPD_ERRORS)            *
*                                                               *
***********************@FUNCTION_HEADER_END*********************/
ENUM_SMC_ZPD_ERRORS SMC_ZPD_GetStallData(unsigned char ucSMCNr, STRUCT_SMC_ZPD_STALL_DATA* pstcStallData);

/*********************@FUNCTION_HEADER_START*********************
*@FUNCTION NAME:    SMC_ZPD_SetTestSignal_???()                 *
*                                                               *
*@DESCRIPTION:      Test signal generation for marking the:     *
*                   - blinding time of each electrical step.    *
*                   - sampling time of each electrical step.    *
*                     (normally longer than ADC sampling itself)*
*                   - ADC sampling of a selected stepper motor  *
*                     unit (set by ucSMCNr)                     *
*                   - stall detected event of a selected stepper*
*                     motor unit (set by ucSMCNr)               *
*                                                               *
*                   These functions can be called before        *
*                   starting a ZPD run to set a port pin which  *
*                   should be toggled (active high).            *
*                                                               *
*  Attention: Be careful when setting the port output pointers. *
*             There is no safety check for overwriting memory   *
*             areas!                                            *
*                                                               *
*  Note: The port pins must be configured to output by the      *
*        application itself.                                    *
*                                                               *
*                                                               *
*@PARAMETER:        bEnable - enable/disable (def.) test signal *
*                   pio8Port - pointer to port output register  *
*                   ucBitMask - bit mask which sets/clears pin  *
*                                                               *
*@RETURN:           Error code (ENUM_SMC_ZPD_ERRORS)            *
*                                                               *
***********************@FUNCTION_HEADER_END*********************/
ENUM_SMC_ZPD_ERRORS SMC_ZPD_SetTestSignal_StepBlindingTime(bool bEnable, volatile __io __BYTE* pio8Port, unsigned char ucBitMask);
ENUM_SMC_ZPD_ERRORS SMC_ZPD_SetTestSignal_StepSamplingTime(bool bEnable, volatile __io __BYTE* pio8Port, unsigned char ucBitMask);
ENUM_SMC_ZPD_ERRORS SMC_ZPD_SetTestSignal_ADCSampling(unsigned char ucSMCNr, bool bEnable, volatile __io __BYTE* pio8Port, unsigned char ucBitMask);
ENUM_SMC_ZPD_ERRORS SMC_ZPD_SetTestSignal_StallDetected(unsigned char ucSMCNr, bool bEnable, volatile __io __BYTE* pio8Port, unsigned char ucBitMask);

/*********************@FUNCTION_HEADER_START*************************
*@FUNCTION NAME:    SMC_ZPD_SetFormThreshold()                      *
*                                                                   *
*@DESCRIPTION:      sets the form factor threshold value for a SMC  *
*                   unit                                            *
*                                                                   *
*@PARAMETER:        ucSMCNr - stepper motor unit number (0...5, 255)*
*                             255 = set for all units               *
*                   ulThreshold - form factor threshold value       *
*                                 (underflow causes stall detection)*
*                                                                   *
*@RETURN:           Error code (ENUM_SMC_ZPD_ERRORS)                *
*                                                                   *
***********************@FUNCTION_HEADER_END*************************/
ENUM_SMC_ZPD_ERRORS SMC_ZPD_SetFormThreshold(unsigned char ucSMCNr, unsigned long ulThreshold);

/*********************@FUNCTION_HEADER_START*********************
*@FUNCTION NAME:    SMC_ZPD_GetInformation()                    *
*                                                               *
*@DESCRIPTION:      Return information on the MB96300_SMC_ZPD   *
*                   software.                                   *
*                                                               *
*@PARAMETER:        pstcInfo - pointer to stucture of type      *
*                                    STRUCT_SMC_ZPD_INFO        *
*                                                               *
*@RETURN:           none                                        *
*                                                               *
***********************@FUNCTION_HEADER_END*********************/
void SMC_ZPD_GetInformation(STRUCT_SMC_ZPD_INFO* pstcInfo);

/*********************@FUNCTION_HEADER_START*********************
*@FUNCTION NAME:    ISR_RLT0()                                  *
*                                                               *
*@DESCRIPTION:      Interrupt service routine for the reload    *
*                   timer 0 (RLT0) for stepper motor driving.   *
*                   (must be put into interrupt vector table    *
*                   for IRQ35 and ICR must be set < 7)          *
*    ATTENTION: ICR level for ISR_ADC must be equal or          *
*               smaller than the level for ISR_RLT0!            *
*                                                               *
*@PARAMETER:        none                                        *
*                                                               *
*@RETURN:           none                                        *
*                                                               *
***********************@FUNCTION_HEADER_END*********************/
__interrupt void ISR_RLT2(void);

/*********************@FUNCTION_HEADER_START*********************
*@FUNCTION NAME:    ISR_ADC()                                   *
*                                                               *
*@DESCRIPTION:      Interrupt service routine for the ADC end   *
*                   interrupt for sampled adc data.             *
*                   (must be put into interrupt vector table    *
*                   for IRQ59 and ICR must be set < 7)          *
*    ATTENTION: ICR level for ISR_ADC must be equal or          *
*               smaller than the level for ISR_RLT0!            *
*                                                               *
*@PARAMETER:        none                                        *
*                                                               *
*@RETURN:           none                                        *
*                                                               *
***********************@FUNCTION_HEADER_END*********************/
__interrupt void ISR_ADC(void);

#endif	// __96300_SMC_ZPD_H__
