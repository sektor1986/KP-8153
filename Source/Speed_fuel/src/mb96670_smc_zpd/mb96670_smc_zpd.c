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
  mb96300_smc_zpd.c

  (documentation, version and history -> see mb96300_smc_zpd.h)
-----------------------------------------------------------------------------*/

 
/*************************@INCLUDE_START************************/
#include "mcu.h"
#include "mb96670_smc_zpd.h"
#include "zpd_settings.h"
#include "smc.h"
#include "adc.h"

/**************************@INCLUDE_END*************************/

// Version (30.07.2009)
#define SMC_ZPD_VERSION		0x01020003	// = v1.2.0.3

/*********************@GLOBAL_VARIABLES_START*******************/

// Base addresses of some basic MB96300 registers (not always defined in mb96xxxx.h)
#define PWC(o)		(((volatile unsigned char*)0x05E0)[(o) * 10])
#define PWEC(o)		(((volatile unsigned char*)0x05E1)[(o) * 10])
#define PWS(o)		(((volatile unsigned short*)0x05E6)[(o) * 5])
#define ADER(o)		(((volatile unsigned char*)0x04D0)[(o) * 1])
	
// Masks for several bits in stepper motor registers
#define PWC_CE		((unsigned char)0x08)
#define PWEC_OE1	((unsigned char)0x01)
#define PWEC_OE2	((unsigned char)0x02)
#define PWS_BS		((unsigned short)0x4000)

// ----------------------------------------------------------------------------

// Defines and macros für SMC output settings
#define L			0x00	// = low level
#define H			0x01	// = high level
#define P			0x02	// = PWM
#define Z			0x04	// = tri-state

#define PM(p1, m1, p2, m2)		(PWS_BS | (((p2) & 0x7) << 11) | (((m2) & 0x7) << 8) | (((p1) & 0x7) << 3) | ((m1) & 0x7))

// ----------------------------------------------------------------------------

//  Macro to set reload value in [us]
#define SET_RLT2_US(ulIntervalUs)	TMR2 = (ulIntervalUs) >> 2
//#define SET_RLT2_US(ulIntervalUs)	TMR2 = (ulIntervalUs) >> 1

// ----------------------------------------------------------------------------

// Calculate length of floating averaging filter
#define ADC_FLOATING_AVERAGING_SIZE		(0x01 << ADC_FLOATING_AVERAGING_POW)

// ----------------------------------------------------------------------------

#define ADC_CALIBRATION_LENGTH			(0x01 << ADC_CALIBRATION_POW_2)		// = nr. of samples
#define ADC_CALIBRATION_STEP_WIDTH_US	((ADC_CALIBRATION_LENGTH + ADC_CALIBRATION_BLIND_LENGTH) * ADC_SAMPLING_INTERVALL_US * SMC_ZPD_MAX_STEPPER_UNITS + 500)

/*********************@GLOBAL_VARIABLES_START*******************/
static const unsigned short gausStepsPWS[]  = { PM(H, L, Z, Z), PM(Z, Z, H, L), PM(L, H, Z, Z), PM(Z, Z, L, H) };
static const unsigned char  gaucStepsPWEC[] = { PWEC_OE1, PWEC_OE2, PWEC_OE1, PWEC_OE2 };

// ----------------------------------------------------------------------------

static const struct _STRUCT_SMC_REGISTERS
{			
	volatile unsigned char* pucPWC;
	volatile unsigned char* pucPWEC;
	volatile unsigned short* pusPWS;
	volatile unsigned char* pucPUCR;
	__io unsigned char* pucPDR;
	volatile unsigned char* pucDDR;
	volatile unsigned char* pucPHDR;
	bool bPUCRUpperNibble;
} gstcSMCRegisters[SMC_ZPD_MAX_STEPPER_UNITS] = {
#if (SMC_ZPD_MAX_STEPPER_UNITS > 0)
													{ &PWC(0), &PWEC(0), &PWS(0), &PUCR08, &PDR08, &DDR08, &PHDR08, false },
#endif
#if (SMC_ZPD_MAX_STEPPER_UNITS > 1)
													{ &PWC(1), &PWEC(1), &PWS(1), &PUCR08, &PDR08, &DDR08, &PHDR08, true },
#endif											
};

// ----------------------------------------------------------------------------

typedef union _UNION_AD_CHANNEL
{
	unsigned short usADSR;
	struct _STRUCT_ADSR
	{
		unsigned short b5ANE : 5;
		unsigned short b5ANS : 5;
		unsigned short b3CT  : 3;
		unsigned short b3ST  : 3;
	} stcADSR;
} UNION_AD_CHANNEL;

// ----------------------------------------------------------------------------

// Stepping states
typedef enum _ENUM_STEP_STATES
{
	STEP_STATE_START,
	STEP_STATE_RUN,
	STEP_STATE_STALL
} ENUM_STEP_STATES;

typedef struct _STRUCT_SMC
{
	STRUCT_SMC_ZPD_SETTINGS stcSettings;

	bool          bEnabled;
	unsigned char ucMeasuringSMCPair;

	unsigned char ucHoldCounter;
	unsigned char ucChangeDirectionCounter;
	unsigned char ucNextStep;
	ENUM_STEP_STATES eState;
	
	UNION_AD_CHANNEL unCurADCChannel;
	bool bStallDetected;

	bool bSample;
	unsigned char ucFillFormHistoryCounter;
	unsigned char* pucSampleBuffer;
} STRUCT_SMC;

static STRUCT_SMC gastcSMC[SMC_ZPD_MAX_STEPPER_UNITS];

// ----------------------------------------------------------------------------

typedef struct _STRUCT_TEST_SIGNALS
{
	struct _STRUCT_PORT_PIN
	{
		bool		   bEnable;
		volatile __io __BYTE* pio8Port;
		unsigned char  ucBitMask;
	} stcStepBlindingTime, stcStepSamplingTime, stcADCSampling[SMC_ZPD_MAX_STEPPER_UNITS], stcStallDetected[SMC_ZPD_MAX_STEPPER_UNITS];
} STRUCT_TEST_SIGNALS;

static STRUCT_TEST_SIGNALS gstcTestSignals;

// ----------------------------------------------------------------------------

// ADC states
typedef enum _ENUM_ADC_STATES
{
	ADC_STATE_START,
	ADC_STATE_CONNECTION_CHECK,
	ADC_STATE_CALIBRATION,
	ADC_STATE_ZPD
} ENUM_ADC_STATES;

typedef struct _STRUCT_ADC
{
	ENUM_ADC_STATES	eState;
	unsigned char* pucSampleBuffer;
	unsigned short usIndex;
	unsigned short usStopIndex;
	unsigned short usBufferSize;
	unsigned short usSampleCount;
	unsigned char  ucCurSMCIndex;
	UNION_AD_CHANNEL unADCFallbackChannel;
} STRUCT_ADC;

