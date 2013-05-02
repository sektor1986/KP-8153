/*****************************************************************************/
/*  F2MC-16FX Family Template Project V01L01                                 */
/*  ALL RIGHTS RESERVED, COPYRIGHT (C) FUJITSU SEMICONDUCTOR LIMITED 2011    */
/*  LICENSED MATERIAL - PROGRAM PROPERTY OF FUJITSU SEMICONDUCTOR LIMITED    */
/*****************************************************************************/
/*****************************************************************************
  MAIN.C
  - description
  - See README.TXT for project description and disclaimer.
******************************************************************************/

#include "_ffmc16.h"
#include "vectors.h"
#include "adc.h"
#include "timer.h"
#include "mb96670_smc_zpd.h"
#include "options.h"
#include "button.h"
#include "j1939.h"
#include "rtc.h"
#include "driver_update.h"
#include "icu.h"
#include "sound.h"


#if (SMC_TYPE == SMC_TYPE_R200)
// --- ZPD values R200-----------------------------------
	#define STEP_BLINDING_TIME_US		1500
	#define STEP_SAMPLING_TIME_US		7000
	#define ADC_SAMPLE_COUNT			50
	#define THRESHOLD					30
	#define DIRECTION					true

#elif (SMC_TYPE == SMC_TYPE_WCHINA)
// White china
	#define STEP_BLINDING_TIME_US		1000
	#define STEP_SAMPLING_TIME_US		20000
	#define ADC_SAMPLE_COUNT			50
	#define THRESHOLD					500
	#define DIRECTION					true
#else
// VID23
	#define STEP_BLINDING_TIME_US		1000
	#define STEP_SAMPLING_TIME_US		20000
	#define ADC_SAMPLE_COUNT			50
	#define THRESHOLD					400
	#define DIRECTION					false
#endif /* SMC_TYPE */

#define MAX_STEP_COUNT				1000
//#define MIN_CONNECTION_CHECK_LEVEL	164
//#define MAX_CONNECTION_CHECK_LEVEL	203

#define MIN_CONNECTION_CHECK_LEVEL	50
#define MAX_CONNECTION_CHECK_LEVEL	180

#define START_STEP					1
#define POST_STALL_STEPS			0

/*********************@GLOBAL_VARIABLES_START*******************/

STRUCT_SMC_ZPD_SETTINGS gstcZPDSettings = {	{ 0, 0, 0, 0 }, THRESHOLD, MAX_STEP_COUNT, MAX_CONNECTION_CHECK_LEVEL, MIN_CONNECTION_CHECK_LEVEL, START_STEP, DIRECTION, POST_STALL_STEPS };
								
#define STEPPER_COUNT	2					
STRUCT_SMC_ZPD_STALL_DATA gastcStallData[STEPPER_COUNT];
unsigned char aucStartStep[STEPPER_COUNT] = {START_STEP, START_STEP};

unsigned char gaaucADSampleBuffer[300][STEPPER_COUNT];

unsigned long gulStepBlindingTimeUs = STEP_BLINDING_TIME_US;
unsigned long gulStepSamplingTimeUs = STEP_SAMPLING_TIME_US;
unsigned short gusADCSampleCount = ADC_SAMPLE_COUNT;

STRUCT_SMC_ZPD_INFO gstcInfo;

bool gbStepperRunning = false;
bool gbStallDetected = false;
bool gbOutputStallData = false;

void SMC_ZPD_StallDetected(void);
void StopStepper(void);

void ZPD_Init(void);

/*****************************************************************************/
/*  Main application to control the program flow                             */
/*****************************************************************************/
void main(void)
{	
	Vectors_InitIrqLevels();
	// Allow all interrupt levels
	__set_il(7);
	// Enable interrupts
	__EI();
	
	PIER04_IE5 = 1;
	PIER04_IE4 = 1;	/* i2c 0 */
	initial_set();
	init_i2c_0_slave();
	
	 #if ((SMC_TYPE != SMC_TYPE_R200) && (ZPD == ZPD_ENABLE))
		ZPD_Init();	
		//Ожидание окончания ZPD
		while (m_enSmcMode == Zpd)
		{
			WDTCP = 0x00;
		}
 	#else 	
	 	m_enSmcMode = NormalDriving;
	#endif

	InitSMC(250);						// Таймер SMC 250*2=500  mks
	Timer_Init();
	InitADC();
	InitRTC();
	J1939_init();
	InitICU1();
	InitEI0();
	InitSoundGen();
	
// Если двигатель R200 или ZPD не активно
	#if ((SMC_TYPE == SMC_TYPE_R200) || (ZPD == ZPD_DISABLE))	
		ZeroPosSMC();					// Возврат стрелок на 0
		Timer_Wait(TIMER_ID_MAIN, 2000, TRUE);  
	#endif
	
	ClearPosSMC();						// Установка нулевых позиций для моторов
	
	DriverInit();
	Button_Init(ButtonCallback);
	// Endless loop
	while(1)
	{
		WDTCP_ = 0x00;//Watchdog clearing
		Timer_Main();
	}
}

void ZPD_Init(void)
{
	ENUM_SMC_ZPD_ERRORS Error;
    // Initialize MB96300_SCM_ZPD
	Error = SMC_ZPD_Init((unsigned char*)gaaucADSampleBuffer, sizeof(gaaucADSampleBuffer), SMC_ZPD_StallDetected);
	SMC_ZPD_SetADCSampleCount(gusADCSampleCount);
	SMC_ZPD_SetStepBlindingTime(gulStepBlindingTimeUs);
	SMC_ZPD_SetStepSamplingTime(gulStepSamplingTimeUs);	
	
	gstcZPDSettings.aucADCChannel[0] = gstcZPDSettings.aucADCChannel[2] = 19;
	gstcZPDSettings.aucADCChannel[1] = gstcZPDSettings.aucADCChannel[3] = 17;
	gstcZPDSettings.ucStartStep = aucStartStep[0];
	SMC_ZPD_EnableSMC(0, &gstcZPDSettings);	// SMC0		
	
	gstcZPDSettings.aucADCChannel[0] = gstcZPDSettings.aucADCChannel[2] = 23;
	gstcZPDSettings.aucADCChannel[1] = gstcZPDSettings.aucADCChannel[3] = 21;
	gstcZPDSettings.ucStartStep = aucStartStep[1];
	SMC_ZPD_EnableSMC(1, &gstcZPDSettings);	// SMC1	
	
	gbStallDetected = false;
	gbStepperRunning = true;
	
	Error = SMC_ZPD_Start();
}

void SMC_ZPD_StallDetected(void)
{
	unsigned int s;
	
	gbStallDetected = true;
	StopStepper();

	for (s=0; s<STEPPER_COUNT; s++)
	{
		SMC_ZPD_GetStallData(s, &gastcStallData[s]);
		aucStartStep[s] = gastcStallData[s].ucCurrentStep;
	}
	
	gbOutputStallData = true;
}

void StopStepper(void)
{
	SMC_ZPD_Stop();
	gbStepperRunning = false;
}