static STRUCT_ADC gstcADC;

// ----------------------------------------------------------------------------

static STRUCT_SMC_ZPD_STALL_DATA gastcStallData[SMC_ZPD_MAX_STEPPER_UNITS];

// ----------------------------------------------------------------------------
static struct _STRUCT_COMMON_CONTROL
{
	unsigned long	ulBlindingTimeUs;
	unsigned long	ulSamplingTimeUs;
	unsigned short	usEnabledSMCCount;
	unsigned char	ucConnectionCheckSteps;
	unsigned char	ucCalibrationSteps;
	bool			bStartBlindingTime;
	bool			bStartADC;
	bool  			bZPDRunning;
	void (*ptStallDetectedCallbackFunction)(void);
} gstcCommonCtrl;



/** SMC ZPD operation mode */
en_smc_mode_t        m_enSmcMode = Zpd;

/**********************@GLOBAL_VARIABLES_END********************/

/*******************@FUNCTION_DECLARATION_START*****************/
static void StartADCSampling(void);
static void EvaluateBEMFSignal(void);
/*******************@FUNCTION_DECLARATION_END*******************/

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
ENUM_SMC_ZPD_ERRORS SMC_ZPD_Init(unsigned char* pucADCSampleBuffer, unsigned short usADCSampleBufferSize, void (*pCallbackFunction)(void))
{
	int s;
	
	// CONFIGURE RELOAD TIMER 0 FOR STEPPER
//#if (PERIPHERAL_CLOCK_HZ == 16000000UL)
	// does not start it - CNTE = 0
    TMCSR2 = 0x0810;	// prescaler 1:2^6 (= 4 µs period @ CLKP = 16MHz, Interrupt enable)    
    //TMCSR2 = 0x0810;    //prescaler 1:2^6 (= 2 µs period @ CLKP = 32MHz, Interrupt enable) 
//#else
//	#error "Check reload timer 0 settings for PERIPHERAL_CLOCK_HZ != 16 MHz"
//#endif
    
	// ------------------------------------------------------------------------
	
	// CONFIGURE RELOAD TIMER 1 FOR ADC
//#if (PERIPHERAL_CLOCK_HZ == 16000000UL)
	// does not start it - CNTE = 0
	TMCSR1 = 0x1418;	// prescaler 1:2^3 (= 0.5 µs period @ CLKP = 16MHz), Interrupt disable)
	//TMCSR1 = 0x0418;	// prescaler 1:2^3 (= 0.5 µs period @ CLKP = 32MHz), Interrupt disable)
	// Set reload value according to sampling interval
	TMR1 = ADC_SAMPLING_INTERVALL_US - 1;
//#else
//	#error "Check reload timer 1 settings for PERIPHERAL_CLOCK_HZ != 16 MHz"
//#endif

	// ------------------------------------------------------------------------

	// CONFIGURE ADC
	// Set sampling and comparison time
	for (s = 0; s < SMC_ZPD_MAX_STEPPER_UNITS; s++)
	{
		gastcSMC[s].unCurADCChannel.stcADSR.b3ST = ADC_SAMPLING_TIME_SETTING;
		gastcSMC[s].unCurADCChannel.stcADSR.b3CT = ADC_COMPARISON_TIME_SETTING;
	}
	gstcADC.unADCFallbackChannel.stcADSR.b3ST = ADC_SAMPLING_TIME_SETTING;
	gstcADC.unADCFallbackChannel.stcADSR.b3CT = ADC_COMPARISON_TIME_SETTING;

	ADCSH_INTE = 1;		// Enable ADC end interrupt
	ADCSH_STS  = 2;		// trigger by RLT7
	ADCSL_MD   = 0;		// single shot
	ADCSL_S10  = 1;		// 8 bit resolution
	
	//ADC triger
	ADTGCRH0_RLTE = 1;  //Triggering by RLT is enabled 
	ADTGISEL0_RLT0 = 1; //RLT 1 selected 

	// ------------------------------------------------------------------------

	// SET PARAMETERS
	gstcCommonCtrl.ptStallDetectedCallbackFunction = pCallbackFunction;

	gstcADC.pucSampleBuffer = pucADCSampleBuffer;
	gstcADC.usBufferSize = usADCSampleBufferSize;
	gstcADC.usSampleCount = ADC_SAMPLE_COUNT_DEFAULT;

	// disable all SMC units
	for (s = 0; s < SMC_ZPD_MAX_STEPPER_UNITS; s++)
		gastcSMC[s].bEnabled = false;
			
	gstcCommonCtrl.ulBlindingTimeUs  = STEP_BLINDING_TIME_US_DEFAULT;
	gstcCommonCtrl.ulSamplingTimeUs  = STEP_SAMPLING_TIME_US_DEFAULT;
	gstcCommonCtrl.usEnabledSMCCount = 0;
	gstcCommonCtrl.bZPDRunning = false;

	return ZPD_ERROR_NO;
}

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
ENUM_SMC_ZPD_ERRORS SMC_ZPD_SetStepBlindingTime(unsigned long ulBlindingTimeUs)
{
	// If ZPD is running -> exit
	if (gstcCommonCtrl.bZPDRunning == true)
		return ZPD_ERROR_ZPD_RUNNING;
		
	if (ulBlindingTimeUs < STEP_BLINDING_TIME_US_MIN)
		return ZPD_ERROR_BLINDING_TIME_TOO_LOW;
	if (ulBlindingTimeUs > STEP_BLINDING_TIME_US_MAX)
		return ZPD_ERROR_BLINDING_TIME_TOO_HIGH;
		
	// set new blinding time
	gstcCommonCtrl.ulBlindingTimeUs = ulBlindingTimeUs;
	
	return ZPD_ERROR_NO;
}

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
ENUM_SMC_ZPD_ERRORS SMC_ZPD_SetStepSamplingTime(unsigned long ulSamplingTimeUs)
{
	// If ZPD is running -> exit
	if (gstcCommonCtrl.bZPDRunning == true)
		return ZPD_ERROR_ZPD_RUNNING;
		
	if (ulSamplingTimeUs < STEP_SAMPLING_TIME_US_MIN)
		return ZPD_ERROR_SAMPLING_TIME_TOO_LOW;
	if (ulSamplingTimeUs > STEP_SAMPLING_TIME_US_MAX)
		return ZPD_ERROR_SAMPLING_TIME_TOO_HIGH;
		
	// set new sampling time
	gstcCommonCtrl.ulSamplingTimeUs = ulSamplingTimeUs;
	
	return ZPD_ERROR_NO;
}

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
ENUM_SMC_ZPD_ERRORS SMC_ZPD_SetADCSampleCount(unsigned short usSamples)
{
	// If ZPD is running -> exit
	if (gstcCommonCtrl.bZPDRunning == true)
		return ZPD_ERROR_ZPD_RUNNING;
		
	if (usSamples < ADC_SAMPLE_COUNT_MIN)
		return ZPD_ERROR_ADC_SAMPLE_COUNT_TOO_LOW;
	if (usSamples > ADC_SAMPLE_COUNT_MAX)
		return ZPD_ERROR_ADC_SAMPLE_COUNT_TOO_HIGH;
		
	// Store value without check (limitation is done by size
	// information of gstcADC.pucSampleBuffer
	gstcADC.usSampleCount = usSamples;
	
	return ZPD_ERROR_NO;
}

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
ENUM_SMC_ZPD_ERRORS SMC_ZPD_EnableSMC(unsigned char ucSMCNr, STRUCT_SMC_ZPD_SETTINGS* pstcSMCZPDSettings)
{
	// If ZPD is running -> exit
	if (gstcCommonCtrl.bZPDRunning == true)
		return ZPD_ERROR_ZPD_RUNNING;
		
	// Check max. stepper number
	if (ucSMCNr >= SMC_ZPD_MAX_STEPPER_UNITS)
		return ZPD_ERROR_INVALID_SMC;
	
	gastcSMC[ucSMCNr].stcSettings = *pstcSMCZPDSettings;

	gastcSMC[ucSMCNr].ucNextStep = gastcStallData[ucSMCNr].ucCurrentStep = gastcStallData[ucSMCNr].ucStallStep = pstcSMCZPDSettings->ucStartStep % 4;
	
	gastcSMC[ucSMCNr].bEnabled = true;
	
	return ZPD_ERROR_NO;
}

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
ENUM_SMC_ZPD_ERRORS SMC_ZPD_DisableSMC(unsigned char ucSMCNr)
{
	// If ZPD is running -> exit
	if (gstcCommonCtrl.bZPDRunning == true)
		return ZPD_ERROR_ZPD_RUNNING;

	// Check max. stepper number
	if (ucSMCNr >= SMC_ZPD_MAX_STEPPER_UNITS)
		return ZPD_ERROR_INVALID_SMC;
	
	gastcSMC[ucSMCNr].bEnabled = false;
	
	return ZPD_ERROR_NO;
}

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
ENUM_SMC_ZPD_ERRORS SMC_ZPD_Start(void)
{
	unsigned int s, a;
	unsigned long ulMeasurementTimeUs;
	
	// If ZPD is running -> exit
	if (gstcCommonCtrl.bZPDRunning == true)
		return ZPD_ERROR_ZPD_RUNNING;
		
	// Check step sampling time > (ADC sampling interval * sample count + evaluating time) * max. SMC units
	// Note: signal evalution time = approx. 180 µs / SMC unit (worst case, no compiler optimization level, no __DEBUG__)
	ulMeasurementTimeUs = ((unsigned long)gstcADC.usSampleCount * (unsigned long)ADC_SAMPLING_INTERVALL_US + 180) * (unsigned long)SMC_ZPD_MAX_STEPPER_UNITS;
	if (ulMeasurementTimeUs >= gstcCommonCtrl.ulSamplingTimeUs)
		return ZPD_ERROR_ADC_SAMPLE_STEP_WIDTH_MISSMATCH;
		
	// Enable ZPD (to lock access to SMC configuration)
	gstcCommonCtrl.bZPDRunning = true;
	
	// Count enabled SMC units
	gstcCommonCtrl.usEnabledSMCCount = 0;
	for (s = 0; s < SMC_ZPD_MAX_STEPPER_UNITS; s++)
	{
		if (gastcSMC[s].bEnabled == true)
		{	
			// Enable stepper motor unit
			*gstcSMCRegisters[s].pusPWS = PM(Z, Z, Z, Z);
			*gstcSMCRegisters[s].pucPWC = PWC_CE;		// count enable (CE = 1)
			// Set pull-up resistors configuration
			// Circuit B:
			//  - SMC1Pn/SMC2Pn: int. pull-up, ext. pull-down
			//  - SMC1Mn/SMC2Mn: none
			if (gstcSMCRegisters[s].bPUCRUpperNibble == true)
			{
				*gstcSMCRegisters[s].pucPUCR |= 0xF0;		// pull-up on all pins
				*gstcSMCRegisters[s].pucDDR  &= 0x0F;		// set all pins to input
				//*gstcSMCRegisters[s].pucPHDR &= 0x0F;
			}
			else
			{
				*gstcSMCRegisters[s].pucPUCR |= 0x0F;		// pull-up on all pins
				*gstcSMCRegisters[s].pucDDR  &= 0xF0;		// set all pins to input
				//*gstcSMCRegisters[s].pucPHDR &= 0xF0;
			}
			*gstcSMCRegisters[s].pucPWEC = 0;				// disable all PWM outputs (to enable pull-up resistor)

			// Enable analog channels
			//for (a = 0; a < 4; a++)
			//{
			//	unsigned char ucADERIndex = gastcSMC[s].stcSettings.aucADCChannel[a] >> 3;
			//	unsigned char ucADERBitNr = gastcSMC[s].stcSettings.aucADCChannel[a] - (ucADERIndex << 3);
			//	ADER(ucADERIndex) |= 1 << ucADERBitNr;
			//}
			
			// Set ADC fallback channel (for start-up with a
			// disabled SMC channel, to ensure, that a channel
			// is set that is also enabled in ADER)
			gstcADC.unADCFallbackChannel.stcADSR.b5ANS = gstcADC.unADCFallbackChannel.stcADSR.b5ANE = gastcSMC[s].stcSettings.aucADCChannel[0];

			gstcCommonCtrl.usEnabledSMCCount++;
		}
	}
			
	// If no stepper motor unit enabled -> exit
	if (gstcCommonCtrl.usEnabledSMCCount == 0)
	{
		SMC_ZPD_Stop();
		return ZPD_ERROR_NO_SMC_ENABLED;
	}

	// Check, whether the size of the application sample buffer is sufficient for the enabled SMC units	
	if (   (gstcADC.usBufferSize < ((ADC_CALIBRATION_LENGTH + ADC_CALIBRATION_BLIND_LENGTH) * gstcCommonCtrl.usEnabledSMCCount))
	    || (gstcADC.usBufferSize < (gstcADC.usSampleCount * gstcCommonCtrl.usEnabledSMCCount)) )
	{
		SMC_ZPD_Stop();
		return ZPD_ERROR_ADC_SAMPLE_BUFFER_SIZE_TOO_LOW;
	}
			
	for (s=0; s<SMC_ZPD_MAX_STEPPER_UNITS; s++)
	{
		gastcStallData[s].eStallError = ZPD_ERROR_NO;
		gastcStallData[s].usStepCount = 0;

		gastcSMC[s].bStallDetected   = false;
		gastcSMC[s].eState           = STEP_STATE_START;
		gastcSMC[s].ucHoldCounter    = 0;
		gastcSMC[s].ucChangeDirectionCounter = 0;
	}

	// ADC start state
	gstcADC.eState = ADC_STATE_START;
	
	// Set interval for calibration
	SET_RLT2_US(ADC_CALIBRATION_STEP_WIDTH_US);

	// Enable and start reload timer
	TMCSR2_INTE = 1;
	TMCSR2_CNTE = 1;
	TMCSR2_TRG  = 1;

	return ZPD_ERROR_NO;
}

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
void SMC_ZPD_Stop(void)
{
	unsigned int s;
	
	// disable reload timer 0
	TMCSR2_CNTE = 0;
	TMCSR2_INTE = 0;
	
	// disable reload timer 1
	TMCSR1_CNTE = 0;
	TMCSR1_INTE = 0;
	
	// Switch SMC outputs to Low
	for (s = 0; s < SMC_ZPD_MAX_STEPPER_UNITS; s++)
	{
		if (gastcSMC[s].bEnabled == true)
		{
			*gstcSMCRegisters[s].pusPWS = PM(L, L, L, L);
			*gstcSMCRegisters[s].pucPWEC = PWEC_OE1 | PWEC_OE2;
		}
	}

	gstcCommonCtrl.bZPDRunning = false;
}

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
ENUM_SMC_ZPD_ERRORS SMC_ZPD_GetStallData(unsigned char ucSMCNr, STRUCT_SMC_ZPD_STALL_DATA* pstcStallData)
{
	// If ZPD is running -> exit
	if (gstcCommonCtrl.bZPDRunning == true)
		return ZPD_ERROR_ZPD_RUNNING;

	if (ucSMCNr >= SMC_ZPD_MAX_STEPPER_UNITS)
		return ZPD_ERROR_INVALID_SMC;
				
	*pstcStallData = gastcStallData[ucSMCNr];
	
	return ZPD_ERROR_NO;
}

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
ENUM_SMC_ZPD_ERRORS SMC_ZPD_SetTestSignal_StepBlindingTime(bool bEnable, volatile __io __BYTE* pio8Port, unsigned char ucBitMask)
{
	// If ZPD is running -> exit
	if (gstcCommonCtrl.bZPDRunning == true)
		return ZPD_ERROR_ZPD_RUNNING;

	gstcTestSignals.stcStepBlindingTime.bEnable   = bEnable;
	gstcTestSignals.stcStepBlindingTime.pio8Port  = pio8Port;
	gstcTestSignals.stcStepBlindingTime.ucBitMask = ucBitMask;
	
	return ZPD_ERROR_NO;
}

ENUM_SMC_ZPD_ERRORS SMC_ZPD_SetTestSignal_StepSamplingTime(bool bEnable, volatile __io __BYTE* pio8Port, unsigned char ucBitMask)
{
	// If ZPD is running -> exit
	if (gstcCommonCtrl.bZPDRunning == true)
		return ZPD_ERROR_ZPD_RUNNING;

	gstcTestSignals.stcStepSamplingTime.bEnable   = bEnable;
	gstcTestSignals.stcStepSamplingTime.pio8Port  = pio8Port;
	gstcTestSignals.stcStepSamplingTime.ucBitMask = ucBitMask;

	return ZPD_ERROR_NO;
}

ENUM_SMC_ZPD_ERRORS SMC_ZPD_SetTestSignal_ADCSampling(unsigned char ucSMCNr, bool bEnable, volatile __io __BYTE* pio8Port, unsigned char ucBitMask)
{
	// If ZPD is running -> exit
	if (gstcCommonCtrl.bZPDRunning == true)
		return ZPD_ERROR_ZPD_RUNNING;

	if (ucSMCNr >= SMC_ZPD_MAX_STEPPER_UNITS)
		return ZPD_ERROR_INVALID_SMC;

	gstcTestSignals.stcADCSampling[ucSMCNr].bEnable   = bEnable;
	gstcTestSignals.stcADCSampling[ucSMCNr].pio8Port  = pio8Port;
	gstcTestSignals.stcADCSampling[ucSMCNr].ucBitMask = ucBitMask;

	return ZPD_ERROR_NO;
}

ENUM_SMC_ZPD_ERRORS SMC_ZPD_SetTestSignal_StallDetected(unsigned char ucSMCNr, bool bEnable, volatile __io __BYTE* pio8Port, unsigned char ucBitMask)
{
	// If ZPD is running -> exit
	if (gstcCommonCtrl.bZPDRunning == true)
		return ZPD_ERROR_ZPD_RUNNING;

	if (ucSMCNr >= SMC_ZPD_MAX_STEPPER_UNITS)
		return ZPD_ERROR_INVALID_SMC;

	gstcTestSignals.stcStallDetected[ucSMCNr].bEnable   = bEnable;
	gstcTestSignals.stcStallDetected[ucSMCNr].pio8Port  = pio8Port;
	gstcTestSignals.stcStallDetected[ucSMCNr].ucBitMask = ucBitMask;

	return ZPD_ERROR_NO;
}

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
ENUM_SMC_ZPD_ERRORS SMC_ZPD_SetFormThreshold(unsigned char ucSMCNr, unsigned long ulThreshold)
{
	int s;

	// Check max. stepper number
	if ((ucSMCNr >= SMC_ZPD_MAX_STEPPER_UNITS) && (ucSMCNr != 255))
		return ZPD_ERROR_INVALID_SMC;

	if (ucSMCNr == 255)
	{
		for (s=0; s<SMC_ZPD_MAX_STEPPER_UNITS; s++)
		{
			gastcSMC[s].stcSettings.ulThreshold = ulThreshold;
		}
	}
	else
	{
		gastcSMC[ucSMCNr].stcSettings.ulThreshold = ulThreshold;
	}

	return ZPD_ERROR_NO;
}

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
void SMC_ZPD_GetInformation(STRUCT_SMC_ZPD_INFO* pstcInfo)
{
	pstcInfo->ulVersion = SMC_ZPD_VERSION;
	
	pstcInfo->aulParameter[0] = SMC_ZPD_MAX_STEPPER_UNITS;
	pstcInfo->aulParameter[1] = RESISTOR_CIRCUIT;
	pstcInfo->aulParameter[2] = ADC_SAMPLING_TIME_SETTING;
	pstcInfo->aulParameter[3] = ADC_COMPARISON_TIME_SETTING;
	pstcInfo->aulParameter[4] = ADC_SAMPLING_INTERVALL_US;
	pstcInfo->aulParameter[5] = ADC_FLOATING_AVERAGING;
	pstcInfo->aulParameter[6] = ADC_FLOATING_AVERAGING_POW;
	pstcInfo->aulParameter[7] = ADC_CALIBRATION_BLIND_LENGTH;
	pstcInfo->aulParameter[8] = ADC_CALIBRATION_POW_2;
	pstcInfo->aulParameter[9] = ADC_FORM_FACTOR_ALGORITHM;
}

// ############################################################################
// ############################################################################
// ############################################################################

/*********************@FUNCTION_HEADER_START*********************
*@FUNCTION NAME:    StartADCSampling()                          *
*                                                               *
*@DESCRIPTION:      Generates A/D sampling settings and starts  *
*                   RLT7 for generating the sampling interval.  *
*                                                               *
*@PARAMETER:        none                                        *
*                                                               *
*@RETURN:           none                                        *
*                                                               *
***********************@FUNCTION_HEADER_END*********************/
static void StartADCSampling(void)
{
	int s, iEnabledStepperCount;
	
	// If timer is running (= sampling in progress) -> exit
	if (TMCSR1_CNTE == 1)
		return;
		
	if (gstcADC.eState != ADC_STATE_ZPD)
	{
#if (ADC_CALIBRATION_LENGTH + ADC_CALIBRATION_BLIND_LENGTH) == 0
	#error "Check ADC_CALIBRATION_LENGTH + ADC_CALIBRATION_BLIND_LENGTH (must not be 0)"
#endif
		gstcADC.usStopIndex = ADC_CALIBRATION_LENGTH + ADC_CALIBRATION_BLIND_LENGTH;		
	}
	else
	{
		gstcADC.usStopIndex = gstcADC.usSampleCount;	
	}
	if (gstcADC.usStopIndex > gstcADC.usBufferSize)
		gstcADC.usStopIndex = gstcADC.usBufferSize;

	// Set base addresses for ADC buffers
	iEnabledStepperCount = 0;
	for (s = 0; s < SMC_ZPD_MAX_STEPPER_UNITS; s++)
	{
		if (gastcSMC[s].bEnabled == true)
		{
			gastcSMC[s].pucSampleBuffer = &gstcADC.pucSampleBuffer[iEnabledStepperCount * gstcADC.usSampleCount];
			iEnabledStepperCount++;
		}
		else
		{
			gastcSMC[s].pucSampleBuffer = null;
		}
	}

	gstcADC.ucCurSMCIndex = 0;
	gstcADC.usIndex = 0;
	
	// Set ADC channel
	if (gastcSMC[gstcADC.ucCurSMCIndex].bEnabled == true)
		ADSR = gastcSMC[gstcADC.ucCurSMCIndex].unCurADCChannel.usADSR;
	else
	{
		ADSR = gstcADC.unADCFallbackChannel.usADSR;
	}
	
	// Set adc test signal, if enabled
	if (gstcTestSignals.stcADCSampling[gstcADC.ucCurSMCIndex].bEnable == true)
		*gstcTestSignals.stcADCSampling[gstcADC.ucCurSMCIndex].pio8Port |= gstcTestSignals.stcADCSampling[gstcADC.ucCurSMCIndex].ucBitMask;
	
	// enable reload timer (triggers ADC)
	TMCSR1_CNTE = 1;
	TMCSR1_TRG  = 1;
}

/*********************@FUNCTION_HEADER_START*********************
*@FUNCTION NAME:    EvaluateBEMFSignal()                        *
*                                                               *
*@DESCRIPTION:      Evaluates the sampled signal waveforms      *
*                   according to the configured algorithm in    *
*                   zpd.h.                                      *
*                   During calibration/error check phase, the   *
*                   appropriate checks are done.                *
*                   If debug mode is enabled, selected data is  *
*                   transmitted via uart to PC.                 *
*                                                               *
*@PARAMETER:        none                                        *
*                                                               *
*@RETURN:           none                                        *
*                                                               *
***********************@FUNCTION_HEADER_END*********************/
static void EvaluateBEMFSignal(void)
{
	unsigned int s;

	// Iterate over SMC ADC buffers
	for (s=0; s<SMC_ZPD_MAX_STEPPER_UNITS; s++)
	{
#if (ADC_FLOATING_AVERAGING == 1)
		int iAverageSum = 0;
		unsigned int uiAverageIndex;
		int aiAverageBuffer[ADC_FLOATING_AVERAGING_SIZE];
#endif	// ADC_FLOATING_AVERAGING

		unsigned int i;
		unsigned long ulForm = 0;
		int iValue;
		
#if (ADC_FORM_FACTOR_ALGORITHM == ADC_FORM_FACTOR_ALGORITHM_MINMAX)
		int iMin =  1000;
		int iMax = -1000;
#elif (ADC_FORM_FACTOR_ALGORITHM == ADC_FORM_FACTOR_ALGORITHM_EQUAL_DIFF)
		int iDiffPos = 0;
		int iDiffNeg = 0;
		int iDiffPosMax = 0;
		int iDiffNegMax = 0;
		int iPrevValue = -1;
#elif ((ADC_FORM_FACTOR_ALGORITHM == ADC_FORM_FACTOR_ALGORITHM_EQUAL_INTEGRATE) || (ADC_FORM_FACTOR_ALGORITHM == ADC_FORM_FACTOR_ALGORITHM_INTEGRATE))
		int iAreaPos = 0;
		int iAreaNeg = 0;
#else
	#error "No or unknown ADC_FORM_FACTOR_ALGORITHM defined!"
#endif	// ADC_FORM_FACTOR_ALGORITHM
			
		STRUCT_SMC* pstcSMC = &gastcSMC[s];
		STRUCT_SMC_ZPD_STALL_DATA* pstcStallData = &gastcStallData[s];
		unsigned long ulFormOldest;
		
		// If this SMC is disabled -> skip
		if (pstcSMC->bEnabled == false)
			continue;
			
		// If this SMC was not set up to be sampled -> go on with next SMC
		if (pstcSMC->bSample == false)
		{
			continue;
		}
			
		// If this is a calibration cycle -> calculate ADC zero line and go on with next SMC
		if (gstcADC.eState != ADC_STATE_ZPD)
		{
			unsigned long ulSum = 0;
			unsigned char ucAverage;
				
			// Build average value
			for (i = ADC_CALIBRATION_BLIND_LENGTH; i < (ADC_CALIBRATION_BLIND_LENGTH + ADC_CALIBRATION_LENGTH); i++)
			{
				ulSum += pstcSMC->pucSampleBuffer[i];
			}
			ucAverage = (unsigned char)(ulSum >> ADC_CALIBRATION_POW_2);
			
			if (gstcADC.eState == ADC_STATE_CONNECTION_CHECK)
			{
				pstcStallData->aucConnectionCheck[pstcSMC->ucMeasuringSMCPair] = ucAverage;
				//NumToTopStr(ucAverage);
				// Check, if active low level on PWM1Pn/PWM2Pn can be seen on PWM1Mn/PWM2Mn
				if (   (ucAverage > pstcSMC->stcSettings.ucMaxConnectionCheckLevel)
				    || (ucAverage < pstcSMC->stcSettings.ucMinConnectionCheckLevel))
				{
					
					pstcStallData->eStallError |= ZPD_ERROR_CONNECTION_CHECK;	// only OR operation, because this check is done for all coils of one stepper
				}
			}
			else // ADC_STATE_CALIBRATION
			{	
				pstcStallData->aucSMCZeroLine[pstcSMC->ucMeasuringSMCPair] = ucAverage;
			}	
			
			continue;
		}
			
#if (ADC_FLOATING_AVERAGING == 1)
		// pre-fill floating average buffer (with first value)
		for (i = 0; i < ADC_FLOATING_AVERAGING_SIZE; i++)
		{
			aiAverageBuffer[i] = pstcSMC->pucSampleBuffer[i];
			iAverageSum += aiAverageBuffer[i];
		}

		// iterate over adc buffer, find min/max and calculate sum
		uiAverageIndex = 0;
		for (; i < gstcADC.usSampleCount; i++)
		{			
			// floating averaging		
			iAverageSum -= aiAverageBuffer[uiAverageIndex];
			aiAverageBuffer[uiAverageIndex] = (int)pstcSMC->pucSampleBuffer[i];
			iAverageSum += aiAverageBuffer[uiAverageIndex];
			uiAverageIndex = (uiAverageIndex + 1) % ADC_FLOATING_AVERAGING_SIZE;
	
			// calculate new average value
			iValue = iAverageSum >> ADC_FLOATING_AVERAGING_POW;
#else
		for (i = 0; i < gstcADC.usSampleCount; i++)
		{			
			iValue = (int)pstcSMC->pucSampleBuffer[i];
#endif	// ADC_FLOATING_AVERAGING

			// ------------------------------------------------------------
			
			// FORM FACTOR ALGORITHM (building statistical values from BEMF profile)
			
#if (ADC_FORM_FACTOR_ALGORITHM == ADC_FORM_FACTOR_ALGORITHM_MINMAX)
			// check for new maximum value
			if (iValue > iMax)
				iMax = iValue;
			
			// check for new minimum value			
			if (iValue < iMin)
				iMin = iValue;
#elif (ADC_FORM_FACTOR_ALGORITHM == ADC_FORM_FACTOR_ALGORITHM_EQUAL_DIFF)
			if (iPrevValue >= 0)
			{
				// Rising part
				if (iValue > iPrevValue)
				{
					iDiffPos++;
					// Store longest rising part
					if (iDiffPos > iDiffPosMax)
						iDiffPosMax = iDiffPos;
					iDiffNeg = 0;
				}
				// falling part
				else if (iValue < iPrevValue)
				{
					iDiffNeg++;
					// Store longest falling part
					if (iDiffNeg > iDiffNegMax)
						iDiffNegMax = iDiffNeg;
					iDiffPos = 0;
				}
			}
			iPrevValue = iValue;
#elif ((ADC_FORM_FACTOR_ALGORITHM == ADC_FORM_FACTOR_ALGORITHM_EQUAL_INTEGRATE) || (ADC_FORM_FACTOR_ALGORITHM == ADC_FORM_FACTOR_ALGORITHM_INTEGRATE))
			iValue -= (int)pstcStallData->aucSMCZeroLine[pstcSMC->ucMeasuringSMCPair];
			if (iValue > 0)
			{
				iAreaPos += iValue;
			}
			else
			{
				iAreaNeg -= iValue;
			}
#else
	#error "No or unknown ADC_FORM_FACTOR_ALGORITHM defined!"
#endif	// ADC_FORM_FACTOR_ALGORITHM
		}

		// --------------------------------------------------------------------

		// FORM FACTOR CALCULATION (out of statistical values)
		
#if (ADC_FORM_FACTOR_ALGORITHM == ADC_FORM_FACTOR_ALGORITHM_MINMAX)
		ulForm = iMax - iMin;
#elif (ADC_FORM_FACTOR_ALGORITHM == ADC_FORM_FACTOR_ALGORITHM_EQUAL_DIFF)
		// Take smaller value of longest rising and falling parts (= common value of both parts)
		if (iDiffNegMax > iDiffPosMax)
			ulForm += iDiffPosMax;
		else
			ulForm += iDiffNegMax;
#elif (ADC_FORM_FACTOR_ALGORITHM == ADC_FORM_FACTOR_ALGORITHM_INTEGRATE)
		// Take sum of of pos. and neg. area (above/below ADC zero line)
		ulForm = iAreaPos + iAreaNeg;
#elif (ADC_FORM_FACTOR_ALGORITHM == ADC_FORM_FACTOR_ALGORITHM_EQUAL_INTEGRATE)
		// Take smaller value of pos. and neg. area (above/below ADC zero line) (= common value of both parts)
		if (iAreaNeg > iAreaPos)
			ulForm += iAreaPos;
		else
			ulForm += iAreaNeg;
#else
	#error "No or unknown ADC_FORM_FACTOR_ALGORITHM defined!"
#endif	// ADC_FORM_FACTOR_ALGORITHM

		// --------------------------------------------------------------------
	
		// check stall condition
		//NumToTopStr(ulForm);
		if (ulForm  < pstcSMC->stcSettings.ulThreshold)
		{
			pstcSMC->bStallDetected = true;

			// Clear stall detected test signal, if enabled
			if (gstcTestSignals.stcStallDetected[s].bEnable == true)
				*gstcTestSignals.stcStallDetected[s].pio8Port &= ~gstcTestSignals.stcStallDetected[s].ucBitMask;
		}
		
		// Save oldest Form Factor		
		ulFormOldest = pstcStallData->aulForm_History[pstcStallData->ucCurrentStep];
		if (pstcSMC->ucFillFormHistoryCounter == 0)
		{
			// check for min/max value
			if (pstcStallData->ulForm_MinDuringRun > ulFormOldest)
				pstcStallData->ulForm_MinDuringRun = ulFormOldest;
			if (pstcStallData->ulForm_MaxDuringRun < ulFormOldest)
				pstcStallData->ulForm_MaxDuringRun = ulFormOldest;
		}
		else
			pstcSMC->ucFillFormHistoryCounter--;

		// Store current Form value in history buffer
		pstcStallData->aulForm_History[pstcStallData->ucCurrentStep] = ulForm;	
	}				
}

void SMC_pzd(void)
{
	unsigned char ucRunningSMCCount = gstcCommonCtrl.usEnabledSMCCount;
	int s;
	
	// --------------------------------------------------------------	
	//NumToTopStr(ucRunningSMCCount);
	switch (gstcADC.eState)
	{
		case ADC_STATE_START:
			gstcCommonCtrl.ucConnectionCheckSteps = 4;	// Do four steps error check
			gstcCommonCtrl.ucCalibrationSteps     = 4;	// Do four calibration cycles
			// Start normal steps with blinding
			gstcCommonCtrl.bStartBlindingTime = true;
			gstcCommonCtrl.bStartADC = false;
			gstcADC.eState = ADC_STATE_CONNECTION_CHECK;	
		case ADC_STATE_CONNECTION_CHECK:
			if (gstcCommonCtrl.ucConnectionCheckSteps == 0)
			{
				for (s = 0; s < SMC_ZPD_MAX_STEPPER_UNITS; s++)
				{
					if (gastcSMC[s].bEnabled == false)
						continue;
		
					// Set pull-up resistors for ZPD:
					// PWM1Pn/PWM2Pn: int. pull-up to VCC, ext. pull-down to GND
					// PWM1Mn/PWM2Mn: floating
					if (gstcSMCRegisters[s].bPUCRUpperNibble == true)
					{
						*gstcSMCRegisters[s].pucPUCR &= 0x0F;
						*gstcSMCRegisters[s].pucPUCR |= 0x50;	// port pins Pxx.6 and Pxx.4
					}
					else
					{
						*gstcSMCRegisters[s].pucPUCR &= 0xF0;
						*gstcSMCRegisters[s].pucPUCR |= 0x05;	// port pins Pxx.2 and Pxx.0
					}
				}
				gstcADC.eState = ADC_STATE_CALIBRATION;
				return;
			}
			gstcCommonCtrl.ucConnectionCheckSteps--;
			break;
			
		case ADC_STATE_CALIBRATION:
			if (gstcCommonCtrl.ucCalibrationSteps == 0)
			{
				gstcADC.eState = ADC_STATE_ZPD;
				return;
			}
			gstcCommonCtrl.ucCalibrationSteps--;
			break;
		
		case ADC_STATE_ZPD:
		default:
			if (gstcCommonCtrl.bStartBlindingTime == true)
			{
				// Set sampling time (for next reload event)
				SET_RLT2_US(gstcCommonCtrl.ulSamplingTimeUs);
				gstcCommonCtrl.bStartBlindingTime = false;

				// Clear sampling test signal, if enabled
				if (gstcTestSignals.stcStepSamplingTime.bEnable == true)
					*gstcTestSignals.stcStepSamplingTime.pio8Port &= ~gstcTestSignals.stcStepSamplingTime.ucBitMask;
				// Set blinding test signal, if enabled
				if (gstcTestSignals.stcStepBlindingTime.bEnable == true)
					*gstcTestSignals.stcStepBlindingTime.pio8Port |= gstcTestSignals.stcStepBlindingTime.ucBitMask;
			}
			else
			{
				// Set blinding time (for next reload event)
				SET_RLT2_US(gstcCommonCtrl.ulBlindingTimeUs);
				// Sample now
				gstcCommonCtrl.bStartADC = true;
				gstcCommonCtrl.bStartBlindingTime = true;

				// Clear blinding test signal, if enabled
				if (gstcTestSignals.stcStepBlindingTime.bEnable == true)
					*gstcTestSignals.stcStepBlindingTime.pio8Port &= ~gstcTestSignals.stcStepBlindingTime.ucBitMask;
				// Set sampling test signal, if enabled
				if (gstcTestSignals.stcStepSamplingTime.bEnable == true)
					*gstcTestSignals.stcStepSamplingTime.pio8Port |= gstcTestSignals.stcStepSamplingTime.ucBitMask;

				// Go directly to sampling
				goto Label_Sample;
			}
			break;
	}
	
	for (s = 0; s < SMC_ZPD_MAX_STEPPER_UNITS; s++)
	{
		STRUCT_SMC*        pstcSMC;
		STRUCT_SMC_ZPD_STALL_DATA* pstcStallData;
		bool bHoldStep;

		if (gastcSMC[s].bEnabled == false)
			continue;

		pstcSMC       = &gastcSMC[s];
		pstcStallData = &gastcStallData[s];
		bHoldStep     = false;

		// Reset ADC sample flag (is only set, when sampling is required)
		pstcSMC->bSample = false;
					
		// If this step must be hold -> go on with next SMC
		if (pstcSMC->ucHoldCounter != 0)
		{
			pstcSMC->ucHoldCounter--;
			// Hold currently active step
			bHoldStep = true;
		}
		else
		{
			switch (pstcSMC->eState)
			{				
				default:
				case STEP_STATE_START:
					// Wait for end of calibration first
					if (gstcADC.eState != ADC_STATE_ZPD)
					{
						gstcCommonCtrl.bStartADC = true;
						pstcSMC->bSample = true;
						break;
					}
					// if stepper motor coil connection is faulty
					else if (pstcStallData->eStallError != ZPD_ERROR_NO)
					{
						// go directly to the end
						pstcSMC->stcSettings.ucPostStallSteps = 0;
						pstcSMC->eState = STEP_STATE_STALL;
						break;
					}
					// wait after first phase is powered to correctly align the rotor
					pstcSMC->ucHoldCounter = 5;
					pstcStallData->usStepCount = 0;
					pstcSMC->eState = STEP_STATE_RUN;
					break;
				
				case STEP_STATE_RUN:
					// If max. nr. of steps is reached -> emergency stop
					if (pstcStallData->usStepCount >= pstcSMC->stcSettings.usMaxSteps)
					{
						pstcStallData->eStallError = ZPD_ERROR_MAX_STEPS_REACHED;
						// go directly to the end
						pstcSMC->stcSettings.ucPostStallSteps = 0;
						pstcSMC->eState = STEP_STATE_STALL;
					}
					// If stall was not detected
					if (pstcSMC->bStallDetected == false)
					{
						// Don't sample the first step
						// (otherwise wrong stall could be detected
						// if rotor is at opposite stall position)
						if (pstcStallData->usStepCount != 0)
						{
							// Do ADC sampling
							pstcSMC->bSample = true;
						}
						else
						{
							gastcStallData[s].ulForm_MinDuringRun = 0xFFFFFFFF;
							gastcStallData[s].ulForm_MaxDuringRun = 0;
							gastcSMC[s].ucFillFormHistoryCounter = sizeof(gastcStallData[0].aulForm_History) / sizeof(gastcStallData[0].aulForm_History[0]);
						}
						pstcStallData->usStepCount++;
					}
					else
					{
						// (Re)Store stall step number
						if (pstcStallData->usStepCount == 1)
							pstcSMC->ucNextStep = pstcStallData->ucCurrentStep = pstcStallData->ucStallStep;
						else
							pstcSMC->ucNextStep = pstcStallData->ucStallStep = pstcStallData->ucCurrentStep;
						// Switch direction to go to final position
						pstcSMC->stcSettings.bDirection ^= true;
						// Do a soft direction change
						pstcSMC->ucChangeDirectionCounter = 3;
						pstcSMC->eState = STEP_STATE_STALL;
					}
					break;
				
				case STEP_STATE_STALL:
					// If post stall is in progress
					if (pstcSMC->stcSettings.ucPostStallSteps != 0)
					{
						pstcSMC->stcSettings.ucPostStallSteps--;
					}
					else
					{
						// This SMC is stalled
						ucRunningSMCCount--;
						// Hold currently active step
						bHoldStep = true;

						// Set stall detected test signal, if enabled
						if (gstcTestSignals.stcStallDetected[s].bEnable == true)
							*gstcTestSignals.stcStallDetected[s].pio8Port |= gstcTestSignals.stcStallDetected[s].ucBitMask;
					}
					break;
			}
		
			// If soft direction change is in progress
			if (pstcSMC->ucChangeDirectionCounter != 0)
			{
				pstcSMC->ucHoldCounter = pstcSMC->ucChangeDirectionCounter << 1;
				pstcSMC->ucChangeDirectionCounter--;
			}
			
			// Set ADC channel for next step
			pstcSMC->unCurADCChannel.stcADSR.b5ANS = pstcSMC->unCurADCChannel.stcADSR.b5ANE = pstcSMC->stcSettings.aucADCChannel[pstcSMC->ucNextStep];
		}
		
		// Set next step
		if (bHoldStep == false)
		{
			pstcStallData->ucCurrentStep = pstcSMC->ucNextStep;
			// Calculate next step number
			if (pstcSMC->stcSettings.bDirection == false)
				pstcSMC->ucNextStep = (pstcSMC->ucNextStep + 1) % 4;
			else
				pstcSMC->ucNextStep = (pstcSMC->ucNextStep + 3) % 4;
		}
		
		// Set next commutation step (or set the last step again, if this step is to hold)
		switch (gstcADC.eState)
		{
			case ADC_STATE_CONNECTION_CHECK:
				// configuration set in Stepper_Start
				break;
			case ADC_STATE_CALIBRATION:
				// same configuration
				break;
			case ADC_STATE_ZPD:
			default:
				*gstcSMCRegisters[s].pusPWS  = gausStepsPWS[pstcStallData->ucCurrentStep];
				*gstcSMCRegisters[s].pucPWEC = gaucStepsPWEC[pstcStallData->ucCurrentStep];	// disable outputs of measuring pair (to enable pull-up resistor)
				break;
		}
		pstcSMC->ucMeasuringSMCPair = pstcStallData->ucCurrentStep & 0x01;
	}

	// If all steppers are stalled
	if (ucRunningSMCCount == 0)
	{
		// Disable reload timer
		TMCSR2_CNTE = 0;
		TMCSR2_INTE = 0;
		
		gstcCommonCtrl.bZPDRunning = false;
		m_enSmcMode = NormalDriving;
			
		// Call callback function
		if (gstcCommonCtrl.ptStallDetectedCallbackFunction != fnull)
		{
			gstcCommonCtrl.ptStallDetectedCallbackFunction();
		}
		return;
	}

	// ------------------------------------------------------------------------

Label_Sample:
	if (gstcCommonCtrl.bStartADC == true)
	{
		// Set the first and last ADC channel that will be sampled
		StartADCSampling();
		gstcCommonCtrl.bStartADC = false;
	}
}
/*********************@FUNCTION_HEADER_START*********************
*@FUNCTION NAME:    ISR_RLT0()                                  *
*                                                               *
*@DESCRIPTION:      Interrupt service routine for the reload    *
*                   timer 0 (RLT0) for stepper motor driving.   *
*                   (must be put into interrupt vector table    *
*                   for IRQ32 and ICR08 must be set < 31)       *
*    ATTENTION: ICR level for ISR_ADC must be equal or          *
*               smaller than the level for ISR_RLT0!            *
*                                                               *
*@PARAMETER:        none                                        *
*                                                               *
*@RETURN:           none                                        *
*                                                               *
***********************@FUNCTION_HEADER_END*********************/
__interrupt void ISR_RLT2(void)
{	
     		// reset underflow interrupt request flag
	if (m_enSmcMode == Zpd)
	{
		TMCSR2_UF = 0;  
		SMC_pzd();	
	}
	else
	{
		SMC_IRQ();
	}
}


void ADC_zpd(void)
{
// Clear INT of ADC
	ADCSH_INT = 0;
	// Read ADC value (if this stepper motor unit is enabled)
	if (gastcSMC[gstcADC.ucCurSMCIndex].bEnabled == true)
	{
		gastcSMC[gstcADC.ucCurSMCIndex].pucSampleBuffer[gstcADC.usIndex] = ADCR;
		//NumToBottomStr(ADCRLH);
		
		// Clear adc test signal, if enabled
		if (gstcTestSignals.stcADCSampling[gstcADC.ucCurSMCIndex].bEnable == true)
			*gstcTestSignals.stcADCSampling[gstcADC.ucCurSMCIndex].pio8Port &= ~gstcTestSignals.stcADCSampling[gstcADC.ucCurSMCIndex].ucBitMask;
	}

	// Goto next SMC unit
	gstcADC.ucCurSMCIndex++;
	if (gstcADC.ucCurSMCIndex >= SMC_ZPD_MAX_STEPPER_UNITS)
	{
		gstcADC.ucCurSMCIndex = 0;
		gstcADC.usIndex++;
		// Check stop condition
		if (gstcADC.usIndex >= gstcADC.usStopIndex)
		{
			// Stop timer
			TMCSR1_CNTE = 0;
			
			EvaluateBEMFSignal();
			
			return;
		}
	}

	// Write next ADC channel configuration
	// Only if channel is enabled
	if (gastcSMC[gstcADC.ucCurSMCIndex].bEnabled == true)
	{
		ADSR = gastcSMC[gstcADC.ucCurSMCIndex].unCurADCChannel.usADSR;

		// Set adc test signal, if enabled
		if (gstcTestSignals.stcADCSampling[gstcADC.ucCurSMCIndex].bEnable == true)
			*gstcTestSignals.stcADCSampling[gstcADC.ucCurSMCIndex].pio8Port |= gstcTestSignals.stcADCSampling[gstcADC.ucCurSMCIndex].ucBitMask;
	}
}
/*********************@FUNCTION_HEADER_START*********************
*@FUNCTION NAME:    ISR_ADC()                                   *
*                                                               *
*@DESCRIPTION:      Interrupt service routine for the ADC end   *
*                   interrupt for sampled adc data.             *
*                   (must be put into interrupt vector table    *
*                   for IRQ134 and ICR59 must be set < 31)      *
*    ATTENTION: ICR level for ISR_ADC must be equal or          *
*               smaller than the level for ISR_RLT0!            *
*                                                               *
*@PARAMETER:        none                                        *
*                                                               *
*@RETURN:           none                                        *
*                                                               *
***********************@FUNCTION_HEADER_END*********************/
__interrupt void ISR_ADC(void)
{
	static unsigned char AdcCnt = 0;

	if (m_enSmcMode == Zpd)
	{
		ADC_zpd();
	}
	else
	{
		ADC_IRQ();
	}
}

/********************@FUNCTION_DECLARATION_END******************/
